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

void printUsage() {
  std::cout << "Usage: vdms -cfg config-file.json" << std::endl;

  std::cout << "Usage: vdms -restore db.tar.gz" << std::endl;
  exit(0);
}

static void *start_request_thread(void *server) {
  ((VDMS::Server *)(server))->process_requests();
  return NULL;
}
static void *start_replication_thread(void *server) {
  VDMS::Server *srv = (VDMS::Server *)server;
  // If replication time is not set, use auto-replication interval
  srv->auto_replicate_interval();

  return NULL;
}

static void *start_autodelete_thread(void *server) {
  ((VDMS::Server *)(server))->autodelete_expired_data();
  return NULL;
}

int main(int argc, char **argv) {
  pthread_t request_thread, autodelete_thread, auto_replicate_thread;
  int request_thread_flag, autodelete_thread_flag, auto_replcation_flag;

  printf("VDMS Server\n");

  if (argc != 3 && argc != 1) {
    printUsage();
  }

  std::string config_file = "config-vdms.json";

  if (argc == 3) {
    std::string option(argv[1]);

    if (option != "-cfg" && option != "-restore" && option != "-backup")
      printUsage();
    if (option == "-cfg")
      config_file = std::string(argv[2]);

    else if (option == "-restore") {
      void *server;

      std::string db_name(argv[2]);
      size_t file_ext1 = db_name.find_last_of(".");

      std::string temp_name_1 = db_name.substr(0, file_ext1);

      size_t file_ext2 = temp_name_1.find_last_of(".");

      std::string temp_name_2 = temp_name_1.substr(0, file_ext2);

      ((VDMS::Server *)(server))->untar_data(db_name);

      config_file = temp_name_2 + ".json";
    }
  }

  printf("Server will start processing requests... \n");
  VDMS::Server server(config_file);

  // create a thread for processing request and a thread for the autodelete
  // timer
  request_thread_flag = pthread_create(&request_thread, NULL,
                                       start_request_thread, (void *)(&server));
  autodelete_thread_flag = pthread_create(
      &autodelete_thread, NULL, start_autodelete_thread, (void *)(&server));
  auto_replcation_flag =
      pthread_create(&auto_replicate_thread, NULL, start_replication_thread,
                     (void *)(&server));

  pthread_join(request_thread, NULL);
  pthread_join(autodelete_thread, NULL);
  pthread_join(auto_replicate_thread, NULL);

  printf("Server shutting down... \n");

  return 0;
}
