/**
 * @file   Neo4jHandlerCommands.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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

#include "ExceptionsCommand.h"
#include "ImageLoop.h"
#include "Neo4JCommands.h"
#include "VDMSConfig.h"
#include "defines.h"
#include "vcl/VCL.h"
#include <jsoncpp/json/writer.h>

#include "OpsIOCoordinator.h"
#include <ctime>
#include <sys/time.h>

#include <chrono>

using namespace VDMS;

//ADD DESCRIPTOR SET
Neo4jNeoAddDescSet::Neo4jNeoAddDescSet() : Neo4jCommand("NeoAddDescSet") {}

int Neo4jNeoAddDescSet::data_processing(std::string &tx, const Json::Value &root,
                                        const std::string &blob, int grp_id, Json::Value &error){

    return 0;
}

Json::Value Neo4jNeoAddDescSet::construct_responses(Json::Value &json_responses,
                                                    const Json::Value &json,
                                                    protobufs::queryMessage &response,
                                                    const std::string &blob) {

    Json::Value ret;
    return ret;
}



//FIND DESCRIPTOR SET
Neo4jNeoFindDescSet::Neo4jNeoFindDescSet() : Neo4jCommand("NeoFindDescSet") {}

int Neo4jNeoFindDescSet::data_processing(std::string &tx, const Json::Value &root,
                                         const std::string &blob, int grp_id,
                                         Json::Value &error) {

    return 0;
}

Json::Value Neo4jNeoFindDescSet::construct_responses(Json::Value &json_responses,
                                                     const Json::Value &json,
                                                     protobufs::queryMessage &response,
                                                     const std::string &blob) {
    Json::Value ret;
    return ret;
}

//ADD DESCRIPTOR
Neo4jNeoAddDesc::Neo4jNeoAddDesc() : Neo4jCommand("NeoAddDesc") {}

int Neo4jNeoAddDesc::data_processing(std::string &tx, const Json::Value &root,
                                     const std::string &blob, int grp_id,
                                     Json::Value &error) {

    return 0;
}

Json::Value Neo4jNeoAddDesc::construct_responses(Json::Value &json_responses,
                                                 const Json::Value &json,
                                                 protobufs::queryMessage &response,
                                                 const std::string &blob) {

    Json::Value ret;
    return ret;
}

//FIND DESCRIPTOR
Neo4jNeoFindDesc::Neo4jNeoFindDesc() : Neo4jCommand("NeoFindDesc") {}

bool Neo4jNeoFindDesc::need_blob(const Json::Value &cmd) {
    return cmd[_cmd_name].isMember("k_neighbors");
}

int Neo4jNeoFindDesc::data_processing(std::string &tx, const Json::Value &root,
                                      const std::string &blob, int grp_id,
                                      Json::Value &error) {

    return 0;
}

Json::Value Neo4jNeoFindDesc::construct_responses(Json::Value &json_responses,
                                                  const Json::Value &json,
                                                  protobufs::queryMessage &response,
                                                  const std::string &blob) {

    Json::Value ret;
    return ret;
}