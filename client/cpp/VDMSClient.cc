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

#include "VDMSClient.h"
#include "protobuf/queryMessage.pb.h"

using namespace VDMS;

VDMSClient::VDMSClient(std::string addr, int port) : _conn(addr, port)
{
}

VDMS::Response VDMSClient::query(const std::string &json,
                                 const std::vector<std::string *> blobs)
{
    protobufs::queryMessage cmd;
    cmd.set_json(json);

    for (auto& it : blobs) {
        std::string *blob = cmd.add_blobs();
        *blob = *it;
    }

    std::basic_string<uint8_t> msg(cmd.ByteSize(),0);
    cmd.SerializeToArray((void*)msg.data(), msg.length());
    _conn.send_message(msg.data(), msg.length());

    // Wait for response (blocking call)
    msg = _conn.recv_message();

    protobufs::queryMessage protobuf_response;
    protobuf_response.ParseFromArray((const void*)msg.data(), msg.length());

    VDMS::Response response;
    response.json = protobuf_response.json();

    for (auto& it : protobuf_response.blobs()) {
        response.blobs.push_back(it);
    }

    return response;
}
