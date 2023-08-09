/**
 * @file   BlobCommand.cc
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

#include <iostream>

#include "BlobCommand.h"
#include "VDMSConfig.h"
#include "defines.h"

using namespace VDMS;

//========= AddBlob definitions =========

BlobCommand::BlobCommand(const std::string &cmd_name) : RSCommand(cmd_name) {}

AddBlob::AddBlob() : BlobCommand("AddBlob") {

  _storage_bin = VDMSConfig::instance()->get_path_bin();
}

int AddBlob::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                const std::string &blob, int grp_id,
                                Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  // std::cout << " inside ADDBLOB" <<std::endl;
  int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

  std::string format = "bin";
  char binary_img_flag = 1;
  VCL::Image img((void *)blob.data(), blob.size(), binary_img_flag);

  std::string blob_root = _storage_bin;
  VCL::Image::Format blob_format = VCL::Image::Format::BIN;
  std::string file_name = VCL::create_unique(blob_root, format);
  // std::cout << "Blob was added in " <<_storage_bin << "\t"<< file_name <<
  // std::endl;
  Json::Value props = get_value<Json::Value>(cmd, "properties");
  props[VDMS_EN_BLOB_PATH_PROP] = file_name;

  query.AddNode(node_ref, VDMS_BLOB_TAG, props, Json::Value());

  img.store(file_name, blob_format);

  error["Blob_added"] = file_name;

  if (cmd.isMember("link")) {
    add_link(query, cmd["link"], node_ref, VDMS_BLOB_EDGE_TAG);
  }

  return 0;
}

//========= UpdateBLOB definitions =========

UpdateBlob::UpdateBlob() : BlobCommand("UpdateBlob") {}

int UpdateBlob::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                   const std::string &blob, int grp_id,
                                   Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  // Update Image node
  query.UpdateNode(get_value<int>(cmd, "_ref", -1), VDMS_BLOB_TAG,
                   cmd["properties"], cmd["remove_props"], cmd["constraints"],
                   get_value<bool>(cmd, "unique", false));

  return 0;
}

//========= FindBLOB definitions =========

FindBlob::FindBlob() : BlobCommand("FindBlob") {}

int FindBlob::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                 const std::string &blob, int grp_id,
                                 Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  Json::Value results = get_value<Json::Value>(cmd, "results");

  // Unless otherwhis specified, we return the blob.
  if (get_value<bool>(results, "blob", true)) {
    results["list"].append(VDMS_EN_BLOB_PATH_PROP);
  }

  query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_BLOB_TAG, cmd["link"],
                  cmd["constraints"], results,
                  get_value<bool>(cmd, "unique", false));

  return 0;
}

Json::Value FindBlob::construct_responses(Json::Value &responses,
                                          const Json::Value &json,
                                          protobufs::queryMessage &query_res,
                                          const std::string &blob) {
  const Json::Value &cmd = json[_cmd_name];

  Json::Value ret;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  if (responses.size() != 1) {
    Json::Value return_error;
    return_error["status"] = RSCommand::Error;
    return_error["info"] = "PMGD Response Bad Size";
    return error(return_error);
  }

  Json::Value &findBlob = responses[0];

  if (findBlob["status"] != 0) {
    findBlob["status"] = RSCommand::Error;
    // Uses PMGD info error.
    return error(findBlob);
  }

  Json::Value results = get_value<Json::Value>(cmd, "results");

  bool flag_empty = false;

  // Check if blob (image) must be returned
  if (get_value<bool>(results, "blob", true)) {

    for (auto &ent : findBlob["entities"]) {

      assert(ent.isMember(VDMS_EN_BLOB_PATH_PROP));

      std::string blob_path = ent[VDMS_EN_BLOB_PATH_PROP].asString();
      ent.removeMember(VDMS_EN_BLOB_PATH_PROP);

      if (ent.getMemberNames().size() == 0) {
        flag_empty = true;
      }

      try {
        VCL::Image blob_im(blob_path);

        // We will return the image in the format the user
        // request, or on its format in disk, except for the case
        // of .tdb, where we will encode as png.
        VCL::Image::Format format = VCL::Image::Format::BIN;

        std::vector<unsigned char> blob_buffer;
        blob_buffer = blob_im.get_encoded_image(format);

        if (!blob_buffer.empty()) {

          std::string *blob_str = query_res.add_blobs();
          blob_str->resize(blob_buffer.size());
          std::memcpy((void *)blob_str->data(), (void *)blob_buffer.data(),
                      blob_buffer.size());
        } else {
          Json::Value return_error;
          return_error["status"] = RSCommand::Error;
          return_error["info"] = "Blob Data not found";
          return error(return_error);
        }
      } catch (VCL::Exception e) {
        print_exception(e);
        Json::Value return_error;
        return_error["status"] = RSCommand::Error;
        return_error["info"] = "VCL Exception";
        return error(return_error);
      }
    }
  }

  if (flag_empty) {
    findBlob.removeMember("entities");
  }

  ret[_cmd_name].swap(findBlob);
  return ret;
}