/**
 * @file   Video.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <algorithm>
#include <fstream>

#include "vcl/Video.h"

using namespace VCL;

/*  *********************** */
/*        CONSTRUCTORS      */
/*  *********************** */

Video::Video()
    : _size({.width = 0, .height = 0, .frame_count = 0}), _fps(0),
      _video_id(""), _flag_stored(true), _codec(Video::Codec::NOCODEC),
      _video_read(nullptr), _remote(nullptr) {}

Video::Video(const std::string &video_id) : Video() {
  _video_id = video_id;
  _remote = nullptr;
}

Video::Video(void *buffer, long size) : Video() {
  std::string uname = create_unique("/tmp/tmp/", "vclvideoblob");
  std::ofstream outfile(uname, std::ofstream::binary);
  _remote = nullptr;

  if (outfile.is_open()) {
    outfile.write((char *)buffer, size);
    outfile.close();
  } else
    throw VCLException(OpenFailed, "Cannot create temporary file");

  _video_id = uname;
}

Video::Video(const Video &video) {
  _video_id = video._video_id;
  _remote = nullptr;

  _size = video._size;

  _fps = video._fps;
  _codec = video._codec;

  _video_id = video.get_video_id();
  _codec = video.get_codec();

  _flag_stored = video._flag_stored;

  //_frames     = video._frames;
  _operations = video._operations;

  _video_read = video._video_read;

  for (const auto &op : video._operations)
    _operations.push_back(op);
}

Video &Video::operator=(Video vid) {
  swap(vid);
  return *this;
}

Video::~Video() {
  _video_read = nullptr;
  _operations.clear();
  _key_frame_decoder.reset();
}

/*  *********************** */
/*        GET FUNCTIONS     */
/*  *********************** */

std::string Video::get_video_id() const { return _video_id; }

Video::Codec Video::get_codec() const { return _codec; }

Image *Video::read_frame(int index) {
  if (_video_read == nullptr) {
    throw VCLException(UnsupportedOperation, "Video file not opened");
  }

  Image *pframe = _video_read->read_frame(index);
  if (pframe == nullptr)
    _video_read = nullptr; // Reaching the end, close the input video
  return pframe;
}

// FIXME video read object is not released correctly.
cv::Mat Video::get_frame(unsigned frame_number) {
  cv::Mat frame;

  if (_key_frame_decoder == nullptr) {
    bool new_read = false;
    std::shared_ptr<Read> video_read;
    //_video_read not initialized, the current function is called directly
    if (_video_read == nullptr) {
      video_read = std::make_shared<Read>(this);
      // open the video file
      (*video_read)(0);
      new_read = true;
    }
    // _video_read initialized, the current function is called by get_frames
    else {
      video_read = _video_read;
    }
    VCL::Image *pframe = video_read->read_frame(frame_number);
    if (new_read) {
      _video_read = nullptr;
    }
    if (pframe == nullptr)
      throw VCLException(OutOfBounds, "Frame requested is out of bounds");

    frame = pframe->get_cvmat();
  } else {

    std::vector<unsigned> frame_list = {frame_number};
    EncodedFrameList list = _key_frame_decoder->decode(frame_list);

    auto &f = list[0];
    VCL::Image tmp((void *)&f[0], f.length());
    frame = tmp.get_cvmat();
  }

  return frame;
}

// FIXME video read object is not released correctly.
std::vector<cv::Mat> Video::get_frames(std::vector<unsigned> frame_list) {
  std::vector<cv::Mat> image_list;

  if (frame_list.size() < 1) {
    return image_list;
  }

  if (_key_frame_decoder == nullptr) {
    // Key frame information is not available: video will be decoded using
    // OpenCV.
    _video_read = std::make_shared<Read>(this);
    // open the video file
    (*_video_read)(0);

    for (const auto &f : frame_list)
      image_list.push_back(get_frame(f));

    _video_read = nullptr;
  } else {
    // Key frame information is set, video will be partially decoded using
    // _key_frame_decoder object.

    EncodedFrameList list = _key_frame_decoder->decode(frame_list);

    for (const auto &f : list) {
      VCL::Image tmp((void *)&f[0], f.length());
      image_list.push_back(tmp.get_cvmat());
    }
  }

  return image_list;
}

long Video::get_frame_count() {
  perform_operations();
  return _size.frame_count;
}

