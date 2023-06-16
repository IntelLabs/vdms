/**
 * @file   Server.h
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

#include <csignal>

#include "CommunicationManager.h"
#include "pmgd.h"
#include <chrono>

namespace VDMS {
class Server {
  static const int DEFAULT_PORT = 55555;
  static const int DEFAULT_AUTODELETE_INTERVAL = -1;
  static const int DEFAULT_AUTOREPLICATE_INTERVAL = -1;
  std::string DEFAULT_AUTOREPLICATE_UNIT = "s";
  std::string DEFAULT_BACKUP_PATH = ".";
  std::string DEFAULT_DB_ROOT = "db";
  std::string DEFAULT_AUTOREPLICATE_FLAG = "false";

  CommunicationManager *_cm;

  // TODO: Partitioner here

  int _server_port;
  int _autodelete_interval;
  int _autoreplecate_interval;
  std::string _replication_unit;
  std::string _backup_path;
  std::string _db_path;
  std::string _backup_flag;

  bool _untar;

  // Handle ^c
  static bool shutdown;
  void install_handler();
  static void sighandler(int signo) {
    Server::shutdown =
        (signo == SIGINT) || (signo == SIGTERM) || (signo == SIGQUIT);
  }

public:
  Server(std::string config_file);
  void process_requests();
  void autodelete_expired_data();
  void auto_replicate_data();
  void untar_data(std::string &);
  ~Server();
};
}; // namespace VDMS
