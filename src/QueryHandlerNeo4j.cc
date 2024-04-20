/**
 * @file   QueryHandlerNeo4j.cc
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
#include "QueryHandlerNeo4j.h"
#include "../include/vcl/RemoteConnection.h"
#include "APISchema.h"
#include "BackendNeo4j.h"
#include "Neo4JCommands.h"
#include "Neo4jQueryHelpers.h"
#include "OpsIOCoordinator.h"
#include "VDMSConfig.h"
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>

#include <chrono>

using namespace VDMS;

std::unordered_map<std::string, Neo4jCommand *> QueryHandlerNeo4j::_rs_cmds;
BackendNeo4j *QueryHandlerNeo4j::neoconn_pool;
// VCL::RemoteConnection *global_s3_connection;

void QueryHandlerNeo4j::init() {

  _rs_cmds["NeoAdd"] = new Neo4jNeoAdd();
  _rs_cmds["NeoFind"] = new Neo4jNeoFind();

  char *env_4j_port = getenv("NEO_TEST_PORT");
  char *user = getenv("NEO4J_USER");
  char *pass = getenv("NEO4J_PASS");

  std::string tgtdb_base = "neo4j://localhost:";
  std::string tgtdb_port(env_4j_port);
  std::string tgtdb_addr = tgtdb_base + tgtdb_port;
  const char *tgtdb = tgtdb_addr.c_str();

  uint_fast32_t flags = NEO4J_INSECURE;
  int nr_conns = 16;

  neoconn_pool = new BackendNeo4j(nr_conns, (char *)tgtdb, user, pass, flags);

  // Load the string containing the schema (api_schema/APISchema.h)
  Json::Reader reader;
  Json::Value api_schema;
  bool parseSuccess = reader.parse(schema_json.c_str(), api_schema);
  if (!parseSuccess) {
    std::cerr << "Failed to parse API reference schema." << std::endl;
    std::cerr << "PANIC! Aborting." << std::endl;
    exit(0);
  }

  // Parse the json schema into an internal schema format
  valijson::SchemaParser parser;
  valijson::adapters::JsonCppAdapter schemaDocumentAdapter(api_schema);
  try {
    parser.populateSchema(schemaDocumentAdapter, *_schema);
  } catch (std::exception &e) {
    std::cerr << "Failed to load schema: " << e.what() << std::endl;
    std::cerr << "PANIC! Aborting." << std::endl;
    exit(0);
  }
}

QueryHandlerNeo4j::QueryHandlerNeo4j() {}

bool QueryHandlerNeo4j::syntax_checker(const Json::Value &root,
                                       Json::Value &error) {
  valijson::ValidationResults results;
  valijson::adapters::JsonCppAdapter user_query(root);
  std::cerr << root.toStyledString() << std::endl; // TEMPORARY
  if (!_validator.validate(*_schema, user_query, &results)) {
    std::cerr << "API validation failed for:" << std::endl;
    std::cerr << root.toStyledString() << std::endl;

    // Will attempt to find the simple error
    // To avoid valijson dump
    for (int j = 0; j < root.size(); j++) {
      const Json::Value &query = root[j];
      if (query.getMemberNames().size() != 1) {
        error["info"] = "Error: Only one command per element allowed";
        return false;
      }

      const std::string cmd_str = query.getMemberNames()[0];
      auto it = _rs_cmds.find(cmd_str);
      if (it == _rs_cmds.end()) {
        error["info"] = cmd_str + ": Command not found!";
        return false;
      }
    }

    valijson::ValidationResults::Error va_error;
    unsigned int errorNum = 1;
    std::stringstream str_error;
    while (results.popError(va_error)) {
      std::string context;
      std::vector<std::string>::iterator itr = va_error.context.begin();
      for (; itr != va_error.context.end(); itr++) {
        context += *itr;
      }

      str_error << "Error #" << errorNum << std::endl
                << "  context: " << context << std::endl
                << "  desc:    " << va_error.description << std::endl;
      ++errorNum;
    }
    std::cerr << str_error.str();
    error["info"] = str_error.str();
    return false;
  }

  for (auto &cmdTop : root) {
    const std::string cmd_str = cmdTop.getMemberNames()[0];
    auto &cmd = cmdTop[cmd_str];
    if (cmd.isMember("constraints")) {
      for (auto &member : cmd["constraints"].getMemberNames()) {
        if (!cmd["constraints"][member].isArray()) {
          error["info"] =
              "Constraint for property '" + member + "' must be an array";
          return false;
        }
        auto size = cmd["constraints"][member].size();
        if (size != 2 && size != 4) {
          error["info"] = "Constraint for property '" + member +
                          "' must be an array of size 2 or 4";
          return false;
        }
      }
    }
  }

  return true;
}

void QueryHandlerNeo4j::process_query(protobufs::queryMessage &proto_query,
                                      protobufs::queryMessage &proto_res) {

  std::chrono::steady_clock::time_point dbconn_start, dbconn_end;
  std::chrono::steady_clock::time_point pre_proc_start, pre_proc_end;
  std::chrono::steady_clock::time_point resp_start, resp_end;
  std::chrono::steady_clock::time_point total_start, total_end;
  std::chrono::steady_clock::time_point db_trans_time_start, db_trans_time_end;
  std::chrono::steady_clock::time_point db_cmt_time_start, db_cmt_time_end;
  double total_runtime, db_conn_time, pre_proc_time, cons_resp_time,
      db_trans_time, db_cmt_time;

  neo4j_transaction *tx;
  neo4j_connection_t *conn;
  neo4j_result_stream_t *res_stream;

  total_start = std::chrono::steady_clock::now();
  dbconn_start = std::chrono::steady_clock::now();
  conn = neoconn_pool->get_conn();
  ///// connection retrieved
  dbconn_end = std::chrono::steady_clock::now();

  int rc;

  Json::FastWriter fastWriter;
  Json::Value hello_res;
  Json::Value json_responses;
  Json::Value cmd_result;

  Json::Value root;
  int blob_count = 0;

  rc = parse_commands(proto_query, root);

  // begin neo4j transaction
  tx = neoconn_pool->open_tx(conn, 10000, "w");
  for (int j = 0; j < root.size(); j++) {
    Json::Value neo4j_resp;
    std::string cypher;

    const Json::Value &query = root[j];
    std::string cmd = query.getMemberNames()[0];

    Neo4jCommand *rscmd = _rs_cmds[cmd];

    cypher = query[cmd]["cypher"].asString();

    const std::string &blob =
        rscmd->need_blob(query) ? proto_query.blobs(blob_count++) : "";

    pre_proc_start = std::chrono::steady_clock::now();
    rscmd->data_processing(cypher, query, blob, 0, cmd_result);
    pre_proc_end = std::chrono::steady_clock::now();

    db_trans_time_start = std::chrono::steady_clock::now();
    res_stream = neoconn_pool->run_in_tx((char *)cypher.c_str(), tx);
    db_trans_time_end = std::chrono::steady_clock::now();

    neo4j_resp = neoconn_pool->results_to_json(res_stream);

    resp_start = std::chrono::steady_clock::now();
    rscmd->construct_responses(neo4j_resp, query, proto_res, blob);
    resp_end = std::chrono::steady_clock::now();

    if (neo4j_resp.isMember("metadata_res")) {

      hello_res["metadata_res"] = neo4j_resp["metadata_res"];
    }

    json_responses.append(hello_res);

    proto_res.set_json(fastWriter.write(json_responses));
  }
  // commit neo4j transaction

  db_cmt_time_start = std::chrono::steady_clock::now();
  neoconn_pool->commit_tx(tx);
  db_cmt_time_end = std::chrono::steady_clock::now();
  neoconn_pool->put_conn(conn);
  total_end = std::chrono::steady_clock::now();

  db_conn_time = std::chrono::duration_cast<std::chrono::microseconds>(
                     dbconn_end - dbconn_start)
                     .count();
  pre_proc_time = std::chrono::duration_cast<std::chrono::microseconds>(
                      pre_proc_end - pre_proc_start)
                      .count();
  cons_resp_time = std::chrono::duration_cast<std::chrono::microseconds>(
                       resp_end - resp_start)
                       .count();
  total_runtime = std::chrono::duration_cast<std::chrono::microseconds>(
                      total_end - total_start)
                      .count();
  db_trans_time = std::chrono::duration_cast<std::chrono::microseconds>(
                      db_trans_time_end - db_trans_time_start)
                      .count();
  db_cmt_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    db_cmt_time_end - db_cmt_time_start)
                    .count();
}

int QueryHandlerNeo4j::parse_commands(
    const protobufs::queryMessage &proto_query, Json::Value &root) {
  Json::Reader reader;
  const std::string commands = proto_query.json();

  try {
    bool parseSuccess = reader.parse(commands.c_str(), root);

    if (!parseSuccess) {
      root["info"] = "Error parsing the query, ill formed JSON";
      root["status"] = Neo4jCommand::Error;
      return -1;
    }

    Json::Value error;
    if (!syntax_checker(root, error)) {
      root = error;
      root["status"] = Neo4jCommand::Error;
      return -1;
    }

    unsigned blob_counter = 0;
    for (int j = 0; j < root.size(); j++) {
      const Json::Value &query = root[j];
      assert(query.getMemberNames().size() == 1);
      std::string cmd = query.getMemberNames()[0];

      if (_rs_cmds[cmd]->need_blob(query)) {
        blob_counter++;
      }
    }

    if ((blob_counter != 0) && (blob_counter != proto_query.blobs().size())) {
      root = error;
      root["info"] = std::string(
          "Expected blobs: " + std::to_string(blob_counter) +
          ". Received blobs: " + std::to_string(proto_query.blobs().size()));
      root["status"] = Neo4jCommand::Error;
      std::cerr << "Number of Blobs Mismatch!" << std::endl;
      return -1;
    }

  } catch (Json::Exception const &) {
    root["info"] = "Json Exception at Parsing";
    root["status"] = Neo4jCommand::Error;
    return -1;
  }

  return 0;
}
