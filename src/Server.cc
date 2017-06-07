#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "Server.h"
#include "comm/Connection.h"
#include "Exception.h"

using namespace athena;

bool Server::shutdown = false;

Server::Server()
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (install_handler() != 0)
        throw ExceptionServer(SignalHandler);

    // Create the query handler here assuming database is valid now.
    _dblock = new std::mutex();
    _cm = new CommunicationManager(_dblock);
}

void Server::process_requests()
{
    try {
        comm::ConnServer server(SERVER_PORT);

        while (!shutdown) {
            // Listening thread for CQE
            comm::Connection *conn_server = new comm::Connection(server.accept());
            _cm->add_connection(conn_server);
        }
    }
    catch (comm::ExceptionComm e) {
        print_exception(e);
        return;
    }
}

int Server::install_handler()
{
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = Server::sighandler;
    return sigaction(SIGINT, &action, 0);
}

Server::~Server()
{
    _cm->shutdown();
}

