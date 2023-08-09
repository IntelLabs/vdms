/**
 * @file   RemoteConnection.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the C++ API for RemoteConnection, which allows users to
 *  connect to different file systems. At the moment, S3 is enabled.
 */

#pragma once

#include <fstream>
#include <iostream>

#include "Exception.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/PutObjectRequest.h>

namespace VCL {

class RemoteConnection {
public:
  RemoteConnection();
  ~RemoteConnection();

  void Write(const std::string &path, std::vector<unsigned char> data);
  void Write(const std::string &filename);
  std::vector<unsigned char> Read(const std::string &path);
  void RetrieveFile(const std::string &filename);
  std::vector<std::string> ListFilesInFolder(const std::string &folder_name);
  void Read_Video(const std::string &path);
  void Remove_Object(const std::string &path);
  void start();
  void end();
  bool connected() { return _remote_connected; };

  Aws::String _bucket_name;

private:
  bool _remote_connected = false;

  Aws::SDKOptions *_aws_sdk_options;
  Aws::S3::S3Client *_aws_client;

  void ConfigureAws();
  // void SetLogLevelDebug();
  void ShutdownAws();
  void write_s3(const std::string &path, std::vector<unsigned char> data);
  void write_s3(const std::string &filename);
  std::vector<unsigned char> read_s3(const std::string &path);
  void retrieve_file(const std::string &filename);
  std::vector<std::string> get_file_list(const std::string &path);
  void read_s3_video(const std::string &file_path);
  void remove_s3_object(const std::string &file_path);
  // void LogEntry(std::string functionName);
};
} // namespace VCL
