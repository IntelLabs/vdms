/**
 * @file   KeyFrameParser.h
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

#pragma once

#include <string>
#include <vector>

#include "Exception.h"

extern "C" {
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
}

namespace VCL {

struct KeyFrame {
  unsigned idx;
  int64_t base;
};

typedef std::vector<KeyFrame> KeyFrameList;
typedef std::vector<std::string> EncodedFrameList;

class KeyFrameOp {
protected:
  struct FormatContext {
    AVFormatContext *fmt_context;

    // For now, we only process videos with a SINGLE video stream.
    AVStream *video_stream;    // Pointer to the video stream in the ctx
    unsigned video_stream_idx; // Index to the video stream in the ctx
  };

  FormatContext _fctx;
  std::string _filename;

  int64_t _time_base;
  int64_t _nb_frames;

  int init_stream(void);
  std::string error_msg(int errnum, const std::string &opt = "");

public:
  KeyFrameOp(std::string filename);
  virtual ~KeyFrameOp();
};

/*  *********************** */
/*    KEY_FRAME_PARSER      */
/*  *********************** */
class KeyFrameParser : public KeyFrameOp {
private:
  KeyFrameList _frame_list;

  int fill_frame_list(void) noexcept;

public:
  KeyFrameParser(std::string filename) : KeyFrameOp(filename){};
  ~KeyFrameParser() override{};

  const KeyFrameList &parse(void);
};

/*  *********************** */
/*    KEY_FRAME_DECODER     */
/*  *********************** */
class KeyFrameDecoder : public KeyFrameOp {
private:
  enum class H264Format { AVCC = 0, AnnexB = 1 };

  struct DecoderContext {
    AVBSFContext *bsf_context;
    AVCodecContext *video_codec_context;
    AVCodecContext *frame_codec_context;
    SwsContext *sws_context;
    H264Format byte_stream_format;

    DecoderContext()
        : bsf_context(NULL), video_codec_context(NULL),
          frame_codec_context(NULL), sws_context(NULL),
          byte_stream_format(H264Format::AVCC){};
  };

  struct FrameInterval {
    KeyFrame start;
    KeyFrame end;
  };

  struct DecodedFrame {
    AVFrame *frame;
    unsigned idx;
  };

  std::vector<std::pair<FrameInterval, std::vector<unsigned>>> _interval_map;
  std::vector<DecodedFrame> _frame_list;
  EncodedFrameList _enc_frame_list;
  DecoderContext _ctx;
  int64_t _last_consumed_frame;

  int init_decoder(void) noexcept;
  int init_bsf(void) noexcept;
  void clear(void);

  int decode_interval(const KeyFrame &start, const KeyFrame &end,
                      const std::vector<unsigned> &frames);
  int populate_intervals(const KeyFrameList &key_frames);
  int populate_interval_map(const std::vector<unsigned> &frames);
  int encode_frames(void);

public:
  KeyFrameDecoder(std::string filename);
  ~KeyFrameDecoder() override;

  void set_key_frames(const KeyFrameList &key_frames);
  EncodedFrameList &decode(const std::vector<unsigned> &frames);
};
} // namespace VCL
