/**
 * @file   Video.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 Intel Corporation
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

#include "../VDMSConfig.h"
#include "VDMSConfigHelper.h"
#include "vcl/Video.h"

using namespace VCL;

/*  *********************** */
/*        CONSTRUCTORS      */
/*  *********************** */

Video::Video()
    : _size({.width = 0, .height = 0, .frame_count = 0}), _fps(0),
      _video_id(""), _flag_stored(true), _codec(Video::Codec::NOCODEC),
      _remote(nullptr) {}

Video::Video(const std::string &video_id, bool no_blob) : Video() {
  _video_id = video_id;
  _remote = nullptr;

  _no_blob = no_blob;

  initialize_video_attributes(_video_id);
}

Video::Video(void *buffer, long size) : Video() {
  std::string uname = create_unique(
      VDMS::VDMSConfig::instance()->get_path_tmp(), "vclvideoblob");
  std::ofstream outfile(uname, std::ofstream::binary);
  _remote = nullptr;

  if (outfile.is_open()) {
    outfile.write((char *)buffer, size);
    outfile.close();
  } else
    throw VCLException(OpenFailed, "Cannot create temporary file");

  _video_id = uname;

  initialize_video_attributes(_video_id);
}

Video::Video(const Video &video) {
  _video_id = video._video_id;
  _remote = nullptr;

  _no_blob = video._no_blob;

  _size = video._size;

  _fps = video._fps;
  _codec = video._codec;

  _video_id = video.get_video_id();
  _codec = video.get_codec();

  _flag_stored = video._flag_stored;

  _operations = video._operations;

  _operated_video_id = video._operated_video_id;

  remoteOp_params = video.remoteOp_params;

  _query_error_response = video._query_error_response;
}

Video &Video::operator=(Video vid) {
  swap(vid);
  return *this;
}

Video::~Video() {
  _operations.clear();
  _key_frame_decoder.reset();
}

/*  *********************** */
/*        GET FUNCTIONS     */
/*  *********************** */

bool Video::is_blob_not_stored() const { return _no_blob; }

std::string Video::get_video_id() const { return _video_id; }

Video::Codec Video::get_codec() const { return _codec; }

cv::Mat Video::get_frame(unsigned frame_number, bool performOp) {
  cv::Mat frame;

  if (_key_frame_decoder == nullptr) {
    if (performOp)
      perform_operations();
    if (frame_number >= _size.frame_count)
      throw VCLException(OutOfBounds, "Frame requested is out of bounds");

    cv::VideoCapture inputVideo(_video_id);

    // Set the index of the frame to be read
    if (!inputVideo.set(cv::CAP_PROP_POS_FRAMES, frame_number)) {
      throw VCLException(UnsupportedOperation, "Set the frame index failed");
    }

    // Read the frame
    if (!inputVideo.read(frame)) {
      throw VCLException(UnsupportedOperation,
                         "Frame requested cannot be read");
    }

    inputVideo.release();
  } else {

    std::vector<unsigned> frame_list = {frame_number};
    EncodedFrameList list = _key_frame_decoder->decode(frame_list);

    auto &f = list[0];
    VCL::Image tmp((void *)&f[0], f.length());
    frame = tmp.get_cvmat();
  }

  return frame;
}

