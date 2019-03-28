/**
 * @file  Video_test.cc
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

#include "vcl/Video.h"
#include "gtest/gtest.h"

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <string>
#include <iostream>
#include <fstream>

#include "helpers.h"

using namespace std;

class VideoTest : public ::testing::Test {

protected:

    std::string _video_path_avi_xvid;
    std::string _video_path_mp4_h264;
    std::vector<cv::Mat> _frames_xvid;
    std::vector<cv::Mat> _frames_h264;

    virtual void SetUp() {
        _video_path_avi_xvid = "videos/Megamind.avi";
        _video_path_mp4_h264 = "videos/Megamind.mp4";

        cv::VideoCapture testVideo_xvid(_video_path_avi_xvid);

        // Read the video once for speed
        while (true) {
            cv::Mat frame;
            testVideo_xvid >> frame;

            if (frame.empty())
                break;

            _frames_xvid.push_back(frame);
        }

        cv::VideoCapture testVideo_h264(_video_path_mp4_h264);

        // Read the video once for speed
        while (true) {
            cv::Mat frame;
            testVideo_h264 >> frame;

            if (frame.empty())
                break;

            _frames_h264.push_back(frame);
        }
    }

    int get_fourcc() {
        return CV_FOURCC('H', '2', '6', '4');
    }
};

TEST_F(VideoTest, DefaultConstructor)
{
    VCL::Video video_data;
    long input_frame_count = video_data.get_frame_count();

    ASSERT_EQ(input_frame_count, 0);
}

TEST_F(VideoTest, StringConstructor)
{
    VCL::Video video_data(_video_path_avi_xvid);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(_video_path_avi_xvid);
    long test_frame_count  = testVideo.get(CV_CAP_PROP_FRAME_COUNT);
    ASSERT_EQ(input_frame_count, test_frame_count);
}

TEST_F(VideoTest, StringConstructorNoFormat)
{
    VCL::Video video_data("videos/megamind");
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(_video_path_mp4_h264);
    long test_frame_count  = testVideo.get(CV_CAP_PROP_FRAME_COUNT);
    ASSERT_EQ(input_frame_count, test_frame_count);
}

TEST_F(VideoTest, StringConstructorNoExists)
{
    VCL::Video video_data("this/path/does/not/exist.wrongformat");
    ASSERT_THROW(video_data.get_frame_count(), VCL::Exception);
}


TEST_F(VideoTest, CopyConstructor)
{
    VCL::Video testVideo4copy(_video_path_avi_xvid);

    VCL::Video video_data(testVideo4copy);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(_video_path_avi_xvid);
    long test_frame_count  = testVideo.get(CV_CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
        cv::Mat input_frame = video_data.get_frame(i);
        compare_mat_mat(input_frame, _frames_xvid.at(i));
    }
}

TEST_F(VideoTest, BlobConstructor)
{
    std::ifstream ifile;
    ifile.open(_video_path_avi_xvid);

    int fsize;
    char* inBuf;
    ifile.seekg(0, std::ios::end);
    fsize = (long)ifile.tellg();
    ifile.seekg(0, std::ios::beg);
    inBuf = new char[fsize];
    ifile.read(inBuf, fsize);
    ifile.close();

    std::string vcl_from_buffer("videos_tests/from_buffer.avi");
    {
        VCL::Video video_data(inBuf, fsize);
        video_data.store(vcl_from_buffer, VCL::Video::Codec::XVID);
    }

    delete[] inBuf;

    // OpenCV writing the video H264
    // We need to write again to make sure we use the same parameters
    // when writting the video.
    std::string write_output_ocv("videos_tests/write_test_ocv.avi");
    {
        cv::VideoCapture testWriteVideo(_video_path_avi_xvid);

        cv::VideoWriter testResultVideo(
                        write_output_ocv,
                        CV_FOURCC('X', 'V', 'I', 'D'),
                        testWriteVideo.get(CV_CAP_PROP_FPS),
                        cv::Size(
                            testWriteVideo.get(CV_CAP_PROP_FRAME_WIDTH),
                            testWriteVideo.get(CV_CAP_PROP_FRAME_HEIGHT))
                        );

        for (auto& frame : _frames_xvid) {
            testResultVideo << frame;
        }
    }

    VCL::Video video_data(vcl_from_buffer);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(write_output_ocv);
    long test_frame_count = testVideo.get(CV_CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
        cv::Mat input_frame = video_data.get_frame(i);
        cv::Mat test_frame;
        testVideo >> test_frame;

        if (test_frame.empty())
            break; // should not happen

        compare_mat_mat(input_frame, test_frame);
    }
}

TEST_F(VideoTest, CreateUnique)
{
    try {
        VCL::Video video_data(_video_path_mp4_h264);
        std::string uname = VCL::create_unique("videos_tests/", "mp4");
        video_data.store(uname, VCL::Video::Codec::H264);

        VCL::Video video_read(uname);

        ASSERT_GE(video_data.get_frame_count(), 1);
        ASSERT_EQ(video_data.get_frame_count(), video_read.get_frame_count());

    } catch(VCL::Exception &e) {
        print_exception(e);
        ASSERT_TRUE(false);
    }
}
