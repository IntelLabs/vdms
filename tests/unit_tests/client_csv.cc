#include "CSVParser.h"
#include "meta_data_helper.h"
TEST(CLIENT_CPP_CSV, parse_csv_entity) {

  std::string filename = "../tests/csv_samples/CSVformat100.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;
  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddEntity"]["status"].asInt(), 0);
  }
}

// TEST(CLIENT_CPP_CSV, parse_update_csv_entity)
// {

//     std::string filename = "../tests/csv_samples/update_entity.csv";
//     size_t num_threads = 2;
//     std::string vdms_server ="localhost";
//     int port = 55558;
//     std::vector <VDMS::Response> all_results;
//     VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

//    all_results = csv_parser.parse();
//     Json::Value result;
//     Json::Reader _reader;
//     for (int k=0; k<all_results.size(); k++)
//     {
//         _reader.parse(all_results[k].json.c_str(), result);
//          EXPECT_EQ(result[k]["UpdateEntity"]["status"].asInt(), 0);

//     }

// }
TEST(CLIENT_CPP_CSV, parse_csv_connection) {

  std::string filename = "../tests/csv_samples/connection.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;

  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddConnection"]["status"].asInt(), 0);
  }
}
TEST(CLIENT_CPP_CSV, parse_csv_images) {
  std::string filename = "../tests/csv_samples/Image.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;
  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddImage"]["status"].asInt(), 0);
  }
}

TEST(CLIENT_CPP_CSV, parse_csv_descriptor_set) {
  std::string filename = "../tests/csv_samples/DescriptorSet.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;

  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddDescriptorSet"]["status"].asInt(), 0);
  }
}

TEST(CLIENT_CPP_CSV, parse_csv_descriptor) {
  std::string filename = "../tests/csv_samples/Descriptor.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;

  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddDescriptor"]["status"].asInt(), 0);
  }
}
TEST(CLIENT_CPP_CSV, parse_csv_bb) {
  std::string filename = "../tests/csv_samples/Rectangle.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;

  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddBoundingBox"]["status"].asInt(), 0);
  }
}
TEST(CLIENT_CPP_CSV, parse_csv_video) {
  std::string filename = "../tests/csv_samples/Video.csv";
  size_t num_threads = 5;
  std::string vdms_server = "localhost";
  int port = 55558;
  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["AddVideo"]["status"].asInt(), 0);
  }
}

TEST(CLIENT_CPP_CSV, parse_csv_invalid_entity) {
  std::string filename = "../tests/csv_samples/invalid.csv";
  std::ofstream csv_file;
  csv_file.open(filename);
  csv_file << "EntityInvalidTest,prop_name,prop_lastname,prop_id,prop_age\n";
  csv_file << "Person,Ali,Hum,1,2\n";
  csv_file.close();

  size_t num_threads = 1;
  std::string vdms_server = "localhost";
  int port = 55558;
  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  remove(filename.c_str());

  Json::Value result;
  Json::Reader _reader;
  _reader.parse(all_results[0].json.c_str(), result);
  EXPECT_EQ(result["status"].asInt(), -1);
  EXPECT_EQ(result["info"].asString(), "Command does not exist");
}

TEST(CLIENT_CPP_CSV, parse_csv_invalid_image) {
  std::string filename = "../tests/csv_samples/invalid_file.csv";
  std::ofstream csv_file;
  csv_file.open(filename);
  csv_file << "ImagePath,ops_threshold,ops_crop,ops_resize,ops_flip,ops_rotate,"
              "prop_type,prop_part,format,cons_1\n";
  csv_file << "../tests/test_images/"
              "large1_invalid.jpg,350,,,,,,image1,jpg,part==image1\n";
  csv_file.close();

  size_t num_threads = 1;
  std::string vdms_server = "localhost";
  int port = 55558;
  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  remove(filename.c_str());

  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["status"].asInt(), -1);
  }
}

TEST(CLIENT_CPP_CSV, parse_csv_invalid_video) {
  std::string filename = "../tests/csv_samples/invalid_file.csv";
  std::ofstream csv_file;
  csv_file.open(filename);
  csv_file << "VideoPath,format,compressto,prop_name,ops_resize,ops_interval\n";
  csv_file << "../tests/test_videos/Megamind_invalid.avi,avi,h264,Good,,\n";
  csv_file.close();

  size_t num_threads = 1;
  std::string vdms_server = "localhost";
  int port = 55558;
  std::vector<VDMS::Response> all_results;
  VDMS::CSVParser csv_parser(filename, num_threads, vdms_server, port);

  all_results = csv_parser.parse();
  remove(filename.c_str());

  Json::Value result;
  Json::Reader _reader;
  for (int k = 0; k < all_results.size(); k++) {
    _reader.parse(all_results[k].json.c_str(), result);
    EXPECT_EQ(result[k]["status"].asInt(), -1);
  }
}
