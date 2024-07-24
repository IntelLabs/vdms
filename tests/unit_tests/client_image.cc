#include "meta_data_helper.h"

TEST(CLIENT_CPP, add_image) {

  std::string filename = "../tests/test_images/large1.jpg";

  std::vector<std::string *> blobs;

  Meta_Data *meta_obj = new Meta_Data();
  blobs.push_back(meta_obj->read_blob(filename));
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->constuct_image();

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, add_image_resize_operation) {

  std::string image;

  std::fstream file("../tests/test_images/large1.jpg",
                    std::ios::in | std::ios::binary | std::ios::ate);

  image.resize(file.tellg());

  file.seekg(0, std::ios::beg);
  if (!file.read(&image[0], image.size()))
    std::cout << "error" << std::endl;

  std::vector<std::string *> blobs;

  std::string *bytes_str = new std::string(image);

  blobs.push_back(bytes_str);
  Json::Value op;
  op["type"] = "resize";
  op["width"] = 100;
  op["height"] = 100;
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->constuct_image(true, op);

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, find_image) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->construct_find_image();

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["FindImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, find_image_noentity) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->construct_find_image_no_entity();

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  std::string info1 = result[0]["FindImage"]["info"].asString();
  delete meta_obj;
  EXPECT_STREQ(info1.data(), "No entities found");
}

TEST(CLIENT_CPP, find_image_remote) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  Json::Value op;
  op["type"] = "remoteOp";
  op["url"] = "http://localhost:5010/image";
  op["options"]["id"] = "flip";
  op["options"]["format"] = "jpg";
  tuple = meta_obj->construct_find_image_withop(op);

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["FindImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
  delete meta_obj;
}

TEST(CLIENT_CPP, find_image_syncremote) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  Json::Value op;
  op["type"] = "syncremoteOp";
  op["url"] = "http://localhost:5010/image";
  op["options"]["id"] = "flip";
  op["options"]["format"] = "jpg";
  tuple = meta_obj->construct_find_image_withop(op);

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["FindImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
  delete meta_obj;
}

TEST(CLIENT_CPP, find_image_udf) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  Json::Value op;
  op["type"] = "userOp";
  op["options"]["id"] = "flip";
  op["options"]["format"] = "jpg";
  op["options"]["port"] = 5555;
  tuple = meta_obj->construct_find_image_withop(op);

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["FindImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
  delete meta_obj;
}

TEST(CLIENT_CPP, add_image_dynamic_metadata) {

  std::string filename = "../tests/test_images/metadata_image.jpg";
  std::vector<std::string *> blobs;

  Json::Value op;
  op["type"] = "userOp";
  op["options"]["id"] = "metadata";
  op["options"]["format"] = "jpg";
  op["options"]["media_type"] = "image";
  op["options"]["otype"] = "face";
  op["options"]["port"] = 5555;
  Meta_Data *meta_obj = new Meta_Data();
  blobs.push_back(meta_obj->read_blob(filename));
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;

  tuple = meta_obj->constuct_image(true, op, "image_dynamic_metadata");
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
  delete meta_obj;
}

TEST(CLIENT_CPP, add_image_dynamic_metadata_remote) {

  std::string filename = "../tests/test_images/metadata_image.jpg";
  std::vector<std::string *> blobs;

  Json::Value op;
  op["type"] = "syncremoteOp";
  op["options"]["id"] = "metadata";
  op["options"]["format"] = "jpg";
  op["options"]["media_type"] = "image";
  op["options"]["otype"] = "face";
  op["url"] = "http://localhost:5010/image";
  Meta_Data *meta_obj = new Meta_Data();
  blobs.push_back(meta_obj->read_blob(filename));
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;

  tuple = meta_obj->constuct_image(true, op, "image_dynamic_metadata_remote");
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddImage"]["status"].asInt();
  EXPECT_EQ(status1, 0);
  delete meta_obj;
}

TEST(CLIENT_CPP, find_image_dynamic_metadata) {

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->construct_find_image_with_dynamic_metadata();
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);
  int status_i = result[0]["FindImage"]["status"].asInt();
  int status_b = result[1]["FindImage"]["status"].asInt();
  std::string objectId =
      result[1]["FindImage"]["entities"][0]["objectID"].asString();
  EXPECT_EQ(status_i, 0);
  EXPECT_EQ(status_b, 0);
  EXPECT_STREQ(objectId.data(), "face");
  delete meta_obj;
}