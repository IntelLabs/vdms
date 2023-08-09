/**
 * @file   QueryMessage.cc
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

#include "QueryMessage.h"
#include "Exception.h"

using namespace VDMS;

QueryMessage::QueryMessage(comm::Connection *conn) : _conn(conn) {
  if (_conn == NULL)
    throw ExceptionServer(NullConnection);
}

protobufs::queryMessage QueryMessage::get_query() {
  const std::basic_string<uint8_t> &msg = _conn->recv_message();

  protobufs::queryMessage cmd;
  cmd.ParseFromArray((const void *)msg.data(), msg.length());

  return cmd;
}

void QueryMessage::send_response(protobufs::queryMessage cmd) {
  std::basic_string<uint8_t> msg(cmd.ByteSize(), 0);
  cmd.SerializeToArray((void *)msg.data(), msg.length());
  _conn->send_message(msg.data(), msg.length());
}