std::vector<cv::Mat> Video::get_frames(std::vector<unsigned> frame_list) {
  std::vector<cv::Mat> image_list;

  if (frame_list.size() < 1) {
    return image_list;
  }

  if (_key_frame_decoder == nullptr) {
    // Key frame information is not available: video will be decoded using
    // OpenCV.
    for (const auto &f : frame_list)
      image_list.push_back(get_frame(f));
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

long Video::get_frame_count(bool performOp) {
  if (performOp)
    perform_operations();
  return _size.frame_count;
}

float Video::get_fps() { return _fps; }

cv::Size Video::get_frame_size(bool performOp) {
  if (performOp)
    perform_operations();
  cv::Size dims((int)_size.width, (int)_size.height);
  return dims;
}

Video::VideoSize Video::get_size(bool performOp) {
  if (performOp)
    perform_operations();
  return _size;
}

int Video::get_enqueued_operation_count() { return _operations.size(); }

std::vector<unsigned char> Video::get_encoded(std::string container,
                                              VCL::Video::Codec vcl_codec) {

  // Check if the video codec and container are same as the ones requested by
  // the client If not then encode the video with the respective codec/container
  if (_codec != vcl_codec) {
    std::string id = _operated_video_id;

    // Retrieve container from file
    char *s = const_cast<char *>(id.data());
    std::string format = "";
    if (std::strcmp(s, "") == 0) {
      std::string delimiter = ".";
      char *p = std::strtok(s, delimiter.data());
      while (p != NULL) {
        p = std::strtok(NULL, delimiter.data());
        if (p != NULL) {
          format.assign(p, std::strlen(p));
        }
      }
    }

    // Check if container (format) matches client container
    if (format != "" && format != container) {

      cv::VideoCapture inputVideo(_operated_video_id);

      _fps = static_cast<float>(inputVideo.get(cv::CAP_PROP_FPS));
      _size.frame_count =
          static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_COUNT));
      _size.width = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
      _size.height =
          static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));
      int fourcc = get_video_fourcc(vcl_codec);

      auto time_now = std::chrono::system_clock::now();
      std::chrono::duration<double> utc_time = time_now.time_since_epoch();
      std::string fname =
          "tmp/tempfile" + std::to_string(utc_time.count()) + container;

      // check sufficient memory
      bool memory_avail = check_sufficient_memory(_size);
      if (!memory_avail) {
        throw VCLException(UnsupportedOperation,
                           "System out of memory, please retry later");
      }

      cv::VideoWriter outputVideo(fname, fourcc, _fps,
                                  cv::Size(_size.width, _size.height));

      // Write the video with the client codec and container
      while (true) {

        cv::Mat mat_frame;
        inputVideo >> mat_frame; // Read frame

        if (mat_frame.empty())
          break;

        outputVideo << mat_frame;

        mat_frame.release();
      }

      inputVideo.release();
      outputVideo.release();

      if (std::remove(_operated_video_id.data()) != 0) {
        throw VCLException(ObjectEmpty,
                           "Error encountered while removing the file.");
      }
      if (std::rename(fname.data(), _operated_video_id.data()) != 0) {
        throw VCLException(ObjectEmpty,
                           "Error encountered while renaming the file.");
      }
    }
  }

  std::ifstream ifile(_operated_video_id, std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  size_t encoded_size = (long)ifile.tellg();
  ifile.seekg(0, std::ios::beg);

  std::vector<unsigned char> encoded(encoded_size);

  ifile.read((char *)encoded.data(), encoded_size);
  ifile.close();

  if (std::remove(_operated_video_id.data()) != 0) {
    throw VCLException(ObjectEmpty,
                       "Error encountered while removing the file.");
  }

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
std::string Video::get_query_error_response() { return _query_error_response; }

int Video::get_video_fourcc(VCL::Video::Codec _codec) {
  switch (_codec) {
  case VCL::Video::Codec::MJPG:
    return cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  case VCL::Video::Codec::XVID:
    return cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
  case VCL::Video::Codec::H263:
    return cv::VideoWriter::fourcc('U', '2', '6', '3');
  case VCL::Video::Codec::H264:
    return cv::VideoWriter::fourcc('X', '2', '6', '4');
  case VCL::Video::Codec::AVC1:
    return cv::VideoWriter::fourcc('A', 'V', 'C', '1');
  default:
    throw VCLException(UnsupportedFormat,
                       std::to_string((int)_codec) + " is not a valid format");
  }
}

Json::Value Video::get_remoteOp_params() { return remoteOp_params; }

/*  *********************** */
/*        SET FUNCTIONS     */
/*  *********************** */

std::string Video::get_operated_video_id() { return _operated_video_id; }

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

void Video::set_remoteOp_params(Json::Value options, std::string url) {
  remoteOp_params["options"] = options;
  remoteOp_params["url"] = url;
}

void Video::set_operated_video_id(std::string filename) {
  _operated_video_id = filename;
}

/*  *********************** */
/*       UTILITIES          */
/*  *********************** */

bool Video::is_read(void) { return (_size.frame_count > 0); }

void Video::initialize_video_attributes(std::string vname) {
  if (vname == "") {
    return;
  }
  cv::VideoCapture inputVideo(vname);

  _fps = static_cast<float>(inputVideo.get(cv::CAP_PROP_FPS));
  _size.frame_count =
      static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_COUNT));
  _size.width = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
  _size.height = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));

  inputVideo.release();
}

