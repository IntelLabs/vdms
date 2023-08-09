/**
 * @file   ImageCommand.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 Intel Corporation
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
#include "vcl/CustomVCL.h"
#include "vcl/Image.h"
#include <mutex>
#include <string>
#include <vector>

#include "ExceptionsCommand.h"
#include "RSCommand.h"

#include <curl/curl.h>

namespace VDMS {

// Helper classes for handling various JSON commands.

class ImageCommand : public RSCommand {
public:
  ImageCommand(const std::string &cmd_name);

  virtual int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                                 const std::string &blob, int grp_id,
                                 Json::Value &error) = 0;

  virtual bool need_blob(const Json::Value &cmd) { return false; }

  // We use this function for enqueueing operations for an 'Image' object
  // that is allocated outside of <*>Image operations
  int enqueue_operations(VCL::Image &img, const Json::Value &op,
                         bool is_addition = false);

  // Checks if 'format' parameter is specified, and if so, returns the
  // corresponding VCL::Image::Format type.
  VCL::Image::Format get_requested_format(const Json::Value &cmd);
};

class AddImage : public ImageCommand {
  std::string _storage_tdb;
  std::string _storage_png;
  std::string _storage_jpg;
  std::string _storage_bin;
  // bool _use_aws_storage;

public:
  AddImage();

  int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                         const std::string &blob, int grp_id,
                         Json::Value &error);

  bool need_blob(const Json::Value &cmd) { return true; }
};

class UpdateImage : public ImageCommand {
public:
  UpdateImage();

  int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                         const std::string &blob, int grp_id,
                         Json::Value &error);

  // TODO In order to support "format" or "operations", we could
  // implement VCL save operation by adding construct_responses method.
};

class FindImage : public ImageCommand {
  // bool _use_aws_storage;

public:
  FindImage();
  int construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                         const std::string &blob, int grp_id,
                         Json::Value &error);

  Json::Value construct_responses(Json::Value &json_responses,
                                  const Json::Value &json,
                                  protobufs::queryMessage &response,
                                  const std::string &blob);
};

}; // namespace VDMS
