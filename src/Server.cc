/**
 * @file   Server.cc
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

#include <chrono>
#include <stdio.h>
#include <stdlib.h> /* system, NULL, EXIT_FAILURE */
#include <thread>

#include "Exception.h"
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h> //to create the config file

#include "Server.h"
#include "comm/Connection.h"

#include "DescriptorsManager.h"
#include "QueryHandler.h"
#include "VDMSConfig.h"

#include "pmgdMessages.pb.h" // Protobuff implementation

using namespace VDMS;

bool Server::shutdown = false;

Server::Server(std::string config_file) {
  VDMSConfig::init(config_file);
  _autoreplicate_settings.server_port =
      VDMSConfig::instance()->get_int_value("port", DEFAULT_PORT);

  _autoreplicate_settings.max_simultaneous_clients =
      VDMSConfig::instance()->get_int_value(
          "max_simultaneous_clients",
          500); // Default from CommunicationManager.h

  _autoreplicate_settings.autodelete_interval =
      VDMSConfig::instance()->get_int_value("autodelete_interval_s",
                                            DEFAULT_AUTODELETE_INTERVAL);
  _autoreplicate_settings.backup_flag =
      VDMSConfig::instance()->get_string_value("backup_flag",
                                               DEFAULT_AUTOREPLICATE_FLAG);

  _autoreplicate_settings.autoreplicate_interval =
      VDMSConfig::instance()->get_int_value("autoreplicate_interval",
                                            DEFAULT_AUTOREPLICATE_INTERVAL);
  _autoreplicate_settings.autoreplication_unit =
      VDMSConfig::instance()->get_string_value("unit",
                                               DEFAULT_AUTOREPLICATE_UNIT);

  _autoreplicate_settings.replication_time =
      VDMSConfig::instance()->get_string_value("replication_time",
                                               DEFAULT_AUTOREPLICATE_UNIT);
  _autoreplicate_settings.backup_path =
      VDMSConfig::instance()->get_string_value("backup_path",
                                               DEFAULT_BACKUP_PATH);
  _autoreplicate_settings.db_path =
      VDMSConfig::instance()->get_string_value("db_root_path", DEFAULT_DB_ROOT);

  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh;
  qh.set_autodelete_init_flag();
  qh.build_autodelete_queue();  // create priority queue of nodes with
                                // _expiration property
  qh.regualar_run_autodelete(); // delete nodes that have expired since server
                                // previous closed
  qh.reset_autodelete_init_flag(); // set flag to show autodelete queue has been
                                   // initialized

  // Verify that the version of the library that we linked against is
  // compatible with the version of the headers we compiled against.
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  install_handler();

  _cm = new CommunicationManager();
}

void Server::process_requests() {
  comm::ConnServer *server;
  try {
    server = new comm::ConnServer(_autoreplicate_settings.server_port);
  } catch (comm::ExceptionComm e) {
    print_exception(e);
    delete server;
    return;
  }

  while (!shutdown) {
    try {
      comm::Connection *conn_server = new comm::Connection(server->accept());
      _cm->add_connection(conn_server);
    }

    catch (comm::ExceptionComm e) {
      print_exception(e);
    }
  }

  delete server;
}
void Server::untar_data(std::string &name) {

  std::string command = "tar -xvSf" + name;
  system(command.c_str());
}
void Server::auto_replicate_interval() {
  long replication_period = 0;
  QueryHandler qh;

  if (_autoreplicate_settings.backup_path.empty()) {
    _autoreplicate_settings.backup_path =
        _autoreplicate_settings.db_path; // set the default path to be db
  }

  if (_autoreplicate_settings.autoreplicate_interval > 0) {
    if (_autoreplicate_settings.autoreplication_unit.compare("h") == 0) {
      replication_period =
          _autoreplicate_settings.autoreplicate_interval * 60 * 60;
    } else if (_autoreplicate_settings.autoreplication_unit.compare("m") == 0) {
      replication_period = _autoreplicate_settings.autoreplicate_interval * 60;
    } else {
      replication_period = _autoreplicate_settings.autoreplicate_interval;
    }
  }
  if (replication_period <= 0) {
    std::cout << "Error: auto-replication interval must be a positive number."
              << std::endl;
    return;
  }

  while (!shutdown) {
    // Sleep for the replication period
    std::this_thread::sleep_for(std::chrono::seconds(replication_period));

    // Execute the auto-replicate function
    qh.regualar_run_autoreplicate(_autoreplicate_settings);
  }
}

void Server::auto_replicate_data_exact_time() {
  QueryHandler qh;

  std::istringstream iss(_autoreplicate_settings.replication_time);
  std::string time;
  char delimiter;
  std::getline(iss, time);

  int hour, minute;
  char period;
  std::istringstream timeTokenIss(time);
  timeTokenIss >> std::setw(2) >> hour >> delimiter >> std::setw(2) >> minute >>
      period; // Extract hour, minute, and period
  if (period == 'P') {
    hour += 12; // Convert to 24-hour format
  }

  while (!shutdown) {
    // Get the current time
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    struct std::tm *now_tm = std::localtime(&now_time);

    // Calculate the next replication time
    std::tm replicate_tm = *now_tm;
    replicate_tm.tm_hour = hour;  // set the desired hour
    replicate_tm.tm_min = minute; // set the desired minute
    replicate_tm.tm_sec = 0;      // set seconds to 0
    auto replicate_time =
        std::chrono::system_clock::from_time_t(std::mktime(&replicate_tm));
    if (now > replicate_time) {
      replicate_time += std::chrono::hours(
          24); // if the specified time has passed, set it for the next day
    }

    // Sleep until the next replication time
    auto duration = replicate_time - now;
    std::this_thread::sleep_for(duration);

    // Execute the auto-replicate function
    qh.regualar_run_autoreplicate(_autoreplicate_settings);
  }
}

void Server::autodelete_expired_data() {
  if (_autoreplicate_settings.autodelete_interval >
      0) // check to ensure valid autodelete_interval
  {
    QueryHandler qh;
    while (!shutdown) {
      sleep(_autoreplicate_settings.autodelete_interval);
      qh.regualar_run_autodelete(); // delete data expired since startup
    }
  }
}

void Server::install_handler() {
  struct sigaction action;
  memset(&action, 0, sizeof(action));
  action.sa_handler = Server::sighandler;
  if (sigaction(SIGINT, &action, 0) != 0)
    throw ExceptionServer(SignalHandler);
  if (sigaction(SIGTERM, &action, 0) != 0)
    throw ExceptionServer(SignalHandler);
  if (sigaction(SIGQUIT, &action, 0) != 0)
    throw ExceptionServer(SignalHandler);
}

Server::~Server() {
  _cm->shutdown();
  delete _cm;
  PMGDQueryHandler::destroy();
  DescriptorsManager::instance()->flush();
  VDMSConfig::destroy();
}