bool Video::check_sufficient_memory(const struct VideoSize &size) {
  SystemStats systemStats;

  int frameSizeB = size.width * size.height * 3; // frame size in bytes
  int videoSizeMb =
      frameSizeB * size.frame_count / (1024 * 1024); // video size in MB

  return systemStats.query_sufficient_memory(videoSizeMb);
}

std::string Video::get_video_format(char *video_id) {
  std::string format = "";
  if (std::strcmp(video_id, "") == 0) {
    std::string delimiter = ".";
    char *p = std::strtok(video_id, delimiter.data());
    while (p != NULL) {
      p = std::strtok(NULL, delimiter.data());
      if (p != NULL) {
        format.assign(p, std::strlen(p));
      }
    }
  } else {
    format = "mp4";
  }

  return format;
}

void Video::set_video_writer_size(int op_count) {
  for (int j = op_count; j < _operations.size(); j++) {
    auto it = std::next(_operations.begin(), j);
    std::shared_ptr<Operation> op = *it;

    if ((*op).get_type() == VCL::Video::OperationType::RESIZE ||
        (*op).get_type() == VCL::Video::OperationType::CROP) {
      cv::Size r_size = (*op).get_video_size();
      _size.width = r_size.width;
      _size.height = r_size.height;
    } else if ((*op).get_type() == VCL::Video::OperationType::INTERVAL ||
               (*op).get_type() ==
                   VCL::Video::OperationType::SYNCREMOTEOPERATION ||
               (*op).get_type() == VCL::Video::OperationType::USEROPERATION ||
               (*op).get_type() == VCL::Video::OperationType::REMOTEOPERATION) {
      break;
    }
  }
}

void Video::store_video_no_operation(std::string id, std::string store_id,
                                     std::string fname) {
  cv::VideoCapture inputVideo(id);

  _fps = static_cast<float>(inputVideo.get(cv::CAP_PROP_FPS));
  _size.frame_count =
      static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_COUNT));
  _size.width = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
  _size.height = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));
  int fourcc = static_cast<int>(inputVideo.get(cv::CAP_PROP_FOURCC));

  if (_codec != NOCODEC) {
    fourcc = get_video_fourcc(_codec);
  }

  cv::VideoWriter outputVideo(fname, fourcc, _fps,
                              cv::Size(_size.width, _size.height));

  // check sufficient memory
  bool memory_avail = check_sufficient_memory(_size);
  if (!memory_avail) {
    throw VCLException(UnsupportedOperation,
                       "System out of memory, please retry later");
  }

  int fcount = 0;
  while (true) {
    fcount++;
    cv::Mat mat_frame;
    inputVideo >> mat_frame;

    if (mat_frame.empty()) {
      break;
    }

    outputVideo << mat_frame;
    mat_frame.release();
  }
  inputVideo.release();
  outputVideo.release();

  if (_video_id.find(VDMS::VDMSConfig::instance()->get_path_tmp()) !=
      std::string::npos) {
    if (std::remove(_video_id.data()) != 0) {
      throw VCLException(ObjectEmpty,
                         "Error encountered while removing the file.");
    }
  }
  _video_id = store_id;
  if (std::rename(fname.data(), _video_id.data()) != 0) {
    throw VCLException(ObjectEmpty,
                       "Error encountered while renaming the file.");
  }
}

int Video::perform_single_frame_operations(std::string id, int op_count,
                                           std::string fname) {
  cv::VideoCapture inputVideo(id);

  _fps = static_cast<float>(inputVideo.get(cv::CAP_PROP_FPS));
  _size.frame_count =
      static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_COUNT));
  _size.width = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
  _size.height = static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));
  int fourcc = static_cast<int>(inputVideo.get(cv::CAP_PROP_FOURCC));

  // Check if Crop or Resize operations are in the pipeline
  // to set the height and width of the VideoWriter object
  set_video_writer_size(op_count);

  // check sufficient memory
  bool memory_avail = check_sufficient_memory(_size);
  if (!memory_avail) {
    throw VCLException(UnsupportedOperation,
                       "System out of memory, please retry later");
  }

  cv::VideoWriter outputVideo(fname, fourcc, _fps,
                              cv::Size(_size.width, _size.height));
  int i = 0;
  while (true) {
    cv::Mat mat_frame;
    inputVideo >> mat_frame; // Read frame

    if (mat_frame.empty()) {
      op_count = i;
      break;
    }

    // Perform operations frame by frame except the ones
    // that work with the complete video
    for (i = op_count; i < _operations.size(); i++) {
      auto it = std::next(_operations.begin(), i);
      std::shared_ptr<Operation> op = *it;

      if ((*op).get_type() != VCL::Video::OperationType::SYNCREMOTEOPERATION &&
          (*op).get_type() != VCL::Video::OperationType::INTERVAL &&
          (*op).get_type() != VCL::Video::OperationType::USEROPERATION &&
          (*op).get_type() != VCL::Video::OperationType::REMOTEOPERATION) {

        (*op)(this, mat_frame);
        if (i == _operations.size() - 1) {
          outputVideo << mat_frame;
        }
      } else {
        outputVideo << mat_frame;
        break;
      }
    }
    mat_frame.release();
  }

  outputVideo.release();
  inputVideo.release();

  return op_count;
}

