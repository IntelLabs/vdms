#pragma once
#include "QueryHandler.h"

namespace vdms {
    class QueryHandlerTester
    {
        QueryHandler& _qh;
    public:

        QueryHandlerTester(QueryHandler& qh): _qh(qh)
        {}

        void pq(protobufs::queryMessage& proto_query,
                               protobufs::queryMessage& response)
        {
            _qh.process_query(proto_query, response);
        }
    };
};