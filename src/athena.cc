/*
    Athena Server Application
*/
#include <iostream>

#include "Server.h"

void printUsage()
{
    std::cout << "Usage: athena -cfg config-file.json" << std::endl;
    exit(0);
}

int main(int argc, char **argv)
{
    printf("Athena Server\n");

    if (argc != 3 && argc != 1) {
        printUsage();
    }

    std::string config_file = "config-athena.json";

    if (argc == 3){
        std::string option(argv[1]);
        if (option != "-cfg")
            printUsage();

        config_file = std::string (argv[2]);
    }

    printf("Server will start processing requests... \n");
    athena::Server server(config_file);
    server.process_requests();

    printf("Server shutting down... \n");

    return 0;
}
