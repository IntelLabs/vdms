/**
 * @file   Neo4jCommands.h
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

#pragma once

#include "queryMessage.pb.h"
#include "vcl/Image.h"
#include "vcl/VCL.h"
#include <jsoncpp/json/value.h>

namespace VDMS {

class Neo4jCommand {
protected:
  const std::string _cmd_name;
  std::map<std::string, int> _valid_params_map;

  template <typename T>
  T get_value(const Json::Value &json, const std::string &key, T def = T());
  void add_link(std::string &tx, const Json::Value &link, int node_ref,
                const std::string tag);
  virtual Json::Value check_responses(Json::Value &responses);
  bool _use_aws_storage;

public:
  enum ErrorCode {
    Success = 0,
    Error = -1,
    Empty = 1,
    Exists = 2,
    NotUnique = 3
  };

  Neo4jCommand(const std::string &cmd_name);
  virtual bool need_blob(const Json::Value &cmd) { return false; }
  virtual int data_processing(std::string &tx, const Json::Value &root,
                              const std::string &blob, int grp_id,
                              Json::Value &error) = 0;
  virtual Json::Value construct_responses(Json::Value &json_responses,
                                          const Json::Value &json,
                                          protobufs::queryMessage &response,
                                          const std::string &blob);
};

// Cypher Based Commands
class Neo4jNeoAdd : public Neo4jCommand {
  std::string _storage_tdb;
  std::string _storage_png;
  std::string _storage_jpg;
  std::string _storage_bin;

public:
  Neo4jNeoAdd();
  bool need_blob(const Json::Value &cmd);
  int data_processing(std::string &tx, const Json::Value &root,
                      const std::string &blob, int grp_id, Json::Value &error);
  Json::Value construct_responses(Json::Value &neo4j_responses,
                                  const Json::Value &orig_query,
                                  protobufs::queryMessage &query_res,
                                  const std::string &blob);
};

class Neo4jNeoFind : public Neo4jCommand {

public:
  Neo4jNeoFind();
  bool need_blob(const Json::Value &cmd) { return false; }
  int data_processing(std::string &tx, const Json::Value &root,
                      const std::string &blob, int grp_id, Json::Value &error);
  Json::Value construct_responses(Json::Value &json_responses,
                                  const Json::Value &json,
                                  protobufs::queryMessage &response,
                                  const std::string &blob);
};

class Neo4jNeoAddDescSet : public Neo4jCommand{

public:
    Neo4jNeoAddDescSet();
    bool need_blob(const Json::Value &cmd){return false;};
    int data_processing(std::string &tx, const Json::Value &root,
                        const std::string &blob, int grp_id, Json::Value &error);
    Json::Value construct_responses(Json::Value &json_responses,
                                    const Json::Value &json,
                                    protobufs::queryMessage &response,
                                    const std::string &blob);

};

class Neo4jNeoFindDescSet : public Neo4jCommand{

public:
    Neo4jNeoFindDescSet();
    bool need_blob(const Json::Value &cmd){return false;};
    int data_processing(std::string &tx, const Json::Value &root,
                        const std::string &blob, int grp_id, Json::Value &error);
    Json::Value construct_responses(Json::Value &json_responses,
                                    const Json::Value &json,
                                    protobufs::queryMessage &response,
                                    const std::string &blob);

};

class Neo4jNeoAddDesc : public Neo4jCommand{
    public:
        Neo4jNeoAddDesc();
        bool need_blob(const Json::Value &cmd) {return true;};
        int data_processing(std::string &tx, const Json::Value &root,
                            const std::string &blob, int grp_id, Json::Value &error);
        Json::Value construct_responses(Json::Value &json_responses,
                                        const Json::Value &json,
                                        protobufs::queryMessage &response,
                                        const std::string &blob);
};


class Neo4jNeoFindDesc : public Neo4jCommand{
public:
    Neo4jNeoFindDesc();
    bool need_blob(const Json::Value &cmd);
    int data_processing(std::string &tx, const Json::Value &root,
                        const std::string &blob, int grp_id, Json::Value &error);
    Json::Value construct_responses(Json::Value &json_responses,
                                    const Json::Value &json,
                                    protobufs::queryMessage &response,
                                    const std::string &blob);
};

} // namespace VDMS
