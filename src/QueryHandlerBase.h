//
// Created by ifadams on 7/19/2023.
//

#ifndef VDMS_QUERYHANDLERBASE_H
#define VDMS_QUERYHANDLERBASE_H

#include "QueryMessage.h" // Protobuff implementation
#include <string>
#include <vector>
//#include "Server.h"
#include "chrono/Chrono.h"

// Json parsing files
#include <jsoncpp/json/value.h>
#include <valijson/schema.hpp>
#include <valijson/validator.hpp>

namespace VDMS {

class QueryHandlerBase {

protected:
  // valijson
  valijson::Validator _validator;
  static valijson::Schema *_schema;

#ifdef CHRONO_TIMING
  ChronoCpu ch_tx_total;
  ChronoCpu ch_tx_query;
  ChronoCpu ch_tx_send;
#endif

  void virtual cleanup_query(const std::vector<std::string> &images,
                             const std::vector<std::string> &videos);

  // process query is the core logic of any derived handler
  // it takes in a protobuf serialized JSON that can be indexed/mapped
  // into using CPP JSON (see query handler example)
  // any json can be serialized and used as response that is handled
  // by communication logic elsewhere.
  void virtual process_query(protobufs::queryMessage &proto_query,
                             protobufs::queryMessage &response) = 0;

public:
  QueryHandlerBase();

  void virtual process_connection(comm::Connection *c);
};
} // namespace VDMS

#endif // VDMS_QUERYHANDLERBASE_H
