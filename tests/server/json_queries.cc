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

#include "QueryHandlerExample.h"
#include "QueryHandlerPMGD.h"
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
std::string singleAddImage_SameFormat(" \
        { \
            \"AddImage\": { \
               \"properties\": { \
                    \"name\": \"brain_0\", \
                    \"doctor\": \"Dr. Strange Love\" \
                }, \
                \"format\": \"png\" \
            } \
        } \
    ");

void _pmgd_generate_desc_linear_increase(int d, int nb, float *xb, float init) {
    float val = init;
    for (int i = 1; i <= nb * d; ++i) {
        xb[i - 1] = val;
        if (i % d == 0)
            val++;
    }
}

//num of dimensions
//number of unique descriptors
//initial value (generally hand in zero)
float *pmgd_generate_desc_linear_increase(int d, int nb, float init) {
    float *xb = new float[d * nb];
    _pmgd_generate_desc_linear_increase(d, nb, xb, init);
    return xb;
}

TEST(AutoReplicate, default_replicate) {

  std::string path = "server/config-auto-replicate-tests.json";

  VDMSConfig::init(path);
  PMGDQueryHandler::init();
  QueryHandlerPMGD::init();

  ReplicationConfig replication_test;
  replication_test.backup_path = "backups";
  replication_test.db_path = "db_backup";
  replication_test.autoreplicate_interval = 5;
  replication_test.autoreplication_unit = "s";
  replication_test.server_port = 55557;

  QueryHandlerPMGD qh_base;
  qh_base.regular_run_autoreplicate(
      replication_test); // set flag to show autodelete queue has been
                         // initialized
}

TEST(ExampleHandler, simplePing) {

  // query contents don't actually matter here, as the example handler ignores
  // them as long as they're in a valid format
  // so we're just gonna copy the add image query from above
  std::string addImg;
  addImg += "[" + singleAddImage + "]";

  VDMSConfig::init("server/config-tests.json");
  PMGDQueryHandler::init();
  QueryHandlerExample::init();

  QueryHandlerExample qh_base;
  QueryHandlerExampleTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(addImg);

  VDMS::protobufs::queryMessage response;
  query_handler.pq(proto_query, response);

  Json::Reader json_reader;
  Json::Value json_response;
  json_reader.parse(response.json(), json_response);

  EXPECT_EQ(json_response[0]["HiThere"].asString(), "Hello, world!");
}

