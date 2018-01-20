#pragma once

#include "comm/Connection.h"
#include "protobuf/queryMessage.pb.h"

namespace athena {
    class QueryMessage
    {
        comm::Connection* _conn;

    public:
        QueryMessage(comm::Connection* conn);

        protobufs::queryMessage get_query();
        void send_response(protobufs::queryMessage cmd);
    };
};
