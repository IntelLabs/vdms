#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "PMGDQueryHandler.h" // to provide the database connection
#include "RSCommand.h"
#include "comm/Connection.h"

// Json parsing files
#include <jsoncpp/json/value.h>
#include <valijson/schema.hpp>
#include <valijson/validator.hpp>

namespace athena {

typedef ::google::protobuf::RepeatedPtrField<std::string> BlobArray;

    // Instance created per worker thread to handle all transactions on a given
    // connection.
    class QueryHandler
    {
        friend class QueryHandlerTester;

        static std::unordered_map<std::string, RSCommand* > _rs_cmds;
        PMGDQueryHandler _pmgd_qh;

        bool syntax_checker(const Json::Value &root, Json::Value& error);
        int parse_commands(const std::string& commands, Json::Value& root);
        void process_query(protobufs::queryMessage& proto_query,
                           protobufs::queryMessage& response);

        // valijson
        valijson::Validator _validator;
        static valijson::Schema* _schema;

    public:
        static void init();

        QueryHandler(Jarvis::Graph *db, std::mutex *mtx);
        ~QueryHandler();

        void process_connection(comm::Connection *c);
    };
};
