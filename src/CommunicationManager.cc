/**
 * @file   CommunicationManager.cc
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

#include "CommunicationManager.h"
#include "QueryHandler.h"

#include "VDMSConfig.h"

using namespace VDMS;
using namespace PMGD;

CommunicationManager::CommunicationManager() {
  _num_threads = VDMSConfig::instance()->get_int_value(
      "max_simultaneous_clients", MAX_CONNECTED_CLIENTS);

  if (_num_threads > MAX_CONNECTED_CLIENTS)
    _num_threads = MAX_CONNECTED_CLIENTS;

  _shutdown = false;
  for (int i = 0; i < _num_threads; ++i)
    _pool.push_back(std::thread(&CommunicationManager::process_queue, this));
}

void CommunicationManager::process_queue() {
  comm::Connection *c;
  while (true) {
    {
      std::unique_lock<std::mutex> lock(_mlock);
      _cv.wait(lock, [&] { return !_workq.empty(); });
      if (_shutdown)
        break;
      c = _workq.front();
      _workq.pop();
    }
    if (c != NULL) {
      _conn_list_lock.lock();
      auto c_it = _conn_list.insert(_conn_list.begin(), c);
      _conn_list_lock.unlock();

      QueryHandler qh;
      printf("Connection received...\n");
      qh.process_connection(c);

      std::unique_lock<std::mutex> conn_list_lock(_conn_list_lock);
      _conn_list.erase(c_it);
      delete c;
    }
  }
}

CommunicationManager::~CommunicationManager() {
  // Kill all connections by closing the sockets
  // If not, QueryHandler will be blocked on process_connection()
  for (auto connection : _conn_list) {
    connection->shutdown();
  }

  for (int i = 0; i < _num_threads; ++i) {
    _pool[i].join();
  }
}

void CommunicationManager::add_connection(comm::Connection *c) {
  {
    std::unique_lock<std::mutex> lock(_mlock);
    _workq.push(c);
  }
  _cv.notify_one();
}

void CommunicationManager::shutdown() {
  _shutdown = true;
  add_connection(NULL);
  _cv.notify_all();
}
