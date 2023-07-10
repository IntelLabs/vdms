
/**
 * @file   RSCommand.cc
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "ExceptionsCommand.h"
#include "QueryHandler.h"
#include "VDMSConfig.h"
#include "defines.h"
#include "vcl/VCL.h"

using namespace VDMS;

RSCommand::RSCommand(const std::string &cmd_name) : _cmd_name(cmd_name) {
  _use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

Json::Value RSCommand::construct_responses(Json::Value &response,
                                           const Json::Value &json,
                                           protobufs::queryMessage &query_res,
                                           const std::string &blob) {
  Json::Value ret;
  ret[_cmd_name] = check_responses(response);

  return ret;
}

Json::Value RSCommand::check_responses(Json::Value &responses) {
  bool flag_error = false;
  Json::Value ret;

  if (responses.size() == 0) {
    ret["status"] = RSCommand::Error;
    ret["info"] = "No responses!";
    return ret;
  }

  for (auto &res : responses) {
    if (res["status"] != PMGDCmdResponse::Success &&
        res["status"] != PMGDCmdResponse::Exists) {
      flag_error = true;
      break;
    }
  }

  ret = responses[0];

  if (!flag_error) {
    ret["status"] = RSCommand::Success;
  }

  return ret;
}

namespace VDMS {
template <>
int RSCommand::get_value(const Json::Value &json, const std::string &key,
                         int def) {
  if (json.isMember(key))
    return json[key].asInt();

  return def;
}

template <>
double RSCommand::get_value(const Json::Value &json, const std::string &key,
                            double def) {
  if (json.isMember(key))
    return json[key].asDouble();

  return def;
}

template <>
bool RSCommand::get_value(const Json::Value &json, const std::string &key,
                          bool def) {
  if (json.isMember(key))
    return json[key].asBool();

  return def;
}

template <>
std::string RSCommand::get_value(const Json::Value &json,
                                 const std::string &key, std::string def) {
  if (json.isMember(key))
    return json[key].asString();

  return def;
}

template <>
Json::Value RSCommand::get_value(const Json::Value &json,
                                 const std::string &key, Json::Value def) {
  return json[key];
}
} // namespace VDMS

void RSCommand::add_link(PMGDQuery &query, const Json::Value &link,
                         int node_ref, const std::string tag) {
  // ref is guaranteed to exist at this point
  int dst = get_value<int>(link, "ref"); // Default is "out"
  int src = node_ref;
  if (link.isMember("direction") && link["direction"] == "in") {
    src = dst;
    dst = node_ref;
  }

  query.AddEdge(-1, src, dst, get_value<std::string>(link, "class", tag),
                link["properties"]);
}

//========= AddEntity definitions =========

AddEntity::AddEntity() : RSCommand("AddEntity") {
  _storage_blob = VDMSConfig::instance()->get_path_blobs();
}

bool AddEntity::need_blob(const Json::Value &jsoncmd) {
  const Json::Value &cmd = jsoncmd[_cmd_name];
  return get_value<bool>(cmd, "blob", false);
}

int AddEntity::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                  const std::string &blob, int grp_id,
                                  Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];
  bool link = cmd.isMember("link");

  int node_ref =
      get_value<int>(cmd, "_ref", link ? query.get_available_reference() : -1);

  // Modifiyng the existing properties that the user gives
  // is a good option to make the AddNode more simple.
  // This is not ideal since we are manupulating with user's
  // input, but for now it is an acceptable solution.
  Json::Value props = get_value<Json::Value>(cmd, "properties");

  if (get_value<bool>(cmd, "blob", false)) {
    std::ostringstream oss;
    oss << std::hex << VCL::get_uint64();
    std::string file_name = _storage_blob + "/" + oss.str();

    props[VDMS_EN_BLOB_PATH_PROP] = file_name;

    std::ofstream file;
    file.open(file_name);
    file << blob;
    file.close();
  }

  query.AddNode(node_ref, get_value<std::string>(cmd, "class"), props,
                cmd["constraints"]);

  if (link) {
    add_link(query, cmd["link"], node_ref, VDMS_GENERIC_LINK);
  }

  return 0;
}

//========= UpdateEntity definitions =========

UpdateEntity::UpdateEntity() : RSCommand("UpdateEntity") {}

int UpdateEntity::construct_protobuf(PMGDQuery &query,
                                     const Json::Value &jsoncmd,
                                     const std::string &blob, int grp_id,
                                     Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  query.UpdateNode(get_value<int>(cmd, "_ref", -1),
                   get_value<std::string>(cmd, "class"), cmd["properties"],
                   cmd["remove_props"], cmd["constraints"],
                   get_value<bool>(cmd, "unique", false));

  return 0;
}

//========= AddConnection definitions =========

AddConnection::AddConnection() : RSCommand("AddConnection") {}

int AddConnection::construct_protobuf(PMGDQuery &query,
                                      const Json::Value &jsoncmd,
                                      const std::string &blob, int grp_id,
                                      Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  query.AddEdge(get_value<int>(cmd, "_ref", -1),
                get_value<int>(cmd, "ref1", -1),      // src
                get_value<int>(cmd, "ref2", -1),      // dst
                get_value<std::string>(cmd, "class"), // tag
                cmd["properties"]);

  return 0;
}

//========= UpdateConnection definitions =========

UpdateConnection::UpdateConnection() : RSCommand("UpdateConnection") {}

int UpdateConnection::construct_protobuf(PMGDQuery &query,
                                         const Json::Value &jsoncmd,
                                         const std::string &blob, int grp_id,
                                         Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  query.UpdateEdge(
      get_value<int>(cmd, "_ref", -1), get_value<int>(cmd, "ref1", -1),
      get_value<int>(cmd, "ref2", -1), get_value<std::string>(cmd, "class"),
      cmd["properties"], cmd["remove_props"], cmd["constraints"],
      get_value<bool>(cmd, "unique", false));

  return 0;
}

//========= FindEntity definitions =========

FindEntity::FindEntity() : RSCommand("FindEntity") {}

int FindEntity::construct_protobuf(PMGDQuery &query, const Json::Value &jsoncmd,
                                   const std::string &blob, int grp_id,
                                   Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  Json::Value results = get_value<Json::Value>(cmd, "results");

  if (get_value<bool>(results, "blob", false)) {
    results["list"].append(VDMS_EN_BLOB_PATH_PROP);
  }

  query.QueryNode(get_value<int>(cmd, "_ref", -1),
                  get_value<std::string>(cmd, "class"), cmd["link"],
                  cmd["constraints"], results,
                  get_value<bool>(cmd, "unique", false));

  return 0;
}

Json::Value FindEntity::construct_responses(Json::Value &response,
                                            const Json::Value &json,
                                            protobufs::queryMessage &query_res,
                                            const std::string &blob) {
  assert(response.size() == 1);

  Json::Value ret;
  Json::Value &findEnt = response[0];

  const Json::Value &cmd = json[_cmd_name];

  if (get_value<bool>(cmd["results"], "blob", false)) {
    for (auto &ent : findEnt["entities"]) {

      if (ent.isMember(VDMS_EN_BLOB_PATH_PROP)) {
        std::string blob_path = ent[VDMS_EN_BLOB_PATH_PROP].asString();
        ent.removeMember(VDMS_EN_BLOB_PATH_PROP);

        std::string *blob_str = query_res.add_blobs();
        std::ifstream t(blob_path);
        t.seekg(0, std::ios::end);
        size_t size = t.tellg();
        blob_str->resize(size);
        t.seekg(0);
        t.read((char *)blob_str->data(), size);

        // For those cases the entity does not have a blob.
        // We need to indicate which entities have blobs.
        ent["blob"] = true;
      }
    }
  }

  // This will change the response tree,
  // but it is ok and avoids a copy
  ret[_cmd_name].swap(findEnt);

  return ret;
}

//========= DeleteExpired definitions =========

DeleteExpired::DeleteExpired() : RSCommand("DeleteExpired") {}

int DeleteExpired::construct_protobuf(PMGDQuery &query,
                                      const Json::Value &jsoncmd,
                                      const std::string &blob, int grp_id,
                                      Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];
  query.DeleteExpired();
  return 0;
}

Json::Value DeleteExpired::construct_responses(
    Json::Value &response, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {
  Json::Value ret;
  Json::Value ret_internal;
  ret_internal["status"] = RSCommand::Success;
  ret_internal["info"] = "AutoDelete";
  ret["DeleteExpired"] = ret_internal;
  return ret;
}

//========= FindConnection definitions =========

FindConnection::FindConnection() : RSCommand("FindConnection") {}

int FindConnection::construct_protobuf(PMGDQuery &query,
                                       const Json::Value &jsoncmd,
                                       const std::string &blob, int grp_id,
                                       Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  query.QueryEdge(get_value<int>(cmd, "_ref", -1),
                  get_value<int>(cmd, "ref1", -1),
                  get_value<int>(cmd, "ref2", -1),
                  get_value<std::string>(cmd, "class"), cmd["constraints"],
                  cmd["results"], get_value<bool>(cmd, "unique", false));

  return 0;
}
