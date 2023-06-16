/**
 * @file   CommunicationManager.h
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

#include <condition_variable>
#include <list>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "comm/Connection.h"
#include "pmgd.h"

namespace VDMS {
class CommunicationManager {
  static const int MAX_CONNECTED_CLIENTS = 500;

  // For the thread pool
  std::mutex _mlock;
  std::condition_variable _cv;
  int _num_threads;
  std::vector<std::thread> _pool;

  std::mutex _conn_list_lock;
  std::list<comm::Connection *> _conn_list;

  // Monitor new connections queued in for worker threads
  std::queue<comm::Connection *> _workq;

  bool _shutdown;

public:
  CommunicationManager();
  ~CommunicationManager();
  void process_queue();
  void add_connection(comm::Connection *c);
  void shutdown();
};
}; // namespace VDMS
