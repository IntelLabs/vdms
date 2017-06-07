#include <stdio.h>
#include <string.h>

#include "Connection.h"

void print_exception(const comm::ExceptionComm &e, FILE *f)
{
    fprintf(f, "[Exception] %s at %s:%d\n", e.name, e.file, e.line);
    if (e.errno_val != 0)
        fprintf(f, "%s: %s\n", e.msg.c_str(), strerror(e.errno_val));
    else if (!e.msg.empty())
        fprintf(f, "%s\n", e.msg.c_str());
}
