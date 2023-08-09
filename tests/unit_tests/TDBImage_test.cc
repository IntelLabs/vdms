/**
 * @file   TDBImage_test.cc
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

#include "TDBImage.h"
#include "TDBObject.h"
#include "gtest/gtest.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

class TDBImageTest : public ::testing::Test {

protected:
  virtual void SetUp() {
    tdb_img_ = "tdb/test_image.tdb";
    tdb_test_ = "tdb/write_test.tdb";
    cv_img_ = cv::imread("test_images/large1.jpg", cv::IMREAD_ANYCOLOR);
    rect_ = VCL::Rectangle(100, 100, 100, 100);
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

  void compare_buffer_buffer(unsigned char *buffer1, unsigned char *buffer2,
                             int length) {
    for (int i = 0; i < length; ++i) {
      ASSERT_EQ(buffer1[i], buffer2[i]);
    }
  }

  std::string tdb_img_;
  std::string tdb_test_;
  cv::Mat cv_img_;
  VCL::Rectangle rect_;
};

TEST_F(TDBImageTest, DefaultConstructor) {
  VCL::TDBImage tdb;

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);
}

TEST_F(TDBImageTest, StringConstructor) {
  VCL::TDBImage tdb(tdb_img_);

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);

  EXPECT_EQ("tdb/test_image.tdb", tdb.get_object_id());
}

TEST_F(TDBImageTest, BufferConstructor) {
  unsigned char *buffer = cv_img_.data;

  long size = long(cv_img_.rows) * long(cv_img_.cols) * cv_img_.channels();

  VCL::TDBImage tdb(buffer, size);

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);

  unsigned char *buf = new unsigned char[size];
  tdb.get_buffer(buf, size);

  compare_mat_buffer(cv_img_, buf);

  delete[] buf;
}

TEST_F(TDBImageTest, CopyConstructorNoData) {
  VCL::TDBImage tdb("tdb/copy_construct.tdb");

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);

  EXPECT_EQ("tdb/copy_construct.tdb", tdb.get_object_id());

  VCL::TDBImage imgcopy(tdb);
  ASSERT_THROW(imgcopy.get_image_height(), VCL::Exception);
  ASSERT_THROW(imgcopy.get_image_width(), VCL::Exception);
  ASSERT_THROW(imgcopy.get_image_channels(), VCL::Exception);
  ASSERT_FALSE(imgcopy.has_data());
}

TEST_F(TDBImageTest, CopyConstructorData) {
  VCL::TDBImage tdb("tdb/copy_construct.tdb");

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);

  EXPECT_EQ("tdb/copy_construct.tdb", tdb.get_object_id());
  tdb.write(cv_img_);

  VCL::TDBImage imgcopy(tdb);

  EXPECT_EQ(tdb.get_image_size(), imgcopy.get_image_size());
  ASSERT_TRUE(imgcopy.has_data());

  long img_size = tdb.get_image_size();
  unsigned char *buffer1 = new unsigned char[img_size];
  unsigned char *buffer2 = new unsigned char[img_size];

  tdb.get_buffer(buffer1, img_size);
  imgcopy.get_buffer(buffer2, img_size);

  compare_buffer_buffer(buffer1, buffer2, img_size);
  compare_mat_buffer(cv_img_, buffer1);
  compare_mat_buffer(cv_img_, buffer2);

  delete[] buffer2;
  delete[] buffer1;
}

TEST_F(TDBImageTest, CopyConstructor) {
  VCL::TDBImage tdb("tdb/copy_construct.tdb");

  EXPECT_EQ("tdb/copy_construct.tdb", tdb.get_object_id());
  ASSERT_FALSE(tdb.has_data());

  long size = long(cv_img_.rows) * long(cv_img_.cols) * cv_img_.channels();
  unsigned char *buf = new unsigned char[size];
  tdb.get_buffer(buf, size);

  compare_mat_buffer(cv_img_, buf);

  VCL::TDBImage imgcopy(tdb);

  EXPECT_EQ(tdb.get_image_size(), imgcopy.get_image_size());
  ASSERT_TRUE(imgcopy.has_data());
  ASSERT_TRUE(tdb.has_data());

  tdb.delete_image();
  ASSERT_FALSE(tdb.has_data());

  cv::Mat copy = imgcopy.get_cvmat();
  compare_mat_mat(copy, cv_img_);

  imgcopy.write("tdb/copied.tdb");
}

TEST_F(TDBImageTest, OperatorEqualsNoData) {
  VCL::TDBImage tdb("tdb/operator_equals.tdb");

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);

  EXPECT_EQ("tdb/operator_equals.tdb", tdb.get_object_id());

  VCL::TDBImage imgcopy;

  imgcopy = tdb;

  ASSERT_THROW(imgcopy.get_image_height(), VCL::Exception);
  ASSERT_THROW(imgcopy.get_image_width(), VCL::Exception);
  ASSERT_THROW(imgcopy.get_image_channels(), VCL::Exception);
  ASSERT_FALSE(imgcopy.has_data());
}

TEST_F(TDBImageTest, OperatorEqualsData) {
  VCL::TDBImage tdb("tdb/operator_equals.tdb");

  ASSERT_THROW(tdb.get_image_height(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_width(), VCL::Exception);
  ASSERT_THROW(tdb.get_image_channels(), VCL::Exception);

  EXPECT_EQ("tdb/operator_equals.tdb", tdb.get_object_id());

  tdb.write(cv_img_);

  VCL::TDBImage imgcopy;

  imgcopy = tdb;

  EXPECT_EQ(tdb.get_image_size(), imgcopy.get_image_size());
  ASSERT_TRUE(imgcopy.has_data());

  long size = tdb.get_image_size();
  unsigned char *buffer1 = new unsigned char[size];
  unsigned char *buffer2 = new unsigned char[size];

  tdb.get_buffer(buffer1, size);
  imgcopy.get_buffer(buffer2, size);

  compare_buffer_buffer(buffer1, buffer2, size);

  delete[] buffer2;
  delete[] buffer1;
}

TEST_F(TDBImageTest, OperatorEquals) {
  VCL::TDBImage tdb("tdb/operator_equals.tdb");
  EXPECT_EQ("tdb/operator_equals.tdb", tdb.get_object_id());

  EXPECT_EQ(tdb.get_image_height(), cv_img_.rows);

  VCL::TDBImage imgcopy;

  imgcopy = tdb;

  EXPECT_EQ(tdb.get_image_size(), imgcopy.get_image_size());
  ASSERT_TRUE(imgcopy.has_data());

  long size = tdb.get_image_size();
  unsigned char *buffer1 = new unsigned char[size];
  unsigned char *buffer2 = new unsigned char[size];

  tdb.get_buffer(buffer1, size);
  imgcopy.get_buffer(buffer2, size);

  compare_buffer_buffer(buffer1, buffer2, size);

  delete[] buffer1;
  delete[] buffer2;
}

TEST_F(TDBImageTest, GetImageSize) {
  VCL::TDBImage tdb(tdb_img_);
  tdb.write(cv_img_);

  long h = tdb.get_image_height();
  long w = tdb.get_image_width();
  long c = tdb.get_image_channels();

  EXPECT_EQ(h * w * c, tdb.get_image_size());
}

TEST_F(TDBImageTest, GetCVMat) {
  VCL::TDBImage tdb(tdb_img_);

  cv::Mat cv_img = tdb.get_cvmat();
  compare_mat_mat(cv_img, cv_img_);
}

TEST_F(TDBImageTest, GetBuffer) {
  VCL::TDBImage tdb(tdb_img_);

  long size = tdb.get_image_size();

  unsigned char *buf = new unsigned char[size];

  tdb.get_buffer(buf, size);

  compare_mat_buffer(cv_img_, buf);

  delete[] buf;
}

TEST_F(TDBImageTest, SetProperties) {
  VCL::TDBImage tdb("tdb/no_metadata.tdb");
  tdb.write(cv_img_, false);

  tdb.set_image_properties(cv_img_.rows, cv_img_.cols, cv_img_.channels());

  EXPECT_EQ(cv_img_.rows * cv_img_.cols * cv_img_.channels(),
            tdb.get_image_size());
}

TEST_F(TDBImageTest, WriteCVMat) {
  VCL::TDBImage tdb(tdb_img_);

  tdb.write(cv_img_);

  EXPECT_EQ(cv_img_.rows, tdb.get_image_height());
  EXPECT_EQ(cv_img_.cols, tdb.get_image_width());
}

TEST_F(TDBImageTest, WriteCVMatNoMetadata) {
  VCL::TDBImage tdb("tdb/no_metadata.tdb");

  tdb.write(cv_img_, false);

  tdb.set_image_properties(cv_img_.rows, cv_img_.cols, cv_img_.channels());

  EXPECT_EQ(cv_img_.rows, tdb.get_image_height());
  EXPECT_EQ(cv_img_.cols, tdb.get_image_width());
}

TEST_F(TDBImageTest, WriteString) {
  VCL::TDBImage tdb(tdb_img_);

  ASSERT_THROW(tdb.write(tdb_test_), VCL::Exception);

  tdb.read();

  tdb.write(tdb_test_);

  EXPECT_EQ(cv_img_.rows, tdb.get_image_height());
  EXPECT_EQ(cv_img_.cols, tdb.get_image_width());
}

TEST_F(TDBImageTest, Read) {
  VCL::TDBImage tdb(tdb_img_);

  tdb.read();

  EXPECT_EQ(cv_img_.rows, tdb.get_image_height());
  EXPECT_EQ(cv_img_.cols, tdb.get_image_width());
}

TEST_F(TDBImageTest, ReadRectangle) {
  VCL::TDBImage tdb(tdb_test_);

  tdb.read(rect_);

  EXPECT_EQ(100, tdb.get_image_height());
  EXPECT_EQ(100, tdb.get_image_width());
}

TEST_F(TDBImageTest, Resize) {
  VCL::TDBImage tdb(tdb_img_);

  tdb.resize(rect_);

  cv::Mat cv_small = tdb.get_cvmat();

  EXPECT_EQ(100, tdb.get_image_height());
  EXPECT_EQ(100, tdb.get_image_width());
}

TEST_F(TDBImageTest, Threshold) {
  VCL::TDBImage tdb(tdb_img_);

  tdb.read();

  tdb.threshold(200);

  cv::Mat cv_bright = tdb.get_cvmat();

  cv::threshold(cv_img_, cv_img_, 200, 200, cv::THRESH_TOZERO);

  compare_mat_mat(cv_bright, cv_img_);
}

TEST_F(TDBImageTest, DeleteImage) {
  VCL::TDBImage tdb("tdb/operator_equals.tdb");

  tdb.delete_image();

  ASSERT_FALSE(tdb.has_data());
  ASSERT_THROW(tdb.get_image_size(), VCL::Exception);
}

TEST_F(TDBImageTest, DeleteImageAfterRead) {
  VCL::TDBImage tdb("tdb/copied.tdb");

  tdb.read();
  ASSERT_TRUE(tdb.has_data());
  tdb.delete_image();

  ASSERT_FALSE(tdb.has_data());
}

TEST_F(TDBImageTest, SetMinimum) {
  VCL::TDBImage tdb;
  tdb.set_minimum(3);
}
