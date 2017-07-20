#pragma once

#include <string>
#include "ExceptionComm.h"

namespace comm {

class Connection
{

public:

    Connection();
    Connection(int socket_fd);
    ~Connection();

    Connection(Connection &&);

    Connection& operator=(Connection &&);
    Connection& operator=(const Connection &) = delete;
    Connection(const Connection &) = delete;

    void send_message(const uint8_t *data, uint32_t size);
    std::basic_string<uint8_t> recv_message();

protected:

    const unsigned MAX_PORT_NUMBER  = 65535;
    const unsigned MAX_RETRIES      = 100;
    const unsigned MAX_BUFFER_SIZE  = (32*1024*1024);

    int _socket_fd;
};

// Implements a TCP/IP server
class ConnServer
{

public:

    ConnServer(int port);
    ~ConnServer();
    ConnServer& operator=(const ConnServer &) = delete;
    ConnServer (const ConnServer &) = delete;
    Connection accept();

private:

    const unsigned MAX_CONN_QUEUE  = 2048;
    const unsigned MAX_PORT_NUMBER = 65535;

    int _port; // Server port
    int _socket_fd;
};

// Implements a TCP/IP client
class ConnClient : public Connection
{

public:

    struct ServerAddress
    {
        std::string addr;
        int port;
    };

    ConnClient(struct ServerAddress srv);
    ConnClient(std::string addr, int port);
    ConnClient& operator=(const ConnClient &) = delete;
    ConnClient (const ConnClient &) = delete;

private:

    ConnClient();
    void connect();

    ServerAddress _server;
};

};
