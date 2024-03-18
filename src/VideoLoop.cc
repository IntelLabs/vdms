#include "VideoLoop.h"
#include "vcl/Exception.h"
#include <curl/curl.h>

#include "VDMSConfig.h"

VideoLoop::~VideoLoop() noexcept {
  VCL::Video video(videoMap.begin()->first);
  m_running = false;
  r_running = false;
  destroyed = true;

  enqueue(video);
  m_thread.join();

  r_enqueue(video);
  r_thread.join();
}

bool VideoLoop::is_loop_running() {
  if (m_running || r_running) {
    return true;
  } else {
    return false;
  }
}

void VideoLoop::close_no_operation_loop(std::string videoid) {
  VCL::Video video(videoid);
  auto const result =
      videoMap.insert(std::pair<std::string, VCL::Video>(videoid, video));
  if (not result.second) {
    result.first->second = video;
  }
}

void VideoLoop::set_nrof_entities(int nrof_entities) {
  _nrof_entities = nrof_entities;
}

void VideoLoop::enqueue(VCL::Video video) noexcept {
  {
    std::lock_guard<std::mutex> guard(m_mutex);
    m_writeBuffer.push_back(video);
  }
  m_condVar.notify_one();
}

void VideoLoop::r_enqueue(VCL::Video video) noexcept {
  {
    std::lock_guard<std::mutex> guard(r_mutex);
    r_writeBuffer.push_back(video);
  }
  r_condVar.notify_one();
}

std::map<std::string, VCL::Video> VideoLoop::get_video_map() {
  return videoMap;
}

void VideoLoop::operationThread() noexcept {
  std::vector<VCL::Video> readBuffer;

  while (m_running) {
    {
      std::unique_lock<std::mutex> lock(m_mutex);
      m_condVar.wait(lock, [this] { return !m_writeBuffer.empty(); });
      readBuffer.swap(m_writeBuffer);
    }
    int flag = 0;
    for (VCL::Video video : readBuffer) {
      // Execute operations on the video
      int response = video.execute_operations();

      if (response == -1) {
        // An exception occured while executing the operations
        // Terminate the eventloop
        auto const result = videoMap.insert(
            std::pair<std::string, VCL::Video>(video.get_video_id(), video));
        if (not result.second) {
          result.first->second = video;
        }
        _remote_running = false;
        flag = 0;
        m_writeBuffer.clear();
        r_writeBuffer.clear();
        m_running = false;
        r_running = false;
        break;
      } else {
        if (video.get_enqueued_operation_count() > 0) {
          // Remote operation encountered
          response = video.execute_operations(true);
          if (response == -1) {
            // An exception occured while executing the operations
            // Terminate the eventloop
            auto const result =
                videoMap.insert(std::pair<std::string, VCL::Video>(
                    video.get_video_id(), video));
            if (not result.second) {
              result.first->second = video;
            }
            _remote_running = false;
            flag = 0;
            m_writeBuffer.clear();
            r_writeBuffer.clear();
            m_running = false;
            r_running = false;
            break;
          } else {
            // Enqueue the video onto the remote queue
            r_enqueue(video);
            flag = 1;
          }
        } else {
          // All operations executed
          // Finalize the videomap
          auto const result = videoMap.insert(
              std::pair<std::string, VCL::Video>(video.get_video_id(), video));
          if (not result.second) {
            result.first->second = video;
            result.first->second.set_operated_video_id(
                video.get_operated_video_id());
          }
        }
      }
    }
    readBuffer.clear();
    if (flag == 0 && _remote_running == false && m_writeBuffer.size() == 0 &&
        r_writeBuffer.size() == 0) {
      // All eventloop tasks are completed
      // setup terminating conditions
      m_running = false;
      r_running = false;
    }
  }
}

/**
 * Write the remote response to a local file
 */
