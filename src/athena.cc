/*
    Athena Server Application
*/
#include "Server.h"

int main(int argc, char **argv)
{
    printf("Athena App :)\n");

    printf("Server will start processing requests... \n");
    athena::Server server("GraphDB");
    server.process_requests();

    printf("Server shutting down... \n");

    return 0;
}
