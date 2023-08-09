/**
 * @file   BoundingBoxCommand.h
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
#include <mutex>
#include <string>
#include <vector>

#include "ExceptionsCommand.h"
#include "RSCommand.h"

namespace VDMS {

class AddBoundingBox : public RSCommand {
public:
  AddBoundingBox();

  int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                         const std::string &blob, int grp_id,
                         Json::Value &error);
};

class UpdateBoundingBox : public RSCommand {
public:
  UpdateBoundingBox();

  int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                         const std::string &blob, int grp_id,
                         Json::Value &error);
};

class FindBoundingBox : public RSCommand {
  // bool _use_aws_storage;

public:
  FindBoundingBox();

  int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                         const std::string &blob, int grp_id,
                         Json::Value &error);

  Json::Value construct_responses(Json::Value &json_responses,
                                  const Json::Value &json,
                                  protobufs::queryMessage &response,
                                  const std::string &blob);
};

}; // namespace VDMS
