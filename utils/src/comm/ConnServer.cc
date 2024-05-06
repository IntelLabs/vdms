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
#include <iostream>
#include <string>
#include <unistd.h>

#include <netdb.h>

#include "Connection.h"

using namespace comm;

ConnServer::ConnServer(int port, const std::string &cert_file = "",
                       const std::string &key_file = "",
                       const std::string &ca_file = "")
    : _port(port), _cert_file(cert_file), _key_file(key_file),
      _ca_file(ca_file) {
  if (_port > MAX_PORT_NUMBER || _port <= 0) {
    throw ExceptionComm(PortError);
  }

  int ret;

  // Setup TLS Context if a cert and key were passed
  _ssl_ctx = nullptr;
  if (!_cert_file.empty() && !_key_file.empty()) {
    const SSL_METHOD *method;
    method = TLS_server_method();
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
      SSL_CTX_set_verify(
          _ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, nullptr);
      if (SSL_CTX_load_verify_locations(_ssl_ctx, _ca_file.c_str(), nullptr) !=
          1) {
        throw ExceptionComm(SSL_CA_FAIL);
      }
    }
  }

  // create TCP/IP socket
  _socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if (_socket_fd < 0) {
    throw ExceptionComm(SocketFail);
  }

  int option = 1; // To set REUSEADDR to true
  ret =
      setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
  if (ret < 0) {
    throw ExceptionComm(SocketFail);
  }

  struct sockaddr_in svr_addr;
  memset((char *)&svr_addr, 0, sizeof(svr_addr));
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_addr.s_addr = INADDR_ANY;
  svr_addr.sin_port = htons(_port);

  // bind socket : "assigning a name to a socket"
  ret = ::bind(_socket_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
  if (ret < 0) {
    throw ExceptionComm(BindFail);
  }

  // mark socket as passive
  if (::listen(_socket_fd, MAX_CONN_QUEUE) == -1) {
    throw ExceptionComm(ListentFail);
  }
}

ConnServer::~ConnServer() {
  ::close(_socket_fd);
  if (_ssl_ctx != nullptr) {
    SSL_CTX_free(_ssl_ctx);
  }
}

Connection ConnServer::accept() {
  struct sockaddr_in clnt_addr;
  socklen_t len = sizeof(clnt_addr); // store size of the address

  // This is where client connects.
  // Server will stall here until incoming connection
  // unless the socket is marked and nonblocking
  int connfd = ::accept(_socket_fd, (struct sockaddr *)&clnt_addr, &len);
  if (connfd < 0) {
    throw ExceptionComm(ConnectionError);
  }

  SSL *ssl = nullptr;
  if (_ssl_ctx != nullptr) {
    ssl = SSL_new(_ssl_ctx);
    if (ssl == nullptr || SSL_up_ref(ssl) == 0) {
      throw ExceptionComm(SSL_CREATION_FAIL);
    }

    int ret = SSL_set_fd(ssl, connfd);
    if (ret != 1) {
      throw ExceptionComm(SSL_SET_FD_FAIL);
    };

    ret = SSL_accept(ssl);
    if (ret != 1)
      throw ExceptionComm(SSL_ACCEPT_FAIL);
  }

  return Connection(connfd, ssl);
}
