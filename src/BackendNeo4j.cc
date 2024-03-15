#include "BackendNeo4j.h"

int val_check(neo4j_value_t val) {

  if (neo4j_instanceof(val, NEO4J_BOOL)) {
    return NEO4J_BOOL;
  } else if (neo4j_instanceof(val, NEO4J_STRING)) {
    return NEO4J_STRING;
  } else if (neo4j_instanceof(val, NEO4J_NULL)) {
    return NEO4J_NULL;
  } else if (neo4j_instanceof(val, NEO4J_FLOAT)) {
    return NEO4J_FLOAT;
  } else if (neo4j_instanceof(val, NEO4J_BYTES)) {
    return NEO4J_BYTES;
  } else if (neo4j_instanceof(val, NEO4J_INT)) {
    return NEO4J_INT;
  }

  return -1;
}
/*Hat Tip
 * https://stackoverflow.com/questions/4800605/iterating-through-objects-in-jsoncpp*/
void print_val(neo4j_value_t val, int val_type) {
  unsigned int nr_bytes;
  char strbuf[8192];
  char *strptr;

  if (val_type == NEO4J_BOOL) {
    printf("%d\n", neo4j_bool_value(val));
  } else if (val_type == NEO4J_STRING) {
    strptr = neo4j_string_value(val, strbuf, 8192);
    printf("%s\n", strptr);
  } else if (val_type == NEO4J_NULL) {
    printf("NULL\n");
  } else if (val_type == NEO4J_FLOAT) {
    printf("%f\n", neo4j_float_value(val));
  } else if (val_type == NEO4J_BYTES) {
    printf("<Cannot print byte value>\n");
  } else if (val_type == NEO4J_INT) {
    printf("%lld\n", neo4j_int_value(val));
  }
}

// constructor
BackendNeo4j::BackendNeo4j(unsigned int nr_conns, char *tgt_url, char *user,
                           char *pass, uint_fast32_t flags) {

  // Client initialization
  neo4j_client_init();

  // initializing configuration structure: will be re-used
  config = neo4j_new_config();
  neo4j_config_set_username(config, user);
  neo4j_config_set_password(config, pass);
  neo4j_config_set_supported_versions(config, "4,5");

  // initialize connection pool
  for (int i = 0; i < nr_conns; i++) {
    neo4j_connection_t *connection = neo4j_connect(tgt_url, config, flags);
    // TODO need to have error checks for connection failures
    conn_pool.push(connection);
  }
}

// For Putting/retrieving connections
neo4j_connection_t *BackendNeo4j::get_conn() {

  neo4j_connection_t *conn;
  // TODO need to understand functionality when all conns are taken
  conn_pool.pop(conn);
  return conn;
}
void BackendNeo4j::put_conn(neo4j_connection_t *connection) {
  conn_pool.push(connection);
}

int BackendNeo4j::nr_avail_conn() { return conn_pool.size(); }

// Transaction Helpers
neo4j_transaction_t *BackendNeo4j::open_tx(neo4j_connection_t *connection,
                                           int timeout_ms, char *mode) {
  neo4j_transaction_t *my_tx;
  my_tx = neo4j_begin_tx(connection, timeout_ms, mode, tgt_db.c_str());
  return my_tx;
}
neo4j_result_stream_t *BackendNeo4j::run_in_tx(char *cypher_string,
                                               neo4j_transaction_t *tx) {

  neo4j_result_stream_t *results =
      neo4j_run_in_tx(tx, cypher_string, neo4j_null);

  return results;
}
int BackendNeo4j::commit_tx(neo4j_transaction_t *tx) {
  int rc = 0;
  rc = neo4j_commit(tx);
  neo4j_free_tx(tx);

  return rc;
}

int BackendNeo4j::rollback_tx(neo4j_transaction *tx) {
  int rc = 0;
  rc = neo4j_rollback(tx);

  return rc;
}

// Result helpers
Json::Value BackendNeo4j::results_to_json(neo4j_result_stream_t *results) {

  neo4j_result_t *res;
  neo4j_value_t res_val;
  int val_type;
  char *fname_ptr;
  unsigned int nr_fields;
  char *fields[32];
  int val_types[32];
  Json::Value agg_response;
  Json::Value row_resp;
  char *kv_val_ptr;
  int rowctr;
  char strbuf[4096];
  char *strptr;
  char *retstr;
  Json::StyledWriter styled;

  nr_fields = neo4j_nfields(results);

  for (unsigned int i = 0; i < nr_fields; i++) {
    fname_ptr = (char *)neo4j_fieldname(results, i);
    fields[i] = fname_ptr;
  }

  rowctr = 0;
  while (true) {

    res = neo4j_fetch_next(results);
    if (res == NULL) {
      break;
    }

    for (unsigned int i = 0; i < nr_fields; i++) {
      res_val = neo4j_result_field(res, i);
      val_type = val_check(res_val);
      fname_ptr = fields[i];
      std::string cpp_fname = fname_ptr;

      if (val_type == NEO4J_BOOL) {
        row_resp[rowctr][cpp_fname] = neo4j_bool_value(res_val);
      } else if (val_type == NEO4J_STRING) {
        kv_val_ptr = neo4j_string_value(res_val, strbuf, 8192);
        std::string cpp_val = kv_val_ptr;
        row_resp[rowctr][cpp_fname] = cpp_val;
      } else if (val_type == NEO4J_NULL) {
        row_resp[rowctr][cpp_fname] = Json::nullValue;
      } else if (val_type == NEO4J_FLOAT) {
        row_resp[rowctr][cpp_fname] = neo4j_float_value(res_val);
      } else if (val_type == NEO4J_BYTES) {
        printf("<Cannot convert byte value to JSON yet>\n");
      } else if (val_type == NEO4J_INT) {
        row_resp[rowctr][cpp_fname] =
            (Json::Value::Int64)neo4j_int_value(res_val);
      }
    }
    rowctr++;
  }

  // add rows to primary results structure
  agg_response["metadata_res"] = row_resp;

  return agg_response;
}
void BackendNeo4j::print_results_stream(neo4j_result_stream_t *results) {

  neo4j_result_t *res;
  neo4j_value_t res_val;
  int val_type;
  char *fname_ptr;
  unsigned int nr_fields;
  char *fields[32];

  nr_fields = neo4j_nfields(results);

  for (unsigned int i = 0; i < nr_fields; i++) {
    fname_ptr = (char *)neo4j_fieldname(results, i);
    printf("Field: %s\n", neo4j_fieldname(results, i));
    fields[i] = fname_ptr;
  }

  while (true) {
    printf("---Result Row---\n");
    res = neo4j_fetch_next(results);

    if (res == NULL) {
      break;
    }

    for (unsigned int i = 0; i < nr_fields; i++) {
      printf("Fieldname: %s\n", fields[i]);
      res_val = neo4j_result_field(res, i);
      val_type = val_check(res_val);
      print_val(res_val, val_type);
    }
  }
}

BackendNeo4j::~BackendNeo4j() {

  neo4j_connection_t *conn;
  // TODO need to understand functionality when all conns are taken
  while (conn_pool.size() > 0) {
    conn_pool.pop(conn);
    neo4j_close(conn);
  }
  neo4j_client_cleanup();
}