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

// hat-tip
// https://stackoverflow.com/questions/440133/how-do-i-create-a-random-alpha-numeric-string-in-c
std::string gen_random(const int len) {
  static const char alphanum[] = "0123456789"
                                 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                 "abcdefghijklmnopqrstuvwxyz";
  std::string tmp_s;
  tmp_s.reserve(len);

  for (int i = 0; i < len; ++i) {
    tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return tmp_s;
}

// Constructor + superclass call
Neo4jNeoAdd::Neo4jNeoAdd() : Neo4jCommand("NeoAdd") {
  _storage_tdb = VDMSConfig::instance()->get_path_tdb();
  _storage_png = VDMSConfig::instance()->get_path_png();
  _storage_jpg = VDMSConfig::instance()->get_path_jpg();
  _storage_bin = VDMSConfig::instance()->get_path_bin();
}

bool Neo4jNeoAdd::need_blob(const Json::Value &cmd) {

  Json::Value first_level = cmd["NeoAdd"];
  std::string tgt_data_type =
      first_level.get("target_data_type", "tgt_type_not_specified").asString();
  if (tgt_data_type == "md_only") {
    return false;
  } else if (tgt_data_type == "tgt_type_not_specified") {
    return false;
  } else {
    return true;
  }
};

int Neo4jNeoAdd::data_processing(std::string &cypher,
                                 const Json::Value &orig_query,
                                 const std::string &blob, int grp_id,
                                 Json::Value &error) {

  std::chrono::steady_clock::time_point ops_start, ops_end;
  VCL::RemoteConnection *connection;
  std::vector<unsigned char> enc_img;

  const Json::Value &cmd = orig_query[_cmd_name];
  int operation_flags = 0;

  std::string format = get_value<std::string>(cmd, "target_format", "");
  std::string tgt_data_type =
      get_value<std::string>(cmd, "target_data_type", "");

  char binary_img_flag = 0;

  // if we're only doing metadata, we can ignore the ingest processing and just
  // return
  if (tgt_data_type == "md_only") {
    return 0;
  }

  std::vector<unsigned char> raw_data(blob.begin(), blob.end());
  connection = get_existing_connection();

  try {
    enc_img = do_single_img_ops(orig_query, raw_data, _cmd_name);
  } catch (VCL::Exception &e) {
    print_exception(e, stdout);
    exit(1); // brutal exit, future iterations should throw exception for
             // handling and graceful rollback
  }

  std::string img_obj_id;
  img_obj_id = gen_random(32);

  s3_upload(img_obj_id, enc_img, connection);

  // In case we need to cleanup the query
  error["image_added"] = img_obj_id;
  error["data_type"] = tgt_data_type;

  // Later this will require validation/checks but for experimental should be
  // okay
  cypher = cypher + " SET VDMSNODE:" + error["data_type"].asString();
  cypher = cypher + " SET VDMSNODE.img_loc = \"" +
           error["image_added"].asString() + "\"";

  return 0;
}

Json::Value Neo4jNeoAdd::construct_responses(Json::Value &neo4j_responses,
                                             const Json::Value &orig_query,
                                             protobufs::queryMessage &query_res,
                                             const std::string &blob) {

  Json::Value ret;
  ret[_cmd_name] = "Filler";
  return ret;
}

// Neo find
Neo4jNeoFind::Neo4jNeoFind() : Neo4jCommand("NeoFind") {}

int Neo4jNeoFind::data_processing(std::string &tx, const Json::Value &jsoncmd,
                                  const std::string &blob, int grp_id,
                                  Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  Json::Value results = get_value<Json::Value>(cmd, "results");

  // Unless otherwise specified, we return the blob.
  if (get_value<bool>(results, "blob", true)) {
    results["list"].append(VDMS_IM_PATH_PROP);
  }

  return 0;
}

Json::Value Neo4jNeoFind::construct_responses(
    Json::Value &neo4j_responses, const Json::Value &orig_query,
    protobufs::queryMessage &query_res, const std::string &blob) {

  std::chrono::steady_clock::time_point min_conn_start, min_conn_end;
  std::chrono::steady_clock::time_point min_conn_run_start, min_conn_run_end;
  std::chrono::steady_clock::time_point ops_start, ops_end;

  Json::FastWriter fastWriter;
  const Json::Value &cmd = orig_query[_cmd_name];
  int operation_flags = 0;
  bool has_operations = false;
  std::string no_op_def_image;
  Json::Value ret;
  std::map<std::string, VCL::Format> formats;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  auto empty = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  std::vector<std::string> img_paths;
  for (int i = 0; i < neo4j_responses["metadata_res"].size(); i++) {
    Json::Value res_row = neo4j_responses["metadata_res"][i];
    std::string img_loc = res_row["VDMSNODE.img_loc"].asString();

    img_paths.push_back(img_loc);
  }

  Json::Value results = get_value<Json::Value>(cmd, "results");

  // Check if blob (image) must be returned
  if (get_value<bool>(results, "blob", true)) {

    for (int img_idx = 0; img_idx < img_paths.size(); img_idx++) {
      std::vector<unsigned char> raw_data;
      std::string im_path = img_paths[img_idx];

      try {
        // NOTE CURRENTLY FIXED TO USE ONLY S3
        raw_data = s3_retrieval(im_path, global_s3_connection);

        std::vector<unsigned char> img_enc;
        img_enc = do_single_img_ops(orig_query, raw_data, _cmd_name);

        std::string *img_str = query_res.add_blobs();
        img_str->resize(img_enc.size());
        std::memcpy((void *)img_str->data(), (void *)img_enc.data(),
                    img_enc.size());

      } catch (VCL::Exception e) {
        print_exception(e);
        Json::Value return_error;
        return_error["status"] = Neo4jCommand::Error;
        return_error["info"] = "VCL Exception";
        return error(return_error);
      }
    }
  }

  return ret;
}