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

const std::string TMP_DIRNAME = "tests_output_dir/";

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
