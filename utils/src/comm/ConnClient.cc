#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include <netdb.h>

#include "Connection.h"

using namespace comm;

ConnClient::ConnClient()
{
    //create TCP/IP socket
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket_fd < 0) {
        throw ExceptionComm(SocketFail);
    }

    int option = 1; // To set REUSEADDR to true
    if (setsockopt(_socket_fd, SOL_SOCKET,
            SO_REUSEADDR, &option, sizeof option) == -1) {
        throw ExceptionComm(SocketFail);
    }
}

ConnClient::ConnClient(ServerAddress srv) :
                    ConnClient(srv.addr, srv.port)
{
}

ConnClient::ConnClient(std::string addr, int port) : ConnClient()
{
    if (port > MAX_PORT_NUMBER || port <= 0) {
        throw ExceptionComm(PortError);
    }

    _server.addr = addr;
    _server.port = port;
    connect();
}

void ConnClient::connect()
{
    struct hostent *server = gethostbyname(_server.addr.c_str());

    if (server == NULL) {
        throw ExceptionComm(ServerAddError);
    }

    struct sockaddr_in svrAddr;
    memset(&svrAddr, 0, sizeof(svrAddr));
    svrAddr.sin_family = AF_INET;

    memcpy(&svrAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    svrAddr.sin_port = htons(_server.port);

    if (::connect(_socket_fd,(struct sockaddr *) &svrAddr,
                  sizeof(svrAddr)) < 0) {
        throw ExceptionComm(ConnectionError);
    }
}
