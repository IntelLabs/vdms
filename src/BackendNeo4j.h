//
// Created by ifadams on 9/28/2023.
//

#pragma once

#include <cstdint>
#include <jsoncpp/json/writer.h>
#include <neo4j-client.h>
#include <string>
#include <tbb/concurrent_queue.h>
#include <transaction.h>

void print_val(neo4j_value_t val, int val_type);
int val_check(neo4j_value_t val);

class BackendNeo4j {

  tbb::concurrent_bounded_queue<neo4j_connection_t *> conn_pool;
  neo4j_config_t *config;
  std::string tgt_db;

public:
  // constructor
  BackendNeo4j(unsigned int nr_conns, char *tgt_url, char *user, char *pass,
               uint_fast32_t flags);

  // For Putting/retrieving connections
  neo4j_connection_t *get_conn();
  void put_conn(neo4j_connection_t *connection);
  int nr_avail_conn();

  // Transaction Helpers
  neo4j_transaction_t *open_tx(neo4j_connection_t *connection, int timeout_ms,
                               char *mode);
  neo4j_result_stream_t *run_in_tx(char *cypher_string,
                                   neo4j_transaction_t *tx);
  int commit_tx(neo4j_transaction_t *tx);
  int rollback_tx(neo4j_transaction_t *tx);

  // Result helpers
  Json::Value results_to_json(neo4j_result_stream_t *results);
  void print_results_stream(neo4j_result_stream_t *results);

  // cleanup/desctructors
  ~BackendNeo4j();
};
