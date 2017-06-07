#pragma once

#include <csignal>

#include "jarvis.h"
#include "util.h"

#include "QueryHandler.h"
#include "CommunicationManager.h"

namespace athena {
    class Server
    {
        static const int SERVER_PORT = 55551;
        static const int QUERY_BUFFER_SIZE = 4096;  // in bytes

        Jarvis::Graph *_db;
        // TODO: Global dblock until GraphDb supports concurrency internally.
        std::mutex _dblock;

        CommunicationManager *_cm;

        // TODO: Partitioner here

        // Handle ^c
        static bool shutdown;
        int install_handler();
        static void sighandler(int signo)
            { Server::shutdown = (signo == SIGINT); }

    public:
        Server(std::string db_name);
        void process_requests();
        ~Server();
    };
};
