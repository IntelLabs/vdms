/**
 * @file   QueryHandler.h
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

#pragma once
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "PMGDQueryHandler.h" // to provide the database connection
#include "RSCommand.h"
#include "Server.h"
#include "chrono/Chrono.h"

// Json parsing files
#include <jsoncpp/json/value.h>
#include <valijson/schema.hpp>
#include <valijson/validator.hpp>

namespace VDMS {

typedef ::google::protobuf::RepeatedPtrField<std::string> BlobArray;

// Instance created per worker thread to handle all transactions on a given
// connection.
class QueryHandler {
  friend class QueryHandlerTester;

  static std::unordered_map<std::string, RSCommand *> _rs_cmds;
  PMGDQueryHandler _pmgd_qh;
  bool _autodelete_init;
  bool _autoreplicate_init;

  bool syntax_checker(const Json::Value &root, Json::Value &error);
  int parse_commands(const protobufs::queryMessage &proto_query,
                     Json::Value &root);
  void cleanup_query(const std::vector<std::string> &images,
                     const std::vector<std::string> &videos);

  void process_query(protobufs::queryMessage &proto_query,
                     protobufs::queryMessage &response);

  // valijson
  valijson::Validator _validator;
  static valijson::Schema *_schema;

#ifdef CHRONO_TIMING
  ChronoCpu ch_tx_total;
  ChronoCpu ch_tx_query;
  ChronoCpu ch_tx_send;
#endif

public:
  static void init();

  QueryHandler();

  void process_connection(comm::Connection *c);
  void reset_autodelete_init_flag();
  void set_autodelete_init_flag();
  void regualar_run_autodelete();
  void build_autodelete_queue();
  void set_autoreplicate_init_flag();
  void reset_autoreplicate_init_flag();
  void regualar_run_autoreplicate(ReplicationConfig &);
};
} // namespace VDMS
