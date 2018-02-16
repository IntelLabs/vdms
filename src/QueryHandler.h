#pragma once

#include <map>
#include <mutex>

#include "protobuf/queryMessage.pb.h" // Protobuff implementation
#include "CommandHandler.h"
#include "jarvis.h"

namespace athena {
    // Instance created per worker thread to handle all transactions on a given
    // connection.
    class QueryHandler
    {
        // Need this lock till we have concurrency support in JL
        // TODO: Make this reader writer.
        std::mutex *_dblock;

        /// Map an integer ID to a Node (reset at the end of each transaction).
        std::map<int, Jarvis::Node *> mNodes;
        /// Map an integer ID to an Edge (reset at the end of each transaction).
        std::map<int, Jarvis::Edge *> mEdges;

        // void create_node(CommandHandler &handler, const protobufs::CreateNode &cn);

    public:
        QueryHandler(std::mutex *mtx);
        void process_query(comm::Connection *c);
    };
};
