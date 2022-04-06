//
// Created by moony on 2022-04-01.
//

#include "Remux.h"
#include <stdexcept>
#include <sstream>
#include <iostream>
extern "C"  {
#include "remuxing.h"
}

Remux::Remux(const char *in_filename, const char *out_filename) : in_filename{in_filename}, out_filename{out_filename}  {
}

void Remux::init() {
    if ((ret = avformat_open_input(&ifmt_ctx, in_filename, 0, 0)) < 0)    {
        std::stringstream ss;
        ss << "Could not open input file " << in_filename;
        throw std::runtime_error(ss.str());
    }

    if ((ret = avformat_find_stream_info(ifmt_ctx, 0)) < 0)    {
        throw std::runtime_error("Failed to retrieve input stream information");
    }

    av_dump_format(ifmt_ctx, 0, in_filename, 0);

    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx)
    {
        ret = AVERROR_UNKNOWN;
        throw std::runtime_error("Could not create output context");
    }

    stream_mapping_size = ifmt_ctx->nb_streams;
    stream_mapping = static_cast<int *>(av_mallocz_array(stream_mapping_size, sizeof(*stream_mapping)));
    if (!stream_mapping)
    {
        ret = AVERROR(ENOMEM);
        throw std::runtime_error("No memory on allocating stream_mapping");
    }

    ofmt = ofmt_ctx->oformat;

    for (i = 0; i < ifmt_ctx->nb_streams; i++)
    {
        AVStream *out_stream;
        AVStream *in_stream = ifmt_ctx->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;

        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE)   {
            stream_mapping[i] = -1;
            continue;
        }

        stream_mapping[i] = stream_index++;

        out_stream = avformat_new_stream(ofmt_ctx, NULL);
        if (!out_stream)        {
            ret = AVERROR_UNKNOWN;
            throw std::runtime_error("Failed allocating output stream");
        }

        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0)        {
            throw std::runtime_error("Failed to copy codec parameters");
        }
        out_stream->codecpar->codec_tag = 0;
    }
    av_dump_format(ofmt_ctx, 0, out_filename, 1);

    if (!(ofmt->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            std::stringstream ss;
            ss << "Could not open output file " << out_filename;
            throw std::runtime_error(ss.str());
        }
    }
}

void Remux::run() {
    init();

    ret = avformat_write_header(ofmt_ctx, NULL);
    if (ret < 0)    {
        throw std::runtime_error("Error occurred when opening output file");
    }

    while (1)
    {
        AVStream *in_stream, *out_stream;

        ret = av_read_frame(ifmt_ctx, &pkt);
        if (ret < 0)
            throw std::runtime_error("Can not read frame");

        in_stream = ifmt_ctx->streams[pkt.stream_index];
        if (pkt.stream_index >= stream_mapping_size ||
            stream_mapping[pkt.stream_index] < 0)
        {
            av_packet_unref(&pkt);
            continue;
        }

        pkt.stream_index = stream_mapping[pkt.stream_index];
        out_stream = ofmt_ctx->streams[pkt.stream_index];
        log_packet(ifmt_ctx, &pkt, "in");

        /* copy packet */
        pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, out_stream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, out_stream->time_base, static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
        pkt.pos = -1;
        log_packet(ofmt_ctx, &pkt, "out");

        ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
        if (ret < 0)    {
            throw std::runtime_error("Error muxing packet");
        }
        av_packet_unref(&pkt);
    }

    av_write_trailer(ofmt_ctx);
}

Remux::~Remux() {
    avformat_close_input(&ifmt_ctx);

    /* close output */
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_closep(&ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);

    av_freep(&stream_mapping);

    if (ret < 0 && ret != AVERROR_EOF)
    {
        char errorString[AV_ERROR_MAX_STRING_SIZE];
        av_make_error_string(errorString, AV_ERROR_MAX_STRING_SIZE, ret);
        std::stringstream ss;
        ss << "Error occurred: " << errorString;
        std::cerr << ss.str() << std::endl;
    }
}