float Video::get_fps() { return _fps; }

cv::Size Video::get_frame_size() {
  perform_operations();
  cv::Size dims((int)_size.width, (int)_size.height);
  return dims;
}

Video::VideoSize Video::get_size() {
  perform_operations();
  return _size;
}

std::vector<unsigned char> Video::get_encoded() {
  if (_flag_stored == false)
    throw VCLException(ObjectEmpty, "Object not written");

  std::ifstream ifile(_video_id, std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  size_t encoded_size = (long)ifile.tellg();
  ifile.seekg(0, std::ios::beg);

  std::vector<unsigned char> encoded(encoded_size);

  ifile.read((char *)encoded.data(), encoded_size);
  ifile.close();

  return encoded;
}

const KeyFrameList &Video::get_key_frame_list() {
  if (_key_frame_list.empty()) {
    VCL::KeyFrameParser parser(_video_id);
    _key_frame_list = parser.parse();
  }

  set_key_frame_list(_key_frame_list);
  return _key_frame_list;
}

/*  *********************** */
/*        SET FUNCTIONS     */
/*  *********************** */

void Video::set_video_id(const std::string &video_id) { _video_id = video_id; }

void Video::set_codec(Video::Codec codec) { _codec = codec; }

void Video::set_dimensions(const cv::Size &dimensions) {
  _size.height = dimensions.height;
  _size.width = dimensions.width;
}

void Video::set_key_frame_list(KeyFrameList &key_frames) {
  if (_key_frame_decoder == nullptr) {
    _key_frame_decoder =
        std::unique_ptr<KeyFrameDecoder>(new VCL::KeyFrameDecoder(_video_id));
  }

  _key_frame_decoder->set_key_frames(key_frames);
}

/*  *********************** */
/*       UTILITIES          */
/*  *********************** */

void Video::perform_operations() {
  try {
    // At this point, there are three different potential callees:
    //
    // - An object is instantiated through the default constructor with
    //   no name: an exception is thrown as no operations can be applied.
    //
    // - An object is instantiated through one-arg string constructor,
    //   but has no operations set explicitely (i.e. when calling
    //   get_frame_count()): a 'read' operation is pushed to the head of
    //   the queue.
    //
    // - An object is instantiated through any of the non-default
    //   constructors, and has pushed operations explicitely: a 'read'
    //   operation is pushed to the head of the queue.
    if (_operations.empty() || _operations.front()->get_type() != READ) {
      //&& !is_read()) {
      if (_video_id.empty())
        throw VCLException(OpenFailed, "video_id is not initialized");
      _operations.push_front(std::make_shared<Read>(this));
    }

    if (_operations.size() == 1) {
      // If only read operation exists, we should add another operation to
      // avoid the useless loop.
      _operations.push_back(
          std::make_shared<Interval>(this, Video::FRAMES, 0, 0, 1));
    }

    for (const auto &op : _operations) {
      if (op == NULL)
        throw VCLException(ObjectEmpty, "Nothing to be done");
    }

    Video::OperationResult res = PASS;
    for (int index = 0; res != BREAK; index++) {
      for (const auto &op : _operations) {
        res = (*op)(index);
        if (res != PASS)
          break;
      }
    }

    for (const auto &op : _operations) {
      op->finalize();
    }
    // FIXME Do we need to clear _operations when some exception happened?
    // Right now, we assume that we should have another try and hence the
    // vector _operations should be kept.
  } catch (cv::Exception &e) {
    throw VCLException(OpenCVError, e.what());
  }

  _operations.clear();
}

void Video::swap(Video &rhs) noexcept {
  using std::swap;

  swap(_video_id, rhs._video_id);
  swap(_flag_stored, rhs._flag_stored);
  // swap(_frames, rhs._frames);
  swap(_size, rhs._size);
  swap(_fps, rhs._fps);
  swap(_codec, rhs._codec);
  swap(_operations, rhs._operations);
  swap(_video_read, rhs._video_read);
}

void Video::set_connection(RemoteConnection *remote) {
  if (!remote->connected())
    remote->start();

  if (!remote->connected()) {
    throw VCLException(SystemNotFound, "No remote connection started");
  }

  _remote = remote;
  _storage = Storage::AWS;
}

/*  *********************** */
/*   VIDEO INTERACTION      */
/*  *********************** */

void Video::resize(int width, int height) {
  _flag_stored = false;
  _operations.push_back(
      std::make_shared<Resize>(this, cv::Size(width, height)));
}

void Video::interval(Video::Unit u, int start, int stop, int step) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Interval>(this, u, start, stop, step));
}

