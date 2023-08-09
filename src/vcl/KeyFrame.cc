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

#include <algorithm>
#include <errno.h>
#include <vector>

#include "vcl/KeyFrame.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/error.h>
#include <libavutil/log.h>
}

using namespace VCL;

/*  *********************** */
/*    KEY_FRAME_OP          */
/*  *********************** */

int KeyFrameOp::init_stream(void) {
  int ret = 0;
  unsigned n_video_stream = 0;

  _fctx.fmt_context = avformat_alloc_context();
  ret = avformat_open_input(&_fctx.fmt_context, _filename.c_str(), NULL, NULL);
  if (ret != 0)
    return ret;

  ret = avformat_find_stream_info(_fctx.fmt_context, NULL);
  if (ret != 0)
    return ret;

  AVCodecParameters *codec = NULL;
  for (unsigned i = 0; i < _fctx.fmt_context->nb_streams && !codec; i++) {

    AVStream *stream = _fctx.fmt_context->streams[i];

    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {

      codec = stream->codecpar;

      _fctx.video_stream = stream;
      _fctx.video_stream_idx = i;

      int64_t time_base_num = stream->r_frame_rate.num;
      int64_t time_base_den = stream->r_frame_rate.den;
      _nb_frames = stream->nb_frames;
      _time_base = (time_base_den * AV_TIME_BASE) / time_base_num;

      n_video_stream++;
    }
  }

  if (n_video_stream == 0) {
    throw VCLException(FFmpegInitFailed, "No video stream found");
  }

  if (n_video_stream > 1) {
    throw VCLException(FFmpegInitFailed,
                       "Cannot handle more than 1 video stream per file");
  }

  if (!codec)
    return AVERROR_ENCODER_NOT_FOUND;
  else if (codec->codec_id != AV_CODEC_ID_H264)
    return AVERROR_INVALIDDATA;

  return 0;
}

std::string KeyFrameOp::error_msg(int errnum, const std::string &opt) {
  char errbuf[128];

  int ret = av_strerror(errnum, errbuf, sizeof(errbuf));
  if (ret != 0)
    sprintf(errbuf, "unknown ffmpeg error");

  std::string cause = "";
  if (!opt.empty())
    cause += (opt + ": ");

  return cause + errbuf;
}

KeyFrameOp::KeyFrameOp(std::string filename) : _filename(filename) {
  int ret = init_stream();
  if (ret != 0)
    throw VCLException(FFmpegInitFailed,
                       error_msg(ret, "init_parser() failed"));

  av_log_set_level(AV_LOG_QUIET);
}

KeyFrameOp::~KeyFrameOp() {
  if (_fctx.fmt_context) {
    avformat_close_input(&_fctx.fmt_context);
    avformat_free_context(_fctx.fmt_context);
  }
}

/*  *********************** */
/*    KEY_FRAME_PARSER      */
/*  *********************** */

int KeyFrameParser::fill_frame_list(void) noexcept {
  AVPacket *pkt = av_packet_alloc();
  if (!pkt)
    return AVERROR_EXTERNAL;

  unsigned frame_idx = 0;

  while (true) {
    av_packet_unref(pkt);
    int ret = av_read_frame(_fctx.fmt_context, pkt);

    if (ret != 0 && ret != AVERROR_EOF) {
      return ret;
    } else if (ret == AVERROR_EOF) {
      return 0;
    }

    if (pkt->stream_index != _fctx.video_stream_idx) {
      continue;
    }

    if (pkt->flags & AV_PKT_FLAG_KEY) {
      KeyFrame frame = {.idx = frame_idx, .base = pkt->pos};
      _frame_list.push_back(frame);
    }
    frame_idx++;
  };

  av_packet_unref(pkt);

  return 0;
}

const KeyFrameList &KeyFrameParser::parse(void) {
  int ret = fill_frame_list();
  if (ret != 0)
    throw VCLException(FFmpegParseFailed,
                       error_msg(ret, "fill_frame_list() failed"));

  return _frame_list;
}

/*  *********************** */
/*    KEY_FRAME_DECODER     */
/*  *********************** */

