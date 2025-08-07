/**
 * @file   ImageData_test.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <string>
#include <filesystem>

#include "gtest/gtest.h"
#include <exception>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include "Image.h"
#include "TDBImage.h"

#include "RemoteConnection.h"
#include "VDMSConfig.h"
#include "vcl/Exception.h"

#include "QueryHandlerPMGD.h"

using namespace VDMS;
using namespace PMGD;
using namespace std;

const std::string TMP_DIRNAME = "/tmp/tests_output_dir/";

class RemoteConnectionTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    VDMS::VDMSConfig::init(TMP_DIRNAME + "config-aws-tests.json");
    img_ = "test_images/large1.jpg";
    tdb_img_ = TMP_DIRNAME + "tdb/test_image.tdb";
    video_ = "test_videos/Megamind.avi";
    cv_img_ = cv::imread(img_, cv::IMREAD_ANYCOLOR);
    rect_ = VCL::Rectangle(100, 100, 100, 100);

    connection_ = new VCL::RemoteConnection();
    connection_->_bucket_name = "minio-bucket";
    connection_->start();
  }

  virtual void TearDown() {
    VDMS::VDMSConfig::destroy();
    if (connection_) {
      connection_->end();
      delete connection_;
      connection_ = nullptr;
    }
  }

  void compare_mat_mat(cv::Mat &cv_img, cv::Mat &img) {
    int rows = img.rows;
    int columns = img.cols;
    int channels = img.channels();

    if (img.isContinuous()) {
      columns *= rows;
      rows = 1;
    }

    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < columns; ++j) {
        if (channels == 1) {
          unsigned char pixel = img.at<unsigned char>(i, j);
          unsigned char test_pixel = cv_img.at<unsigned char>(i, j);
          ASSERT_EQ(pixel, test_pixel);
        } else {
          cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
          cv::Vec3b test_colors = cv_img.at<cv::Vec3b>(i, j);
          for (int x = 0; x < channels; ++x) {
            ASSERT_EQ(colors.val[x], test_colors.val[x]);
          }
        }
      }
    }
  }

  // needed a special compare function for JPGs because of small encoding
  // differences pixel values can vary by up to 19 in my observations (tmcourie)
  void compare_mat_mat_jpg(cv::Mat &cv_img, cv::Mat &img) {
    int rows = img.rows;
    int columns = img.cols;
    int channels = img.channels();

    if (img.isContinuous()) {
      columns *= rows;
      rows = 1;
    }

    // TODO determine an appropriate value for this
    int pixel_similarity_threshhold = 20;

    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < columns; ++j) {
        if (channels == 1) {
          unsigned char pixel = img.at<unsigned char>(i, j);
          unsigned char test_pixel = cv_img.at<unsigned char>(i, j);
          ASSERT_LE(abs(pixel - test_pixel), pixel_similarity_threshhold);
        } else {
          cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
          cv::Vec3b test_colors = cv_img.at<cv::Vec3b>(i, j);
          for (int x = 0; x < channels; ++x) {
            ASSERT_LE(abs(colors.val[x] - test_colors.val[x]),
                      pixel_similarity_threshhold);
          }
        }
      }
    }
  }

  void compare_mat_buffer(cv::Mat &img, unsigned char *buffer) {
    int index = 0;

    int rows = img.rows;
    int columns = img.cols;
    int channels = img.channels();

    if (img.isContinuous()) {
      columns *= rows;
      rows = 1;
    }

    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < columns; ++j) {
        if (channels == 1) {
          unsigned char pixel = img.at<unsigned char>(i, j);
          ASSERT_EQ(pixel, buffer[index]);
        } else {
          cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
          for (int x = 0; x < channels; ++x) {
            ASSERT_EQ(colors.val[x], buffer[index + x]);
          }
        }
        index += channels;
      }
    }
  }

  std::string img_;
  std::string video_;
  std::string tdb_img_;
  std::string test_img_;
  cv::Mat cv_img_;
  VCL::Rectangle rect_;
  VCL::RemoteConnection *connection_;
};

namespace VCL {

class ImageTest : public Image {

public:
  ImageTest() : Image() {}
  ImageTest(std::string a) : Image(a) {}
  ImageTest(cv::Mat &a) : Image(a) {}

  using Image::perform_operations;
  using Image::read;
  using Image::set_data_from_encoded;
  using Image::set_data_from_raw;
  using Image::set_format;
};
}; // namespace VCL

// U T I L I T A R Y   F U N C T I O N S
void printErrorMessage(const std::string &functionName) {
  FAIL() << "Error: Unhandled exception in " << functionName << "()"
         << std::endl;
}

// Basic Remote Connection Tests

TEST_F(RemoteConnectionTest, RemoteWriteFilename) {
  try {
    ASSERT_TRUE(connection_);
    EXPECT_TRUE(connection_->Write(img_));
  } catch (...) {
    printErrorMessage("RemoteWriteFilename");
  }
}

TEST_F(RemoteConnectionTest, RemoteReadWriteBuffer) {
  try {
    ASSERT_TRUE(connection_);
    std::vector<unsigned char> img_data = connection_->Read(img_);
    EXPECT_TRUE(connection_->Write(img_, img_data));
  } catch (...) {
    printErrorMessage("RemoteReadWriteBuffer");
  }
}

TEST_F(RemoteConnectionTest, RemoteListRetrieveFile) {
  try {
    ASSERT_TRUE(connection_);
    // Add the file to S3
    EXPECT_TRUE(connection_->Write(img_));

    std::vector<std::string> file_list =
        connection_->ListFilesInFolder("test_images");
    EXPECT_FALSE(file_list.empty()); // It should have at least one element

    // This is the test
    EXPECT_TRUE(connection_->RetrieveFile(file_list[0]));

    // It removes the file so it doesn't affect the other tests
    EXPECT_TRUE(connection_->Remove_Object(file_list[0]));

  } catch (...) {
    printErrorMessage("RemoteListRetrieveFile");
  }
}

TEST_F(RemoteConnectionTest, RemoteWriteVideoFilename) {
  try {
    ASSERT_TRUE(connection_);
    EXPECT_TRUE(connection_->Write(video_));
  } catch (...) {
    printErrorMessage("RemoteWriteVideoFilename");
  }
}

TEST_F(RemoteConnectionTest, RemoteReadVideoFilename) {
  try {
    // Prepare the test
    ASSERT_TRUE(connection_);
    EXPECT_TRUE(connection_->Write(video_));

    // Execute the test
    EXPECT_TRUE(connection_->Read_Video(video_));

    // Cleanup
    EXPECT_TRUE(connection_->Remove_Object(video_));
  } catch (...) {
    printErrorMessage("RemoteReadVideoFilename");
  }
}

//#### Regular Image tests ####

TEST_F(RemoteConnectionTest, ImageRemoteWritePNG) {
  try {
    ASSERT_TRUE(connection_);
    VCL::ImageTest img(cv_img_);

    img.set_connection(connection_);
    std::string path = "pngs/test_image.png";

    img.store(path, VCL::Format::PNG);
    img.perform_operations();
  } catch (...) {
    printErrorMessage("ImageRemoteWritePNG");
  }
}

TEST_F(RemoteConnectionTest, ImageRemoteReadPNG) {
  try {
    ASSERT_TRUE(connection_);
    VCL::ImageTest img(cv_img_);

    img.set_connection(connection_);
    std::string path = "pngs/test_image.png";

    // First, add the image
    img.store(path, VCL::Format::PNG);
    img.perform_operations();

    // Then, execute the test
    img.read(path);
    cv::Mat data = img.get_cvmat();

    // Check the results
    compare_mat_mat(data, cv_img_);
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    std::cerr << "RemoteConnectionTest.ImageRemoteReadPNG() failed: "
              << ex.what() << std::endl;
  } catch (...) {
    printErrorMessage("ImageRemoteReadPNG");
  }
}

TEST_F(RemoteConnectionTest, ImageRemoteRemovePNG) {
  try {
    ASSERT_TRUE(connection_);
    VCL::Image img("pngs/test_image.png");
    img.set_connection(connection_);
    EXPECT_TRUE(img.delete_image());
  } catch (...) {
    printErrorMessage("ImageRemoteRemovePNG");
  }
}

TEST_F(RemoteConnectionTest, ImageRemoteWriteJPG) {
  try {
    ASSERT_TRUE(connection_);
    VCL::Image img(cv_img_);

    img.set_connection(connection_);
    std::string path = "jpgs/large1.jpg";
    img.store(path, VCL::Format::JPG);
  } catch (...) {
    printErrorMessage("ImageRemoteWriteJPG");
  }
}

TEST_F(RemoteConnectionTest, ImageRemoteReadJPG) {
  try {

    // Prepare the test
    ASSERT_TRUE(connection_);
    VCL::Image img(cv_img_);
    std::string path = "jpgs/large1.jpg";
    img.store(path, VCL::Format::JPG);

    // Execute the test
    cv::Mat mat = img.get_cvmat();

    // Compare the results
    compare_mat_mat_jpg(mat, cv_img_);
    // Remove the image to avoid affecting the other tests
    EXPECT_TRUE(img.delete_image());
  } catch (...) {
    printErrorMessage("ImageRemoteReadJPG");
  }
}

TEST_F(RemoteConnectionTest, ImageRemoteRemoveJPG) {
  try {
    ASSERT_TRUE(connection_);
    VCL::Image img("jpgs/large1.jpg");
    img.set_connection(connection_);
    EXPECT_TRUE(img.delete_image());
  } catch (...) {
    printErrorMessage("ImageRemoteRemoveJPG");
  }
}

//#### TileDB Image tests ####
TEST_F(RemoteConnectionTest, TDBImageWriteS3) {
  try {
    ASSERT_TRUE(connection_);
    VCL::TDBImage tdb(TMP_DIRNAME + "tdb/test_image.tdb", *connection_);
    tdb.write(cv_img_);
  } catch (...) {
    printErrorMessage("TDBImageWriteS3");
  }
}

// Basic Remote Connection Tests (no remote connected, expected failures)

TEST_F(RemoteConnectionTest, RemoteDisconnectedWriteFilename) {
  try {
    VCL::RemoteConnection not_a_connection;
    EXPECT_FALSE(not_a_connection.Write(img_));
  } catch (...) {
    printErrorMessage("RemoteDisconnectedWriteFilename");
  }
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedReadBuffer) {
  try {
    VCL::RemoteConnection not_a_connection;
    std::vector<unsigned char> img_data = not_a_connection.Read(img_);
    EXPECT_FALSE(not_a_connection.Write(img_, img_data));
  } catch (...) {
    printErrorMessage("RemoteDisconnectedReadBuffer");
  }
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedWriteBuffer) {
  try {
    VCL::RemoteConnection not_a_connection;
    std::vector<unsigned char> img_data = connection_->Read(img_);
    EXPECT_FALSE(not_a_connection.Write(img_, img_data));
  } catch (...) {
    printErrorMessage("RemoteDisconnectedWriteBuffer");
  }
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedListFiles) {
  try {
    VCL::RemoteConnection not_a_connection;
    std::vector<std::string> file_list =
        not_a_connection.ListFilesInFolder("test_images");
  } catch (...) {
    printErrorMessage("RemoteDisconnectedListFiles");
  }
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedRetrieveFile) {
  try {
    VCL::RemoteConnection not_a_connection;
    std::vector<std::string> file_list =
        connection_->ListFilesInFolder("test_images");
    EXPECT_FALSE(not_a_connection.RetrieveFile(file_list[0]));
  } catch (...) {
    printErrorMessage("RemoteDisconnectedRetrieveFile");
  }
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedWriteVideoFilename) {
  try {
    VCL::RemoteConnection not_a_connection;
    EXPECT_FALSE(not_a_connection.Write(video_));
  } catch (...) {
    printErrorMessage("RemoteDisconnectedWriteVideoFilename");
  }
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedReadVideoFilename) {
  try {
    VCL::RemoteConnection not_a_connection;
    EXPECT_FALSE(not_a_connection.Read_Video(video_));
  } catch (...) {
    printErrorMessage("RemoteDisconnectedReadVideoFilename");
  }
}

TEST_F(RemoteConnectionTest, ImageAddCropFailure) {
  try {
    std::string string_query_add_image_failure("[");
    string_query_add_image_failure += " \
      { \
          \"AddImage\": { \
              \"operations\": [{ \
                  \"type\": \"crop\", \
                  \"x\": 250, \
                  \"y\": 250, \
                  \"width\": 100, \
                  \"height\": 100  \
              }], \
              \"properties\": { \
                  \"name\": \"simpleCropFailure_brain_sample_image\", \
                  \"doctor\": \"Dr. Strange Love\" \
              }, \
              \"format\": \"png\" \
          } \
      } \
    ";
    string_query_add_image_failure += "]";

    VDMS::Server VDMS_server("unit_tests/config-aws-tests.json", "", "", "");

    QueryHandlerPMGD query_handler;
    query_handler.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                          // been initialized

    VDMS::protobufs::queryMessage proto_query;
    proto_query.set_json(string_query_add_image_failure);

    std::string image;
    std::ifstream file("test_images/brain.png",
                       std::ios::in | std::ios::binary | std::ios::ate);

    image.resize(file.tellg());

    file.seekg(0, std::ios::beg);
    if (!file.read(&image[0], image.size()))
      std::cout << "error" << std::endl;

    proto_query.add_blobs(image);

    VDMS::protobufs::queryMessage response;
    query_handler.process_query(proto_query, response);

    Json::Reader json_reader;
    Json::Value json_response;

    json_reader.parse(response.json(), json_response);

    EXPECT_EQ(json_response[0]["status"].asString(), "-1");
    EXPECT_EQ(json_response[0]["info"].asString(), "Internal Server Error: VCL Exception at QH\n");

  } catch (...) {
    printErrorMessage("ImageAddCropFailure");
  }
}

TEST_F(RemoteConnectionTest, ImageTransactionRollback) {
  try {
    int s3_num_objects;
    const char *s3_num_objects_cmd = "mc ls --recursive myminio/minio-bucket | wc -l";
    std::array<char, 8> buffer;
    std::string result;

    std::string string_query_simple_add_image("[ \
       { \
          \"AddImage\": { \
              \"properties\": { \
                  \"name\": \"SampleImage\" \
              }, \
              \"format\": \"png\" \
          } \
      } \
    ]");

    std::string string_query_image_rollback("[");
    string_query_image_rollback += " \
      { \
          \"AddImage\": { \
              \"properties\": { \
                  \"name\": \"ImageTransactionRollback_1\" \
              }, \
              \"format\": \"png\" \
          } \
      }, \
      { \
          \"AddImage\": { \
              \"operations\": [{ \
                  \"type\": \"crop\", \
                  \"x\": 250, \
                  \"y\": 250, \
                  \"width\": 100, \
                  \"height\": 100  \
              }], \
              \"properties\": { \
                  \"name\": \"ImageTransactionRollback_2\" \
              }, \
              \"format\": \"png\" \
          } \
      } \
    ";
    string_query_image_rollback += "]";

    VDMS::Server VDMS_server("unit_tests/config-aws-tests.json", "", "", "");

    QueryHandlerPMGD query_handler;
    query_handler.reset_autodelete_init_flag(); // set flag to show autodelete queue has
                                                // been initialized

    VDMS::protobufs::queryMessage proto_query;
    proto_query.set_json(string_query_simple_add_image);

    std::string image;
    std::ifstream file("test_images/brain.png",
                       std::ios::in | std::ios::binary | std::ios::ate);

    image.resize(file.tellg());

    file.seekg(0, std::ios::beg);
    if (!file.read(&image[0], image.size()))
      std::cout << "error" << std::endl;

    proto_query.add_blobs(image);

    VDMS::protobufs::queryMessage response;
    query_handler.process_query(proto_query, response);

    // Get initial number of objects stored in S3
    std::unique_ptr<FILE, decltype(&pclose)> pipe1(popen(s3_num_objects_cmd, "r"), pclose);
    if (!pipe1) {
      throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe1.get()) != nullptr) {
      result += buffer.data();
    }
    s3_num_objects = stoi(result);
    result.clear();

    proto_query.clear_blobs();
    proto_query.set_json(string_query_image_rollback);
    proto_query.add_blobs(image);
    proto_query.add_blobs(image);
    query_handler.process_query(proto_query, response);

    Json::Reader json_reader;
    Json::Value json_response;

    json_reader.parse(response.json(), json_response);

    EXPECT_EQ(json_response[0]["status"].asString(), "-1");
    EXPECT_EQ(json_response[0]["info"].asString(), "Internal Server Error: VCL Exception at QH\n");

    std::string string_query_image_lookup("[");
    string_query_image_lookup += " \
          { \
              \"FindImage\": { \
                  \"results\": { \
                      \"list\": [\"name\"] \
                  }, \
                  \"constraints\": { \
                      \"name\": [ \"==\", \"ImageTransactionFailureRollback_1\" ] \
                  } \
              } \
          }, \
          { \
              \"FindImage\": { \
                  \"results\": { \
                      \"list\": [\"name\"] \
                  }, \
                  \"constraints\": { \
                      \"name\": [ \"==\", \"ImageTransactionFailureRollback_2\" ] \
                  } \
              } \
          } \
      ";
    string_query_image_lookup += "]";

    proto_query.clear_blobs();
    proto_query.set_json(string_query_image_lookup);

    query_handler.process_query(proto_query, response);
    json_reader.parse(response.json(), json_response);

    EXPECT_EQ(json_response[0]["FindImage"]["status"].asString(), "0");
    EXPECT_EQ(json_response[0]["FindImage"]["info"], "No entities found");
    EXPECT_EQ(json_response[1]["FindImage"]["status"].asString(), "0");
    EXPECT_EQ(json_response[1]["FindImage"]["info"], "No entities found");

    // Make sure number of objects in S3 is still the same
    std::unique_ptr<FILE, decltype(&pclose)> pipe2(popen(s3_num_objects_cmd, "r"), pclose);
    if (!pipe2) {
      throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe2.get()) != nullptr) {
      result += buffer.data();
    }

    EXPECT_EQ(s3_num_objects, stoi(result));

  } catch (...) {
    printErrorMessage("ImageTransactionRollback");
  }
}