void Video::crop(const Rectangle &rect) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Crop>(this, rect));
}

void Video::threshold(int value) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Threshold>(this, value));
}

void Video::store(const std::string &video_id, Video::Codec video_codec) {
  // out_name cannot be assigned to _video_id here as the read operation
  // may be pending and the input file name is needed for the read.
  _operations.push_back(std::make_shared<Write>(this, video_id, video_codec));
  perform_operations();
}

void Video::store() {
  if (_codec == NOCODEC || _video_id.empty()) {
    throw VCLException(ObjectEmpty, "Cannot write video without codec"
                                    "or ID");
  }
  store(_video_id, _codec);
}

void Video::delete_video() {
  if (exists(_video_id)) {
    std::remove(_video_id.c_str());
  }
}

/*  *********************** */
/*       READ OPERATION    */
/*  *********************** */

Video::Read::~Read() {
  if (_inputVideo.isOpened()) {
    _inputVideo.release();
    _frames.clear();
    _frame_index_starting = 0;
    _frame_index_ending = 0;
    _video_id = "";
  }
}

void Video::Read::finalize() { reset(); }

void Video::Read::open() {
  _video_id = _video->_video_id;
  if (!_inputVideo.open(_video_id)) {
    throw VCLException(OpenFailed, "Could not open the output video for read");
  }

  _video->_fps = static_cast<float>(_inputVideo.get(cv::CAP_PROP_FPS));
  _video->_size.frame_count =
      static_cast<int>(_inputVideo.get(cv::CAP_PROP_FRAME_COUNT));
  _video->_size.width =
      static_cast<int>(_inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
  _video->_size.height =
      static_cast<int>(_inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));

  // Get Codec Type- Int form
  int ex = static_cast<int>(_inputVideo.get(cv::CAP_PROP_FOURCC));
  char fourcc[] = {(char)((ex & 0XFF)), (char)((ex & 0XFF00) >> 8),
                   (char)((ex & 0XFF0000) >> 16),
                   (char)((ex & 0XFF000000) >> 24), 0};

  _video->_codec = read_codec(fourcc);

  _video->_video_read = shared_from_this();
}

void Video::Read::reset() {
  if (_inputVideo.isOpened()) {
    _inputVideo.release();
    _frames.clear();
    _frame_index_starting = 0;
    _frame_index_ending = 0;
    _video_id = "";

    if (_video->_video_read == shared_from_this()) {
      _video->_video_read = nullptr;
    }
  }
}

void Video::Read::reopen() {
  reset();
  open();
}

VCL::Image *Video::Read::read_frame(int index) {
  cv::Mat mat;

  if (!_inputVideo.isOpened()) {
    open();
  }

  if (index < _frame_index_starting) { // Read the video file all over again
    reopen();                          // _frame_index_ending = 0;
    _frame_index_starting = index;
  } else if (index > _frame_index_starting + 30) { // The cached vector is full
    _frames.clear();
    _frame_index_starting = index;
  }

  // Skip the frames that are too "old"
  while (_frame_index_ending < _frame_index_starting) {
    _inputVideo >> mat;
    if (mat.empty())
      return nullptr;
    _frame_index_ending++;
  }

  // Read the frames with indices up to <index>
  while (_frame_index_ending <= index) {
    _inputVideo >> mat;
    if (mat.empty())
      return nullptr;
    _frames.push_back(VCL::Image(mat, false));
    _frame_index_ending++;
  }

  return &_frames[index - _frame_index_starting];
}

Video::Codec Video::Read::read_codec(char *fourcc) {
  std::string codec(fourcc);
  std::transform(codec.begin(), codec.end(), codec.begin(), ::tolower);

  if (codec == "mjpg")
    return Codec::MJPG;
  else if (codec == "xvid")
    return Codec::XVID;
  else if (codec == "u263")
    return Codec::H263;
  else if (codec == "avc1" || codec == "x264")
    return Codec::H264;
  else
    throw VCLException(UnsupportedFormat, codec + " is not supported");
}