KeyFrameDecoder::KeyFrameDecoder(std::string filename)
    : KeyFrameOp(filename), _last_consumed_frame(-1) {
  int ret = init_decoder();
  if (ret != 0)
    throw VCLException(FFmpegDecodeFailed, error_msg(ret, "init_decoder"));

  ret = init_bsf();
  if (ret != 0)
    throw VCLException(FFmpegDecodeFailed, error_msg(ret, "init_bsf"));
}

KeyFrameDecoder::~KeyFrameDecoder() {
  for (auto &f : _frame_list)
    av_frame_free(&f.frame);
  if (_ctx.video_codec_context)
    ;
  avcodec_close(_ctx.video_codec_context);
  if (_ctx.frame_codec_context)
    ;
  avcodec_close(_ctx.frame_codec_context);
  if (_ctx.bsf_context)
    av_bsf_free(&_ctx.bsf_context);
}

int KeyFrameDecoder::init_decoder(void) noexcept {
  // Initialize H264 video decoder
  AVCodecParameters *video_codec = _fctx.video_stream->codecpar;

  _ctx.byte_stream_format =
      (video_codec->bit_rate) ? H264Format::AVCC : H264Format::AnnexB;

  const AVCodec *codec_ptr = avcodec_find_decoder(video_codec->codec_id);

  if (!codec_ptr)
    return AVERROR_DECODER_NOT_FOUND;

  _ctx.video_codec_context = avcodec_alloc_context3(codec_ptr);
  if (!_ctx.video_codec_context)
    return AVERROR_DECODER_NOT_FOUND;

  int ret = avcodec_open2(_ctx.video_codec_context, codec_ptr, NULL);
  if (ret < 0)
    return ret;

  return 0;
}

int KeyFrameDecoder::init_bsf(void) noexcept {
  int ret = 0;
  const AVBitStreamFilter *bsf;

  bsf = av_bsf_get_by_name("h264_mp4toannexb");
  if (!bsf)
    return AVERROR_BSF_NOT_FOUND;

  ret = av_bsf_alloc(bsf, &_ctx.bsf_context);
  if (ret != 0)
    return ret;

  AVRational time_base;
  AVCodecParameters *codec;

  time_base = _fctx.video_stream->time_base;
  codec = _fctx.video_stream->codecpar;

  ret = avcodec_parameters_copy(_ctx.bsf_context->par_in, codec);
  if (ret < 0)
    return ret;

  _ctx.bsf_context->time_base_in = time_base;

  ret = av_bsf_init(_ctx.bsf_context);
  if (ret != 0)
    return ret;

  return 0;
}

void KeyFrameDecoder::clear(void) {
  _enc_frame_list.clear();

  for (auto &f : _frame_list)
    av_frame_free(&f.frame);
  _frame_list.clear();

  for (auto &interval : _interval_map)
    interval.second.clear();
}

void KeyFrameDecoder::set_key_frames(const KeyFrameList &key_frames) {
  int ret = populate_intervals(key_frames);
  if (ret != 0)
    throw VCLException(FFmpegDecodeFailed,
                       error_msg(AVERROR_EXTERNAL, "populate_intervals"));
}

