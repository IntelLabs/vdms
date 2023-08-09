
#include "meta_data_helper.h"

TEST(CLIENT_CPP, add_two_CLIENT_CPP_with_connection) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_add_query(1, false, false));

  tuple.append(meta_obj->construct_add_area(2, false));
  tuple.append(meta_obj->construct_add_connection(1, 2, false));

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddEntity"]["status"].asInt();
  int status2 = result[1]["AddEntity"]["status"].asInt();
  int status3 = result[1]["AddConnection"]["status"].asInt();

  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  EXPECT_EQ(status3, 0);
}

TEST(CLIENT_CPP, add_single_entity) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_add_query(1, false, false));
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddEntity"]["status"].asInt();

  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, add_single_entity_expiration) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_add_query(1, false, true));
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddEntity"]["status"].asInt();

  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, add_single_entity_constraints) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_add_query(1, true, false));
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddEntity"]["status"].asInt();

  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, add_multiple_CLIENT_CPP) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  int num_queries = 4;
  for (int i = 1; i <= num_queries; i++) {
    tuple.append(meta_obj->construct_add_query(i, false, false));
  }

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  for (int i = 0; i < result.size(); i++) {
    int status = result[i]["AddEntity"]["status"].asInt();

    EXPECT_EQ(status, 0);
  }
}
TEST(CLIENT_CPP, add_multiple_from_file) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("../tests/unit_tests/queries.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  VDMS::Response response = meta_obj->_aclient->query(json_query);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  for (int i = 0; i < result.size(); i++) {
    int status = result[i]["AddEntity"]["status"].asInt();
    EXPECT_EQ(status, 0);
  }
}

TEST(CLIENT_CPP, add_two_from_file) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("../tests/unit_tests/two_entities.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  VDMS::Response response = meta_obj->_aclient->query(json_query);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  for (int i = 0; i < result.size(); i++) {
    int status = result[i]["AddEntity"]["status"].asInt();
    EXPECT_EQ(status, 0);
  }
}

TEST(CLIENT_CPP, add_connection_from_file) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));

  std::ifstream ifile;
  int fsize;
  char *inBuf;
  ifile.open("../tests/unit_tests/connection.json", std::ifstream::in);
  ifile.seekg(0, std::ios::end);
  fsize = (int)ifile.tellg();
  ifile.seekg(0, std::ios::beg);
  inBuf = new char[fsize];
  ifile.read(inBuf, fsize);
  std::string json_query = std::string(inBuf);
  ifile.close();
  delete[] inBuf;

  VDMS::Response response = meta_obj->_aclient->query(json_query);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  for (int i = 0; i < result.size() - 1; i++) {
    int status = result[i]["FindEntity"]["status"].asInt();
    EXPECT_EQ(status, 0);
  }
}

TEST(CLIENT_CPP, add_multiple_CLIENT_CPP_constraints) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  int num_queries = 4;
  for (int i = 1; i <= num_queries; i++) {
    tuple.append(meta_obj->construct_add_query(i, true, false));
  }

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  for (int i = 0; i < result.size(); i++) {
    int status = result[i]["AddEntity"]["status"].asInt();
    EXPECT_EQ(status, 0);
  }
}
