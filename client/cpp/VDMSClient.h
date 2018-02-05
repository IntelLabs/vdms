#pragma once

#include <string>
#include <vector>
#include "comm/Connection.h"

namespace VDMS {
    class VDMSClient {
        static const int VDMS_PORT = 55555;

        // The constructor of the ConnClient class already connects to the
        // server if instantiated with the right address and port and it gets
        // disconnected when the class goes out of scope. For now, we
        // will leave the functioning like that. If the client has a need to
        // disconnect and connect specifically, then we can add explicit calls.
        comm::ConnClient _conn;

    public:
        VDMSClient(std::string addr = "localhost", int port = VDMS_PORT)
            : _conn(addr, port)
        { }

        // Return value will be the JSON responses.
        const std::string query(const std::string &json_query);
        // Function is synchronous
        const std::string query(const std::string &json_query, const std::vector<std::string *> blobs);

        // Provide an output vector in case expecting blobs as return.
        //const std::string query(const std::string &json_query, const std::vector<std::string *> blobs,
          //                      std::vector<const std::string> &outblobs);
    };
};
