#pragma once

#include <string>

namespace comm {

    enum ExceptionCommType {
        FATAL_Internal_Error,

        WriteFail,    // For write/send failure
        ReadFail,     // For read/recv failure
        BindFail,     // Fail to bind a port
        SocketFail,
        ListentFail,

        ServerAddError,
        PortError,
        ConnectionError,
        ConnectionShutDown,

        InvalidMessageSize,
        Undefined = 100,// Any undefined error
    };

    struct ExceptionComm {
        // Which exception
        int num;            // Exception number
        const char *name;   // Exception name

        // Additional information
        std::string msg;
        int errno_val;

        // Where it was thrown
        const char *file;   // Source file name
        int line;           // Source line number

        ExceptionComm(int exc, const char *exc_name, const char *f, int l)
            : num(exc), name(exc_name),
              msg(), errno_val(0),
              file(f), line(l)
        {}

        ExceptionComm(int exc, const char *exc_name,
                  const std::string &m,
                  const char *f, int l)
            : num(exc), name(exc_name),
              msg(m), errno_val(0),
              file(f), line(l)
        {}

        ExceptionComm(int exc, const char *exc_name,
                  int err, const std::string &m,
                  const char *f, int l)
            : num(exc), name(exc_name),
              msg(m), errno_val(err),
              file(f), line(l)
        {}
    };

#define ExceptionComm(name, ...) \
    ExceptionComm(comm::name, #name, ##__VA_ARGS__, __FILE__, __LINE__)
};

extern void print_exception(const comm::ExceptionComm &e, FILE *f = stdout);
