#pragma once

#include <string>


namespace athena {

    enum ExceptionServerType {
        FATAL_Server_Error,

        SignalHandler,
        NullConnection,

        Undefined = 100,// Any undefined error
    };

    struct ExceptionServer {
        // Which exception
        int num;            ///< Exception number
        const char *name;   ///< Exception name

        // Additional information
        std::string msg;
        int errno_val;

        // Where it was thrown
        const char *file;   ///< Source file name
        int line;           ///< Source line number

        ExceptionServer(int exc, const char *exc_name, const char *f, int l)
            : num(exc), name(exc_name),
              msg(), errno_val(0),
              file(f), line(l)
        {}

        ExceptionServer(int exc, const char *exc_name,
                  const std::string &m,
                  const char *f, int l)
            : num(exc), name(exc_name),
              msg(m), errno_val(0),
              file(f), line(l)
        {}

        ExceptionServer(int exc, const char *exc_name,
                  int err, const std::string &m,
                  const char *f, int l)
            : num(exc), name(exc_name),
              msg(m), errno_val(err),
              file(f), line(l)
        {}
    };

#define ExceptionServer(name, ...) \
    ExceptionServer(athena::name, #name, ##__VA_ARGS__, __FILE__, __LINE__)
};

extern void print_exception(const athena::ExceptionServer &e, FILE *f = stdout);
