/**
 * @file   VideoLoop.h
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

#include "vcl/Image.h"
#include "vcl/Video.h"
#include <condition_variable>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>

class VideoLoop {
public:
  VideoLoop() = default;
  VideoLoop(const VideoLoop &) = delete;
  VideoLoop(VideoLoop &&) noexcept = delete;
  ~VideoLoop() noexcept;

  VideoLoop &operator=(const VideoLoop &) = delete;
  VideoLoop &operator=(VideoLoop &&) noexcept = delete;

  /**
   * Sets the number of entities to be filled in the queue
   * @param nrof_entities Number of entities in the query response
   */
  void set_nrof_entities(int nrof_entities);

  /**
   * Enqueue into the local queue
   * @param video The video object to be enqueued
   */
  void enqueue(VCL::Video video) noexcept;

  /**
   * Enqueue into the remote queue
   * @param video The video object to be enqueued
   */
  void r_enqueue(VCL::Video video) noexcept;

  /**
   * Get the map containing the operated video objects
   */
  std::map<std::string, VCL::Video> get_video_map();

  /**
   * Check if the event loop is running
   */
  bool is_loop_running();

  /**
   * If no operations are to be executed then create a dummy entry
   * in the event loop and destroy it.
   */
  void close_no_operation_loop(std::string videoId);

private:
  // Number of entities in the VDMS query response
  int _nrof_entities = 0;

  // Is the event loop ready to be destroyed
  bool destroyed = false;

  // Are any remote operations running
  bool _remote_running = false;

  // Stores the operated videos. Key is the video id
  std::map<std::string, VCL::Video> videoMap;

  /**
   * The Local Queue parameters
   */

  std::vector<VCL::Video> m_writeBuffer;
  std::mutex m_mutex;
  std::condition_variable m_condVar;
  bool m_running{true};
  std::thread m_thread{&VideoLoop::operationThread, this};
  // Local thread function
  void operationThread() noexcept;

  /**
   * The Remote Queue parameters
   */
  std::vector<VCL::Video> r_writeBuffer;
  std::mutex r_mutex;
  std::condition_variable r_condVar;
  bool r_running{true};
  std::thread r_thread{&VideoLoop::remoteOperationThread, this};
  // Local thread function
  void remoteOperationThread() noexcept;

  /**
   * Get the curl easy handles that will be used for multi-curl
   * @param video The video object on which the remote operation will be
   * performed
   * @param response_filepath Path to the local file where the remote response
   * file will be stored
   */
  CURL *get_easy_handle(VCL::Video video, std::string response_filepath);

  /**
   * Execute the remote operation using multi-curl
   * @param readBuffer Stores all the videos on which the remote operation will
   * be performed
   */
  void execute_remote_operations(std::vector<VCL::Video> &readBuffer);
};