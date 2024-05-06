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

void ignore_sigpipe() { signal(SIGPIPE, SIG_IGN); }

void printUsage() {
  std::cout << "Usage: vdms" << std::endl
            << "  -cfg <config_file> : Specify the configuration file "
               "(default: config-vdms.json)."
            << std::endl
            << "  -restore <backup_file> : Restore data from a backup file."
            << std::endl
            << "  -cert <certificate_file> -key <key_file> : Use certificate "
               "and key files for secure communication."
            << std::endl
            << "  -ca <ca_file> : Trust clients with certificates signed by "
               "this ca cert."
            << std::endl
            << "  -help : Print this help message." << std::endl
            << std::endl
            << "Note: -cfg and -restore cannot be used together. -cert and "
               "-key must both be provided if used."
            << std::endl;
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

  std::string cfg_value = "config-vdms.json", restore_value, cert_value,
              key_value, ca_value;
  bool cfg_set = false, restore_set = false, cert_set = false, key_set = false,
       ca_set = false, help_set = false;
  std::vector<std::string> valid_args = {"-cfg", "-restore", "-cert",
                                         "-key", "-ca",      "-help"};

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (std::find(valid_args.begin(), valid_args.end(), arg) ==
        valid_args.end()) {
      std::cerr << "Error: Invalid argument: " << arg << std::endl;
      printUsage();
      return 1;
    }
    if (arg == "-cfg") {
      if (++i < argc) {
        cfg_value = argv[i];
        cfg_set = true;
      } else {
        std::cerr << "Error: -cfg option requires one argument." << std::endl;
        return 1;
      }
    } else if (arg == "-restore") {
      if (++i < argc) {
        restore_value = argv[i];
        restore_set = true;
      } else {
        std::cerr << "Error: -restore option requires one argument."
                  << std::endl;
        return 1;
      }
    } else if (arg == "-cert") {
      if (++i < argc) {
        cert_value = argv[i];
        cert_set = true;
      } else {
        std::cerr << "Error: -cert option requires one argument." << std::endl;
        return 1;
      }
    } else if (arg == "-key") {
      if (++i < argc) {
        key_value = argv[i];
        key_set = true;
      } else {
        std::cerr << "Error: -key option requires one argument." << std::endl;
        return 1;
      }
    } else if (arg == "-ca") {
      if (++i < argc) {
        ca_value = argv[i];
        ca_set = true;
      } else {
        std::cerr << "Error: -ca option requires one argument." << std::endl;
        return 1;
      }
    } else if (arg == "-help") {
      help_set = true;
    }
  }

  if (help_set) {
    printUsage();
    return 0;
  }

  if (cfg_set && restore_set) {
    std::cerr << "Error: -cfg and -restore cannot be used together.\n";
    printUsage();
    return 1;
  }

  if ((cert_set && !key_set) || (!cert_set && key_set)) {
    std::cerr << "Error: -cert and -key must be used together and both require "
                 "a value.\n";
    printUsage();
    return 1;
  }

  if (!ca_set && (cert_set || key_set)) {
    std::cerr
        << "Warning: -ca not set. Client authentication will be disabled.\n";
  }

  if (restore_set) {
    void *server;

    std::string db_name(restore_value);
    size_t file_ext1 = db_name.find_last_of(".");
    std::string temp_name_1 = db_name.substr(0, file_ext1);
    size_t file_ext2 = temp_name_1.find_last_of(".");
    std::string temp_name_2 = temp_name_1.substr(0, file_ext2);
    ((VDMS::Server *)(server))->untar_data(db_name);
    cfg_value = temp_name_2 + ".json";
  }

  // Applications running OpenSSL over network connections may crash if SIGPIPE
  // is not ignored. This is because OpenSSL uses SIGPIPE to signal a broken
  // connection. This is a known issue and is documented in the OpenSSL FAQ.
  ignore_sigpipe();

  VDMS::Server server(cfg_value, cert_value, key_value, ca_value);

  // Note: current default is PMGD
  std::string qhandler_type;
  qhandler_type = server.cfg->get_string_value("query_handler", "pmgd");

  // create a thread for processing request and a thread for the autodelete
  // timer
  request_thread_flag = pthread_create(&request_thread, NULL,
                                       start_request_thread, (void *)(&server));

  printf(
      "Server instantiation complete,  will start processing requests... \n");

  // Kick off threads only if PMGD handler is used as its the only one with
  // PMGD this functionality at the moment. May need refactor as more handlers
  // are added.
  if (qhandler_type == "pmgd") {
    autodelete_thread_flag = pthread_create(
        &autodelete_thread, NULL, start_autodelete_thread, (void *)(&server));
    auto_replcation_flag =
        pthread_create(&auto_replicate_thread, NULL, start_replication_thread,
                       (void *)(&server));
  }

  // Only start threads if this is a PMGD handler as its logic is specific to it
  // In the future we probably want a cleaner solution here
  pthread_join(request_thread, NULL);
  if (qhandler_type == "pmgd") {
    pthread_join(autodelete_thread, NULL);
    pthread_join(auto_replicate_thread, NULL);
  }

  printf("Server shutting down... \n");

  return 0;
}
