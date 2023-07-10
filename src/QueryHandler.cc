/**
 * @file   QueryHandler.cc
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

#include "QueryHandler.h"
#include <fstream>
#include <sstream>
#include <string>

#include "BlobCommand.h"
#include "BoundingBoxCommand.h"
#include "DescriptorsCommand.h"
#include "ImageCommand.h"
#include "VideoCommand.h"

#include "ExceptionsCommand.h"

#include "PMGDQuery.h"
#include "QueryMessage.h"
#include "pmgd.h"
#include "util.h"

#include "APISchema.h"
#include <jsoncpp/json/writer.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>

using namespace VDMS;

std::unordered_map<std::string, RSCommand *> QueryHandler::_rs_cmds;
valijson::Schema *QueryHandler::_schema = new valijson::Schema;

void QueryHandler::init() {
  DescriptorsManager::init();

  _rs_cmds["AddEntity"] = new AddEntity();
  _rs_cmds["UpdateEntity"] = new UpdateEntity();
  _rs_cmds["FindEntity"] = new FindEntity();

  _rs_cmds["AddConnection"] = new AddConnection();
  _rs_cmds["UpdateConnection"] = new UpdateConnection();
  _rs_cmds["FindConnection"] = new FindConnection();

  _rs_cmds["AddImage"] = new AddImage();
  _rs_cmds["UpdateImage"] = new UpdateImage();
  _rs_cmds["FindImage"] = new FindImage();
  _rs_cmds["DeleteExpired"] = new DeleteExpired();

  _rs_cmds["AddDescriptorSet"] = new AddDescriptorSet();
  _rs_cmds["AddDescriptor"] = new AddDescriptor();
  _rs_cmds["FindDescriptor"] = new FindDescriptor();
  _rs_cmds["ClassifyDescriptor"] = new ClassifyDescriptor();

  _rs_cmds["AddBoundingBox"] = new AddBoundingBox();
  _rs_cmds["UpdateBoundingBox"] = new UpdateBoundingBox();
  _rs_cmds["FindBoundingBox"] = new FindBoundingBox();

  _rs_cmds["AddVideo"] = new AddVideo();
  _rs_cmds["UpdateVideo"] = new UpdateVideo();
  _rs_cmds["FindVideo"] = new FindVideo();
  _rs_cmds["FindFrames"] = new FindFrames();

  _rs_cmds["AddBlob"] = new AddBlob();
  _rs_cmds["UpdateBlob"] = new UpdateBlob();
  _rs_cmds["FindBlob"] = new FindBlob();

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

QueryHandler::QueryHandler()
    : _pmgd_qh(), _validator(valijson::Validator::kWeakTypes),
      _autodelete_init(false), _autoreplicate_init(false)
#ifdef CHRONO_TIMING
      ,
      ch_tx_total("ch_tx_total"), ch_tx_query("ch_tx_query"),
      ch_tx_send("ch_tx_send")
#endif
{
}

void QueryHandler::process_connection(comm::Connection *c) {
  QueryMessage msgs(c);

  try {
    while (true) {
      protobufs::queryMessage response;
      protobufs::queryMessage query = msgs.get_query();
      CHRONO_TIC(ch_tx_total);

      CHRONO_TIC(ch_tx_query);
      process_query(query, response);
      CHRONO_TAC(ch_tx_query);

      CHRONO_TIC(ch_tx_send);
      msgs.send_response(response);
      CHRONO_TAC(ch_tx_send);

      CHRONO_TAC(ch_tx_total);
      CHRONO_PRINT_LAST_MS(ch_tx_total);
      CHRONO_PRINT_LAST_MS(ch_tx_query);
      CHRONO_PRINT_LAST_MS(ch_tx_send);
    }
  } catch (comm::ExceptionComm e) {
    print_exception(e);
  }
}

bool QueryHandler::syntax_checker(const Json::Value &root, Json::Value &error) {
  valijson::ValidationResults results;
  valijson::adapters::JsonCppAdapter user_query(root);
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

int QueryHandler::parse_commands(const protobufs::queryMessage &proto_query,
                                 Json::Value &root) {
  Json::Reader reader;
  const std::string commands = proto_query.json();

  try {
    bool parseSuccess = reader.parse(commands.c_str(), root);

    if (!parseSuccess) {
      root["info"] = "Error parsing the query, ill formed JSON";
      root["status"] = RSCommand::Error;
      return -1;
    }

    Json::Value error;
    if (!syntax_checker(root, error)) {
      root = error;
      root["status"] = RSCommand::Error;
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

    if (blob_counter != proto_query.blobs().size()) {
      root = error;
      root["info"] = std::string(
          "Expected blobs: " + std::to_string(blob_counter) +
          ". Received blobs: " + std::to_string(proto_query.blobs().size()));
      root["status"] = RSCommand::Error;
      std::cerr << "Not enough blobs!" << std::endl;
      return -1;
    }

  } catch (Json::Exception const &) {
    root["info"] = "Json Exception at Parsing";
    root["status"] = RSCommand::Error;
    return -1;
  }

  return 0;
}

// TODO create a better mechanism to cleanup queries that
// includes feature vectors and user-defined blobs
// For now, we do it for videos/images as a starting point.
void QueryHandler::cleanup_query(const std::vector<std::string> &images,
                                 const std::vector<std::string> &videos) {
  for (auto &img_path : images) {
    VCL::Image img(img_path);
    img.delete_image();
  }

  for (auto &vid_path : videos) {
    VCL::Video vid(vid_path);
    vid.delete_video();
  }
}

void QueryHandler::process_query(protobufs::queryMessage &proto_query,
                                 protobufs::queryMessage &proto_res) {
  Json::FastWriter fastWriter;

  Json::Value root;
  Json::Value exception_error;
  std::stringstream error_msg;
  auto exception_handler = [&]() {
    // When exception is catched, we return the message.
    std::cerr << "Failed Query: " << std::endl;
    std::cerr << root << std::endl;
    std::cerr << error_msg.str();
    std::cerr << "End Failed Query: " << std::endl;
    exception_error["info"] = error_msg.str();
    exception_error["status"] = RSCommand::Error;
    Json::Value response;
    response.append(exception_error);
    proto_res.set_json(fastWriter.write(response));
  };

  try {
    Json::Value json_responses;

    Json::Value cmd_result;
    Json::Value cmd_current;
    std::vector<std::string> images_log;
    std::vector<std::string> videos_log;
    std::vector<Json::Value> construct_results;

    auto error = [&](Json::Value &res, Json::Value &failed_command) {
      cleanup_query(images_log, videos_log);
      res["FailedCommand"] = failed_command;
      json_responses.clear();
      json_responses.append(res);
      proto_res.clear_blobs();
      proto_res.set_json(fastWriter.write(json_responses));
      Json::StyledWriter w;
      std::cerr << w.write(json_responses);
    };

    if (parse_commands(proto_query, root) != 0) {
      cmd_current = "Transaction";
      error(root, cmd_current);
      return;
    }

    PMGDQuery pmgd_query(_pmgd_qh);
    int blob_count = 0;

    // iterate over the list of the queries
    for (int j = 0; j < root.size(); j++) {
      const Json::Value &query = root[j];
      std::string cmd = query.getMemberNames()[0];

      int group_count = pmgd_query.add_group();

      RSCommand *rscmd = _rs_cmds[cmd];

      const std::string &blob =
          rscmd->need_blob(query) ? proto_query.blobs(blob_count++) : "";

      int ret_code = rscmd->construct_protobuf(pmgd_query, query, blob,
                                               group_count, cmd_result);

      if (cmd_result.isMember("image_added")) {
        images_log.push_back(cmd_result["image_added"].asString());
      }
      if (cmd_result.isMember("video_added")) {
        videos_log.push_back(cmd_result["video_added"].asString());
      }

      if (ret_code != 0) {
        error(cmd_result, root[j]);
        return;
      }

      construct_results.push_back(cmd_result);
    }

    Json::Value &tx_responses = pmgd_query.run(_autodelete_init);

    if (!tx_responses.isArray() || tx_responses.size() != root.size()) {
      Json::StyledWriter writer;
      std::cerr << "PMGD Response:" << std::endl;
      std::cerr << writer.write(tx_responses) << std::endl;

      std::string tx_error_msg("Failed PMGD Transaction");
      if (!tx_responses.isArray() && tx_responses.isMember("info")) {
        tx_error_msg += ": " + tx_responses["info"].asString();
      }

      cmd_result["status"] = RSCommand::Error;
      cmd_result["info"] = tx_error_msg;

      cmd_current = "Transaction";
      error(cmd_result, cmd_current);
      return;
    } else {
      blob_count = 0;
      for (int j = 0; j < root.size(); j++) {
        Json::Value &query = root[j];
        std::string cmd = query.getMemberNames()[0];

        RSCommand *rscmd = _rs_cmds[cmd];

        const std::string &blob =
            rscmd->need_blob(query) ? proto_query.blobs(blob_count++) : "";

        query["cp_result"] = construct_results[j];
        cmd_result =
            rscmd->construct_responses(tx_responses[j], query, proto_res, blob);

        // This is for error handling
        if (cmd_result.isMember("status")) {
          int status = cmd_result["status"].asInt();
          if (status != RSCommand::Success || status != RSCommand::Empty ||
              status != RSCommand::Exists) {
            error(cmd_result, root[j]);
            return;
          }
        }
        json_responses.append(cmd_result);
      }
    }
    proto_res.set_json(fastWriter.write(json_responses));
    _pmgd_qh.cleanup_files();

  } catch (VCL::Exception &e) {
    print_exception(e);
    error_msg << "Internal Server Error: VCL Exception at QH" << std::endl;
    exception_handler();
  } catch (PMGD::Exception &e) {
    print_exception(e);
    error_msg << "Internal Server Error: PMGD Exception at QH" << std::endl;
    exception_handler();
  } catch (ExceptionCommand &e) {
    print_exception(e);
    error_msg << "Internal Server Error: Command Exception at QH" << std::endl;
    exception_handler();
  } catch (Json::Exception const &e) {
    // In case of error on the last fastWriter
    error_msg << "Internal Server Error: Json Exception: " << e.what()
              << std::endl;
    exception_handler();
  } catch (google::protobuf::FatalException &e) {
    // Need to be carefull with this, may lead to memory leak.
    // Protoubuf is not exception safe.
    error_msg << "Internal Server Error: Protobuf Exception: " << e.what()
              << std::endl;
    exception_handler();
  } catch (const std::invalid_argument &e) {
    error_msg << "FATAL: Invalid argument: " << e.what() << std::endl;
    exception_handler();
  } catch (const std::exception &e) {
    error_msg << "std Exception: " << e.what() << std::endl;
    exception_handler();
  } catch (...) {
    error_msg << "Unknown Exception" << std::endl;
    exception_handler();
  }
}

void QueryHandler::regualar_run_autoreplicate(
    ReplicationConfig &replicate_settings) {
  std::string command = "bsdtar cvfz ";
  std::string name;
  std::ostringstream oss;
  Json::Value config_file;
  std::ofstream file_id;
  name.clear();
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  oss << asctime(&tm);
  name = oss.str();
  name.erase(remove(name.begin(), name.end(), ' '), name.end());
  name.erase(std::remove(name.begin(), name.end(), '\n'), name.end());
  std::string full_name = replicate_settings.backup_path + "/" + name;

  command = command + " " + full_name + ".tar.gz  " +
            replicate_settings.db_path; // current_date_time

  system(command.c_str());

  if (replicate_settings.server_port != 0) {
    config_file["port"] = replicate_settings.server_port;
  }

  if (!full_name.empty()) {
    config_file["db_root_path"] = full_name;
  }

  if (replicate_settings.autodelete_interval > 0) {
    config_file["autodelete_interval"] =
        replicate_settings
            .autodelete_interval; // expired data removed daily (86400 secs)
  }

  if (replicate_settings.expiration_time > 0) {
    config_file["expiration_time"] = replicate_settings.expiration_time;
  }

  config_file["more-info"] = "github.com/IntelLabs/vdms";

  if (!replicate_settings.replication_time.empty()) {
    config_file["autoreplicate_time"] = replicate_settings.replication_time;
  }

  if (!replicate_settings.autoreplication_unit.empty()) {
    config_file["unit"] = replicate_settings.autoreplication_unit;
  }

  if (replicate_settings.autoreplicate_interval > 0) {
    config_file["autoreplicate_interval"] =
        replicate_settings.autoreplicate_interval;
  }

  if (replicate_settings.max_simultaneous_clients > 0) {
    config_file["max_simultaneous_clients"] =
        replicate_settings.max_simultaneous_clients;
  }

  if (!replicate_settings.backup_flag.empty()) {
    config_file["backup_flag"] = replicate_settings.backup_flag;
  }
  if (!replicate_settings.backup_flag.empty()) {
    config_file["backup_path"] = replicate_settings.backup_path;
  }
  if (!replicate_settings.backup_flag.empty()) {
    config_file["images_path"] = replicate_settings.images_path;
  }
  if (!replicate_settings.backup_flag.empty()) {
    config_file["blobs_path"] = replicate_settings.blobs_path;
  }
  if (!replicate_settings.backup_flag.empty()) {
    config_file["descriptor_path"] = replicate_settings.descriptor_path;
  }
  if (!replicate_settings.backup_flag.empty()) {
    config_file["pmgd_num_allocators"] = replicate_settings.pmgd_num_allocators;
  }
  std::cout << config_file << std::endl;
  // write the configuration file
  std::string config_file_name = full_name + ".json";
  file_id.open(config_file_name.c_str(), std::ios::out);
  file_id << config_file << std::endl;
  file_id.close();

  command = "bsdtar cvfz ";
  oss.str(std::string());
  name.clear();
  config_file.clear();
}
void QueryHandler::reset_autoreplicate_init_flag() {
  _autoreplicate_init = true;
}
void QueryHandler::set_autoreplicate_init_flag() {
  _autoreplicate_init = false;
}
void QueryHandler::reset_autodelete_init_flag() { _autodelete_init = false; }

void QueryHandler::set_autodelete_init_flag() { _autodelete_init = true; }

void QueryHandler::regualar_run_autodelete() {
  std::string *json_string = new std::string(
      "[{\"DeleteExpired\": {\"results\": {\"list\": [\"_expiration\"]}}}]");
  protobufs::queryMessage response;
  protobufs::queryMessage query;
  query.set_json(json_string->c_str());
  process_query(query, response);
  delete json_string;
}

void QueryHandler::build_autodelete_queue() {
  std::string *json_string = new std::string(
      "[{\"FindImage\": {\"results\": {\"list\": [\"_expiration\"]}, "
      "\"constraints\": {\"_expiration\": [\">\", 0]}}}, {\"FindVideo\": "
      "{\"results\": {\"list\": [\"_expiration\"]}, \"constraints\": "
      "{\"_expiration\": [\">\", 0]}}}], {\"FindFrames\": {\"results\": "
      "{\"list\": [\"_expiration\"]}, \"constraints\": {\"_expiration\": "
      "[\">\", 0]}}}], {\"FindDescriptor\": {\"results\": {\"list\": "
      "[\"_expiration\"]}, \"constraints\": {\"_expiration\": [\">\", 0]}}}], "
      "{\"FindEntity\": {\"results\": {\"list\": [\"_expiration\"]}, "
      "\"constraints\": {\"_expiration\": [\">\", 0]}}}");
  protobufs::queryMessage response;
  protobufs::queryMessage query;
  query.set_json(json_string->c_str());
  process_query(query, response);
  delete json_string;
}
