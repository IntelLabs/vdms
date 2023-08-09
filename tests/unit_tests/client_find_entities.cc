
#include "meta_data_helper.h"

TEST(CLIENT_CPP, find_single_entity) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_find_entity(false, false));

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status = result[0]["FindEntity"]["status"].asInt();

  EXPECT_EQ(status, 0);
}
TEST(CLIENT_CPP, find_single_delete_flag) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_find_entity(true, false));

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status = result[0]["FindEntity"]["status"].asInt();

  EXPECT_EQ(status, 0);
}

TEST(CLIENT_CPP, find_single_expiration_flag) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_find_entity(false, true));

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status = result[0]["FindEntity"]["status"].asInt();

  EXPECT_EQ(status, 0);
}

TEST(CLIENT_CPP, find_single_expiration_flag_auto_delete) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple.append(meta_obj->construct_find_entity(true, true));

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status = result[0]["FindEntity"]["status"].asInt();

  EXPECT_EQ(status, 0);
}