void Video::perform_operations(bool is_store, std::string store_id) {
  try {
    int op_count = 0;
    std::string v_id = _video_id;
    std::string s_id = store_id;

    // Get the video container format.
    char *s;
    if (is_store) {
      s = const_cast<char *>(s_id.data());
    } else {
      s = const_cast<char *>(v_id.data());
    }
    std::string format = get_video_format(s);

    // Setup temporary files
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<double> utc_time = time_now.time_since_epoch();
    std::string fname = VDMS::VDMSConfig::instance()->get_path_tmp() +
                        "/tempfile" + std::to_string(utc_time.count()) + "." +
                        format;
    std::string id =
        (_operated_video_id == "") ? _video_id : _operated_video_id;

    // Check for existence of the source video file
    try {
      std::ifstream file;
      file.open(id);
      if (file) {
        file.close();
      } else {
        throw VCLException(OpenFailed,
                           "video_id " + id + " could not be opened");
      }
    } catch (Exception e) {
      throw VCLException(OpenFailed, "video_id " + id + " could not be opened");
    }

    if (_operations.size() == 0) {
      // If the call is made with no operations.
      if (is_store) {
        // If called to store a video into the data store as blob
        if (!_no_blob) {
          store_video_no_operation(id, store_id, fname);
        }
      } else {
        _operated_video_id = _video_id;
      }
    } else {
      // If the call is made with operations.
      while (op_count < _operations.size()) {
        time_now = std::chrono::system_clock::now();
        utc_time = time_now.time_since_epoch();
        fname = VDMS::VDMSConfig::instance()->get_path_tmp() + "/tempfile" +
                std::to_string(utc_time.count()) + "." + format;

        op_count = perform_single_frame_operations(id, op_count, fname);

        // Perform the operations that run on the complete video
        // Note: Async Remote Operation is performed by the event loop
        // in the VideoLoop class.
        if (op_count < _operations.size()) {
          cv::Mat mat;
          auto it = std::next(_operations.begin(), op_count);
          std::shared_ptr<Operation> op = *it;
          if ((*op).get_type() !=
              VCL::Video::OperationType::SYNCREMOTEOPERATION) {
            (*op)(this, mat, fname);
          } else if ((*op).get_type() != VCL::Video::OperationType::INTERVAL) {
            (*op)(this, mat, fname);
          } else if ((*op).get_type() !=
                     VCL::Video::OperationType::USEROPERATION) {
            (*op)(this, mat, fname);
          }
          op_count++;
          id = fname;
        }
      }
      if (is_store) {
        if (_video_id.find(VDMS::VDMSConfig::instance()->get_path_tmp()) !=
            std::string::npos) {
          if (std::remove(_video_id.data()) != 0) {
            throw VCLException(ObjectEmpty,
                               "Error encountered while removing the file.");
          }
        }
        if (!_no_blob) {
          if (std::rename(fname.data(), store_id.data()) != 0) {
            throw VCLException(ObjectEmpty,
                               "Error encountered while renaming the file.");
          }
        } else {
          if (std::rename(fname.data(), _video_id.data()) != 0) {
            throw VCLException(ObjectEmpty,
                               "Error encountered while renaming the file.");
          }
        }
      } else {
        _operated_video_id = fname;
      }
    }
  } catch (cv::Exception &e) {
    throw VCLException(OpenCVError, e.what());
  }

  _operations.clear();
}

