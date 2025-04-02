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

#include <string>
#include <thread>

#include "comm/Connection.h"
#include "gtest/gtest.h"

#include "Server.h"
#include "VDMSConfig.h"
#include "meta_data_helper.h"

#define SERVER_PORT_INTERCHANGE 43444
#define SERVER_PORT_MULTIPLE 43444
#define NUMBER_OF_MESSAGES 1

const std::string TEMPORARY_DIR = "/tmp";

typedef std::basic_string<uint8_t> BytesBuffer;
std::string cert_file_ = TEMPORARY_DIR + "/trusted_server_cert.pem";
std::string key_file_ = TEMPORARY_DIR + "/trusted_server_key.pem";
std::string ca_file_ = TEMPORARY_DIR + "/trusted_ca_cert.pem";

TEST(CommTest, MoveCopy) {
  comm::Connection a;
  comm::Connection conn_server;
  conn_server = std::move(a); // Testing copy with move works
}

TEST(CommTest, ServerConnect) {
  comm::ConnServer conn_server(SERVER_PORT_INTERCHANGE, cert_file_, key_file_,
                               ca_file_);
  ASSERT_NO_FATAL_FAILURE(conn_server);
}

TEST(CommTest, Unreachable) {
  ASSERT_THROW(
      comm::ConnClient conn_client("unreachable.com.ar.something", 5555),
      comm::ExceptionComm);

  ASSERT_THROW(comm::ConnClient conn_client("localhost", -1),
               comm::ExceptionComm);
}

TEST(CommTest, ServerWrongPort) {
  ASSERT_THROW(comm::ConnServer conn_server(-22, "", "", ""),
               comm::ExceptionComm);

  ASSERT_THROW(comm::ConnServer conn_server(0, "", "", ""),
               comm::ExceptionComm);
}

TEST(CommTest, ClientWrongAddrOrPort) {
  ASSERT_THROW(comm::ConnClient conn_client("", 3424), comm::ExceptionComm);

  ASSERT_THROW(comm::ConnClient conn_client("intel.com", -32),
               comm::ExceptionComm);

  ASSERT_THROW(comm::ConnClient conn_client("intel.com", 0),
               comm::ExceptionComm);
}
