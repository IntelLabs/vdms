#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "Server.h"
#include "comm/Connection.h"
#include "Exception.h"

using namespace athena;

bool Server::shutdown = false;

Server::Server(std::string db_name)
{
    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    // TODO: This should probably come from a server command line.
    Jarvis::Graph::Config config;
    try{
        _db = new Jarvis::Graph(db_name.c_str(), Jarvis::Graph::ReadWrite, &config);
    }
    catch(Jarvis::Exception e) {
        print_exception(e);

        // TODO: Log the creation of database

        _db = new Jarvis::Graph(db_name.c_str(), Jarvis::Graph::Create, &config);
        // Don't catch exception here so we exit in case of trouble.
    }

    // TODO: Init partitioner here with network info

    if (install_handler() != 0)
        throw ExceptionServer(SignalHandler);

    // Create the query handler here assuming database is valid now.
    _cm = new CommunicationManager(_db, &_dblock);
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
    delete _db;
}

