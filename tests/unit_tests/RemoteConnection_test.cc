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

#include "Image.h"
#include "TDBImage.h"
#include "gtest/gtest.h"

#include "RemoteConnection.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <string>

class RemoteConnectionTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    img_ = "test_images/large1.jpg";
    tdb_img_ = "tdb/test_image.tdb";
    video_ = "test_videos/Megamind.avi";
    cv_img_ = cv::imread(img_, cv::IMREAD_ANYCOLOR);
    rect_ = VCL::Rectangle(100, 100, 100, 100);

    connection_ = new VCL::RemoteConnection();
    connection_->_bucket_name = "minio-bucket";
    connection_->start();
  }

  virtual void TearDown() {
    connection_->end();
    delete connection_;
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

// Basic Remote Connection Tests

TEST_F(RemoteConnectionTest, RemoteWriteFilename) { connection_->Write(img_); }

TEST_F(RemoteConnectionTest, RemoteReadWriteBuffer) {
  std::vector<unsigned char> img_data = connection_->Read(img_);
  connection_->Write(img_, img_data);
}

TEST_F(RemoteConnectionTest, RemoteListRetrieveFile) {
  std::vector<std::string> file_list =
      connection_->ListFilesInFolder("test_images");
  connection_->RetrieveFile(file_list[0]);
}

TEST_F(RemoteConnectionTest, RemoteWriteVideoFilename) {
  connection_->Write(video_);
}

TEST_F(RemoteConnectionTest, RemoteReadVideoFilename) {
  connection_->Read_Video(video_);
}

//#### Regular Image tests ####

TEST_F(RemoteConnectionTest, ImageRemoteWritePNG) {
  VCL::ImageTest img(cv_img_);

  img.set_connection(connection_);
  std::string path = "pngs/test_image.png";

  img.store(path, VCL::Image::Format::PNG);
  img.perform_operations();
}

TEST_F(RemoteConnectionTest, ImageRemoteReadPNG) {
  VCL::ImageTest img;

  img.set_connection(connection_);
  std::string path = "pngs/test_image.png";

  img.read(path);

  cv::Mat data = img.get_cvmat();
  compare_mat_mat(data, cv_img_);
}

TEST_F(RemoteConnectionTest, ImageRemoteRemovePNG) {
  VCL::Image img("pngs/test_image.png");
  img.set_connection(connection_);
  img.delete_image();
}

TEST_F(RemoteConnectionTest, ImageRemoteWriteJPG) {
  VCL::Image img(cv_img_);

  img.set_connection(connection_);
  std::string path = "jpgs/large1.jpg";

  img.store(path, VCL::Image::Format::JPG);
}

TEST_F(RemoteConnectionTest, ImageRemoteReadJPG) {
  VCL::Image img("jpgs/large1.jpg");
  img.set_connection(connection_);

  cv::Mat mat = img.get_cvmat();
  compare_mat_mat_jpg(mat, cv_img_);
}

TEST_F(RemoteConnectionTest, ImageRemoteRemoveJPG) {
  VCL::Image img("jpgs/large1.jpg");
  img.set_connection(connection_);
  img.delete_image();
}

//#### TileDB Image tests ####
TEST_F(RemoteConnectionTest, TDBImageWriteS3) {
  VCL::TDBImage tdb("tdb/test_image.tdb", *connection_);
  tdb.write(cv_img_);
}

// Basic Remote Connection Tests (no remote connected, expected failures)

TEST_F(RemoteConnectionTest, RemoteDisconnectedWriteFilename) {
  VCL::RemoteConnection not_a_connection;
  not_a_connection.Write(img_);
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedReadBuffer) {
  VCL::RemoteConnection not_a_connection;
  std::vector<unsigned char> img_data = not_a_connection.Read(img_);
  connection_->Write(img_, img_data);
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedWriteBuffer) {
  VCL::RemoteConnection not_a_connection;
  std::vector<unsigned char> img_data = connection_->Read(img_);
  not_a_connection.Write(img_, img_data);
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedListFiles) {
  VCL::RemoteConnection not_a_connection;
  std::vector<std::string> file_list =
      not_a_connection.ListFilesInFolder("test_images");
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedRetrieveFile) {
  VCL::RemoteConnection not_a_connection;
  std::vector<std::string> file_list =
      connection_->ListFilesInFolder("test_images");
  not_a_connection.RetrieveFile(file_list[0]);
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedWriteVideoFilename) {
  VCL::RemoteConnection not_a_connection;
  not_a_connection.Write(video_);
}

TEST_F(RemoteConnectionTest, RemoteDisconnectedReadVideoFilename) {
  VCL::RemoteConnection not_a_connection;
  not_a_connection.Read_Video(video_);
}
