/*
    Athena Server Application
*/
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "chrono/Chrono.h"
#include "Server.h"

int main(int argc, char **argv)
{
    printf("Athena App :)\n");

    printf("Server will start processing requests... \n");
    athena::Server server;
    server.process_requests();

    printf("Server shutting down... \n");

    return 0;
}
