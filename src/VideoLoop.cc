#include "VideoLoop.h"
#include "vcl/Exception.h"

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

void VideoLoop::execute_remote_operations(std::vector<VCL::Video> &readBuffer) {
  try {
    // Finalize the remote operation and enqueue video on local queue
    std::map<std::string, std::string> input_paths;
    std::map<std::string, std::string> output_paths;
    std::map<std::string, std::string> input_metadata;
    std::map<std::string, std::string> output_metadata;

    std::string url;

    for (VCL::Video video : readBuffer) {
      std::string video_id = video.get_operated_video_id();

      Json::Value rParams = video.get_remoteOp_params();
      url = rParams["url"].toStyledString().data();
      url.erase(std::remove(url.begin(), url.end(), '\n'), url.end());
      url = url.substr(1, url.size() - 2);
      Json::Value options = rParams["options"];
      Json::StreamWriterBuilder builder;
      std::string output = Json::writeString(builder, options);
      std::cout<< "URL: " << url << std::endl;

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


      input_paths[video.get_operated_video_id()] = video.get_operated_video_id();
      output_paths[video.get_operated_video_id()] = response_filepath;
      input_metadata[video.get_operated_video_id()] = output;

    }

    GRPCEntityClient client(grpc::CreateChannel(url.data(), grpc::InsecureChannelCredentials()));
    client.ProcessEntities(input_paths, output_paths, input_metadata, output_metadata);


    for (VCL::Video video : readBuffer) {
      std::string video_id = video.get_operated_video_id();
      video.set_operated_video_id(output_paths[video_id]);

      auto const result = videoMap.insert(
          std::pair<std::string, VCL::Video>(video.get_video_id(), video));
      if (not result.second) {
        result.first->second = video;
      }
      enqueue(video);
    }
    _remote_running = false;
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