#pragma once

#include <csignal>

#include "jarvis.h"
#include "CommunicationManager.h"

namespace athena {
    class Server
    {
        static const int DEFAULT_PORT = 55555;

        CommunicationManager *_cm;

        // TODO: Partitioner here
        //until we have a separate PMGD server this db lives here
        Jarvis::Graph *_db;
        // Aux lock, not in use
        std::mutex *_dblock;

        int _server_port;

        // Handle ^c
        static bool shutdown;
        void install_handler();
        static void sighandler(int signo)
            { Server::shutdown = (signo == SIGINT) ||
                                 (signo == SIGTERM)||
                                 (signo == SIGQUIT); }

    public:
        Server(std::string config_file);
        void process_requests();
        ~Server();
    };
};
