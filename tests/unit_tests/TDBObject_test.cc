/**
 * @file   TDBObject_test.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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

#include <gtest/gtest.h>
#include <opencv2/imgcodecs.hpp>

#include "TDBImage.h"
#include "TDBObject.h"

const std::string TMP_DIRNAME = "tests_output_dir/";

class TDBObjectTest : public ::testing::Test {

protected:
  virtual void SetUp() {
    tdb_img_ = TMP_DIRNAME + "tdb/test_image.tdb";
    cv_img_ = cv::imread("test_images/large1.jpg", cv::IMREAD_ANYCOLOR);
  }

  virtual void TearDown() {}

  std::string tdb_img_;
  cv::Mat cv_img_;
};

TEST_F(TDBObjectTest, EqualOperatorInTDBObject) {
  VCL::TDBImage sourceTDB(tdb_img_);
  sourceTDB.write(cv_img_);
  ASSERT_TRUE(sourceTDB.has_data());
  // Sliced the object to get the TDBObject
  VCL::TDBObject destTDBObject = static_cast<VCL::TDBObject>(sourceTDB);

  bool areEqual = (static_cast<VCL::TDBObject>(sourceTDB) == destTDBObject);
  ASSERT_TRUE(areEqual);
}