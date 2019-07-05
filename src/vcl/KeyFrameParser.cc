/**
 * @file   KeyFrameParser.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <iostream>
#include <vector>
#include <errno.h>

#include "vcl/KeyFrameParser.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
}

using namespace VCL;

KeyFrameParser::KeyFrameParser(std::string filename) :
_filename(filename)
{
    int ret = init_stream();
    if (ret != 0) {
        context_cleanup();
        throw VCLException(FFmpegInitFailed,
                           error_msg(ret, "init_parser() failed"));
    }

    av_log_set_level(AV_LOG_QUIET);
}

KeyFrameParser::~KeyFrameParser()
{
    context_cleanup();
}

int KeyFrameParser::init_stream(void) noexcept
{
    int ret = 0;

    _tctx.fmt_context = avformat_alloc_context();
    ret = avformat_open_input(&_tctx.fmt_context,
                              _filename.c_str(), NULL, NULL);
    if (ret != 0)
        return ret;

    ret = avformat_find_stream_info(_tctx.fmt_context, NULL);
    if (ret != 0)
        return ret;

    AVCodecParameters* codec = NULL;
    for (unsigned i = 0; i < _tctx.fmt_context->nb_streams && !codec; i++) {
        if (_tctx.fmt_context->streams[i]->codecpar->codec_type ==
                                                          AVMEDIA_TYPE_VIDEO) {
            codec = _tctx.fmt_context->streams[i]->codecpar;
            _tctx.stream_index = i;
        }
    }

    if (!codec)
        return AVERROR_ENCODER_NOT_FOUND;
    else if (codec->codec_id != AV_CODEC_ID_H264)
        return AVERROR_INVALIDDATA;

    return 0;
}

int KeyFrameParser::fill_frame_list(void) noexcept
{
    AVPacket* pkt = av_packet_alloc();
    if (!pkt)
        return AVERROR_EXTERNAL;

    int success = 0;
    int ret     = 0;
    uint64_t frame_idx = 0;
    while ((success = av_read_frame(_tctx.fmt_context, pkt)) != AVERROR_EOF) {
        if (pkt->stream_index != _tctx.stream_index)
            continue;
        if (pkt->flags & AV_PKT_FLAG_KEY) {
            KeyFrame frame = {.idx = frame_idx, .base = pkt->pos};
            _frame_list.push_back(frame);
        }
        frame_idx++;
    }
    av_packet_unref(pkt);

    return 0;
}

std::string KeyFrameParser::error_msg(int errnum, const std::string& opt)
{
    char errbuf[128];

    int ret = av_strerror(errnum, errbuf, sizeof(errbuf));
    if (ret != 0)
        sprintf(errbuf, "unknown ffmpeg error");

    std::string cause = "";
    if (!opt.empty())
        cause += (opt + ": ");

    return cause + errbuf;
}

void KeyFrameParser::context_cleanup(void)
{
    if (_tctx.fmt_context) {
        avformat_close_input(&_tctx.fmt_context);
        avformat_free_context(_tctx.fmt_context);
    }
}

const KeyFrameList& KeyFrameParser::parse(void)
{
    int ret = fill_frame_list();
    if (ret != 0) {
        context_cleanup();
        throw VCLException(FFmpegParseFailed,
                           error_msg(ret, "fill_frame_list() failed"));
    }

    return _frame_list;
}