TEST(AddImage, simpleAdd) {
  std::string addImg;
  addImg += "[" + singleAddImage + "]";

  VDMSConfig::init("server/config-tests.json");
  PMGDQueryHandler::init();
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  reader.parse(response.json().c_str(), parsed);

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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

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

  json_reader.parse(response.json(), json_response);

  for (int i = 0; i < total_images; ++i) {
    EXPECT_EQ(json_response[i]["AddImage"]["status"].asString(), "0");
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(AddImage, simpleAddSameFormat) {
  int total_images = 2;
  std::string string_query("[");

  for (int i = 0; i < total_images; ++i) {
    string_query += singleAddImage_SameFormat;
    if (i != total_images - 1)
      string_query += ",";
  }
  string_query += "]";

  VDMSConfig::init("server/config-add10-tests.json");
  PMGDQueryHandler::init();
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

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

  json_reader.parse(response.json(), json_response);

  for (int i = 0; i < total_images; ++i) {
    EXPECT_EQ(json_response[i]["AddImage"]["status"].asString(), "0");
  }
  VDMSConfig::destroy();
  PMGDQueryHandler::destroy();
}

TEST(PMGDQueryHandler, AddAndFind) {
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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  reader.parse(response.json().c_str(), parsed);

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

TEST(PMGDQueryHandler, EmptyResultCheck) {
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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

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

TEST(PMGDQueryHandler, DataTypeChecks) {
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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);
  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  Json::Value parsed;
  reader.parse(response.json().c_str(), parsed);

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

TEST(PMGDQueryHandler, AutoDeleteNode) {
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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

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
  qh_base.regular_run_autodelete();     // delete nodes that have expired since
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
  PMGDQueryHandler::destroy();
  VDMSConfig::destroy();
}

TEST(PMGDQueryHandler, CustomFunctionNoProcess) {
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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);
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

TEST(PMGDQueryHandler, AddUpdateFind_Blob) {

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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

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
TEST(PMGDQueryHandler, AddFind_DescriptorSet) {

  Json::StyledWriter writer;

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("server/AddFindDescriptorSet.json", std::ifstream::in);
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
  QueryHandlerPMGD::init();

  QueryHandlerPMGD qh_base;
  qh_base.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                        // been initialized
  QueryHandlerPMGDTester query_handler(qh_base);

  VDMS::protobufs::queryMessage proto_query;
  proto_query.set_json(json_query);

  VDMS::protobufs::queryMessage response;

  query_handler.pq(proto_query, response);

  reader.parse(response.json().c_str(), parsed);

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

TEST(PMGDQueryHandler, AddFind_Descriptor) {

Json::FastWriter fastWriter;


Json::Value add_set_q;
Json::Value add_set_trans;

Json::Value find_desc_q;
Json::Value find_desc_trans;
Json::Value find_desc_constraints;
Json::Value constraints_vals;

Json::Value add_desc_q;
Json::Value add_desc_trans;
Json::Value add_desc_props;
float *vec_val;
int dims;
int nr_vecs;

Json::Value results;
Json::Value results_list;

//Add Descriptor Set Query
dims = 128;
add_set_q["engine"] = "FaissFlat";
add_set_q["metric"] = "L2";
add_set_q["name"] = "test_set_standalone";
add_set_q["dimensions"] = dims;
add_set_trans["AddDescriptorSet"] = add_set_q;

std::string add_set_str = fastWriter.write(add_set_trans);
std::string final_add_set_str = "[" + add_set_str + "]";
//Add Descriptor Query
nr_vecs = 1;
vec_val = pmgd_generate_desc_linear_increase(dims, nr_vecs,0);

std::string vec_bytes_str;
vec_bytes_str.resize(nr_vecs * dims *sizeof(float));
std::memcpy((void *)vec_bytes_str.data(), vec_val,
        nr_vecs * sizeof(float) * dims);

add_desc_props["prop_1"] = 10;
add_desc_props["prop_2"] = "str_prop";

add_desc_q["set"] = "test_set_standalone";
add_desc_q["properties"] = add_desc_props;
add_desc_trans["AddDescriptor"] = add_desc_q;

std::string add_desc_str = fastWriter.write(add_desc_trans);
std::string final_add_desc_str = "[" + add_desc_str + "]";
//Find Descriptor Query
results_list["list"].append("prop_1");
results_list["list"].append("prop_2");

constraints_vals.append("==");
constraints_vals.append(10);

find_desc_constraints["prop_1"] = constraints_vals;

find_desc_q["set"] = "test_set_standalone";
find_desc_q["constraints"] = find_desc_constraints;
find_desc_q["results"] = results_list;
find_desc_trans["FindDescriptor"] = find_desc_q;

std::string find_desc_str = fastWriter.write(find_desc_trans);
std::string final_find_desc_str = "[" + find_desc_str + "]";
//State initialization
Json::Reader reader;
Json::Value root;
Json::Value parsed;
Json::Value parsed_addset;
Json::Value parsed_finddesc;

VDMS::VDMSConfig::init("unit_tests/config-tests.json");
VDMS::PMGDQueryHandler::init();
VDMS::QueryHandlerPMGD::init();
VDMS::QueryHandlerPMGD qh_base;
VDMS::QueryHandlerPMGDTester query_handler(qh_base);

VDMS::protobufs::queryMessage proto_query_add_set;
VDMS::protobufs::queryMessage proto_query_add_desc;
VDMS::protobufs::queryMessage proto_query_find_desc;
VDMS::protobufs::queryMessage addset_response;
VDMS::protobufs::queryMessage adddesc_response;
VDMS::protobufs::queryMessage finddesc_response;

//Adding and verifying set was created correctly
//Issue AddSet Query
Json::Value resp_obj_addset;
Json::Value status_obj_addset;

proto_query_add_set.set_json(final_add_set_str);
query_handler.pq(proto_query_add_set, addset_response);
reader.parse(addset_response.json().c_str(), parsed);
resp_obj_addset = parsed[0];
status_obj_addset = resp_obj_addset["AddDescriptorSet"];
ASSERT_EQ(status_obj_addset["status"], 0);

// Adding a descriptor
Json::Value resp_obj_adddesc;
Json::Value status_obj_adddesc;

proto_query_add_desc.add_blobs(vec_bytes_str);
proto_query_add_desc.set_json(final_add_desc_str);

query_handler.pq(proto_query_add_desc, adddesc_response);

reader.parse(adddesc_response.json().c_str(), parsed);
resp_obj_adddesc = parsed[0];

//Finding a descriptor
Json::Value resp_obj_finddesc;
Json::Value status_obj_finddesc;
proto_query_find_desc.set_json(final_find_desc_str);
//proto_query_find_desc.add_blobs(vec_val);
query_handler.pq(proto_query_find_desc, finddesc_response);

reader.parse(finddesc_response.json().c_str(), parsed);
resp_obj_adddesc = parsed[0];

Json::Value find_desc_base = resp_obj_adddesc["FindDescriptor"];
Json::Value find_desc_entities = find_desc_base["entities"];
Json::Value ind_entity = find_desc_entities[0];
ASSERT_EQ(ind_entity["prop_1"], 10);

}

TEST(PMGDQueryHandler, AddFind_DescriptorBatch_KNN) {

Json::FastWriter fastWriter;

Json::Value add_set_q;
Json::Value add_set_trans;

Json::Value find_desc_q;
Json::Value find_desc_trans;
Json::Value find_desc_constraints;
Json::Value constraints_vals;

Json::Value add_desc_q;
Json::Value add_desc_trans;
Json::Value add_desc_props_1;
Json::Value add_desc_props_2;
Json::Value add_desc_props_3;

float *vec_val;
int dims;
int nr_vecs;

Json::Value results;
Json::Value results_list;

//Add Descriptor Set Query
dims = 128;
add_set_q["engine"] = "FaissFlat";
add_set_q["metric"] = "L2";
add_set_q["name"] = "test_set_batch_knn";
add_set_q["dimensions"] = dims;
add_set_trans["AddDescriptorSet"] = add_set_q;

std::string add_set_str = fastWriter.write(add_set_trans);
std::string final_add_set_str = "[" + add_set_str + "]";

//Add Descriptor Batch Query
Json::Value props_list;
nr_vecs = 3;
vec_val = pmgd_generate_desc_linear_increase(dims, nr_vecs,0);

std::string vec_bytes_str;
vec_bytes_str.resize(nr_vecs * dims *sizeof(float));
std::memcpy((void *)vec_bytes_str.data(), vec_val,
        nr_vecs * sizeof(float) * dims);

add_desc_props_1["prop_1"] = 10;
add_desc_props_1["prop_2"] = "str_prop_1";

add_desc_props_2["prop_1"] = 20;
add_desc_props_2["prop_2"] = "str_prop_2";

add_desc_props_3["prop_1"] = 30;
add_desc_props_3["prop_2"] = "str_prop_3";

props_list.append(add_desc_props_1);
props_list.append(add_desc_props_2);
props_list.append(add_desc_props_3);


add_desc_q["set"] = "test_set_batch_knn";
add_desc_q["batch_properties"] = props_list;
add_desc_trans["AddDescriptor"] = add_desc_q;

std::string add_desc_str = fastWriter.write(add_desc_trans);
std::string final_add_desc_str = "[" + add_desc_str + "]";

//Find Descriptor Query + KNN
results_list["list"].append("prop_1");
results_list["list"].append("prop_2");

constraints_vals.append("==");
constraints_vals.append(10);

find_desc_q["set"] = "test_set_batch_knn";
find_desc_q["results"] = results_list;
find_desc_q["k_neighbors"] = 2;
find_desc_trans["FindDescriptor"] = find_desc_q;

std::string find_desc_str = fastWriter.write(find_desc_trans);
std::string final_find_desc_str = "[" + find_desc_str + "]";

//State initialization
Json::Reader reader;
Json::Value root;
Json::Value parsed;
Json::Value parsed_addset;
Json::Value parsed_finddesc;

VDMS::VDMSConfig::init("unit_tests/config-tests.json");
VDMS::PMGDQueryHandler::init();
VDMS::QueryHandlerPMGD::init();
VDMS::QueryHandlerPMGD qh_base;
VDMS::QueryHandlerPMGDTester query_handler(qh_base);


VDMS::protobufs::queryMessage proto_query_add_set;
VDMS::protobufs::queryMessage proto_query_add_desc;
VDMS::protobufs::queryMessage proto_query_find_desc;
VDMS::protobufs::queryMessage addset_response;
VDMS::protobufs::queryMessage adddesc_response;
VDMS::protobufs::queryMessage finddesc_response;

//Adding and verifying set was created correctly
//Issue AddSet Query
Json::Value resp_obj_addset;
Json::Value status_obj_addset;
proto_query_add_set.set_json(final_add_set_str);
query_handler.pq(proto_query_add_set, addset_response);

reader.parse(addset_response.json().c_str(), parsed);
resp_obj_addset = parsed[0];
status_obj_addset = resp_obj_addset["AddDescriptorSet"];
ASSERT_EQ(status_obj_addset["status"], 0);

// Adding a descriptor
Json::Value resp_obj_adddesc;
Json::Value status_obj_adddesc;

proto_query_add_desc.add_blobs(vec_bytes_str);
proto_query_add_desc.set_json(final_add_desc_str);

query_handler.pq(proto_query_add_desc, adddesc_response);

reader.parse(adddesc_response.json().c_str(), parsed);
resp_obj_adddesc = parsed[0];

//Finding a descriptor
Json::Value resp_obj_finddesc;
Json::Value status_obj_finddesc;

vec_val = pmgd_generate_desc_linear_increase(dims, 1,0);

vec_bytes_str.resize(dims *sizeof(float));
std::memcpy((void *)vec_bytes_str.data(), vec_val,
128 * sizeof(float));

proto_query_find_desc.set_json(final_find_desc_str);
proto_query_find_desc.add_blobs(vec_bytes_str);
query_handler.pq(proto_query_find_desc, finddesc_response);

reader.parse(finddesc_response.json().c_str(), parsed);
resp_obj_adddesc = parsed[0];

Json::Value find_desc_base = resp_obj_adddesc["FindDescriptor"];
Json::Value find_desc_entities = find_desc_base["entities"];
int nr_entities = find_desc_entities.size();
Json::Value ind_entity = find_desc_entities[1];
ASSERT_EQ(ind_entity["prop_1"], 20);
ASSERT_EQ(nr_entities,2);

}


