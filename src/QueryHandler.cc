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

#include <string>
#include <sstream>
#include <fstream>
#include "QueryHandler.h"

#include "ImageCommand.h"
#include "ExceptionsCommand.h"

#include "PMGDQuery.h"
#include "QueryMessage.h"
#include "pmgd.h"
#include "util.h"

#include "api_schema/APISchema.h"
#include <jsoncpp/json/writer.h>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/schema_parser.hpp>

using namespace VDMS;

std::unordered_map<std::string, RSCommand *> QueryHandler::_rs_cmds;
valijson::Schema* QueryHandler::_schema = new valijson::Schema;

void QueryHandler::init()
{
    _rs_cmds["AddEntity"]  = new AddEntity();
    _rs_cmds["UpdateEntity"]  = new UpdateEntity();
    _rs_cmds["Connect"]    = new Connect();
    _rs_cmds["FindEntity"] = new FindEntity();
    _rs_cmds["AddImage"]   = new AddImage();
    _rs_cmds["FindImage"]  = new FindImage();

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
    }
    catch (std::exception &e) {
        std::cerr << "Failed to load schema: " << e.what() << std::endl;
        std::cerr << "PANIC! Aborting." << std::endl;
        exit(0);
    }
}

QueryHandler::QueryHandler()
    : _pmgd_qh(),
    _validator(valijson::Validator::kWeakTypes)
#ifdef CHRONO_TIMING
    ,ch_tx_total("ch_tx_total")
    ,ch_tx_query("ch_tx_query")
    ,ch_tx_send("ch_tx_send")
#endif
{
}

void QueryHandler::process_connection(comm::Connection *c)
{
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

bool QueryHandler::syntax_checker(const Json::Value& root, Json::Value& error)
{
    valijson::ValidationResults results;
    valijson::adapters::JsonCppAdapter user_query(root);
    if (!_validator.validate(*_schema, user_query, &results)) {
        std::cerr << "API validation failed for:" << std::endl;
        Json::StyledWriter swriter;
        std::cerr << swriter.write(root) << std::endl;

        // Will attempt to find the simple error
        // To avoid valijson dump
        for (int j = 0; j < root.size(); j++) {
            const Json::Value& query = root[j];
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

    return true;
}

int QueryHandler::parse_commands(const protobufs::queryMessage& proto_query,
                                 Json::Value& root)
{
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
            const Json::Value& query = root[j];
            assert(query.getMemberNames().size() == 1);
            std::string cmd = query.getMemberNames()[0];

            if (_rs_cmds[cmd]->need_blob(query))
                blob_counter++;
        }

        if (blob_counter != proto_query.blobs().size()) {
            root = error;
            root["info"] = std::string("Expected blobs: " +
                                std::to_string(blob_counter) +
                                ". Received blobs: " +
                                std::to_string(proto_query.blobs().size()));
            root["status"] = RSCommand::Error;
            std::cerr << "Not enough blobs!" << std::endl;
            return -1;
        }

    } catch (Json::Exception const&) {
        root["info"] = "Json Exception at Parsing";
        root["status"] = RSCommand::Error;
        return -1;
    }

    return 0;
}

void QueryHandler::cleanup_query(const std::vector<std::string>& images)
{
    for (auto& img_path : images) {
        VCL::Image img(img_path);
        img.delete_image();
    }
}

void QueryHandler::process_query(protobufs::queryMessage& proto_query,
                                 protobufs::queryMessage& proto_res)
{
    Json::FastWriter fastWriter;

    try {
        Json::Value json_responses;
        Json::Value root;

        Json::Value cmd_result;
        Json::Value cmd_current;
        std::vector<std::string> images_log;

        auto error = [&](Json::Value& res, Json::Value& failed_command)
        {
            cleanup_query(images_log);
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

        //iterate over the list of the queries
        for (int j = 0; j < root.size(); j++) {
            const Json::Value& query = root[j];
            std::string cmd = query.getMemberNames()[0];

            int group_count = pmgd_query.add_group();

            RSCommand* rscmd = _rs_cmds[cmd];

            const std::string& blob = rscmd->need_blob(query) ?
                                      proto_query.blobs(blob_count++) : "";

            int ret_code = rscmd->construct_protobuf(pmgd_query, query, blob,
                                                     group_count, cmd_result);

            if (cmd_result.isMember("image_added")) {
                images_log.push_back(cmd_result["image_added"].asString());
            }

            if (ret_code != 0) {
                error(cmd_result, root[j]);
                return;
            }
        }

        Json::Value& tx_responses = pmgd_query.run();

        if (tx_responses.size() != root.size()) { // error
            cmd_current = "Transaction";
            cmd_result = tx_responses;
            cmd_result["info"] = "Failed PMGDTransaction";
            cmd_result["status"] = RSCommand::Error;
            error(cmd_result, cmd_current);
            return;
        }
        else {
            for (int j = 0; j < root.size(); j++) {
                std::string cmd = root[j].getMemberNames()[0];

                cmd_result = _rs_cmds[cmd]->construct_responses(
                                            tx_responses[j],
                                            root[j], proto_res);

                // This is for error handling
                if (cmd_result.isMember("status")) {
                    int status = cmd_result["status"].asInt();
                    if (status != RSCommand::Success ||
                        status != RSCommand::Empty   ||
                        status != RSCommand::Exists)
                    {
                        error(cmd_result, root[j]);
                        return;
                    }
                }
                json_responses.append(cmd_result);
            }
        }

        proto_res.set_json(fastWriter.write(json_responses));

    } catch (VCL::Exception e) {
        print_exception(e);
        std::cerr << "FATAL ERROR: VCL Exception at QH" << std::endl;
        exit(0);
    } catch (PMGD::Exception e) {
        print_exception(e);
        std::cerr << "FATAL ERROR: PMGD Exception at QH" << std::endl;
        exit(0);
    } catch (ExceptionCommand e) {
        print_exception(e);
        std::cerr << "FATAL ERROR: Command Exception at QH" << std::endl;
        exit(0);
    } catch (Json::Exception const& e) {
        // Should not happen
        // In case of error on the last fastWriter
        std::cerr << "FATAL: Json Exception:" << e.what() << std::endl;
        Json::Value error;
        error["info"] = "Internal Server Error: Json Exception";
        error["status"] = RSCommand::Error;
        proto_res.set_json(fastWriter.write(error));
    } catch (const std::invalid_argument& ex) {
        std::cerr << "Invalid argument: " << ex.what() << '\n';
        exit(0);
    }
}
