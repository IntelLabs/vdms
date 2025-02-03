/**
 * @file   Image_test.cc
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

#include "ImageLoop.h"
#include "VDMSConfig.h"
#include "stats/SystemStats.h"
#include "vcl/Image.h"
#include "gtest/gtest.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "VDMSConfig.h"
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <string>

namespace fs = std::filesystem;

class ImageTest : public ::testing::Test {
protected:
  virtual void SetUp() {
    VDMS::VDMSConfig::init("unit_tests/config-tests.json");
    img_ = "test_images/large1.jpg";
    tdb_img_ = "tdb/test_image.tdb";
    cv_img_ = cv::imread(img_, -1);

    size_ = cv_img_.rows * cv_img_.cols * cv_img_.channels();
    rect_ = VCL::Rectangle(100, 100, 100, 100);
    bad_rect_ = VCL::Rectangle(1000, 1000, 10000, 10000);
    dimension_ = 256;
  }

  void compare_mat_buffer(cv::Mat &img, unsigned char *buffer) {
    int index = 0;

    int rows = img.rows;
    int columns = img.cols;
    int channels = img.channels();
    if (channels > 3) {
      throw VCLException(OpenFailed, "Greater than 3 channels in image");
    }

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

  void compare_mat_mat(cv::Mat &cv_img, cv::Mat &img) {
    int rows = img.rows;
    int columns = img.cols;
    int channels = img.channels();
    if (channels > 3) {
      throw VCLException(OpenFailed, "Greater than 3 channels in image");
    }

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

  VCL::Rectangle rect_;
  VCL::Rectangle bad_rect_;
  std::string img_;
  std::string tdb_img_;

  cv::Mat cv_img_;

  int dimension_;
  int size_;
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

TEST_F(ImageTest, DefaultConstructor) {
  VCL::ImageTest img_data;

  cv::Size dims = img_data.get_dimensions();

  EXPECT_EQ(0, dims.height);
  EXPECT_EQ(0, dims.width);
}

// When setting from a filename, we set the type, number of channels, path, and
// format of the image, We also add a read operation to the list of operations
TEST_F(ImageTest, StringConstructor) {
  VCL::Image img(img_);

  EXPECT_EQ(VCL::Format::JPG, img.get_image_format());
  EXPECT_EQ(img_, img.get_image_id());
}

TEST_F(ImageTest, StringConstructorIMG) {
  VCL::Image img_data(img_);

  cv::Size dims = img_data.get_dimensions();
  EXPECT_EQ(cv_img_.rows, dims.height);
  EXPECT_EQ(cv_img_.cols, dims.width);

  EXPECT_EQ(img_data.get_image_format(), VCL::Format::JPG);
}

TEST_F(ImageTest, StringConstructorTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img_data(tdb_img_);
  cv::Size dims = img_data.get_dimensions();

  EXPECT_EQ(cv_img_.rows, dims.height);
  EXPECT_EQ(cv_img_.cols, dims.width);

  EXPECT_EQ(img_data.get_image_format(), VCL::Format::TDB);
}

// When setting from a cv::mat, we set the type of the image and copy the image
// data We should know the height, width, number of channels, type, and have a
// non-empty Mat
TEST_F(ImageTest, MatConstructor) {
  VCL::Image img(cv_img_);

  ASSERT_FALSE(img.get_cvmat().empty());
  EXPECT_EQ(cv_img_.type(), img.get_image_type());

  cv::Size dims = img.get_dimensions();

  EXPECT_EQ(cv_img_.rows, dims.height);
  EXPECT_EQ(cv_img_.cols, dims.width);

  cv::Mat cv_img = img.get_cvmat();

  compare_mat_mat(cv_img, cv_img_);
}

TEST_F(ImageTest, EncodedBufferConstructor) {
  std::fstream jpgimage("test_images/large1.jpg");

  jpgimage.seekg(0, jpgimage.end);
  int length = jpgimage.tellg();
  jpgimage.seekg(0, jpgimage.beg);

  char *buffer = new char[length];
  jpgimage.read(buffer, length);
  jpgimage.close();

  int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();

  VCL::Image img(buffer, size);

  ASSERT_FALSE(img.get_cvmat().empty());
  cv::Mat raw = img.get_cvmat();

  compare_mat_mat(cv_img_, raw);
}

TEST_F(ImageTest, BufferConstructor) {
  unsigned char *buffer = cv_img_.data;

  int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();

  VCL::Image img_data(buffer, cv::Size(cv_img_.cols, cv_img_.rows),
                      cv_img_.type());

  cv::Size dims = img_data.get_dimensions();

  EXPECT_EQ(cv_img_.rows, dims.height);
  EXPECT_EQ(cv_img_.cols, dims.width);
  EXPECT_EQ(cv_img_.type(), img_data.get_image_type());

  unsigned char *buf = new unsigned char[size];

  img_data.get_raw_data(buf, size);

  compare_mat_buffer(cv_img_, buf);
}

TEST_F(ImageTest, RawBufferConstructor) {
  void *buffer = cv_img_.data;

  VCL::Image img(buffer, cv::Size(cv_img_.cols, cv_img_.rows), cv_img_.type());

  cv::Mat raw = img.get_cvmat();

  compare_mat_mat(cv_img_, raw);
}

TEST_F(ImageTest, CopyConstructor) {
  VCL::Image img(cv_img_);

  EXPECT_EQ(cv_img_.type(), img.get_image_type());

  VCL::Image test_img(img);

  EXPECT_EQ(cv_img_.type(), test_img.get_image_type());

  cv::Mat test_cv = test_img.get_cvmat();
  ASSERT_FALSE(test_cv.empty());

  compare_mat_mat(test_cv, cv_img_);
}

TEST_F(ImageTest, CopyConstructorMat) {
  VCL::Image img_data(cv_img_);

  VCL::Image img_copy(img_data);

  cv::Mat cv_img = img_data.get_cvmat();
  cv::Mat cv_copy = img_copy.get_cvmat();

  compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageTest, CopyConstructorTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img_data(tdb_img_);

  VCL::Image img_copy(img_data);

  cv::Mat cv_img = img_data.get_cvmat();
  cv::Mat cv_copy = img_copy.get_cvmat();

  compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageTest, CopyConstructorComplex) {
  VCL::Image img(cv_img_);

  EXPECT_EQ(cv_img_.type(), img.get_image_type());

  img.crop(rect_);

  VCL::Image test_img(img);

  EXPECT_EQ(cv_img_.type(), test_img.get_image_type());
  cv::Mat test_cv = test_img.get_cvmat();

  cv::Mat cropped_cv(cv_img_, rect_);
  compare_mat_mat(test_cv, cropped_cv);

  cv::Size dims = test_img.get_dimensions();
  EXPECT_EQ(rect_.height, dims.height);
}

TEST_F(ImageTest, OperatorEqualsMat) {
  VCL::ImageTest img_data(cv_img_);

  VCL::ImageTest img_copy;

  img_copy = img_data;

  cv::Mat cv_img = img_data.get_cvmat();
  cv::Mat cv_copy = img_copy.get_cvmat();

  compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageTest, OperatorEqualsTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::ImageTest img_data(tdb_img_);

  VCL::ImageTest img_copy;

  img_copy = img_data;

  cv::Mat cv_img = img_data.get_cvmat();
  cv::Mat cv_copy = img_copy.get_cvmat();

  compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageTest, GetMatFromMat) {
  VCL::Image img(cv_img_);

  cv::Mat cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());

  compare_mat_mat(cv_img, cv_img_);
}

TEST_F(ImageTest, GetMatFromPNG) {
  VCL::Image img(img_);

  cv::Mat cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());

  compare_mat_mat(cv_img, cv_img_);
}

TEST_F(ImageTest, GetMatFromTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);

  EXPECT_EQ(tdb_img_, img.get_image_id());
  EXPECT_EQ(VCL::Format::TDB, img.get_image_format());

  cv::Mat cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());

  compare_mat_mat(cv_img, cv_img_);
}

TEST_F(ImageTest, GetBufferFromMat) {
  VCL::Image img(cv_img_);

  unsigned char *buffer = new unsigned char[img.get_raw_data_size()];

  img.get_raw_data(buffer, img.get_raw_data_size());

  EXPECT_TRUE(buffer != NULL);
  compare_mat_buffer(cv_img_, buffer);

  delete[] buffer;
}

TEST_F(ImageTest, GetBufferFromPNG) {
  VCL::Image img(img_);

  unsigned char *buffer = new unsigned char[img.get_raw_data_size()];

  img.get_raw_data(buffer, img.get_raw_data_size());

  EXPECT_TRUE(buffer != NULL);
  compare_mat_buffer(cv_img_, buffer);

  delete[] buffer;
}

TEST_F(ImageTest, GetBufferFromTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);

  int size = img.get_raw_data_size();
  unsigned char *buffer = new unsigned char[size];

  img.get_raw_data(buffer, size);

  EXPECT_TRUE(buffer != NULL);
  compare_mat_buffer(cv_img_, (unsigned char *)buffer);

  delete[] buffer;
}

TEST_F(ImageTest, GetArea) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img_data(tdb_img_);

  VCL::Image new_data = img_data.get_area(rect_);

  cv::Size dims = new_data.get_dimensions();

  EXPECT_EQ(rect_.height, dims.height);
  EXPECT_EQ(rect_.width, dims.width);
}

TEST_F(ImageTest, GetBuffer) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img_data(tdb_img_);

  int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();
  unsigned char *buf = new unsigned char[size];

  img_data.get_raw_data(buf, size);

  compare_mat_buffer(cv_img_, buf);
}

TEST_F(ImageTest, GetCVMat) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img_data(tdb_img_);

  cv::Mat cv_img = img_data.get_cvmat();

  compare_mat_mat(cv_img_, cv_img);
}

TEST_F(ImageTest, GetRectangleFromPNG) {
  VCL::Image img(img_);

  VCL::Image corner = img.get_area(rect_);

  cv::Size dims = corner.get_dimensions();

  EXPECT_EQ(rect_.height, dims.height);
  EXPECT_EQ(rect_.width, dims.width);
}

TEST_F(ImageTest, GetRectangleFromTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);
  try {
    VCL::Image corner = img.get_area(rect_);
    cv::Size dims = corner.get_dimensions();

    EXPECT_EQ(rect_.height, dims.height);
    EXPECT_EQ(rect_.width, dims.width);
  } catch (VCL::Exception &e) {
    print_exception(e);
  }
}

TEST_F(ImageTest, GetRectangleFromMat) {
  VCL::Image img(cv_img_);

  VCL::Image corner = img.get_area(rect_);

  cv::Size dims = corner.get_dimensions();

  EXPECT_EQ(rect_.height, dims.height);
  EXPECT_EQ(rect_.width, dims.width);
}

TEST_F(ImageTest, SetDataFromRaw) {
  VCL::ImageTest img_data;

  void *buffer = cv_img_.data;
  int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();

  img_data.set_data_from_raw(buffer, size);

  cv::Mat raw = img_data.get_cvmat();

  compare_mat_mat(cv_img_, raw);
}

TEST_F(ImageTest, SetDataFromEncoded) {
  VCL::ImageTest img_data;

  std::vector<unsigned char> buffer;
  cv::imencode(".png", cv_img_, buffer);

  img_data.set_data_from_encoded(static_cast<void *>(&buffer[0]),
                                 buffer.size());

  cv::Mat raw = img_data.get_cvmat();

  compare_mat_mat(raw, cv_img_);
}

TEST_F(ImageTest, Read) {
  VCL::ImageTest img_data;
  img_data.set_format("jpg");

  ASSERT_THROW(img_data.read("test_images/.jpg"), VCL::Exception);

  img_data.read("test_images/large1");

  EXPECT_EQ("test_images/large1.jpg", img_data.get_image_id());
}

TEST_F(ImageTest, WriteMatToJPG) {
  VCL::Image img(cv_img_);
  img.store("test_images/test_image", VCL::Format::JPG);

  cv::Mat test = cv::imread("test_images/test_image.jpg");

  EXPECT_FALSE(test.empty());
}

TEST_F(ImageTest, WriteMatToTDB) {
  VCL::Image img(cv_img_);
  img.store("tdb/mat_to_tdb", VCL::Format::TDB);
}

TEST_F(ImageTest, WriteStringToTDB) {
  VCL::Image img(img_);
  img.store("tdb/png_to_tdb.png", VCL::Format::TDB);
}

TEST_F(ImageTest, ResizeMat) {
  VCL::Image img(img_);
  img.resize(dimension_, dimension_);

  cv::Mat cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());
  EXPECT_EQ(dimension_, cv_img.rows);
}

TEST_F(ImageTest, ResizeTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);
  img.resize(dimension_, dimension_);

  cv::Mat cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());
  EXPECT_EQ(dimension_, cv_img.rows);
}

TEST_F(ImageTest, CropMatThrow) {
  VCL::Image img(img_);
  img.crop(bad_rect_);
  img.get_cvmat();
  ASSERT_STREQ(img.get_query_error_response().data(),
               "Requested area is not within the image");
}

TEST_F(ImageTest, CropMat) {
  VCL::Image img(img_);
  img.crop(rect_);

  cv::Mat cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());

  cv::Size dims = img.get_dimensions();

  EXPECT_EQ(rect_.height, dims.height);
  EXPECT_EQ(rect_.width, dims.width);

  cv::Mat mat(cv_img_, rect_);
  compare_mat_mat(cv_img, mat);
}

TEST_F(ImageTest, Threshold) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::ImageTest img_data(tdb_img_);

  img_data.read(tdb_img_);

  img_data.threshold(200);

  img_data.perform_operations();

  cv::Mat cv_bright = img_data.get_cvmat();

  cv::threshold(cv_img_, cv_img_, 200, 200, cv::THRESH_TOZERO);

  compare_mat_mat(cv_bright, cv_img_);
}

TEST_F(ImageTest, DeleteTDB) {
  // Setup
  VCL::TDBImage tdb("tdb/no_metadata.tdb");
  tdb.write(cv_img_, false);
  VCL::ImageTest img_data("tdb/no_metadata.tdb");

  EXPECT_TRUE(img_data.delete_image());

  img_data.read("tdb/no_metadata.tdb");

  // Verify the results
  ASSERT_THROW(img_data.perform_operations(), VCL::Exception);
}

TEST_F(ImageTest, SetMinimum) {
  VCL::Image img_data(cv_img_);

  img_data.set_minimum_dimension(3);
}

TEST_F(ImageTest, FlipVertical) {
  VCL::Image img(img_);
  cv::Mat cv_img = img.get_cvmat();
  cv::Mat cv_img_flipped = cv::Mat(cv_img.rows, cv_img.cols, cv_img.type());
  cv::flip(cv_img, cv_img_flipped, 0);

  img.flip(0);
  cv::Mat vcl_img_flipped = img.get_cvmat();

  EXPECT_FALSE(vcl_img_flipped.empty());
  compare_mat_mat(vcl_img_flipped, cv_img_flipped);
}

TEST_F(ImageTest, FlipHorizontal) {
  VCL::Image img(img_);
  cv::Mat cv_img = img.get_cvmat();
  cv::Mat cv_img_flipped = cv::Mat(cv_img.rows, cv_img.cols, cv_img.type());
  cv::flip(cv_img, cv_img_flipped, 1);

  img.flip(1);
  cv::Mat vcl_img_flipped = img.get_cvmat();

  EXPECT_FALSE(vcl_img_flipped.empty());
  compare_mat_mat(vcl_img_flipped, cv_img_flipped);
}

TEST_F(ImageTest, FlipBoth) {
  VCL::Image img(img_);
  cv::Mat cv_img = img.get_cvmat();
  cv::Mat cv_img_flipped = cv::Mat(cv_img.rows, cv_img.cols, cv_img.type());
  cv::flip(cv_img, cv_img_flipped, -1);

  img.flip(-1);
  cv::Mat vcl_img_flipped = img.get_cvmat();

  EXPECT_FALSE(vcl_img_flipped.empty());
  compare_mat_mat(vcl_img_flipped, cv_img_flipped);
}

TEST_F(ImageTest, Rotate) {
  float angle = 30;
  VCL::Image img(img_);
  cv::Mat cv_img = img.get_cvmat();
  cv::Mat cv_img_rot = cv::Mat(cv_img.rows, cv_img.cols, cv_img.type());

  cv::Point2f pc(cv_img.cols / 2., cv_img.rows / 2.);
  cv::Mat r = cv::getRotationMatrix2D(pc, angle, 1.0);
  cv::warpAffine(cv_img, cv_img_rot, r, cv_img.size());

  img.rotate(angle, true);
  cv::Mat vcl_img_rot = img.get_cvmat();

  EXPECT_FALSE(vcl_img_rot.empty());
  compare_mat_mat(vcl_img_rot, cv_img_rot);
}

TEST_F(ImageTest, RotateResize) {
  float angle = 30;
  VCL::Image img(img_);
  cv::Mat cv_img = img.get_cvmat();

  cv::Point2f im_c((cv_img.cols - 1) / 2.0, (cv_img.rows - 1) / 2.0);
  cv::Mat r = cv::getRotationMatrix2D(im_c, angle, 1.0);

  cv::Rect2f bbox =
      cv::RotatedRect(cv::Point2f(), cv_img.size(), angle).boundingRect2f();
  // Transformation Matrix
  r.at<double>(0, 2) += bbox.width / 2.0 - cv_img.cols / 2.0;
  r.at<double>(1, 2) += bbox.height / 2.0 - cv_img.rows / 2.0;

  cv::Mat cv_img_rot;
  cv::warpAffine(cv_img, cv_img_rot, r, bbox.size());

  img.rotate(angle, false);
  cv::Mat vcl_img_rot = img.get_cvmat();

  EXPECT_FALSE(vcl_img_rot.empty());
  compare_mat_mat(vcl_img_rot, cv_img_rot);
}

TEST_F(ImageTest, TDBMatThrow) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);
  img.crop(bad_rect_);
  img.get_cvmat();
  ASSERT_STREQ(img.get_query_error_response().data(),
               "Requested area is not within the image");
}

TEST_F(ImageTest, CropTDB) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  cv::Mat cv_img;

  VCL::Image img(tdb_img_);

  img.crop(rect_);

  cv_img = img.get_cvmat();

  EXPECT_FALSE(cv_img.empty());
  EXPECT_EQ(rect_.height, cv_img.rows);
}

TEST_F(ImageTest, CompareMatAndBuffer) {
  VCL::Image img(img_);

  unsigned char *data_buffer = new unsigned char[img.get_raw_data_size()];
  img.get_raw_data(data_buffer, img.get_raw_data_size());

  compare_mat_buffer(cv_img_, data_buffer);
}

TEST_F(ImageTest, TDBToPNG) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);

  img.store("test_images/tdb_to_png", VCL::Format::PNG);
}

TEST_F(ImageTest, TDBToJPG) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);

  img.store("test_images/tdb_to_jpg", VCL::Format::JPG);
}

TEST_F(ImageTest, EncodedImage) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_, false);

  VCL::Image img(tdb_img_);

  std::vector<unsigned char> buffer = img.get_encoded_image(VCL::Format::PNG);

  cv::Mat mat = cv::imdecode(buffer, cv::IMREAD_ANYCOLOR);
  compare_mat_mat(cv_img_, mat);
}

TEST_F(ImageTest, CreateNamePNG) {
  VCL::ImageTest img_data(cv_img_);

  auto unique_name = VCL::create_unique("image_results/", "png");

  img_data.store(unique_name, VCL::Format::PNG);
  img_data.perform_operations();
}

// TODO Review this
TEST_F(ImageTest, CreateNameTDB) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing for "
               << "non remote tests";
  VCL::Image img(cv_img_);

  for (int i = 0; i < 10; ++i) {
    std::string name = VCL::create_unique("tdb/", "tdb");
    img.store(name, VCL::Format::TDB);
  }
}

// TODO Review this
TEST_F(ImageTest, NoMetadata) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing for "
               << "non remote tests";
  VCL::Image img(cv_img_);

  std::string name = VCL::create_unique("tdb/", "tdb");
  img.store(name, VCL::Format::TDB, false);

  cv::Size dims = img.get_dimensions();
  int cv_type = img.get_image_type();

  VCL::Image tdbimg(name);

  tdbimg.set_image_type(cv_type);
  tdbimg.set_dimensions(dims);

  cv::Mat mat = tdbimg.get_cvmat();
}

// TODO This test is failing in the comparison of jpg files
TEST_F(ImageTest, SyncRemote) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing during the "
               << "comparison of images for non remote tests";
  VCL::Image img(img_);

  std::string inputFile = "remote_function_test/syncremote.jpg";
  ASSERT_TRUE(fs::exists(fs::path(inputFile)));
  cv::Mat cv_img_flipped = cv::imread(inputFile);

  std::string _url = "http://localhost:5010/image";
  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "flip";
  img.syncremoteOperation(_url, _options);
  cv::Mat vcl_img_flipped = img.get_cvmat();

  EXPECT_FALSE(vcl_img_flipped.empty());
  ;
  compare_mat_mat(vcl_img_flipped, cv_img_flipped);
}

// This test is failing in the comparison of images
TEST_F(ImageTest, UDF) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing during the "
               << "comparison of images for non remote tests";
  SystemStats systemStats;
  VCL::Image img(img_);
  std::string inputFile = "udf_test/syncremote.jpg";
  ASSERT_TRUE(fs::exists(fs::path(inputFile)));
  cv::Mat cv_img_flipped = cv::imread(inputFile);

  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "flip";
  _options["port"] = 5555;
  img.userOperation(_options);
  cv::Mat vcl_img_flipped = img.get_cvmat();

  systemStats.log_stats("TestUDF");

  FILE *f = fopen(systemStats.m_logFileName.data(), "r");
  ASSERT_TRUE(f != NULL);

  if (f) {
    fclose(f);
  }

  EXPECT_FALSE(vcl_img_flipped.empty());
  compare_mat_mat(vcl_img_flipped, cv_img_flipped);
}

TEST_F(ImageTest, ImageLoop) {
  VCL::Image img(img_);
  ImageLoop imageLoop;

  std::string _url = "http://localhost:5010/image";
  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "flip";

  img.flip(0);
  img.remoteOperation(_url, _options);

  imageLoop.set_nrof_entities(1);

  imageLoop.enqueue(&img);
  while (imageLoop.is_loop_running()) {
    continue;
  }
  std::map<std::string, VCL::Image *> imageMap = imageLoop.get_image_map();
  std::map<std::string, VCL::Image *>::iterator iter = imageMap.begin();

  ASSERT_TRUE(iter->second->get_query_error_response() == "");

  while (iter != imageMap.end()) {
    std::vector<unsigned char> img_enc =
        iter->second->get_encoded_image_async(img.get_image_format());
    ASSERT_TRUE(!img_enc.empty());
    iter++;
  }
}

TEST_F(ImageTest, ImageLoopURLError) {
  VCL::Image img(img_);
  ImageLoop imageLoop;

  std::string _url = "http://localhost:5010/imag";
  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "flip";

  img.flip(0);
  img.remoteOperation(_url, _options);

  imageLoop.set_nrof_entities(1);

  imageLoop.enqueue(&img);

  while (imageLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Image *> imageMap = imageLoop.get_image_map();
  std::map<std::string, VCL::Image *>::iterator iter = imageMap.begin();

  ASSERT_TRUE(iter->second->get_query_error_response() != "");
}

TEST_F(ImageTest, ImageLoopRemoteFunctionError) {
  VCL::Image img(img_);
  ImageLoop imageLoop;

  std::string _url = "http://localhost:5010/image";
  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "gray";

  img.flip(0);
  img.remoteOperation(_url, _options);

  imageLoop.set_nrof_entities(1);

  imageLoop.enqueue(&img);

  while (imageLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Image *> imageMap = imageLoop.get_image_map();
  std::map<std::string, VCL::Image *>::iterator iter = imageMap.begin();

  ASSERT_TRUE(iter->second->get_query_error_response() != "");
}

TEST_F(ImageTest, ImageLoopSyncRemoteFunctionError) {
  VCL::Image img(img_);
  ImageLoop imageLoop;

  std::string _url = "http://localhost:5010/imag";
  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "gray";

  img.flip(0);
  img.syncremoteOperation(_url, _options);

  imageLoop.set_nrof_entities(1);

  imageLoop.enqueue(&img);

  while (imageLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Image *> imageMap = imageLoop.get_image_map();
  std::map<std::string, VCL::Image *>::iterator iter = imageMap.begin();

  ASSERT_TRUE(iter->second->get_query_error_response() != "");
}

TEST_F(ImageTest, PipelineException) {
  VCL::Image img(img_);

  img.threshold(100);
  img.flip(0);
  img.resize(50, 80);
  img.crop(bad_rect_);

  img.get_cvmat();
  ASSERT_STREQ(img.get_query_error_response().data(),
               "Requested area is not within the image");
}

TEST_F(ImageTest, AddImageByPath) {
  VCL::Image img;
  img = VCL::Image(img_, true);

  EXPECT_EQ(VCL::Format::JPG, img.get_image_format());
  EXPECT_EQ(img_, img.get_image_id());
}

TEST_F(ImageTest, ImagePathError) {
  VCL::Image img;
  std::string temp_image_path(VDMS::VDMSConfig::instance()->get_path_tmp() +
                              "/pathimagepatherror.jpg");
  std::filesystem::copy_file(img_, temp_image_path);
  img = VCL::Image(temp_image_path, true);

  EXPECT_TRUE(std::remove(temp_image_path.data()) == 0);

  VCL::Image read_img(temp_image_path);
  ASSERT_THROW(read_img.get_encoded_image_async(read_img.get_image_format()),
               VCL::Exception);
}

TEST_F(ImageTest, UDFMetadata) {
  std::string inputFile = "udf_test/metadata_image.jpg";
  ASSERT_TRUE(fs::exists(fs::path(inputFile)));
  std::string temp_image_path(VDMS::VDMSConfig::instance()->get_path_tmp() +
                              "/pathimageudfmetadata.jpg");
  std::filesystem::copy_file(inputFile, temp_image_path);
  VCL::Image img = VCL::Image(temp_image_path, true);

  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "metadata";
  _options["media_type"] = "image";
  _options["otype"] = "face";
  _options["port"] = 5555;

  img.userOperation(_options);
  cv::Mat vcl_img = img.get_cvmat();
  for (auto metadata : img.get_ingest_metadata()) {
    ASSERT_STREQ(metadata["object"].asString().data(), "face");
  }

  EXPECT_TRUE(std::remove(temp_image_path.data()) == 0);
}

TEST_F(ImageTest, RemoteMetadata) {
  std::string inputFile = "remote_function_test/metadata_image.jpg";
  ASSERT_TRUE(fs::exists(fs::path(inputFile)));
  std::string temp_image_path(VDMS::VDMSConfig::instance()->get_path_tmp() +
                              "/rpathimage.jpg");
  std::filesystem::copy_file(inputFile, temp_image_path);
  VCL::Image img = VCL::Image(temp_image_path, true);

  std::string _url = "http://localhost:5010/image";
  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "metadata";
  _options["media_type"] = "image";
  _options["otype"] = "face";
  _options["ingestion"] = 1;

  img.syncremoteOperation(_url, _options);
  cv::Mat vcl_img = img.get_cvmat();
  ASSERT_TRUE(img.get_ingest_metadata().size() > 0);
  for (auto metadata : img.get_ingest_metadata()) {
    ASSERT_STREQ(metadata["object"].asString().data(), "face");
  }

  EXPECT_TRUE(std::remove(temp_image_path.data()) == 0);
}

TEST_F(ImageTest, UDFNoMetadata) {
  std::string inputFile = "udf_test/metadata_image.jpg";
  ASSERT_TRUE(fs::exists(fs::path(inputFile)));
  std::string temp_image_path(VDMS::VDMSConfig::instance()->get_path_tmp() +
                              "/pathimagenometadata.jpg");
  std::filesystem::copy_file(inputFile, temp_image_path);
  VCL::Image img = VCL::Image(temp_image_path, true);

  Json::Value _options;
  _options["format"] = "jpg";
  _options["id"] = "flip";
  _options["port"] = 5555;

  img.userOperation(_options);
  cv::Mat vcl_img = img.get_cvmat();
  ASSERT_EQ(img.get_ingest_metadata().size(), 0);

  EXPECT_TRUE(std::remove(temp_image_path.data()) == 0);
}
