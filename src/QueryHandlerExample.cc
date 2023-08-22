/**
 * @file   QueryHandler.h
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

#include "QueryHandlerExample.h"

using namespace VDMS;

void QueryHandlerExample::init() {
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

QueryHandlerExample::QueryHandlerExample() {}

void QueryHandlerExample::process_connection(comm::Connection *c) {
  QueryMessage msgs(c);

  try {
    while (true) {
      protobufs::queryMessage response;
      protobufs::queryMessage query = msgs.get_query();
      process_query(query, response);
      msgs.send_response(response);
    }
  } catch (comm::ExceptionComm e) {
    print_exception(e);
  }
}

void QueryHandlerExample::process_query(protobufs::queryMessage &proto_query,
                                        protobufs::queryMessage &proto_res) {

  Json::FastWriter fastWriter;
  Json::Value hello_res;
  Json::Value json_responses;

  hello_res["HiThere"] = "Hello, world!";
  json_responses.append(hello_res);

  proto_res.set_json(fastWriter.write(json_responses));
}