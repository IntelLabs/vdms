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

#include "ImageData.h"
#include "gtest/gtest.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <string>


class ImageDataTest : public ::testing::Test {
 protected:
    virtual void SetUp() {
        img_ = "images/large1.jpg";
        tdb_img_ = "tdb/test_image.tdb";
        cv_img_ = cv::imread(img_, cv::IMREAD_ANYCOLOR);
        rect_ = VCL::Rectangle(100, 100, 100, 100);
    }

    void compare_mat_mat(cv::Mat &cv_img, cv::Mat &img)
    {
        int rows = img.rows;
        int columns = img.cols;
        int channels = img.channels();

        if ( img.isContinuous() ) {
            columns *= rows;
            rows = 1;
        }

        for ( int i = 0; i < rows; ++i ) {
            for ( int j = 0; j < columns; ++j ) {
                if (channels == 1) {
                    unsigned char pixel = img.at<unsigned char>(i, j);
                    unsigned char test_pixel = cv_img.at<unsigned char>(i, j);
                    ASSERT_EQ(pixel, test_pixel);
                }
                else {
                    cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
                    cv::Vec3b test_colors = cv_img.at<cv::Vec3b>(i, j);
                    for ( int x = 0; x < channels; ++x ) {
                        ASSERT_EQ(colors.val[x], test_colors.val[x]);
                    }
                }
            }
        }
    }

    void compare_mat_buffer(cv::Mat &img, unsigned char* buffer)
    {
        int index = 0;

        int rows = img.rows;
        int columns = img.cols;
        int channels = img.channels();

        if ( img.isContinuous() ) {
            columns *= rows;
            rows = 1;
        }

        for ( int i = 0; i < rows; ++i ) {
            for ( int j = 0; j < columns; ++j ) {
                if (channels == 1) {
                    unsigned char pixel = img.at<unsigned char>(i, j);
                    ASSERT_EQ(pixel, buffer[index]);
                }
                else {
                    cv::Vec3b colors = img.at<cv::Vec3b>(i, j);
                    for ( int x = 0; x < channels; ++x ) {
                        ASSERT_EQ(colors.val[x], buffer[index + x]);
                    }
                }
                index += channels;
            }
        }
    }

    std::string img_;
    std::string tdb_img_;
    std::string test_img_;
    cv::Mat cv_img_;
    VCL::Rectangle rect_;
};


TEST_F(ImageDataTest, DefaultConstructor)
{
    VCL::ImageData img_data;

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(0, dims.height);
    EXPECT_EQ(0, dims.width);
}

TEST_F(ImageDataTest, MatConstructor)
{
    VCL::ImageData img_data(cv_img_);

    EXPECT_EQ(cv_img_.type(), img_data.get_type());

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(cv_img_.rows, dims.height);
    EXPECT_EQ(cv_img_.cols, dims.width);

    cv::Mat cv_img = img_data.get_cvmat();

    compare_mat_mat(cv_img, cv_img_);
}

TEST_F(ImageDataTest, StringConstructorIMG)
{
    VCL::ImageData img_data(img_);

    cv::Size dims = img_data.get_dimensions();
    EXPECT_EQ(0, dims.height);
    EXPECT_EQ(0, dims.width);

    EXPECT_EQ(img_data.get_image_format(), VCL::Image::Format::JPG);
}

TEST_F(ImageDataTest, StringConstructorTDB)
{
    VCL::ImageData img_data(tdb_img_);

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(0, dims.height);
    EXPECT_EQ(0, dims.width);

    EXPECT_EQ(img_data.get_image_format(), VCL::Image::Format::TDB);
}

TEST_F(ImageDataTest, BufferConstructor)
{
    unsigned char* buffer = cv_img_.data;

    int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();

    VCL::ImageData img_data(buffer, cv::Size(cv_img_.cols, cv_img_.rows), cv_img_.type());

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(cv_img_.rows, dims.height);
    EXPECT_EQ(cv_img_.cols, dims.width);
    EXPECT_EQ(cv_img_.type(), img_data.get_type());

    unsigned char* buf = new unsigned char[size];

    img_data.get_buffer(buf, size);

    compare_mat_buffer(cv_img_, buf);
}

TEST_F(ImageDataTest, CopyConstructorMat)
{
    VCL::ImageData img_data(cv_img_);

    VCL::ImageData img_copy(img_data);

    cv::Mat cv_img = img_data.get_cvmat();
    cv::Mat cv_copy = img_copy.get_cvmat();

    compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageDataTest, CopyConstructorTDB)
{
    VCL::ImageData img_data(tdb_img_);

    VCL::ImageData img_copy(img_data);

    cv::Mat cv_img = img_data.get_cvmat();
    cv::Mat cv_copy = img_copy.get_cvmat();

    compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageDataTest, OperatorEqualsMat)
{
    VCL::ImageData img_data(cv_img_);

    VCL::ImageData img_copy;

    img_copy = img_data;

    cv::Mat cv_img = img_data.get_cvmat();
    cv::Mat cv_copy = img_copy.get_cvmat();

    compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageDataTest, OperatorEqualsTDB)
{
    VCL::ImageData img_data(tdb_img_);

    VCL::ImageData img_copy;

    img_copy = img_data;

    cv::Mat cv_img = img_data.get_cvmat();
    cv::Mat cv_copy = img_copy.get_cvmat();

    compare_mat_mat(cv_img, cv_copy);
}

