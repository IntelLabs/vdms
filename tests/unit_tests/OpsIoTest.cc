/**
 * @file   OpsIOTest.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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

#include "OpsIOCoordinator.h"
#include "VDMSConfig.h"
#include "gtest/gtest.h"
#include <fstream>
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/reader.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <sstream>
#include <string>

// TODO valid JSON helpers for image transformations
// may want to borrow from existing tests
std::string raw_neoadd_json(
    "{"
    "\n\"NeoAdd\" :"
    "\n{"
    "\n\"cypher\" : \"CREATE (VDMSNODE:USERLABEL {user_prop1:\\\"foo\\\"})\","
    "\n\"operations\" : ["
    "\n{"
    "\n\"height\" : 150,"
    "\n\"type\" : \"crop\","
    "\n\"width\" : 150,"
    "\n\"x\" : 0,"
    "\n\"y\" : 0"
    "\n}"
    "\n],"
    "\n\"target_data_type\" : \"img\","
    "\n\"target_format\" : \"jpg\""
    "\n}"
    "}");

class OpsIOCoordinatorTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    VDMS::VDMSConfig::init("unit_tests/config-aws-tests.json");
    global_s3_connection = instantiate_connection();
  }

  virtual void TearDown() {
    global_s3_connection->end();
    delete global_s3_connection;
  }

  void create_conn_test() {
    VCL::RemoteConnection *local_conn;
    bool is_conn = false;
    local_conn = instantiate_connection();
    is_conn = local_conn->connected();
    local_conn->end();
    delete local_conn;
    ASSERT_EQ(is_conn, true);
  }

  void put_obj_test() {

    int rc;
    VCL::RemoteConnection *connection;

    std::ifstream input("test_images/large1.jpg", std::ios::binary);
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input),
                                      {});

    connection = get_existing_connection();
    rc = s3_upload("test_obj", buffer, connection);
    ASSERT_EQ(rc, 0);
  }

  void get_obj_test() {

    VCL::RemoteConnection *connection;
    connection = get_existing_connection();

    std::ifstream input("test_images/large1.jpg", std::ios::binary);
    std::vector<unsigned char> uploaded(std::istreambuf_iterator<char>(input),
                                        {});
    std::vector<unsigned char> downloaded;

    downloaded = s3_retrieval("test_obj", connection);

    ASSERT_EQ(uploaded.size(), downloaded.size());

    for (int i = 0; i < downloaded.size(); ++i) {
      EXPECT_EQ(downloaded[i], uploaded[i]);
    }
  }

  void do_ops_test() {

    std::ifstream input("test_images/large1.jpg", std::ios::binary);
    std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(input),
                                      {});
    std::vector<unsigned char> trans_img;

    Json::Value root;
    Json::Reader reader;
    std::string json_query(raw_neoadd_json);
    bool success;

    success = reader.parse(json_query, root);
    if (!success) {
      FAIL() << "Failed to parse" << reader.getFormattedErrorMessages();
    }
    ASSERT_EQ(success, true);

    trans_img = do_single_img_ops(root, buffer, "NeoAdd");
    std::cout << trans_img.size() << std::endl;
  }

  void get_conn_test() {
    VCL::RemoteConnection *local_connection;
    local_connection = get_existing_connection();
    ASSERT_EQ(global_s3_connection->connected(), true);
  }
}; // end test class
// TEST_F(OpsIOCoordinatorTest, InstantiateConnTest) {create_conn_test();}
TEST_F(OpsIOCoordinatorTest, PutObjTest) { put_obj_test(); }
TEST_F(OpsIOCoordinatorTest, GetObjTest) { get_obj_test(); }
TEST_F(OpsIOCoordinatorTest, GetConnTest) { get_conn_test(); }
TEST_F(OpsIOCoordinatorTest, DoOpsTest) { do_ops_test(); }