int Video::execute_operations(bool isRemote) {
  if (isRemote) {
    // Setup the remote operation to be run by the eventloop
    auto it = std::next(_operations.begin(), 0);
    std::shared_ptr<Operation> op = *it;
    cv::Mat mat;
    std::string fname =
        (_operated_video_id == "") ? _video_id : _operated_video_id;
    if ((*op).get_type() == VCL::Video::OperationType::REMOTEOPERATION) {
      try {
        (*op)(this, mat, fname);
        _operations.pop_front();
        if (_query_error_response != NOERRORSTRING) {
          return -1;
        }
        return 0;
      } catch (const std::exception &e) {
        _query_error_response =
            "Undefined exception occured while running remote operation";
        return -1;
      }
    } else {
      _query_error_response = "Bad operation sent.";
      return -1;
    }
  } else {
    // Perform the operations till a remote operation is encountered.
    // The _operations list is updated accordingly
    try {
      std::list<std::shared_ptr<Operation>> curr_operations;
      std::list<std::shared_ptr<Operation>> rem_operations;
      bool op_flag = false;
      for (auto op : _operations) {
        if ((*op).get_type() == VCL::Video::OperationType::REMOTEOPERATION) {
          op_flag = true;
        }
        if (op_flag) {
          rem_operations.push_back(op);
        } else {
          curr_operations.push_back(op);
        }
      }
      std::swap(_operations, curr_operations);
      if (_operations.size() > 0) {
        perform_operations();
      }
      if (_query_error_response != NOERRORSTRING) {
        return -1;
      }
      std::swap(_operations, rem_operations);
      return 0;
    } catch (Exception e) {
      _query_error_response = e.msg;
      return -1;
    }
  }
}

void Video::swap(Video &rhs) noexcept {
  using std::swap;

  swap(_video_id, rhs._video_id);
  swap(_no_blob, rhs._no_blob);
  swap(_flag_stored, rhs._flag_stored);
  swap(_size, rhs._size);
  swap(_fps, rhs._fps);
  swap(_codec, rhs._codec);
  swap(_operations, rhs._operations);
}

void Video::set_query_error_response(std::string response_error) {
  _query_error_response = response_error;
}

void Video::set_connection(RemoteConnection *remote) {
  if (!remote->connected())
    remote->start();

  if (!remote->connected()) {
    throw VCLException(SystemNotFound, "No remote connection started");
  }

  _remote = remote;
  _storage = VDMS::StorageType::AWS;
}

/*  *********************** */
/*   VIDEO INTERACTION      */
/*  *********************** */

void Video::resize(int width, int height) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Resize>(cv::Size(width, height)));
}

void Video::interval(Video::Unit u, int start, int stop, int step) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Interval>(u, start, stop, step));
}

void Video::crop(const Rectangle &rect) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Crop>(rect));
}

void Video::threshold(int value) {
  _flag_stored = false;
  _operations.push_back(std::make_shared<Threshold>(value));
}

void Video::syncremoteOperation(std::string url, Json::Value options) {
  _operations.push_back(std::make_shared<SyncRemoteOperation>(url, options));
}

void Video::remoteOperation(std::string url, Json::Value options) {
  _operations.push_back(std::make_shared<RemoteOperation>(url, options));
}

void Video::userOperation(Json::Value options) {
  _operations.push_back(std::make_shared<UserOperation>(options));
}

