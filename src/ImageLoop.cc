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
#include <curl/curl.h>

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
        if (response != 0) {
          r_enqueue(img);
          flag = 1;
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

size_t writeCallback(char *ip, size_t size, size_t nmemb, void *op) {
  ((std::string *)op)->append((char *)ip, size * nmemb);
  return size * nmemb;
}

cv::Mat write_image(std::string readBuffer) {
  std::vector<unsigned char> vectordata(readBuffer.begin(), readBuffer.end());
  cv::Mat data_mat(vectordata, true);
  cv::Mat decoded_mat(cv::imdecode(data_mat, 1));
  return decoded_mat;
}

CURL *ImageLoop::get_easy_handle(VCL::Image *img, std::string &readBuffer) {
  CURL *curl = NULL;
  CURLcode res;
  struct curl_slist *headers = NULL;
  curl_mime *form = NULL;
  curl_mimepart *field = NULL;

  Json::Value rParams = img->get_remoteOp_params();
  std::string url = rParams["url"].toStyledString().data();
  url.erase(std::remove(url.begin(), url.end(), '\n'), url.end());
  url = url.substr(1, url.size() - 2);
  Json::Value options = rParams["options"];

  curl = curl_easy_init();

  if (curl) {
    std::string imageId = img->get_image_id().data();
    form = curl_mime_init(curl);

    auto time_now = std::chrono::system_clock::now();
    std::chrono::duration<double> utc_time = time_now.time_since_epoch();

    VCL::Image::Format img_format = img->get_image_format();
    std::string format = img->format_to_string(img_format);

    if (format == "" && options.isMember("format")) {
      format = options["format"].toStyledString().data();
      format.erase(std::remove(format.begin(), format.end(), '\n'),
                   format.end());
      format = format.substr(1, format.size() - 2);
    } else {
      format = "jpg";
    }

    std::string filePath =
        "/tmp/tempfile" + std::to_string(utc_time.count()) + "." + format;
    cv::imwrite(filePath, img->get_cvmat(false, false));
    _tempfiles.push_back(filePath);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "imageData");
    curl_mime_filedata(field, filePath.data());

    field = curl_mime_addpart(form);
    curl_mime_name(field, "jsonData");
    curl_mime_data(field, options.toStyledString().data(),
                   options.toStyledString().length());

    // Post data
    url = url + "?id=" + imageId;
    curl_easy_setopt(curl, CURLOPT_URL, url.data());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);

    return curl;
  }

  return NULL;
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
  int flag = 0;
  int start_index = 0;
  int step = 10;
  int end_index = readBuffer.size() > step ? step : readBuffer.size();
  std::vector<std::string> responseBuffer(readBuffer.size());
  int rindex = 0;
  std::vector<std::string> redoBuffer;
  std::vector<VCL::Image *> pendingImages;
  while (start_index != readBuffer.size()) {
    CURLM *multi_handle;
    CURLMsg *msg = NULL;
    CURL *eh = NULL;
    CURLcode return_code;
    int still_running = 0, i = 0, msgs_left = 0;
    int http_status_code;
    char *szUrl;

    multi_handle = curl_multi_init();

    auto start = readBuffer.begin() + start_index;
    auto end = readBuffer.begin() + end_index;

    std::vector<VCL::Image *> tempBuffer(start, end);

    for (VCL::Image *img : tempBuffer) {
      CURL *curl = get_easy_handle(img, responseBuffer[rindex]);
      rindex++;
      curl_multi_add_handle(multi_handle, curl);
    }

    do {
      CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
      if (still_running)
        mc = curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);

      if (mc) {
        break;
      }
    } while (still_running);

    while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
      if (msg->msg == CURLMSG_DONE) {
        eh = msg->easy_handle;

        return_code = msg->data.result;

        // Get HTTP status code
        szUrl = NULL;
        long rsize = 0;

        curl_easy_getinfo(eh, CURLINFO_RESPONSE_CODE, &http_status_code);
        curl_easy_getinfo(eh, CURLINFO_EFFECTIVE_URL, &szUrl);
        curl_easy_getinfo(eh, CURLINFO_REQUEST_SIZE, &rsize);

        if (http_status_code != 200) {
          std::string delimiter = "=";

          char *p = std::strtok(szUrl, delimiter.data());
          p = std::strtok(NULL, delimiter.data());

          std::string id(p);
          redoBuffer.push_back(id);
        }

        curl_multi_remove_handle(multi_handle, eh);
        curl_easy_cleanup(eh);
      } else {
        fprintf(stderr, "error: after curl_multi_info_read(), CURLMsg=%d\n",
                msg->msg);
      }
    }

    tempBuffer.clear();
    start_index = end_index;
    end_index = readBuffer.size() > (end_index + step) ? (end_index + step)
                                                       : readBuffer.size();
  }
  rindex = -1;
  for (VCL::Image *img : readBuffer) {
    rindex++;
    if (std::find(redoBuffer.begin(), redoBuffer.end(),
                  img->get_image_id().data()) != redoBuffer.end()) {
      pendingImages.push_back(img);
      continue;
    }
    int rthresh = 0;
    auto t_start = std::chrono::high_resolution_clock::now();
    bool rflag = false;
    while (responseBuffer[rindex].size() == 0) {
      continue;
    }
    cv::Mat dmat = write_image(responseBuffer[rindex]);
    if (dmat.empty()) {
      pendingImages.push_back(img);
    }
    img->shallow_copy_cv(dmat);
    img->update_op_completed();
    auto const result = imageMap.insert(
        std::pair<std::string, VCL::Image *>(img->get_image_id(), img));
    if (not result.second) {
      result.first->second = img;
    }
    if (rindex == readBuffer.size() - 1 && pendingImages.size() == 0) {
      _remote_running = false;
    }
    enqueue(img);
  }
  readBuffer.clear();
  std::swap(readBuffer, pendingImages);
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