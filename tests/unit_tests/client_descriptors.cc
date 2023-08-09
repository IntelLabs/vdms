#include "meta_data_helper.h"

TEST(CLIENT_CPP, add_descriptor) {
  std::vector<float> fv_values;
  srand((unsigned)time(NULL));
  for (int i = 0; i < 1000; i++)
    fv_values.push_back((float)rand() / RAND_MAX);

  std::vector<std::string *> blobs;
  std::string *bytes_str = new std::string();
  bytes_str->resize(fv_values.size() * sizeof(float));
  std::memcpy((void *)bytes_str->data(), fv_values.data(),
              fv_values.size() * sizeof(float));
  blobs.push_back(bytes_str);

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->construct_descriptor();

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddDescriptor"]["status"].asInt();

  EXPECT_EQ(status1, 0);
}
TEST(CLIENT_CPP, add_flinng_descriptor) {
  std::vector<float> fv_values;
  srand((unsigned)time(NULL));
  for (int i = 0; i < 100; i++)
    fv_values.push_back((float)rand() / RAND_MAX);

  std::vector<std::string *> blobs;
  std::string *bytes_str = new std::string();
  bytes_str->resize(fv_values.size() * sizeof(float));
  std::memcpy((void *)bytes_str->data(), fv_values.data(),
              fv_values.size() * sizeof(float));
  blobs.push_back(bytes_str);

  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->construct_descriptor();

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddDescriptor"]["status"].asInt();

  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP, find_descriptor) {

  std::vector<float> fv_values;
  srand((unsigned)time(NULL));

  for (int i = 0; i < 1000; i++) {
    fv_values.push_back((float)rand() / RAND_MAX);
  }

  std::vector<std::string *> blobs;
  std::string *bytes_str = new std::string();
  bytes_str->resize(fv_values.size() * sizeof(float));
  std::memcpy((void *)bytes_str->data(), fv_values.data(),
              fv_values.size() * sizeof(float));

  blobs.push_back(bytes_str);
  Meta_Data *meta_obj = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->construct_find_descriptor();

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);
  int status1;

  if (!result.isArray())
    status1 = -10;

  if (result[0]["FindDescriptor"]["status"] == -1)

    status1 = -1;
  else
    status1 = result[0]["FindDescriptor"]["status"].asInt();

  EXPECT_EQ(status1, 0);
}