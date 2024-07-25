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
  _ssl = nullptr;
  _ssl_ctx = nullptr;

  if (_socket_fd < 0) {
    throw ExceptionComm(SocketFail);
  }

  int option = 1; // To set REUSEADDR to true
  if (setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option,
                 sizeof option) == -1) {
    throw ExceptionComm(SocketFail);
  }
}

ConnClient::ConnClient(ServerAddress srv)
    : ConnClient(srv.addr, srv.port, "", "", "") {}

ConnClient::ConnClient(std::string addr, int port)
    : ConnClient(addr, port, "", "", "") {}

ConnClient::ConnClient(std::string addr, int port,
                       const std::string &cert_file = "",
                       const std::string &key_file = "",
                       const std::string &ca_file = "")
    : ConnClient() {
  _ssl = nullptr;
  _ssl_ctx = nullptr;

  if (port > MAX_PORT_NUMBER || port <= 0) {
    throw ExceptionComm(PortError);
  }

  _server.addr = addr;
  _server.port = port;

  _cert_file = cert_file;
  _key_file = key_file;
  _ca_file = ca_file;

  setupTLS();
  connect();
  if (_ssl != nullptr) {
    initiateTLS();
  }
}

void ConnClient::setupTLS() {

  if (!_cert_file.empty() && !_key_file.empty()) {
    const SSL_METHOD *method;
    method = TLS_client_method();
    _ssl_ctx = SSL_CTX_new(method);
    if (!_ssl_ctx) {
      throw ExceptionComm(SSL_CONTEXT_FAIL);
    }

    if (SSL_CTX_use_certificate_file(_ssl_ctx, _cert_file.c_str(),
                                     SSL_FILETYPE_PEM) <= 0) {
      throw ExceptionComm(SSL_CERT_FAIL);
    }

    if (SSL_CTX_use_PrivateKey_file(_ssl_ctx, _key_file.c_str(),
                                    SSL_FILETYPE_PEM) <= 0) {
      throw ExceptionComm(SSL_KEY_FAIL);
    }

    if (!_ca_file.empty()) {
      if (SSL_CTX_load_verify_locations(_ssl_ctx, _ca_file.c_str(), nullptr) <=
          0) {
        throw ExceptionComm(SSL_CA_FAIL);
      }
    }

    _ssl = SSL_new(_ssl_ctx);
    if (!_ssl) {
      throw ExceptionComm(SSL_CONTEXT_FAIL);
    }
  }
}

void ConnClient::initiateTLS() {
  if (SSL_set_fd(_ssl, _socket_fd) <= 0) {
    throw ExceptionComm(SSL_SET_FD_FAIL);
  }
  if (SSL_connect(_ssl) <= 0) {
    throw ExceptionComm(SSL_CONNECT_FAIL);
  }
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
