#include "meta_data_helper.h"
#include <fcntl.h>
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
