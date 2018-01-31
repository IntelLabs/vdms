/**
 * @file   Server.h
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

#include <csignal>

#include "jarvis.h"
#include "CommunicationManager.h"

namespace vdms {
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