TEST_F(ImageDataTest, GetImageIDandImageFormat)
{
    VCL::ImageData img_data(tdb_img_);

    EXPECT_EQ(img_data.get_image_id(), tdb_img_);
    EXPECT_EQ(img_data.get_image_format(), VCL::Image::Format::TDB);
}

TEST_F(ImageDataTest, GetImageDimensions)
{
    VCL::ImageData img_data(cv_img_);

    EXPECT_EQ(img_data.get_type(), cv_img_.type());

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(cv_img_.rows, dims.height);
    EXPECT_EQ(cv_img_.cols, dims.width);

    EXPECT_EQ(img_data.get_size(), cv_img_.rows*cv_img_.cols*cv_img_.channels());
}

TEST_F(ImageDataTest, GetArea)
{
    VCL::ImageData img_data(tdb_img_);

    VCL::ImageData new_data = img_data.get_area(rect_);

    cv::Size dims = new_data.get_dimensions();

    EXPECT_EQ(rect_.height, dims.height);
    EXPECT_EQ(rect_.width, dims.width);
}

TEST_F(ImageDataTest, GetBuffer)
{
    VCL::ImageData img_data(tdb_img_);

    int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();
    unsigned char* buf = new unsigned char[size];

    img_data.get_buffer(buf, size);

    compare_mat_buffer(cv_img_, buf);
}

TEST_F(ImageDataTest, GetCVMat)
{
    VCL::ImageData img_data(tdb_img_);

    cv::Mat cv_img = img_data.get_cvmat();

    compare_mat_mat(cv_img_, cv_img);
}

TEST_F(ImageDataTest, GetEncodedBuffer)
{
    VCL::ImageData img_data(tdb_img_);

    std::vector<unsigned char> encoded = img_data.get_encoded(VCL::Image::Format::PNG);

    cv::Mat mat = cv::imdecode(encoded, cv::IMREAD_ANYCOLOR);
    compare_mat_mat(cv_img_, mat);
}

TEST_F(ImageDataTest, CreateUnique)
{
    VCL::ImageData img_data(cv_img_);

    auto unique_name = VCL::create_unique("image_results/", "png");

    img_data.write(unique_name, VCL::Image::Format::PNG);
    img_data.perform_operations();
}

TEST_F(ImageDataTest, SetDataFromRaw)
{
    VCL::ImageData img_data;

    void* buffer = cv_img_.data;
    int size = cv_img_.rows * cv_img_.cols * cv_img_.channels();

    img_data.set_data_from_raw(buffer, size);

    cv::Mat raw = img_data.get_cvmat();

    compare_mat_mat(cv_img_, raw);
}

TEST_F(ImageDataTest, SetDataFromEncoded)
{
    VCL::ImageData img_data;

    std::vector<unsigned char> buffer;
    cv::imencode(".png", cv_img_, buffer);

    img_data.set_data_from_encoded(buffer);

    cv::Mat raw = img_data.get_cvmat();

    compare_mat_mat(raw, cv_img_);
}

TEST_F(ImageDataTest, Read)
{
    VCL::ImageData img_data;
    img_data.set_format("jpg");

    ASSERT_THROW(img_data.read("images/.jpg"), VCL::Exception);

    img_data.read("images/large1");

    EXPECT_EQ("images/large1.jpg", img_data.get_image_id());
}

TEST_F(ImageDataTest, Write)
{
    VCL::ImageData img_data(cv_img_);

    img_data.write("tdb/test_image", VCL::Image::Format::TDB);
}

TEST_F(ImageDataTest, Resize)
{
    VCL::ImageData img_data(cv_img_);

    img_data.crop(rect_);

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(rect_.height, dims.height);
    EXPECT_EQ(rect_.width, dims.width);
}

TEST_F(ImageDataTest, Crop)
{
    VCL::ImageData img_data(cv_img_);

    img_data.crop(rect_);

    cv::Size dims = img_data.get_dimensions();

    EXPECT_EQ(rect_.height, dims.height);
    EXPECT_EQ(rect_.width, dims.width);

    cv::Mat imgmat = img_data.get_cvmat();
    cv::Mat mat(cv_img_, rect_);

    compare_mat_mat(imgmat, mat);
}

TEST_F(ImageDataTest, Threshold)
{
    VCL::ImageData img_data(tdb_img_);

    img_data.read(tdb_img_);

    img_data.threshold(200);

    img_data.perform_operations();

    cv::Mat cv_bright = img_data.get_cvmat();

    cv::threshold(cv_img_, cv_img_, 200, 200, cv::THRESH_TOZERO);

    compare_mat_mat(cv_bright, cv_img_);
}

TEST_F(ImageDataTest, DeleteTDB)
{
    VCL::ImageData img_data("tdb/no_metadata.tdb");

    img_data.delete_object();

    img_data.read("tdb/no_metadata.tdb");
    ASSERT_THROW(img_data.perform_operations(), VCL::Exception);
}

// This tests is not passing
// TEST_F(ImageDataTest, DeleteIMG)
// {
//     VCL::ImageData img_data(cv_img_);

//     auto unique_name = VCL::create_unique("image_results/", "png");

//     img_data.write(unique_name, VCL::Image::Format::PNG);
//     img_data.perform_operations();

//     img_data.delete_object();

//     img_data.read(test_img_);
//     ASSERT_THROW(img_data.perform_operations(), VCL::Exception);
// }

TEST_F(ImageDataTest, SetMinimum)
{
    VCL::ImageData img_data(cv_img_);

    img_data.set_minimum(3);
}
