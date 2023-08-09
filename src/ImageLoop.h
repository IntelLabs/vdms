/**
 * @file   ImageLoop.h
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
#include <condition_variable>
#include <functional>
#include <future>
#include <thread>
#include <vector>

class ImageLoop {
public:
  ImageLoop() = default;
  ImageLoop(const ImageLoop &) = delete;
  ImageLoop(ImageLoop &&) noexcept = delete;
  ~ImageLoop() noexcept;

  ImageLoop &operator=(const ImageLoop &) = delete;
  ImageLoop &operator=(ImageLoop &&) noexcept = delete;

  void set_nrof_entities(int nrof_entities);

  void enqueue(VCL::Image *img) noexcept;
  void r_enqueue(VCL::Image *img) noexcept;

  std::map<std::string, VCL::Image *> get_image_map();

  bool is_loop_running();
  void close_no_operation_loop(std::string imageid);

private:
  int _nrof_entities = 0;
  bool destroyed = false;
  bool _remote_running = false;
  std::vector<std::string> _tempfiles;
  std::map<std::string, VCL::Image *> imageMap;

  std::vector<VCL::Image *> m_writeBuffer;
  std::mutex m_mutex;
  std::condition_variable m_condVar;
  bool m_running{true};
  std::thread m_thread{&ImageLoop::operationThread, this};
  void operationThread() noexcept;

  std::vector<VCL::Image *> r_writeBuffer;
  std::mutex r_mutex;
  std::condition_variable r_condVar;
  bool r_running{true};
  std::thread r_thread{&ImageLoop::remoteOperationThread, this};
  void remoteOperationThread() noexcept;

  CURL *get_easy_handle(VCL::Image *img, std::string &readBuffer);
  void execute_remote_operations(std::vector<VCL::Image *> &readBuffer);
};