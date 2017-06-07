#include <string>
#include <cstring>
#include <unistd.h>
#include <cstdlib>

#include <netdb.h>

#include "Connection.h"

using namespace comm;

ConnServer::ConnServer(int port):
        _port(port)
{
    if (_port > MAX_PORT_NUMBER || _port <= 0 ) {
        throw ExceptionComm(PortError);
    }

    int ret;

    //create TCP/IP socket
    _socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (_socket_fd < 0) {
        throw ExceptionComm(SocketFail);
    }

    int option = 1; // To set REUSEADDR to true
    ret = setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR,
                     &option, sizeof(option));
    if (ret < 0) {
        throw ExceptionComm(SocketFail);
    }

    struct sockaddr_in svr_addr;
    memset((char*) &svr_addr,0, sizeof(svr_addr));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = INADDR_ANY;
    svr_addr.sin_port = htons(_port);

    // bind socket : "assigning a name to a socket"
    ret = ::bind(_socket_fd, (struct sockaddr *)&svr_addr, sizeof(svr_addr));
    if (ret < 0) {
        throw ExceptionComm(BindFail);
    }

    //mark socket as pasive
    if (::listen(_socket_fd, MAX_CONN_QUEUE) == -1) {
        throw ExceptionComm(ListentFail);
    }
}

ConnServer::~ConnServer()
{
    ::close(_socket_fd);
}

Connection ConnServer::accept()
{
    struct sockaddr_in clnt_addr;
    socklen_t len = sizeof(clnt_addr); //store size of the address

    // This is where client connects.
    // Server will stall here until incoming connection
    // unless the socket is marked and nonblocking
    int connfd = ::accept(_socket_fd, (struct sockaddr *)&clnt_addr, &len);

    if (connfd < 0) {
        throw ExceptionComm(ConnectionError);
    }

    return Connection(connfd);
}

