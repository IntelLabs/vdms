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

#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>

#include <netdb.h>

#include "Connection.h"

using namespace comm;

ConnClient::ConnClient() {
  _server.port = 0;
  // create TCP/IP socket
  _socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (_socket_fd < 0) {
    throw ExceptionComm(SocketFail);
  }

  int option = 1; // To set REUSEADDR to true
  if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option,
                 sizeof option) == -1) {
    throw ExceptionComm(SocketFail);
  }
}

ConnClient::ConnClient(ServerAddress srv) : ConnClient(srv.addr, srv.port) {}

ConnClient::ConnClient(std::string addr, int port) : ConnClient() {
  if (port > MAX_PORT_NUMBER || port <= 0) {
    throw ExceptionComm(PortError);
  }

  _server.addr = addr;
  _server.port = port;
  connect();
}

void ConnClient::connect() {
  struct hostent *server = gethostbyname(_server.addr.c_str());

  if (server == NULL) {
    throw ExceptionComm(ServerAddError);
  }

  struct sockaddr_in svrAddr;
  memset(&svrAddr, 0, sizeof(svrAddr));
  svrAddr.sin_family = AF_INET;

  memcpy(&svrAddr.sin_addr.s_addr, server->h_addr, server->h_length);
  svrAddr.sin_port = htons(_server.port);

  if (::connect(_socket_fd, (struct sockaddr *)&svrAddr, sizeof(svrAddr)) < 0) {
    throw ExceptionComm(ConnectionError);
  }
}