Video::OperationResult Video::Read::operator()(int index) {
  // The video object is changed, reset the InputCapture handler.
  if (_video_id != _video->_video_id) {
    _video_id = _video->_video_id;
    reset();
  }

  if (!_inputVideo.isOpened()) {
    open();
  }
  if (_video->_size.frame_count <= index)
    return BREAK;
  return PASS;
}

/*  *********************** */
/*       WRITE OPERATION    */
/*  *********************** */

int Video::Write::get_fourcc() {
  switch (_codec) {
  case Codec::MJPG:
    return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  case Codec::XVID:
    return cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
  case Codec::H263:
    return cv::VideoWriter::fourcc('U', '2', '6', '3');
  case Codec::H264:
    return cv::VideoWriter::fourcc('X', '2', '6', '4');
  case Codec::AVC1:
    return cv::VideoWriter::fourcc('A', 'V', 'C', '1');
  default:
    throw VCLException(UnsupportedFormat,
                       std::to_string((int)_codec) + " is not a valid format");
  }
}

Video::OperationResult Video::Write::operator()(int index) {
  VCL::Image *frame = _video->read_frame(index);
  if (frame == NULL)
    return BREAK;

  if (_last_write == index)
    return PASS;
  else if (_last_write > index) {
    // Write the video file all over again.
    // Probably some exceptions happened before.
    _outputVideo.release();
    _last_write = -1;
  }

  if (!_outputVideo.isOpened()) {
    _outputVideo.open(_outname, get_fourcc(), _video->_fps,
                      cv::Size(_video->_size.width, _video->_size.height));

    if (!_outputVideo.isOpened()) {
      throw VCLException(OpenFailed,
                         "Could not open the output video for write");
    }
  }

  _outputVideo << frame->get_cvmat(false);
  _frame_count++;
  _last_write = index;
  return PASS;
}

void Video::Write::finalize() {
  if (_video->_storage == Storage::LOCAL) {
    if (!_outputVideo.isOpened()) {
      _outputVideo.release();

      _video->_video_id = _outname;
      _video->_codec = _codec;
      _video->_flag_stored = true;
      _video->_size.frame_count = _frame_count;
    }
  }
}

Video::Write::~Write() { finalize(); }

/*  *********************** */
/*       RESIZE OPERATION   */
/*  *********************** */

Video::OperationResult Video::Resize::operator()(int index) {
  VCL::Image *frame = _video->read_frame(index);
  if (frame == NULL)
    return BREAK;
  // VCL::Image expect the params (h,w) (contrary to openCV convention)
  frame->resize(_size.height, _size.width);
  _video->_size.width = _size.width;
  _video->_size.height = _size.height;
  return PASS;
}

/*  *********************** */
/*       CROP OPERATION     */
/*  *********************** */

Video::OperationResult Video::Crop::operator()(int index) {
  VCL::Image *frame = _video->read_frame(index);
  if (frame == NULL)
    return BREAK;
  frame->crop(_rect);
  _video->_size.width = _rect.width;
  _video->_size.height = _rect.height;
  return PASS;
}

/*  *********************** */
/*    THRESHOLD OPERATION   */
/*  *********************** */

Video::OperationResult Video::Threshold::operator()(int index) {
  VCL::Image *frame = _video->read_frame(index);
  if (frame == NULL)
    return BREAK;
  frame->threshold(_threshold);
  return PASS;
}

/*  *********************** */
/*   INTERVAL Operation     */
/*  *********************** */

Video::OperationResult Video::Interval::operator()(int index) {
  if (_u != Video::Unit::FRAMES) {
    _fps_updated = false;
    throw VCLException(UnsupportedOperation,
                       "Only Unit::FRAMES supported for interval operation");
  }

  unsigned nframes = _video->_size.frame_count;

  if (_start >= nframes) {
    _fps_updated = false;
    throw VCLException(SizeMismatch,
                       "Start Frame cannot be greater than number of frames");
  }

  if (_stop >= nframes) {
    _fps_updated = false;
    throw VCLException(SizeMismatch,
                       "End Frame cannot be greater than number of frames");
  }

  if (index < _start)
    return CONTINUE;
  if (index >= _stop)
    return BREAK;
  if ((index - _start) % _step != 0)
    return CONTINUE;
  update_fps();
  return PASS;
}

void Video::Interval::update_fps() {
  if (!_fps_updated) {
    _video->_fps /= _step;
    _fps_updated = true;
  }
}
