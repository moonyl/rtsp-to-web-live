//
// Created by moony on 2022-04-01.
//

#pragma once
extern "C"  {
#include <libavformat/avformat.h>
}

class Remux {
    char const* in_filename;
    char const* out_filename;
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx = NULL, *ofmt_ctx = NULL;
    AVPacket pkt;
    int ret, i;
    int stream_index = 0;
    int *stream_mapping = NULL;
    int stream_mapping_size = 0;

public:
    Remux(char const* in_filename, char const* out_filename);
    void run();
    ~Remux();

private:
    void init();
};




