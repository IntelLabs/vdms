/**
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

#include "ExceptionComm.h"
#include <string>

namespace comm {

class Connection {

public:
  Connection();
  Connection(int socket_fd);
  ~Connection();

  Connection(Connection &&);

  Connection &operator=(Connection &&);
  Connection &operator=(const Connection &) = delete;
  Connection(const Connection &) = delete;

  void send_message(const uint8_t *data, uint32_t size);
  const std::basic_string<uint8_t> &recv_message();

  void shutdown();

  void set_buffer_size_limit(uint32_t buffer_size_limit);

protected:
  const unsigned MAX_PORT_NUMBER = 65535;
  const unsigned MAX_RETRIES = 100;

  const unsigned DEFAULT_BUFFER_SIZE = (32 * 1024 * 1024);
  const unsigned MAX_BUFFER_SIZE = (1024 * 1024 * 1024);

  std::basic_string<uint8_t> buffer_str;

  int _socket_fd;
  uint32_t _buffer_size_limit{};
};

// Implements a TCP/IP server
class ConnServer {

public:
  ConnServer(int port);
  ~ConnServer();
  ConnServer &operator=(const ConnServer &) = delete;
  ConnServer(const ConnServer &) = delete;
  Connection accept();

private:
  const unsigned MAX_CONN_QUEUE = 2048;
  const unsigned MAX_PORT_NUMBER = 65535;

  int _port; // Server port
  int _socket_fd;
};

// Implements a TCP/IP client
class ConnClient : public Connection {

public:
  struct ServerAddress {
    std::string addr;
    int port;
  };

  ConnClient(struct ServerAddress srv);
  ConnClient(std::string addr, int port);
  ConnClient &operator=(const ConnClient &) = delete;
  ConnClient(const ConnClient &) = delete;

private:
  ConnClient();
  void connect();

  ServerAddress _server;
};

}; // namespace comm
