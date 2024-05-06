#include "VDMSConfig.h"
#include "meta_data_helper.h"
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

using std::cerr;
using std::cout;
using std::endl;
using std::ifstream;
using std::ostringstream;
using std::string;

int get_fourcc() { return cv::VideoWriter::fourcc('H', '2', '6', '4'); }

string readFileIntoString(const string &path) {
  auto ss = ostringstream{};
  ifstream input_file(path);
  if (!input_file.is_open()) {
    cerr << "Could not open the file - '" << path << "'" << endl;
    exit(EXIT_FAILURE);
  }
  ss << input_file.rdbuf();
  return ss.str();
}

TEST(CLIENT_CPP_Video, add_single_video) {

  // std::string video;
  std::stringstream video;
  std::vector<std::string *> blobs;

  std::string filename = "../tests/videos/Megamind.avi";

  Meta_Data *meta_obj = new Meta_Data();
  blobs.push_back(meta_obj->read_blob(filename));
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  Json::Value tuple;
  tuple = meta_obj->constuct_video(false);

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  int status1 = result[0]["AddVideo"]["status"].asInt();
  EXPECT_EQ(status1, 0);
}

TEST(CLIENT_CPP_Video, add_single_video_multi_client) {

  // std::string video;
  std::stringstream video;
  std::vector<std::string *> blobs;

  VDMS::VDMSConfig::init("unit_tests/config-client-tests.json");

  std::string filename = "../tests/videos/Megamind.mp4";

  std::string temp_video_path(VDMS::VDMSConfig::instance()->get_path_tmp() +
                              "/pathvideo.mp4");
  // std::filesystem::copy_file(filename, temp_video_path);
  copy_video_to_temp(filename, temp_video_path, get_fourcc());

  Meta_Data *meta_obj = new Meta_Data();
  Meta_Data *meta_obj2 = new Meta_Data();
  meta_obj->_aclient.reset(
      new VDMS::VDMSClient(meta_obj->get_server(), meta_obj->get_port()));
  meta_obj2->_aclient.reset(
      new VDMS::VDMSClient(meta_obj2->get_server(), meta_obj2->get_port()));

  Json::Value op;
  op["type"] = "resize";
  op["width"] = 100;
  op["height"] = 100;

  Json::Value op2;
  op2["type"] = "resize";
  op2["width"] = 100;
  op2["height"] = 100;

  Json::Value tuple, tuple2;
  tuple = meta_obj->constuct_video_by_path(1, temp_video_path, op);
  tuple2 = meta_obj2->constuct_video_by_path(2, temp_video_path, op2);

  VDMS::Response response =
      meta_obj->_aclient->query(meta_obj->_fastwriter.write(tuple), blobs);
  Json::Value result;
  meta_obj->_reader.parse(response.json.c_str(), result);

  VDMS::Response response2 =
      meta_obj2->_aclient->query(meta_obj2->_fastwriter.write(tuple2), blobs);
  Json::Value result2;
  meta_obj2->_reader.parse(response2.json.c_str(), result2);

  int status1 = result[0]["AddVideo"]["status"].asInt();
  int status2 = result2[0]["AddVideo"]["status"].asInt();

  if (std::remove(temp_video_path.data()) != 0) {
    throw VCLException(ObjectEmpty,
                       "Error encountered while removing the file.");
  }

  EXPECT_EQ(status1, 0);
  EXPECT_EQ(status2, 0);
  delete meta_obj;
  delete meta_obj2;
}
