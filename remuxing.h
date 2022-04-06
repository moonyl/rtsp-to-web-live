//
// Created by moony on 2022-03-30.
//

#pragma once

#include <libavformat/avformat.h>
void log_packet(const AVFormatContext *fmt_ctx, const AVPacket *pkt, const char *tag);