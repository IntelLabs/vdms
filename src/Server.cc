#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "Server.h"
#include "comm/Connection.h"
#include "Exception.h"

#include "AthenaConfig.h"

using namespace athena;

bool Server::shutdown = false;

Server::Server(std::string config_file)
{
    AthenaConfig::init(config_file);
    std::string dbname = AthenaConfig::instance()
                        ->get_string_value("pmgd_path", "default_pmgd");
    _server_port = AthenaConfig::instance()
                        ->get_int_value("port", DEFAULT_PORT);

    // Verify that the version of the library that we linked against is
    // compatible with the version of the headers we compiled against.
    GOOGLE_PROTOBUF_VERIFY_VERSION;

    if (install_handler() != 0)
        throw ExceptionServer(SignalHandler);
    //creating a db
    _db = new Jarvis::Graph(dbname.c_str(), Jarvis::Graph::Create);
    // Create the query handler here assuming database is valid now.
    _dblock = new std::mutex();
    _cm = new CommunicationManager(_db, _dblock);
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
            // Listening thread for CQE
            comm::Connection *conn_server =
                new comm::Connection(server->accept());
            _cm->add_connection(conn_server);
        }
        catch (comm::ExceptionComm e) {
            print_exception(e);
        }
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
    delete _dblock;
}
