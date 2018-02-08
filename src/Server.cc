/**
 * @file   Server.cc
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

#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "Server.h"
#include "comm/Connection.h"
#include "Exception.h"

#include "VDMSConfig.h"
#include "QueryHandler.h"
#include "DescriptorsManager.h"

#include "protobuf/pmgdMessages.pb.h" // Protobuff implementation

using namespace VDMS;

bool Server::shutdown = false;

Server::Server(std::string config_file)
{
    VDMSConfig::init(config_file);
    _server_port = VDMSConfig::instance()
                        ->get_int_value("port", DEFAULT_PORT);

    PMGDQueryHandler::init();
    QueryHandler::init();

    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    install_handler();

    _cm = new CommunicationManager();
}

void Server::process_requests()
{
    comm::ConnServer *server;
    try {
        server = new comm::ConnServer(_server_port);
    }
    catch (comm::ExceptionComm e) {
        print_exception(e);
        return;
    }

    while (!shutdown) {
        try {
            comm::Connection *conn_server =
                new comm::Connection(server->accept());
            _cm->add_connection(conn_server);
        }
        catch (comm::ExceptionComm e) {
            print_exception(e);
        }
    }

    delete server;
}

void Server::install_handler()
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = Server::sighandler;
    if (sigaction(SIGINT, &action, 0) != 0)
        throw ExceptionServer(SignalHandler);
    if (sigaction(SIGTERM, &action, 0) != 0)
        throw ExceptionServer(SignalHandler);
    if (sigaction(SIGQUIT, &action, 0) != 0)
        throw ExceptionServer(SignalHandler);
}

Server::~Server()
{
    _cm->shutdown();
    delete _cm;
    DescriptorsManager::instance()->flush();
}
