#pragma once

#include "comm/Connection.h"
#include "protobuf/queryMessage.pb.h"

namespace athena {
    class CommandHandler
    {
        comm::Connection* _conn;

    public:
        CommandHandler(comm::Connection* conn);

        protobufs::queryMessage get_command();
        void send_response(protobufs::queryMessage cmd);
    };
};
