/**
 * @file   OpsIoCoordinator.cc
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
 */

#include "OpsIOCoordinator.h"
#include "ExceptionsCommand.h"
#include "VDMSConfig.h"
#include "vcl/Image.h"
#include "vcl/VCL.h"

#include <chrono>

using namespace VDMS;

template <typename T>
T get_json_val(const Json::Value &json, const std::string &key, T def = T());

template <>
int get_json_val(const Json::Value &json, const std::string &key, int def) {
  if (json.isMember(key))
    return json[key].asInt();

  return def;
}

template <>
double get_json_val(const Json::Value &json, const std::string &key,
                    double def) {
  if (json.isMember(key))
    return json[key].asDouble();

  return def;
}

template <>
bool get_json_val(const Json::Value &json, const std::string &key, bool def) {
  if (json.isMember(key))
    return json[key].asBool();

  return def;
}

template <>
std::string get_json_val(const Json::Value &json, const std::string &key,
                         std::string def) {
  if (json.isMember(key))
    return json[key].asString();

  return def;
}

template <>
Json::Value get_json_val(const Json::Value &json, const std::string &key,
                         Json::Value def) {
  return json[key];
}

int img_enqueue_operations(VCL::Image &img, const Json::Value &ops) {

  std::chrono::steady_clock::time_point total_start, total_end;
  double total_runtime;

  total_start = std::chrono::steady_clock::now();
  // Correct operation type and parameters are guaranteed at this point
  for (auto &op : ops) {
    const std::string &type = get_json_val<std::string>(op, "type");
    if (type == "threshold") {
      img.threshold(get_json_val<int>(op, "value"));
    } else if (type == "resize") {
      img.resize(get_json_val<int>(op, "height"),
                 get_json_val<int>(op, "width"));
    } else if (type == "crop") {
      img.crop(VCL::Rectangle(
          get_json_val<int>(op, "x"), get_json_val<int>(op, "y"),
          get_json_val<int>(op, "width"), get_json_val<int>(op, "height")));
    } else if (type == "flip") {
      img.flip(get_json_val<int>(op, "code"));
    } else if (type == "rotate") {
      img.rotate(get_json_val<double>(op, "angle"),
                 get_json_val<bool>(op, "resize"));
    } else {
      throw ExceptionCommand(ImageError, "Operation is not defined");
      return -1;
    }
  }
  total_end = std::chrono::steady_clock::now();
  total_runtime = std::chrono::duration_cast<std::chrono::microseconds>(
                      total_end - total_start)
                      .count();

  return 0;
}

std::vector<unsigned char>
do_single_img_ops(const Json::Value &orig_query,
                  std::vector<unsigned char> &raw_data, std::string cmd_name) {

  std::chrono::steady_clock::time_point total_start, total_end;
  total_start = std::chrono::steady_clock::now();
  double total_runtime;
  Json::Value cmd;
  if (orig_query.isMember(cmd_name)) {
    cmd = orig_query[cmd_name];
  } else {
    // TODO this is clunky and not optimal,but for experimental feature is okay
    // IMO
    printf("CMD Not Found: %s, returning empty image vector!\n",
           cmd_name.c_str());
    return std::vector<unsigned char>();
  }

  int operation_flags = 0;
  char binary_img_flag = 0;

  std::string format = get_json_val<std::string>(cmd, "target_format", "");
  if (format == "bin" || format == "") {
    binary_img_flag = 1;
  }

  VCL::Image img(std::data(raw_data), raw_data.size(), binary_img_flag);
  VCL::Format vcl_format = img.get_image_format();

  if (cmd.isMember("operations")) {
    operation_flags = img_enqueue_operations(img, cmd["operations"]);
  }

  if (cmd.isMember("target_format")) {
    if (format == "png") {
      vcl_format = VCL::Format::PNG;
    } else if (format == "jpg") {
      vcl_format = VCL::Format::JPG;
    } else if (format == "bin") {
      vcl_format = VCL::Format::BIN;
    } else {
      printf("Warning! %s not supported!\n", format.c_str());
    } // FUTURE, add TDB support
  }

  long imgsize = 0;
  std::vector<unsigned char> img_enc;

  // getting the image size performs operation as a side effect
  imgsize = img.get_raw_data_size();
  img_enc = img.get_encoded_image(vcl_format);
  total_end = std::chrono::steady_clock::now();
  total_runtime = std::chrono::duration_cast<std::chrono::microseconds>(
                      total_end - total_start)
                      .count();

  return img_enc;
}

std::vector<unsigned char> s3_retrieval(std::string obj_name,
                                        VCL::RemoteConnection *connection) {

  if (!connection->connected()) {
    printf("Warning, attempting to use uninitialized S3 connection, returning "
           "empty object\n");
    return std::vector<unsigned char>();
  }

  std::chrono::steady_clock::time_point total_start, total_end;
  total_start = std::chrono::steady_clock::now();
  double total_runtime;

  std::vector<unsigned char> raw_data;

  raw_data = connection->Read(obj_name);
  total_end = std::chrono::steady_clock::now();
  total_runtime = std::chrono::duration_cast<std::chrono::microseconds>(
                      total_end - total_start)
                      .count();

  return raw_data;
}

int s3_upload(std::string obj_name, std::vector<unsigned char> upload_data,
              VCL::RemoteConnection *connection) {

  if (!connection->connected()) {
    printf("Warning, attempting to use uninitialized S3 connection, no uploads "
           "will occur\n");
    return -1;
  }

  std::chrono::steady_clock::time_point total_start, total_end;
  total_start = std::chrono::steady_clock::now();
  double total_runtime;

  connection->Write(obj_name, upload_data);
  total_end = std::chrono::steady_clock::now();
  total_runtime = std::chrono::duration_cast<std::chrono::microseconds>(
                      total_end - total_start)
                      .count();

  return 0;
}

VCL::RemoteConnection *instantiate_connection() {
  printf("Instantiating global S3 Connection...\n");
  std::chrono::steady_clock::time_point total_start, total_end;
  total_start = std::chrono::steady_clock::now();
  double total_runtime;

  VCL::RemoteConnection *connection;
  connection = new VCL::RemoteConnection();
  std::string bucket = VDMSConfig::instance()->get_bucket_name();
  connection->_bucket_name = bucket;
  connection->start();

  total_end = std::chrono::steady_clock::now();
  total_runtime = std::chrono::duration_cast<std::chrono::microseconds>(
                      total_end - total_start)
                      .count();

  printf("Global S3 Connection Started!\n");
  return connection;
}

VCL::RemoteConnection *get_existing_connection() {
  return global_s3_connection;
}