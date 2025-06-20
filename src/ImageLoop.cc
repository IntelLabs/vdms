/**
 * @file   ImageLoop.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 Intel Corporation
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

#include "ImageLoop.h"

#include "VDMSConfig.h"

ImageLoop::~ImageLoop() noexcept {
  VCL::Image img(imageMap.begin()->first);
  m_running = false;
  r_running = false;
  destroyed = true;

  enqueue(&img);
  m_thread.join();

  r_enqueue(&img);
  r_thread.join();
}

bool ImageLoop::is_loop_running() {
  if (m_running || r_running) {
    return true;
  } else {
    return false;
  }
}

void ImageLoop::close_no_operation_loop(std::string imageid) {
  VCL::Image img(imageid);
  auto const result =
      imageMap.insert(std::pair<std::string, VCL::Image *>(imageid, &img));
  if (not result.second) {
    result.first->second = &img;
  }
}

void ImageLoop::set_nrof_entities(int nrof_entities) {
  _nrof_entities = nrof_entities;
}

void ImageLoop::enqueue(VCL::Image *img) noexcept {
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_writeBuffer.push_back(new VCL::Image(*img));
  }
  m_condVar.notify_one();
}

void ImageLoop::r_enqueue(VCL::Image *img) noexcept {
  {
    std::lock_guard<std::mutex> guard(r_mutex);
    r_writeBuffer.push_back(new VCL::Image(*img));
  }
  r_condVar.notify_one();
}

std::map<std::string, VCL::Image *> ImageLoop::get_image_map() {
  return imageMap;
}

void ImageLoop::operationThread() noexcept {
  std::vector<VCL::Image *> readBuffer;

  while (m_running) {
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_condVar.wait(lock, [this] { return !m_writeBuffer.empty(); });
      readBuffer.swap(m_writeBuffer);
    }
    int flag = 0;
    for (VCL::Image *img : readBuffer) {
      int enqueued_operations = img->get_enqueued_operation_count();

      for (int i = img->get_op_completed(); i < enqueued_operations; i++) {
        int response = img->execute_operation();
        if (response == -1) {
          // Remote operation encountered. Enqueue to remote thread
          r_enqueue(img);
          flag = 1;
          break;
        } else if (response == -2) {
          // Exception thrown. Terminate eventloop.
          auto const result = imageMap.insert(
              std::pair<std::string, VCL::Image *>(img->get_image_id(), img));
          if (not result.second) {
            result.first->second = img;
          }
          _remote_running = false;
          flag = 0;
          m_writeBuffer.clear();
          r_writeBuffer.clear();
          m_running = false;
          r_running = false;
          break;

        } else {
          auto const result = imageMap.insert(
              std::pair<std::string, VCL::Image *>(img->get_image_id(), img));
          if (not result.second) {
            result.first->second = img;
          }
        }
      }
    }
    readBuffer.clear();
    if (flag == 0 && _remote_running == false && m_writeBuffer.size() == 0 &&
        r_writeBuffer.size() == 0) {
      m_running = false;
      r_running = false;
    }
  }
}

void clear_temp_files(std::vector<std::string> tempfiles) {
  for (std::string fPath : tempfiles) {
    if (std::remove(fPath.data()) != 0) {
      continue;
    }
  }
}

void ImageLoop::execute_remote_operations(
    std::vector<VCL::Image *> &readBuffer) {
  try {
    std::map<std::string, std::string> input_paths;
    std::map<std::string, std::string> output_paths;
    std::map<std::string, std::string> input_metadata;
    std::map<std::string, std::string> output_metadata;

    std::string url;

    for (VCL::Image *img : readBuffer) {
      auto time_now = std::chrono::system_clock::now();
      std::chrono::duration<double> utc_time = time_now.time_since_epoch();

      Json::Value rParams = img->get_remoteOp_params();
      url = rParams["url"].toStyledString().data();
      url.erase(std::remove(url.begin(), url.end(), '\n'), url.end());
      url = url.substr(1, url.size() - 2);

      Json::Value options = rParams["options"];
      Json::StreamWriterBuilder builder;
      std::string output = Json::writeString(builder, options);

      VCL::Format img_format = img->get_image_format();
      std::string format = VCL::format_to_string(img_format);

      if (format == "" && options.isMember("format")) {
        format = options["format"].toStyledString().data();
        format.erase(std::remove(format.begin(), format.end(), '\n'),
                    format.end());
        format = format.substr(1, format.size() - 2);
      } else {
        format = "jpg";
      }

      std::string filePath = VDMS::VDMSConfig::instance()->get_path_tmp() +
                            "/tempfile" + std::to_string(utc_time.count()) +
                            "." + format;
      cv::imwrite(filePath, img->get_cvmat(false, false));
      // _tempfiles.push_back(filePath);

      int fd = open(filePath.c_str(), O_RDONLY);
      if (fd != -1) {
          fsync(fd);
          close(fd);
      }

      std::string imageId = img->get_image_id().data();

      input_paths[imageId] = filePath;
      output_paths[imageId] = filePath;
      input_metadata[imageId] = output;
    }
    GRPCEntityClient client(grpc::CreateChannel(url.data(), grpc::InsecureChannelCredentials()));
    client.ProcessEntities(input_paths, output_paths, input_metadata, output_metadata);

    for (VCL::Image *img : readBuffer) {
      std::string imageId = img->get_image_id().data();
      cv::Mat dmat = cv::imread(output_paths[imageId], cv::IMREAD_ANYCOLOR);
      if (dmat.rows == 0 || dmat.cols == 0) {
        throw VCLException(ObjectEmpty,
                           "Invalid response from the remote server.");
      }

      img->shallow_copy_cv(dmat);
      img->update_op_completed();

      auto const result = imageMap.insert(
          std::pair<std::string, VCL::Image *>(img->get_image_id(), img));
      if (not result.second) {
        result.first->second = img;
      }
      enqueue(img);
    }
    _remote_running = false;
    readBuffer.clear();
  } catch (VCL::Exception e) {
    VCL::Image *img = readBuffer[0];
    img->set_query_error_response(e.msg);
    auto const result = imageMap.insert(
        std::pair<std::string, VCL::Image *>(img->get_image_id(), img));
    if (not result.second) {
      result.first->second = img;
    }
    readBuffer.clear();
    print_exception(e);
    _remote_running = false;
    m_writeBuffer.clear();
    r_writeBuffer.clear();
    m_running = false;
    r_running = false;
    return;
  }
}

void ImageLoop::remoteOperationThread() noexcept {
  std::vector<VCL::Image *> readBuffer;

  while (r_running) {
    {
      std::unique_lock<std::mutex> rlock(r_mutex);
      r_condVar.wait(rlock, [this] { return !r_writeBuffer.empty(); });
      if (r_writeBuffer.size() == _nrof_entities) {
        std::swap(readBuffer, r_writeBuffer);
      }
    }

    if (readBuffer.size() == _nrof_entities && destroyed == false) {
      _remote_running = true;
      while (readBuffer.size() > 0) {
        execute_remote_operations(readBuffer);
      }
      clear_temp_files(_tempfiles);
      _remote_running = false;
    }
  }
}