/**
 * @file   vdms.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

/*
    VDMS Server Application
*/
#include <iostream>

#include "Server.h"

void printUsage()
{
    std::cout << "Usage: vdms -cfg config-file.json" << std::endl;
    exit(0);
}



static void* start_request_thread(void* server)
{
    ((VDMS::Server*)(server))->process_requests();
    return NULL;
}

static void* start_autodelete_thread(void* server)
{
    ((VDMS::Server*)(server))->autodelete_expired_data();
    return NULL;
}


int main(int argc, char **argv)
{
    pthread_t request_thread, autodelete_thread;
    int request_thread_flag, autodelte_thread_flag;

    printf("VDMS Server\n");

    if (argc != 3 && argc != 1) {
        printUsage();
    }

    std::string config_file = "config-vdms.json";

    if (argc == 3){
        std::string option(argv[1]);
        if (option != "-cfg")
            printUsage();

        config_file = std::string (argv[2]);
    }

    printf("Server will start processing requests... \n");
    VDMS::Server server(config_file);

    //create a thread for processing request and a thread for the autodelete timer
    request_thread_flag = pthread_create(&request_thread, NULL, start_request_thread, (void*)( &server ) );
    autodelte_thread_flag = pthread_create(&autodelete_thread, NULL, start_autodelete_thread, (void*)( &server ) );
    pthread_join(request_thread, NULL);
    pthread_join(autodelete_thread, NULL);

    printf("Server shutting down... \n");

    return 0;
}
