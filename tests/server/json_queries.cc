/**
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

#include <fstream>
#include <iostream>
#include <mutex>
#include <stdlib.h> /* system, NULL, EXIT_FAILURE */
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include <jsoncpp/json/writer.h>

#include "QueryHandlerTester.h"
#include "VDMSConfig.h"
#include "pmgd.h"

using namespace VDMS;
using namespace PMGD;
using namespace std;

std::string singleAddImage(" \
        { \
            \"AddImage\": { \
                \"operations\": [{ \
                    \"width\": 512, \
                    \"type\": \"resize\", \
                    \"height\": 512  \
                }], \
                \"properties\": { \
                    \"name\": \"brain_0\", \
                    \"doctor\": \"Dr. Strange Love\" \
                }, \
                \"format\": \"png\" \
            } \
        } \
    ");
TEST(AutoReplicate, default_replicate) {

  std::string path = "server/config-auto-replicate-tests.json";
  std::cout << path << std::endl;
  VDMSConfig::init(path);
  PMGDQueryHandler::init();
  QueryHandler::init();
  ReplicationConfig replication_test;
  replication_test.backup_path = "backups";
  replication_test.db_path = "db_backup";
  replication_test.autoreplicate_interval = 5;
  replication_test.autoreplication_unit = "s";
  replication_test.server_port = 55557;

  QueryHandler qh_base;
  qh_base.regualar_run_autoreplicate(replication_test);
}
TEST(AddImage, simpleAdd) {
  std::string addImg;
  addImg += "[" + singleAddImage + "]";

  VDMSConfig::init("server/config-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(addImg);

  std::string image;
  std::ifstream file("test_images/brain.png",
                     std::ios::in | std::ios::binary | std::ios::ate);

  image.resize(file.tellg());

  file.seekg(0, std::ios::beg);
  if (!file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  proto_query.add_blobs(image);

  VDMS::protobufs::queryMessage response;
  query_handler.pq(proto_query, response);

  Json::Reader json_reader;
  Json::Value json_response;
  json_reader.parse(response.json(), json_response);

  EXPECT_EQ(json_response[0]["AddImage"]["status"].asString(), "0");
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(UpdateEntity, simpleAddUpdate) {

  Json::StyledWriter writer;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/AddFindUpdate.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  Json::Reader reader;
  Json::Value root;
  Json::Value parsed;

  VDMSConfig::init("server/config-update-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  reader.parse(response.json().c_str(), parsed);
  // std::cout << writer.write(parsed) << std::endl;

  // Verify results returned.
  for (int j = 0; j < parsed.size(); j++) {
    const Json::Value &query = parsed[j];
    ASSERT_EQ(query.getMemberNames().size(), 1);
    std::string cmd = query.getMemberNames()[0];

    if (cmd == "UpdateEntity")
      EXPECT_EQ(query[cmd]["count"].asInt(), 1);
    if (cmd == "FindEntity") {
      EXPECT_EQ(query[cmd]["returned"].asInt(), 2);
      EXPECT_EQ(query["FindEntity"]["entities"][0]["fv"].asString(),
                "Missing property");
    }
  }

  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(AddImage, simpleAddx10) {
  int total_images = 10;
  std::string string_query("[");

  for (int i = 0; i < total_images; ++i) {
    string_query += singleAddImage;
    if (i != total_images - 1)
      string_query += ",";
  }
  string_query += "]";

  VDMSConfig::init("server/config-add10-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(string_query);

  std::string image;
  std::ifstream file("test_images/brain.png",
                     std::ios::in | std::ios::binary | std::ios::ate);

  image.resize(file.tellg());

  file.seekg(0, std::ios::beg);
  if (!file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  for (int i = 0; i < total_images; ++i) {
    proto_query.add_blobs(image);
  }

  VDMS::protobufs::queryMessage response;
  query_handler.pq(proto_query, response);

  Json::Reader json_reader;
  Json::Value json_response;

  // std::cout << response.json() << std::endl;
  json_reader.parse(response.json(), json_response);

  for (int i = 0; i < total_images; ++i) {
    EXPECT_EQ(json_response[i]["AddImage"]["status"].asString(), "0");
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(QueryHandler, AddAndFind) {
  Json::StyledWriter writer;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/AddAndFind_query.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  Json::Reader reader;
  Json::Value root;
  Json::Value parsed;
  reader.parse(json_query, root);
  int in_node_num = 0, out_node_num = 0;
  int in_edge_num = 0, out_edge_num = 0;
  int in_query_num = 0, out_query_num = 0;
  int in_props = 0, out_props = 0;
  int success = 0;
  bool list_found_before = false, average_found_before = false;
  bool count_found_before = false, sum_found_before = false;
  bool list_found_after = false, average_found_after = false;
  bool count_found_after = false, sum_found_after = false;
  double average_value = 0;
  int count_value = 4342;

  for (int j = 0; j < root.size(); j++) {
    const Json::Value &query = root[j];
    assert(query.getMemberNames().size() == 1);
    std::string cmd = query.getMemberNames()[0];

    if (cmd == "AddEntity")
      in_node_num++;

    else if (cmd == "AddConnection")
      in_edge_num++;

    else if (cmd == "FindEntity") {
      in_query_num++;
      if (query[cmd]["results"].isMember("list"))
        list_found_before = true;

      if (query[cmd]["results"].isMember("average"))
        average_found_before = true;

      if (query[cmd]["results"].isMember("sum"))
        sum_found_before = true;

      if (query[cmd]["results"].isMember("count")) {
        count_found_before = true;
      }
    } else if (query.isMember("properties"))
      in_props = query["properties"].size();
    else if (cmd == "FindConnection")
      in_query_num++;
    else if (cmd == "UpdateConnection") {
      count_found_before = true;
      in_edge_num++;
    }
  }

  VDMSConfig::init("server/config-addfind-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  reader.parse(response.json().c_str(), parsed);
  // std::cout << writer.write(parsed) << std::endl;

  for (int j = 0; j < parsed.size(); j++) {
    const Json::Value &query = parsed[j];
    ASSERT_EQ(query.getMemberNames().size(), 1);
    std::string cmd = query.getMemberNames()[0];

    if (cmd == "AddEntity")
      out_node_num++;
    if (cmd == "AddConnection")
      out_edge_num++;
    if (cmd == "UpdateConnection")
      out_edge_num++;
    if (cmd == "FindEntity" || cmd == "FindConnection")
      out_query_num++;

    if (j == 11) { // Second Last FindEntity
      EXPECT_EQ(query["FindEntity"]["entities"][2]["Study"].asString(),
                "Missing property");

      EXPECT_EQ(query["FindEntity"]["entities"][3]["Study"].asString(),
                "Missing property");
    }

    if (j == 12) { // Last FindEntiy
      EXPECT_EQ(query["FindEntity"]["entities"][0]["Birthday"].asString(),
                "1946-10-07T17:59:24-07:00");

      EXPECT_EQ(query["FindEntity"]["entities"][1]["Birthday"].asString(),
                "1936-10-01T17:59:24-07:00");
    }
    if (j == 13) { // FindConnection
      EXPECT_EQ(
          query["FindConnection"]["connections"][0]["location"].asString(),
          "residence");

      EXPECT_EQ(query["FindConnection"]["connections"][0]["city"].asString(),
                "Boston");
    }
    if (query[cmd]["status"] == 0)
      success++;

    if (query[cmd].isMember("list"))
      list_found_after = true;

    if (query[cmd].isMember("average")) {
      average_found_after = true;
      average_value = query[cmd]["average"].asDouble();
    }

    if (query[cmd].isMember("sum"))
      sum_found_after = true;

    if (query[cmd].isMember("count")) {
      count_found_after = true;
      count_value = query[cmd]["count"].asInt();
    }
  }

  int total_success = out_node_num + out_query_num + out_edge_num;

  EXPECT_EQ(in_node_num, out_node_num);
  EXPECT_EQ(in_edge_num, out_edge_num);
  EXPECT_EQ(in_query_num, out_query_num);
  EXPECT_EQ(success, total_success);
  EXPECT_EQ(average_found_before, average_found_after);
  EXPECT_EQ(sum_found_before, sum_found_after);
  EXPECT_EQ(count_found_before, count_found_after);
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(QueryHandler, EmptyResultCheck) {
  Json::Reader reader;
  Json::StyledWriter writer;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/EmptyResultChecks.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  VDMSConfig::init("server/config-emptyresult-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  Json::Value parsed;
  reader.parse(response.json().c_str(), parsed);

  for (int j = 0; j < parsed.size(); j++) {
    const Json::Value &query = parsed[j];
    ASSERT_EQ(query.getMemberNames().size(), 1);
    std::string cmd = query.getMemberNames()[0];

    if (j == 6) { // Second last FindEntity
      EXPECT_EQ(query["FindEntity"]["returned"].asInt(), 0);
    }
    if (j == 7) { // Last FindEntity
      EXPECT_EQ(query["FindEntity"]["average"].asDouble(), 0);
    }
    if (j == 8) { // Last FindConnection
      EXPECT_EQ(query["FindConnection"]["count"].asInt(), 0);
    }
  }

  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(QueryHandler, DataTypeChecks) {
  Json::Reader reader;
  Json::StyledWriter writer;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/DataTypeChecks.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  VDMSConfig::init("server/config-datatype-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  Json::Value parsed;
  reader.parse(response.json().c_str(), parsed);

  // std::cout << writer.write(parsed) << std::endl;
  const Json::Value &query = parsed[3];
  EXPECT_EQ(query["FindEntity"]["entities"][0]["Birthday"].asString(),
            "1936-10-01T17:59:24.001-07:00");
  EXPECT_EQ(query["FindEntity"]["entities"][0]["timestamp"].asInt64(),
            1544069566053);
  EXPECT_EQ(query["FindEntity"]["entities"][1]["Birthday"].asString(),
            "1946-10-01T17:49:24.009010-07:00");

  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(QueryHandler, AutoDeleteNode) {
  Json::Reader reader;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/AutoDeleteNodeInit.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query_init = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  ifile.open("server/AutoDeleteNodeTest.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query_test = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  std::string image;
  std::ifstream image_file("test_images/brain.png",
                           std::ios::in | std::ios::binary | std::ios::ate);

  image.resize(image_file.tellg());

  image_file.seekg(0, std::ios::beg);
  if (!image_file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  std::string video;
  std::ifstream video_file("test_videos/Megamind.avi",
                           std::ios::in | std::ios::binary | std::ios::ate);

  video.resize(video_file.tellg());

  video_file.seekg(0, std::ios::beg);
  if (!video_file.read(&video[0], video.size()))
    std::cout << "error" << std::endl;

  VDMSConfig::init("server/config-datatype-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query_init;
  proto_query_init.set_json(json_query_init);
  proto_query_init.add_blobs(image);
  proto_query_init.add_blobs(image);
  proto_query_init.add_blobs(image);
  proto_query_init.add_blobs(image);
  proto_query_init.add_blobs(video);
  proto_query_init.add_blobs(video);

  VDMS::protobufs::queryMessage response_init;
  query_handler.pq(proto_query_init, response_init);

  std::this_thread::sleep_for(12s);

  qh_base.set_autodelete_init_flag();
  qh_base.build_autodelete_queue();     // create priority queue of nodes with
                                        // _expiration property
  qh_base.regualar_run_autodelete();    // delete nodes that have expired since
                                        // server previous closed
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized

  VDMS::protobufs::queryMessage proto_query_test;
  proto_query_test.set_json(json_query_test);
  VDMS::protobufs::queryMessage response_test;
  query_handler.pq(proto_query_test, response_test);
  Json::Value parsed;
  reader.parse(response_test.json().c_str(), parsed);

  const Json::Value &query_1 = parsed[0];
  EXPECT_EQ(query_1["FindEntity"]["returned"], 2);
  EXPECT_EQ(query_1["FindEntity"]["status"], 0);
  const Json::Value &query_2 = parsed[1];
  EXPECT_EQ(query_2["FindImage"]["returned"], 2);
  EXPECT_EQ(query_2["FindImage"]["status"], 0);
  const Json::Value &query_3 = parsed[2];
  EXPECT_EQ(query_3["FindVideo"]["returned"], 1);
  EXPECT_EQ(query_3["FindVideo"]["status"], 0);

  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(QueryHandler, CustomFunctionNoProcess) {
  Json::Reader reader;
  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/CustomFunctionNoProcess.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;
  std::string image;
  std::ifstream image_file("test_images/brain.png",
                           std::ios::in | std::ios::binary | std::ios::ate);

  image.resize(image_file.tellg());

  image_file.seekg(0, std::ios::beg);
  if (!image_file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  VDMSConfig::init("server/config-datatype-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);
  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  proto_query.add_blobs(image);
  VDMS::protobufs::queryMessage response;
  query_handler.pq(proto_query, response);
  Json::Value parsed;

  reader.parse(response.json().c_str(), parsed);
  const Json::Value &query = parsed[0];
  EXPECT_EQ(query["info"], "custom function process not found");
  EXPECT_EQ(query["status"], -1);
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(QueryHandler, AddUpdateFind_Blob) {

  Json::StyledWriter writer;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/AddFindUpdate_blob.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  Json::Reader reader;
  Json::Value root;
  Json::Value parsed;

  VDMSConfig::init("unit_tests/config-tests.json");
  PMGDQueryHandler::init();
  QueryHandler::init();

  QueryHandler qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);

  std::string image;
  std::ifstream file("test_images/brain.png",
                     std::ios::in | std::ios::binary | std::ios::ate);

  image.resize(file.tellg());

  file.seekg(0, std::ios::beg);
  if (!file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  proto_query.add_blobs(image);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  reader.parse(response.json().c_str(), parsed);
  // std::cout << writer.write(parsed) << std::endl;

  // Verify results returned.
  for (int j = 0; j < parsed.size(); j++) {
    const Json::Value &query = parsed[j];
    ASSERT_EQ(query.getMemberNames().size(), 1);
    std::string cmd = query.getMemberNames()[0];
    EXPECT_EQ(query[cmd]["status"].asInt(), 0);
  }

  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}
