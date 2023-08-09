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