static size_t videoCallback(void *ptr, size_t size, size_t nmemb,
                            void *stream) {

  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

CURL *VideoLoop::get_easy_handle(VCL::Video video,
                                 std::string response_filepath) {

  // Get the remote operations parameters shared by the client
  Json::Value rParams = video.get_remoteOp_params();
  std::string url = rParams["url"].toStyledString().data();
  url.erase(std::remove(url.begin(), url.end(), '\n'), url.end());
  url = url.substr(1, url.size() - 2);
  Json::Value options = rParams["options"];

  // Initialize curl
  CURL *curl = NULL;

  CURLcode res;
  struct curl_slist *headers = NULL;
  curl_mime *form = NULL;
  curl_mimepart *field = NULL;

  curl = curl_easy_init();

  if (curl) {

    // Create the form to be sent to the remote operation
    // We send the video file and the set of remote operation paramters
    // as two form fields.
    form = curl_mime_init(curl);

    field = curl_mime_addpart(form);
    curl_mime_name(field, "videoData");
    if (curl_mime_filedata(field, video.get_operated_video_id().data()) !=
        CURLE_OK) {
      throw VCLException(ObjectEmpty,
                         "Unable to retrieve local file for remoting");
    }

    field = curl_mime_addpart(form);
    curl_mime_name(field, "jsonData");
    if (curl_mime_data(field, options.toStyledString().data(),
                       options.toStyledString().length()) != CURLE_OK) {
      throw VCLException(ObjectEmpty,
                         "Unable to create curl mime data for client params");
    }

    // Post data
    FILE *response_file = fopen(response_filepath.data(), "wb");
    url = url + "?id=" + video.get_video_id();

    if (curl_easy_setopt(curl, CURLOPT_URL, url.data()) != CURLE_OK) {
      throw VCLException(UndefinedException, "CURL setup error with URL");
    }
    if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, videoCallback) !=
        CURLE_OK) {
      throw VCLException(UndefinedException, "CURL setup error with callback");
    }

    if (response_file) {
      if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, response_file) !=
          CURLE_OK) {
        throw VCLException(UndefinedException,
                           "CURL setup error callback response file");
      }
      if (curl_easy_setopt(curl, CURLOPT_MIMEPOST, form) != CURLE_OK) {
        throw VCLException(UndefinedException, "CURL setup error with form");
      }
      fclose(response_file);
      return curl;
    }

    return NULL;
  }

  return NULL;
}

void VideoLoop::execute_remote_operations(std::vector<VCL::Video> &readBuffer) {
  int flag = 0;
  int start_index = 0;
  int step = 10;
  int end_index = readBuffer.size() > step ? step : readBuffer.size();
  std::vector<std::string> responseBuffer;
  int rindex = 0;
  std::map<std::string, FILE *> responseFileMaps;
  try {
    // Use multicurl to perform call to the remote API
    // and receive response. We perform multiple amsll multicurl calls
    // instead of a single large call to ensure that the remote server
    // does not suspect an attack.
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

      std::vector<VCL::Video> tempBuffer(start, end);

      for (VCL::Video video : tempBuffer) {
        std::string video_id = video.get_operated_video_id();

        Json::Value rParams = video.get_remoteOp_params();
        Json::Value options = rParams["options"];

        std::string format = "";
        char *s = const_cast<char *>(video_id.data());
        std::string delimiter = ".";
        char *p = std::strtok(s, delimiter.data());
        while (p != NULL) {
          p = std::strtok(NULL, delimiter.data());
          if (p != NULL) {
            format.assign(p, std::strlen(p));
          }
        }

        auto time_now = std::chrono::system_clock::now();
        std::chrono::duration<double> utc_time = time_now.time_since_epoch();
        std::string response_filepath =
            VDMS::VDMSConfig::instance()->get_path_tmp() + "/rtempfile" +
            std::to_string(utc_time.count()) + "." + format;

        responseBuffer.push_back(response_filepath);
        CURL *curl = get_easy_handle(video, responseBuffer[rindex]);
        FILE *response_file = fopen(response_filepath.data(), "wb");
        responseFileMaps.insert(
            std::pair<std::string, FILE *>(response_filepath, response_file));
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
            // Throw exceptions for different error codes received from the
            // remote server
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
    // Finalize the remote operation and enqueue video on local queue
    for (VCL::Video video : readBuffer) {
      rindex++;
      fclose(responseFileMaps[responseBuffer[rindex].data()]);
      video.set_operated_video_id(responseBuffer[rindex]);

      auto const result = videoMap.insert(
          std::pair<std::string, VCL::Video>(video.get_video_id(), video));
      if (not result.second) {
        result.first->second = video;
      }
      if (rindex == readBuffer.size() - 1) {
        _remote_running = false;
      }
      enqueue(video);
    }
    readBuffer.clear();
  } catch (VCL::Exception e) {
    // Exception occured. Terminate the event loop.
    VCL::Video video = readBuffer[0];
    video.set_query_error_response(e.msg);

    auto const result = videoMap.insert(
        std::pair<std::string, VCL::Video>(video.get_video_id(), video));
    if (not result.second) {
      result.first->second = video;
    }

    readBuffer.clear();
    _remote_running = false;
    m_writeBuffer.clear();
    r_writeBuffer.clear();
    m_running = false;
    r_running = false;

    print_exception(e);
    return;
  }
}

void VideoLoop::remoteOperationThread() noexcept {
  std::vector<VCL::Video> readBuffer;

  while (r_running) {
    // Swap the remote queue with a temporary vector on which operations can be
    // performed
    {
      std::unique_lock<std::mutex> rlock(r_mutex);
      r_condVar.wait(rlock, [this] { return !r_writeBuffer.empty(); });
      if (r_writeBuffer.size() == _nrof_entities) {
        std::swap(readBuffer, r_writeBuffer);
      }
    }

    if (readBuffer.size() == _nrof_entities && destroyed == false) {
      // Set flag that remote operations are running and
      // start the execution of remote operations on the temporary vector
      _remote_running = true;
      while (readBuffer.size() > 0) {
        execute_remote_operations(readBuffer);
      }
      _remote_running = false;
    }
  }
}