// This method will only decode a list of frames that are within an
// interval, defined by start and end.
int KeyFrameDecoder::decode_interval(const KeyFrame &start, const KeyFrame &end,
                                     const std::vector<unsigned> &frames) {
  AVPacket *pkt = av_packet_alloc();
  if (!pkt)
    return AVERROR_EXTERNAL;

  AVFrame *current_frame = av_frame_alloc();
  if (!current_frame)
    return AVERROR_EXTERNAL;

  int ret = 0;

  unsigned first_frame = frames.at(0);

  bool do_seek = true;
  if (first_frame > _last_consumed_frame && _last_consumed_frame >= start.idx) {
    do_seek = false;
  }

  if (do_seek) {
    // Compute the time, slightly after a key frame, for seeking.
    int64_t seekTarget = int64_t(start.idx + 1) * _time_base;

    if (_ctx.byte_stream_format == H264Format::AVCC) {
      ret = av_seek_frame(_fctx.fmt_context, -1, seekTarget,
                          AVSEEK_FLAG_BACKWARD);
    } else {
      ret = av_seek_frame(_fctx.fmt_context, _fctx.video_stream_idx, start.base,
                          AVSEEK_FLAG_BYTE);
    }

    avcodec_flush_buffers(_ctx.video_codec_context);
  }

  if (ret != 0)
    return ret;

  unsigned frame_idx = 0;
  bool av_read_eof = false;

  unsigned idx = do_seek ? start.idx : _last_consumed_frame + 1;

  for (; idx < end.idx;) {

    if (!av_read_eof) {
      do {
        ret = av_read_frame(_fctx.fmt_context, pkt);
        if (ret == AVERROR_EOF) {
          av_read_eof = true;
          break;
        }
      } while (pkt->stream_index != _fctx.video_stream_idx);

      if (av_read_eof)
        continue;

      // This is needed to filter (small modifications) packets:
      // https://stackoverflow.com/questions/32028437/what-are-bitstream-filters-in-ffmpeg
      if (_ctx.byte_stream_format != H264Format::AnnexB) {
        ret = av_bsf_send_packet(_ctx.bsf_context, pkt);
        if (ret != 0)
          return ret;

        ret = av_bsf_receive_packet(_ctx.bsf_context, pkt);
        if (ret == AVERROR(EAGAIN)) {
          continue;
        } else if (ret < 0)
          return ret;
      }
    } else {
      // Sometimes, there will be frames in the avcoded buffers
      // waiting to be recieved without new packets.
      // In order to flush those frames, we keep sending
      // null packets (as the operations are always one-send-one-recieve).
      pkt = NULL;
    }

    ret = avcodec_send_packet(_ctx.video_codec_context, pkt);
    if (ret < 0 && ret != AVERROR_EOF) {
      return ret;
    }

    ret = avcodec_receive_frame(_ctx.video_codec_context, current_frame);
    if (ret == AVERROR(EAGAIN)) {
      continue;
    } else if (ret == AVERROR_EOF) {
      // avcoded has no more frames, video has reached to the end.
      break;
    } else if (ret < 0) {
      return ret;
    } else if (ret == 0) {
      _last_consumed_frame = idx;

      if (idx == frames[frame_idx]) {
        AVFrame *frame = av_frame_clone(current_frame);
        _frame_list.push_back({.frame = frame, .idx = idx});
        if (++frame_idx == frames.size()) {
          break;
        }
      }
    }
    ++idx;
  }

  av_frame_free(&current_frame);

  if (pkt != NULL)
    av_packet_unref(pkt);

  return 0;
}

int KeyFrameDecoder::populate_intervals(const KeyFrameList &key_frames) {
  if (key_frames.empty())
    return -1;
  if (!_interval_map.empty())
    return -1;

  std::vector<KeyFrame> sorted_frame_list(key_frames);

  std::sort(sorted_frame_list.begin(), sorted_frame_list.end(),
            [&](KeyFrame l, KeyFrame r) { return l.idx < r.idx; });

  // Frame 0 of a valid H264 stream must be a key-frame
  if (sorted_frame_list.front().idx != 0)
    return -1;

  for (auto i = 0; i < sorted_frame_list.size() - 1; ++i) {
    FrameInterval interval = {.start = sorted_frame_list[i],
                              .end = sorted_frame_list[i + 1]};
    _interval_map.push_back(std::make_pair(interval, std::vector<unsigned>()));
  }

  // We add an auxiliary interval to the end of the interval map to cover
  // the frames between the last-key frame in the 'key_frames' and the end
  // of stream. Since we do not know the index of the last frame,
  // we simply assign end of interval to the maximum unsigned value, as
  // decode_interval() excludes 'FrameInterval.end'
  unsigned max_unsigned = std::numeric_limits<unsigned>::max();
  FrameInterval last_interval = {.start = sorted_frame_list.back(),
                                 .end = {.idx = max_unsigned, .base = 0}};
  _interval_map.push_back(
      std::make_pair(last_interval, std::vector<unsigned>()));

  return 0;
}

int KeyFrameDecoder::populate_interval_map(
    const std::vector<unsigned> &frames) {
  if (frames.empty())
    return -1;

  // Operation below assumes both '_interval_map' and 'frames' list are
  // sorted in ascending order.
  unsigned last_idx = 0;
  for (auto &interval : _interval_map) {
    while (frames[last_idx] < interval.first.end.idx) {
      interval.second.push_back(frames[last_idx]);
      if (++last_idx == frames.size())
        return 0;
    }
  }
  return 0;
}

