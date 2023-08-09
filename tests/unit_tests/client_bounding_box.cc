#include "meta_data_helper.h"

TEST(CLIENT_CPP, add_BB) {
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->constuct_BB(false);
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple));
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddBoundingBox"]["status"].asInt();
  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, add_BB_with_image) {
  std::string filename = "../tests/test_images/large1.jpg";

  std::vector<std::string *> blobs;

  Meta_Data *meta_obj = new Meta_Data();
  blobs.push_back(meta_obj->read_blob(filename));
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->constuct_BB(true);
  // std::cout<<tuple<<std::endl;
  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);
  // std::cout << result <<std::endl;

  int status1 = result[0]["AddBoundingBox"]["status"].asInt();
  EXPECT_EQ(status1, 0);
}
