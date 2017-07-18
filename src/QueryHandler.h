#pragma once
#include <string>
#include <mutex>
#include <vector>

#include "protobuf/queryMessage.pb.h" // Protobuf implementation
#include "CommandHandler.h"
#include "PMGDQueryHandler.h" // to provide the database connection

// Json parsing files
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>


namespace athena {
    // Instance created per worker thread to handle all transactions on a given
    // connection.
    class QueryHandler
    {
        PMGDQueryHandler _pmgd_qh;

        void  construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds, Json::Value root, int txid);

    public:
        QueryHandler(Jarvis::Graph *db, std::mutex *mtx);
        void process_connection(comm::Connection *c);
        void process_query(std::string);
    };
};