void Video::store(const std::string &video_id, Video::Codec video_codec) {
  perform_operations(true, video_id);
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
/*       RESIZE OPERATION   */
/*  *********************** */

void Video::Resize::operator()(Video *video, cv::Mat &frame, std::string args) {
  try {
    cv::resize(frame, frame, cv::Size(_size.width, _size.height),
               cv::INTER_LINEAR);

    video->_size.width = _size.width;
    video->_size.height = _size.height;
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*       CROP OPERATION     */
/*  *********************** */

void Video::Crop::operator()(Video *video, cv::Mat &frame, std::string args) {
  try {
    frame = frame(_rect);

    video->_size.width = _rect.width;
    video->_size.height = _rect.height;
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    THRESHOLD OPERATION   */
/*  *********************** */

void Video::Threshold::operator()(Video *video, cv::Mat &frame,
                                  std::string args) {
  try {
    cv::threshold(frame, frame, _threshold, _threshold, cv::THRESH_TOZERO);
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*   INTERVAL Operation     */
/*  *********************** */

void Video::Interval::operator()(Video *video, cv::Mat &frame,
                                 std::string args) {
  try {
    int nframes = video->get_frame_count(false);

    if (_start >= nframes)
      throw VCLException(SizeMismatch,
                         "Start Frame cannot be greater than number of frames");

    if (_stop >= nframes)
      throw VCLException(SizeMismatch,
                         "End Frame cannot be greater than number of frames");

    std::string fname = args;
    char *s = const_cast<char *>(args.data());
    std::string format = "";
    if (fname != "") {
      std::string delimiter = ".";
      char *p = std::strtok(s, delimiter.data());
      while (p != NULL) {
        p = std::strtok(NULL, delimiter.data());
        if (p != NULL) {
          format.assign(p, std::strlen(p));
        }
      }
    } else {
      throw VCLException(ObjectNotFound, "Video file not available");
    }
    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<double> utc_time = time_now.time_since_epoch();
    std::string tmp_fname = VDMS::VDMSConfig::instance()->get_path_tmp() +
                            "/tempfile_interval" +
                            std::to_string(utc_time.count()) + "." + format;

    cv::VideoCapture inputVideo(fname);

    video->_fps /= _step;
    video->_size.frame_count =
        static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_COUNT));
    video->_size.width =
        static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_WIDTH));
    video->_size.height =
        static_cast<int>(inputVideo.get(cv::CAP_PROP_FRAME_HEIGHT));
    int fourcc = static_cast<int>(inputVideo.get(cv::CAP_PROP_FOURCC));

    // check sufficient memory
    bool memory_avail = video->check_sufficient_memory(video->_size);
    if (!memory_avail) {
      throw VCLException(UnsupportedOperation,
                         "System out of memory, please retry later");
    }
    cv::VideoWriter outputVideo(
        tmp_fname, fourcc, video->_fps,
        cv::Size(video->_size.width, video->_size.height));

    int frame_number = 0;
    int last_frame_written = 0;
    while (true) {
      cv::Mat mat_frame;
      inputVideo >> mat_frame; // Read frame
      frame_number++;

      if (mat_frame.empty())
        break;

      if (frame_number >= _start && frame_number < _stop) {
        if (last_frame_written == 0) {
          outputVideo << mat_frame;
          last_frame_written = frame_number;
        } else {
          if ((frame_number - last_frame_written) == _step) {
            outputVideo << mat_frame;
            last_frame_written = frame_number;
          }
        }
      }

      if (frame_number > _stop) {
        break;
      }
    }

    outputVideo.release();
    inputVideo.release();

    if (std::remove(fname.data()) != 0) {
      throw VCLException(ObjectEmpty,
                         "Error encountered while removing the file.");
    }
    if (std::rename(tmp_fname.data(), fname.data()) != 0) {
      throw VCLException(ObjectEmpty,
                         "Error encountered while renaming the file.");
    }
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    SYNCREMOTE OPERATION  */
/*  *********************** */

// Reads the file sent from the remote server and saves locally
static size_t videoCallback(void *ptr, size_t size, size_t nmemb,
                            void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

void Video::SyncRemoteOperation::operator()(Video *video, cv::Mat &frame,
                                            std::string args) {
  try {
    int frame_count = video->get_frame_count(false);
    if (frame_count > 0) {
      std::string fname = args;

      CURL *curl = NULL;

      CURLcode res;
      struct curl_slist *headers = NULL;
      curl_mime *form = NULL;
      curl_mimepart *field = NULL;

      curl = curl_easy_init();

      if (curl) {

        form = curl_mime_init(curl);

        field = curl_mime_addpart(form);
        curl_mime_name(field, "videoData");
        if (curl_mime_filedata(field, fname.data()) != CURLE_OK) {
          throw VCLException(ObjectEmpty,
                             "Unable to retrieve local file for remoting");
        }

        field = curl_mime_addpart(form);
        curl_mime_name(field, "jsonData");
        if (curl_mime_data(field, _options.toStyledString().data(),
                           _options.toStyledString().length()) != CURLE_OK) {
          throw VCLException(
              ObjectEmpty, "Unable to create curl mime data for client params");
        }

        // Post data
        std::string format = "";
        char *s = const_cast<char *>(args.data());
        if (fname != "") {
          std::string delimiter = ".";
          char *p = std::strtok(s, delimiter.data());
          while (p != NULL) {
            p = std::strtok(NULL, delimiter.data());
            if (p != NULL) {
              format.assign(p, std::strlen(p));
            }
          }
        } else {
          throw VCLException(ObjectNotFound, "Video file not available");
        }

        auto time_now = std::chrono::system_clock::now();
        std::chrono::duration<double> utc_time = time_now.time_since_epoch();
        std::string response_filepath =
            VDMS::VDMSConfig::instance()->get_path_tmp() + "/rtempfile" +
            std::to_string(utc_time.count()) + "." + format;
        FILE *response_file = fopen(response_filepath.data(), "wb");

        if (curl_easy_setopt(curl, CURLOPT_URL, _url.data()) != CURLE_OK) {
          throw VCLException(UndefinedException, "CURL setup error with URL");
        }
        if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, videoCallback) !=
            CURLE_OK) {
          throw VCLException(UndefinedException,
                             "CURL setup error with callback");
        }

        if (response_file) {
          if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_file) !=
              CURLE_OK) {
            throw VCLException(UndefinedException,
                               "CURL setup error callback response file");
          }
          if (curl_easy_setopt(curl, CURLOPT_MIMEPOST, form) != CURLE_OK) {
            throw VCLException(UndefinedException,
                               "CURL setup error with form");
          }
          curl_easy_perform(curl);
          fclose(response_file);
        }

        int http_status_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);

        curl_easy_cleanup(curl);
        curl_mime_free(form);

        // Throw exceptions for different error codes received from the remote
        // server
        if (http_status_code != 200) {
          if (http_status_code == 0) {
            throw VCLException(ObjectEmpty, "Remote server is not running.");
          }
          if (http_status_code == 400) {
            throw VCLException(ObjectEmpty,
                               "Invalid Request to the Remote Server.");
          } else if (http_status_code == 404) {
            throw VCLException(ObjectEmpty,
                               "Invalid URL Request. Please check the URL.");
          } else if (http_status_code == 500) {
            throw VCLException(ObjectEmpty,
                               "Exception occurred at the remote server. "
                               "Please check your query.");
          } else if (http_status_code == 503) {
            throw VCLException(ObjectEmpty, "Unable to reach remote server");
          } else {
            throw VCLException(ObjectEmpty, "Remote Server error.");
          }
        }

        if (std::remove(fname.data()) != 0) {
          throw VCLException(ObjectEmpty,
                             "Error encountered while removing the file.");
        }
        if (std::rename(response_filepath.data(), fname.data()) != 0) {
          throw VCLException(ObjectEmpty,
                             "Error encountered while renaming the file.");
        }
      }
    } else
      throw VCLException(ObjectEmpty, "Video object is empty");
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    REMOTE OPERATION      */
/*  *********************** */
void Video::RemoteOperation::operator()(Video *video, cv::Mat &frame,
                                        std::string args) {
  try {
    video->set_remoteOp_params(_options, _url);
    if (video->get_operated_video_id() == "") {
      video->set_operated_video_id(video->get_video_id());
    }
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  ************************* */
/*    USER DEFINED OPERATION  */
/*  ************************* */
void Video::UserOperation::operator()(Video *video, cv::Mat &frame,
                                      std::string args) {
  try {
    int frame_count = video->get_frame_count(false);
    if (frame_count > 0) {

      std::string fname = args;
      std::string opfile;

      zmq::context_t context(1);
      zmq::socket_t socket(context, zmq::socket_type::req);

      std::string port = _options["port"].asString();
      std::string address = "tcp://127.0.0.1:" + port;

      socket.connect(address.data());

      _options["ipfile"] = fname;

      std::string message_to_send = _options.toStyledString();

      int message_len = message_to_send.length();
      zmq::message_t ipfile(message_len);
      memcpy(ipfile.data(), message_to_send.data(), message_len);

      socket.send(ipfile, 0);

      // Wait for a response from the UDF process
      while (true) {
        char buffer[256];
        int size = socket.recv(buffer, 255, 0);

        buffer[size] = '\0';
        opfile = buffer;

        break;
      }

      std::ifstream rfile;
      rfile.open(opfile);

      if (rfile) {
        rfile.close();
      } else {
        if (std::remove(opfile.data()) != 0) {
        }
        throw VCLException(OpenFailed, "UDF Error");
      }

      if (std::remove(fname.data()) != 0) {
        throw VCLException(ObjectEmpty,
                           "Error encountered while removing the file.");
      }
      if (std::rename(opfile.data(), fname.data()) != 0) {
        throw VCLException(ObjectEmpty,
                           "Error encountered while renaming the file.");
      }

    } else
      throw VCLException(ObjectEmpty, "Image object is empty");
  } catch (VCL::Exception e) {
    video->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}
