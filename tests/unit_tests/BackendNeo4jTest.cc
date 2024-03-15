/**
 * @file   ImageData_test.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <string>

#include "BackendNeo4j.h"
#include "gtest/gtest.h"
#include <cstdlib>

class Neo4jBackendTest : public ::testing::Test {
protected:
  BackendNeo4j *neoconn_pool;
  virtual void SetUp() {
    // Test setup, lets instantiate a connection here
    // neo4j class instationation
    char *env_4j_port;
    env_4j_port = std::getenv("NEO_TEST_PORT");
    std::string tgt_db_base = "neo4j://localhost:";
    std::string tgt_db_port(env_4j_port);
    std::string tgt_db_addr = tgt_db_base + tgt_db_port;
    std::cout << tgt_db_addr;
    // char tgtdb[] = "neo4j://localhost:7687";
    const char *tgt_db = tgt_db_addr.c_str();
    char user[] = "neo4j";
    char pass[] = "neo4jpass";
    uint_fast32_t flags = NEO4J_INSECURE;
    int nr_conns = 16;
    neoconn_pool =
        new BackendNeo4j(nr_conns, (char *)tgt_db, user, pass, flags);
  }

  virtual void TearDown() {
    // Tear down, here lets shut down the neo4j connection
  }
  void verify_conn_put_pop() {
    neo4j_connection_t *conn;
    int nr_conns = neoconn_pool->nr_avail_conn();
    ASSERT_EQ(16, nr_conns);

    conn = neoconn_pool->get_conn();
    nr_conns = neoconn_pool->nr_avail_conn();
    ASSERT_EQ(15, nr_conns);

    neoconn_pool->put_conn(conn);
    nr_conns = neoconn_pool->nr_avail_conn();
    ASSERT_EQ(16, nr_conns);
  }

  void verify_rollback() {

    char myquery[] = "CREATE (:opennode {tag_str: \"open\"})";
    neo4j_transaction *tx;
    neo4j_connection_t *conn;
    neo4j_result_stream_t *res_stream;
    int rc;

    conn = neoconn_pool->get_conn();
    tx = neoconn_pool->open_tx(conn, 1000, "w");
    res_stream = neoconn_pool->run_in_tx(myquery, tx);
    rc = neoconn_pool->rollback_tx(tx);

    ASSERT_EQ(0, rc);
  }

  void verify_commit_trans() {
    char myquery[] = "CREATE (:testnode {tag_str: \"CgESpOVg\"})";
    neo4j_transaction *tx;
    neo4j_connection_t *conn;
    neo4j_result_stream_t *res_stream;
    int rc;

    conn = neoconn_pool->get_conn();
    tx = neoconn_pool->open_tx(conn, 1000, "w");
    res_stream = neoconn_pool->run_in_tx(myquery, tx);
    rc = neoconn_pool->commit_tx(tx);
    // 0 means transaction successfully committed
    ASSERT_EQ(0, rc);
    neoconn_pool->put_conn(conn);
  }

  void verify_simple_query_trans() {
    // TODO technically has a dependency with earlier test for convenience,
    // in future should clean that up

    // used in extracting results
    neo4j_result_t *res;
    neo4j_value_t res_val;
    int val_type;
    char *fname_ptr;
    unsigned int nr_fields;
    char *fields[32];
    unsigned int nr_bytes;
    char strbuf[8192];
    char *strptr;
    neo4j_transaction *tx;
    neo4j_connection_t *conn;
    neo4j_result_stream_t *res_stream;

    // retrieve the result from the earlier test
    conn = neoconn_pool->get_conn();
    char myquery[] =
        "MATCH (n:testnode WHERE n.tag_str = \"CgESpOVg\") RETURN n.tag_str;";
    tx = neoconn_pool->open_tx(conn, 1000, "r");
    res_stream = neoconn_pool->run_in_tx(myquery, tx);

    // verify results
    nr_fields = neo4j_nfields(res_stream);
    ASSERT_EQ(1, nr_fields);

    // Extract field name, verify we're seeing what we expect from earlier
    // insertion
    for (unsigned int i = 0; i < nr_fields; i++) {
      fname_ptr = (char *)neo4j_fieldname(res_stream, i);
      fields[i] = fname_ptr;
      ASSERT_STREQ("n.tag_str", fields[i]);
    }
    // check for specified field and tag-string match
    while (true) {
      res = neo4j_fetch_next(res_stream);
      if (res == NULL) {
        break;
      }
      for (unsigned int i = 0; i < nr_fields; i++) {
        res_val = neo4j_result_field(res, i);
        val_type = val_check(res_val);
        ASSERT_EQ(NEO4J_STRING, val_type);
        strptr = neo4j_string_value(res_val, strbuf, 8192);
        ASSERT_STREQ(strbuf, "CgESpOVg");
      }
    }

    neoconn_pool->commit_tx(tx);
    neoconn_pool->put_conn(conn);
  }
  void verify_results_to_json() {

    neo4j_transaction *tx;
    neo4j_connection_t *conn;
    Json::Value json_res;
    neo4j_result_stream_t *res_stream;
    char myquery[] =
        "MATCH (n:testnode WHERE n.tag_str = \"CgESpOVg\") RETURN n.tag_str;";

    // retrieve the result from the earlier test
    conn = neoconn_pool->get_conn();

    tx = neoconn_pool->open_tx(conn, 1000, "r");
    res_stream = neoconn_pool->run_in_tx(myquery, tx);
    json_res = neoconn_pool->results_to_json(res_stream);
    auto res_str = json_res["metadata_res"][0]["n.tag_str"].asString();
    ASSERT_STREQ(res_str.c_str(), "CgESpOVg");
  }

  void verify_print_results() {

    neo4j_transaction *tx;
    neo4j_connection_t *conn;
    neo4j_result_stream_t *res_stream;
    char myquery[] =
        "MATCH (n:testnode WHERE n.tag_str = \"CgESpOVg\") RETURN n.tag_str;";

    // retrieve the result from the earlier test
    conn = neoconn_pool->get_conn();

    tx = neoconn_pool->open_tx(conn, 1000, "r");
    res_stream = neoconn_pool->run_in_tx(myquery, tx);
    neoconn_pool->print_results_stream(res_stream);
    neoconn_pool->commit_tx(tx);
    neoconn_pool->put_conn(conn);
  }

}; // end test class

// Test connection pooling infrastructure
TEST_F(Neo4jBackendTest, ConnGetPutTest) { verify_conn_put_pop(); }
TEST_F(Neo4jBackendTest, TXCommitTest) { verify_commit_trans(); }
TEST_F(Neo4jBackendTest, TXRetrieveTest) { verify_simple_query_trans(); }
TEST_F(Neo4jBackendTest, TXRollbackTest) { verify_rollback(); }
TEST_F(Neo4jBackendTest, PrintTest) { verify_print_results(); }
TEST_F(Neo4jBackendTest, VerifyJSONTest) { verify_results_to_json(); }