int KeyFrameDecoder::encode_frames(void) {
  int ret;

  if (_frame_list.empty())
    return -1;

  AVFrame *frame = _frame_list[0].frame;

  // In future, we may encode the resulting image with different codecs
  // based on the user input. When that feature is to be implemented,
  // target codecs and pixel formats must be stored in a table. Until then,
  // we hardcode RGB24 as the pixel format when encoding the images, as it
  // is supported by libpng.
  AVPixelFormat dst_format = AV_PIX_FMT_RGB24;
  AVPixelFormat src_format = static_cast<AVPixelFormat>(frame->format);

  if (!_ctx.frame_codec_context) {
    // Initialize frame encoder (PNG for now, may change in the future)
    AVCodec *image_codec = avcodec_find_encoder(AV_CODEC_ID_PNG);
    if (!image_codec)
      return AVERROR_ENCODER_NOT_FOUND;

    _ctx.frame_codec_context = avcodec_alloc_context3(image_codec);
    if (!_ctx.frame_codec_context)
      return AVERROR_EXTERNAL;

    _ctx.frame_codec_context->pix_fmt = dst_format;
    _ctx.frame_codec_context->height = frame->height;
    _ctx.frame_codec_context->width = frame->width;
    _ctx.frame_codec_context->time_base = _fctx.video_stream->time_base;

    ret = avcodec_open2(_ctx.frame_codec_context, image_codec, NULL);
    if (ret < 0)
      return ret;
  }

  AVFrame *dst_frame = av_frame_alloc();
  if (!dst_frame)
    return AVERROR_EXTERNAL;
  if (src_format != dst_format) {
    _ctx.sws_context = sws_getCachedContext(
        _ctx.sws_context, frame->width, frame->height, src_format, frame->width,
        frame->height, dst_format, SWS_BILINEAR, NULL, NULL, NULL);

    dst_frame->format = dst_format;
    dst_frame->width = frame->width;
    dst_frame->height = frame->height;

    ret = av_frame_get_buffer(dst_frame, 0);
    if (ret < 0)
      return ret;
  }

  AVPacket *pkt = av_packet_alloc();
  if (!pkt)
    return AVERROR_EXTERNAL;

  for (const auto &f : _frame_list) {
    // We convert the pixel format of the decoded raw frame to
    // 'dst_format', since the H264 stream is likely to have YUV as pixel
    // format, however, not all image encoders support it.
    if (src_format == dst_format)
      av_frame_ref(dst_frame, f.frame);
    else
      sws_scale(_ctx.sws_context, f.frame->data, f.frame->linesize, 0,
                f.frame->height, dst_frame->data, dst_frame->linesize);

    ret = avcodec_send_frame(_ctx.frame_codec_context, dst_frame);
    if (ret < 0)
      return ret;

    ret = avcodec_receive_packet(_ctx.frame_codec_context, pkt);
    if (ret < 0)
      return ret;

    std::string enc_frame(reinterpret_cast<char *>(pkt->data), pkt->size);

    _enc_frame_list.push_back(enc_frame);

    if (src_format == dst_format)
      av_frame_unref(dst_frame);
  }

  av_packet_unref(pkt);
  av_frame_free(&dst_frame);

  return 0;
}

EncodedFrameList &KeyFrameDecoder::decode(const std::vector<unsigned> &frames) {
  // We perform a cleanup on key-frame decoder's internal structures, in
  // order to avoid processing frames decoded in a previous call to this
  // method.
  clear();

  if (_interval_map.empty())
    throw VCLException(
        FFmpegDecodeFailed,
        error_msg(AVERROR_EXTERNAL, "set_key_frames() is not invoked"));

  int ret = populate_interval_map(frames);
  if (ret != 0)
    throw VCLException(FFmpegDecodeFailed,
                       error_msg(AVERROR_EXTERNAL, "populate_interval_map"));

  for (const auto &interval : _interval_map) {
    if (interval.second.empty())
      continue;

    ret = decode_interval(interval.first.start, interval.first.end,
                          interval.second);
    if (ret != 0)
      throw VCLException(FFmpegDecodeFailed,
                         error_msg(AVERROR_EXTERNAL, "decode_interval"));
  }

  ret = encode_frames();
  if (ret != 0)
    throw VCLException(FFmpegDecodeFailed, error_msg(ret, "encode_frames"));

  return _enc_frame_list;
}
