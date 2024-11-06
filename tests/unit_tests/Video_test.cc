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

#include "VideoLoop.h"
#include "vcl/Video.h"
#include "gtest/gtest.h"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdlib.h> /* srand, rand */
#include <string>
#include <time.h> /* time */

#include "helpers.h"

#include "VDMSConfig.h"

using namespace std;
namespace fs = std::filesystem;
const std::string OUTPUT_VIDEO_DIR = "test_db_1";

class VideoTest : public ::testing::Test {

protected:
  std::string _video_path_avi_xvid;
  std::string _video_path_mp4_h264;
  std::vector<cv::Mat> _frames_xvid;
  std::vector<cv::Mat> _frames_h264;

  virtual void SetUp() {

    VDMS::VDMSConfig::init("unit_tests/config-tests.json");
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

  virtual void TearDown() { VDMS::VDMSConfig::destroy(); }

  int get_fourcc() { return cv::VideoWriter::fourcc('H', '2', '6', '4'); }
};

namespace VCL {

class VideoTest : public Video {

public:
  VideoTest() : Video() {}
  VideoTest(std::string a) : Video(a) {}

  using Video::perform_operations;
};
}; // namespace VCL

/**
 * Create a Video object.
 * Throw an exception as no video file is
 * available to count number of frames
 */
TEST_F(VideoTest, DefaultConstructor) {
  VCL::Video video_data;
  ASSERT_THROW(video_data.get_frame_count(), VCL::Exception);
}

/**
 * Create a video object from a file.
 * Should have the same number of frames as
 * the OpenCV video
 */
TEST_F(VideoTest, StringConstructor) {
  VCL::Video video_data(_video_path_avi_xvid);
  long input_frame_count = video_data.get_frame_count();

  cv::VideoCapture testVideo(_video_path_avi_xvid);
  long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);
  ASSERT_EQ(input_frame_count, test_frame_count);
}

/**
 * Create a video from a filename that has no extension.
 * Should successfully create a video of 'mp4' extension.
 */
TEST_F(VideoTest, StringConstructorNoFormat) {
  VCL::Video video_data("videos/megamind");
  long input_frame_count = video_data.get_frame_count();

  cv::VideoCapture testVideo(_video_path_mp4_h264);
  long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);
  ASSERT_EQ(input_frame_count, test_frame_count);
}

/**
 * Try create a video with an unavailable file location.
 * Should throw an exception.
 */
TEST_F(VideoTest, StringConstructorNoExists) {
  VCL::Video video_data("this/path/does/not/exist.wrongformat");
  ASSERT_THROW(video_data.get_frame_count(), VCL::Exception);
}

/**
 * Create a copy of a Video object.
 * Both videos should have the same frames.
 */
TEST_F(VideoTest, CopyConstructor) {
  VCL::Video testVideo4copy(_video_path_avi_xvid);

  VCL::Video video_data(testVideo4copy);
  long input_frame_count = video_data.get_frame_count();

  cv::VideoCapture testVideo(_video_path_avi_xvid);
  long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

  EXPECT_EQ(input_frame_count, test_frame_count);

  for (int i = 0; i < input_frame_count; ++i) {
    cv::Mat input_frame = video_data.get_frame(i);
    EXPECT_TRUE(compare_mat_mat(input_frame, _frames_xvid.at(i)));
  }
}

/**
 * Create a video object from a blob.
 * Should have the same frames as an OpenCV video object.
 */
TEST_F(VideoTest, BlobConstructor) {
  std::ifstream ifile;
  ifile.open(_video_path_avi_xvid);

  int fsize;
  char *inBuf;
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
        write_output_ocv, cv::VideoWriter::fourcc('X', 'V', 'I', 'D'),
        testWriteVideo.get(cv::CAP_PROP_FPS),
        cv::Size(testWriteVideo.get(cv::CAP_PROP_FRAME_WIDTH),
                 testWriteVideo.get(cv::CAP_PROP_FRAME_HEIGHT)));

    for (auto &frame : _frames_xvid) {
      testResultVideo << frame;
    }
  }

  VCL::Video video_data(vcl_from_buffer);
  long input_frame_count = video_data.get_frame_count();

  cv::VideoCapture testVideo(write_output_ocv);
  long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

  ASSERT_EQ(input_frame_count, test_frame_count);

  for (int i = 0; i < input_frame_count; ++i) {
    cv::Mat input_frame = video_data.get_frame(i);
    cv::Mat test_frame;
    testVideo >> test_frame;

    if (test_frame.empty())
      break; // should not happen

    EXPECT_TRUE(compare_mat_mat(input_frame, test_frame));
  }
}

TEST_F(VideoTest, CreateUnique) {
  try {
    std::string temp_video_input(
        VDMS::VDMSConfig::instance()->get_path_tmp() + "/" +
        fs::path(_video_path_mp4_h264).filename().string());
    copy_video_to_temp(_video_path_mp4_h264, temp_video_input, get_fourcc());

    VCL::Video video_data(temp_video_input);
    std::string uname = VCL::create_unique(OUTPUT_VIDEO_DIR + "/videos", "mp4");
    video_data.store(uname, VCL::Video::Codec::H264);

    VCL::Video video_read(uname);

    ASSERT_GE(video_data.get_frame_count(), 1);
    ASSERT_EQ(video_data.get_frame_count(), video_read.get_frame_count());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Create a Video object using an AVI file.
 * Should have the same frames as an OpenCV video object.
 */
TEST_F(VideoTest, ReadAVI_XVID) {
  try {
    VCL::Video video_data(_video_path_avi_xvid);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(_video_path_avi_xvid);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      EXPECT_TRUE(compare_mat_mat(input_frame, _frames_xvid.at(i)));
    }

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Create a Video object using an MP4 file.
 * Should have the same frames as an OpenCV video object.
 */
TEST_F(VideoTest, ReadMP4_H264) {
  try {
    VCL::Video video_data(_video_path_mp4_h264);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(_video_path_mp4_h264);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      EXPECT_TRUE(compare_mat_mat(input_frame, _frames_h264.at(i)));
    }

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Create a Video object of MP4 format using an AVI file and write to the data
 * store. Imitates the VDMS read then store capability. Should have the same
 * frames as an OpenCV video object.
 */
TEST_F(VideoTest, WriteMP4_H264) {
  try {
    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_WriteMP4_H264_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_WriteMP4_H264_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string write_output_vcl("videos_tests/write_test_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input);
      video_data.store(write_output_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string write_output_ocv("videos_tests/write_test_ocv.mp4");
    { copy_video_to_temp(temp_video_test, write_output_ocv, get_fourcc()); }

    VCL::Video video_data(write_output_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(write_output_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      EXPECT_TRUE(compare_mat_mat(input_frame, test_frame));
    }

    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Create a Video object using an AVI file and write to the data store.
 * Imitates the VDMS read then store capability.
 * Should have the same frames as an OpenCV video object.
 */
TEST_F(VideoTest, WriteAVI_XVID) {
  try {
    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_WriteAVI_XVID_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input,
                       cv::VideoWriter::fourcc('X', 'V', 'I', 'D'));
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_WriteAVI_XVID_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test,
                       cv::VideoWriter::fourcc('X', 'V', 'I', 'D'));

    std::string write_output_vcl("videos_tests/write_test_vcl.avi");
    {
      VCL::Video video_data(temp_video_input);
      video_data.store(write_output_vcl, VCL::Video::Codec::XVID);
    }

    // OpenCV writing the video H264
    std::string write_output_ocv("videos_tests/write_test_ocv.avi");
    {
      copy_video_to_temp(temp_video_test, write_output_ocv,
                         cv::VideoWriter::fourcc('X', 'V', 'I', 'D'));
    }

    VCL::Video video_data(write_output_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(write_output_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      EXPECT_TRUE(compare_mat_mat(input_frame, test_frame));
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates the resize and store operation of VDMS.
 * Should have the same frames as an OpenCV video object
 * that undergoes a resize operation.
 */
TEST_F(VideoTest, ResizeWrite) {
  int new_w = 160;
  int new_h = 90;

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_ResizeWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_ResizeWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string resize_name_vcl("videos_tests/resize_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.resize(new_w, new_h);
      video_data.store(resize_name_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string resize_name_ocv("videos_tests/resize_ocv.mp4");
    {
      cv::VideoCapture testWriteVideo(temp_video_test);

      cv::VideoWriter testResultVideo(resize_name_ocv, get_fourcc(),
                                      testWriteVideo.get(cv::CAP_PROP_FPS),
                                      cv::Size(new_w, new_h));

      while (true) {
        cv::Mat mat_frame;
        testWriteVideo >> mat_frame;

        if (mat_frame.empty()) {
          break;
        }
        cv::Mat cv_resized;
        cv::resize(mat_frame, cv_resized, cv::Size(new_w, new_h));

        testResultVideo << cv_resized;
        mat_frame.release();
      }
    }

    VCL::Video video_data(resize_name_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(resize_name_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      EXPECT_TRUE(compare_mat_mat(input_frame, test_frame));
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates the trim and store operation of VDMS.
 * Should have the same frames as an OpenCV video object
 * that undergoes a trim operation.
 */
TEST_F(VideoTest, IntervalWrite) {
  int init = 10;
  int end = 100;
  int step = 5;

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_IntervalWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_IntervalWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string interval_name_vcl("videos_tests/interval_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.interval(VCL::Video::FRAMES, init, end, step);
      video_data.store(interval_name_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string interval_name_ocv("videos_tests/interval_ocv.mp4");
    {
      cv::VideoCapture testWriteVideo(_video_path_avi_xvid);

      cv::VideoWriter testResultVideo(
          interval_name_ocv, get_fourcc(),
          testWriteVideo.get(cv::CAP_PROP_FPS) / step,
          cv::Size(testWriteVideo.get(cv::CAP_PROP_FRAME_WIDTH),
                   testWriteVideo.get(cv::CAP_PROP_FRAME_HEIGHT)));

      if (end >= _frames_xvid.size())
        ASSERT_TRUE(false);

      int frame_number = 0;
      int last_frame_written = 0;
      while (true) {
        cv::Mat mat_frame;
        testWriteVideo >> mat_frame; // Read frame
        frame_number++;

        if (mat_frame.empty())
          break;

        if (frame_number >= init && frame_number < end) {
          if (last_frame_written == 0) {
            testResultVideo << mat_frame;
            last_frame_written = frame_number;
          } else {
            if ((frame_number - last_frame_written) == step) {
              testResultVideo << mat_frame;
              last_frame_written = frame_number;
            }
          }
        }

        if (frame_number > end) {
          break;
        }
      }

      testWriteVideo.release();
    }

    VCL::Video video_data(interval_name_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(interval_name_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      compare_image_image(input_frame, test_frame);
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Try to trim a video with out of bounds parameters.
 * Should throw an exception.
 */
TEST_F(VideoTest, IntervalOutOfBounds) {
  // Video has 270 frames, we test out of bounds here.

  int init = 10;
  int end = 270; // This should cause error
  int step = 5;
  try {
    VCL::Video video_data(_video_path_avi_xvid); //
    video_data.interval(VCL::Video::FRAMES, init, end, step);
    // It will only throw when the operations are performed
    video_data.get_frame_count();
    ASSERT_STREQ(video_data.get_query_error_response().data(),
                 "End Frame cannot be greater than number of frames");
  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }

  init = 270;
  end = 250;
  try {
    VCL::Video video_data(_video_path_avi_xvid); //
    video_data.interval(VCL::Video::FRAMES, init, end, step);
    // It will only throw when the operations are performed
    video_data.get_frame_count();
    ASSERT_STREQ(video_data.get_query_error_response().data(),
                 "Start Frame cannot be greater than number of frames");
  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates the threshold and store operation of VDMS.
 * Should have the same frames as an OpenCV video object
 * that undergoes a threshold operation.
 */
TEST_F(VideoTest, ThresholdWrite) {
  int ths = 100;

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_ThresholdWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_ThresholdWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string threshold_name_vcl("videos_tests/threshold_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.threshold(ths);
      video_data.store(threshold_name_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string threshold_name_ocv("videos_tests/threshold_ocv.mp4");
    {
      cv::VideoCapture testWriteVideo(temp_video_test);

      cv::VideoWriter testResultVideo(
          threshold_name_ocv, get_fourcc(),
          testWriteVideo.get(cv::CAP_PROP_FPS),
          cv::Size(testWriteVideo.get(cv::CAP_PROP_FRAME_WIDTH),
                   testWriteVideo.get(cv::CAP_PROP_FRAME_HEIGHT)));

      while (true) {
        cv::Mat mat_frame;
        testWriteVideo >> mat_frame;

        if (mat_frame.empty()) {
          break;
        }
        cv::Mat cv_ths;
        cv::threshold(mat_frame, cv_ths, ths, ths, cv::THRESH_TOZERO);

        testResultVideo << cv_ths;
        mat_frame.release();
      }

      testWriteVideo.release();
    }

    VCL::Video video_data(threshold_name_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(threshold_name_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      EXPECT_TRUE(compare_mat_mat(input_frame, test_frame));
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates the crop and store operation of VDMS.
 * Should have the same frames as an OpenCV video object
 * that undergoes a crop operation.
 */
TEST_F(VideoTest, CropWrite) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing for "
               << "non remote tests";
  int new_w = 160;
  int new_h = 90;

  cv::Rect ocv_rect(100, 100, new_w, new_h);
  VCL::Rectangle rect(100, 100, new_w, new_h);

  try {
    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_CropWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_CropWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string crop_name_vcl("videos_tests/crop_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.crop(rect);
      video_data.store(crop_name_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string crop_name_ocv("videos_tests/crop_ocv.mp4");
    {
      cv::VideoCapture testWriteVideo(temp_video_test);

      cv::VideoWriter testResultVideo(crop_name_ocv, get_fourcc(),
                                      testWriteVideo.get(cv::CAP_PROP_FPS),
                                      cv::Size(new_w, new_h));

      while (true) {
        cv::Mat mat_frame;
        testWriteVideo >> mat_frame;

        if (mat_frame.empty()) {
          break;
        }

        cv::Mat roi_frame(mat_frame, ocv_rect);

        testResultVideo << roi_frame;
        mat_frame.release();
      }

      testWriteVideo.release();
    }

    VCL::Video video_data(crop_name_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(crop_name_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      if (!compare_mat_mat(input_frame, test_frame)) {
        throw VCLException(
            SizeMismatch,
            std::string("There is a mismatch in the frame number: " +
                        to_string(i))
                .c_str());
      }
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates performing a remote operation (Adding a caption here)
 * and then storing the video in VDMS.
 * Should have the same frames as an OpenCV video object
 * that undergoes a captioning operation.
 */
TEST_F(VideoTest, SyncRemoteWrite) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing "
               << "for non remote tests. "
               << "Margin of error is higher than the max limit";
  std::string _url = "http://localhost:5010/video";
  Json::Value _options;
  _options["format"] = "mp4";
  _options["text"] = "Video";
  _options["id"] = "caption";

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_SyncRemoteWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_SyncRemoteWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string syncremote_name_vcl("videos_tests/syncremote_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.syncremoteOperation(_url, _options);
      video_data.store(syncremote_name_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string syncremote_name_ocv("videos_tests/syncremote_ocv.mp4");
    {
      cv::VideoCapture testWriteVideo(temp_video_test);

      cv::VideoWriter testResultVideo(
          syncremote_name_ocv, get_fourcc(),
          testWriteVideo.get(cv::CAP_PROP_FPS),
          cv::Size(testWriteVideo.get(cv::CAP_PROP_FRAME_WIDTH),
                   testWriteVideo.get(cv::CAP_PROP_FRAME_HEIGHT)));

      while (true) {
        cv::Mat mat_frame;
        testWriteVideo >> mat_frame;

        if (mat_frame.empty()) {
          break;
        }
        cv::putText(mat_frame, _options["text"].asCString(), cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(255, 255, 255), 2);

        testResultVideo << mat_frame;
        mat_frame.release();
      }
    }

    VCL::Video video_data(syncremote_name_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(syncremote_name_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      compare_image_image(input_frame, test_frame);
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates performing a user defined operation (Adding a caption here)
 * and then storing the video in VDMS.
 * Should have the same frames as an OpenCV video object
 * that undergoes a captioning operation.
 */
TEST_F(VideoTest, UDFWrite) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Reason to be skipped: This test is failing "
               << "for non remote tests";
  Json::Value _options;
  _options["port"] = 5555;
  _options["text"] = "Video";
  _options["id"] = "caption";

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_UDFWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_UDFemoteWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string udf_name_vcl("videos_tests/udf_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.userOperation(_options);
      video_data.store(udf_name_vcl, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string udf_name_ocv("videos_tests/udf_ocv.mp4");
    {
      cv::VideoCapture testWriteVideo(temp_video_test);

      cv::VideoWriter testResultVideo(
          udf_name_ocv, get_fourcc(), testWriteVideo.get(cv::CAP_PROP_FPS),
          cv::Size(testWriteVideo.get(cv::CAP_PROP_FRAME_WIDTH),
                   testWriteVideo.get(cv::CAP_PROP_FRAME_HEIGHT)));

      while (true) {
        cv::Mat mat_frame;
        testWriteVideo >> mat_frame;

        if (mat_frame.empty()) {
          break;
        }
        cv::putText(mat_frame, _options["text"].asCString(), cv::Point(10, 25),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, CV_RGB(255, 255, 255), 2);

        testResultVideo << mat_frame;
        mat_frame.release();
      }
    }

    VCL::Video video_data(udf_name_vcl);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(udf_name_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      compare_image_image(input_frame, test_frame);
    }
    std::remove(temp_video_input.data());
    std::remove(temp_video_test.data());

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Tests the working of the VideoLoop class
 * when a single remote operation is executed.
 * The resulting video being encoded should not be null.
 */
TEST_F(VideoTest, VideoLoopTest) {
  std::string _url = "http://localhost:5010/video";
  Json::Value _options;
  _options["format"] = "mp4";
  _options["text"] = "Video";
  _options["id"] = "caption";

  std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                               "/video_test_VideoLoopTest_input.avi");
  copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());

  std::string vloop_name_vcl("videos_tests/vloop_vcl.mp4");
  {
    VCL::Video video_data(temp_video_input);
    video_data.store(vloop_name_vcl, VCL::Video::Codec::H264);
  }

  VideoLoop videoLoop;
  VCL::Video video_data(vloop_name_vcl);

  video_data.remoteOperation(_url, _options);

  videoLoop.set_nrof_entities(1);

  videoLoop.enqueue(video_data);

  while (videoLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Video> videoMap = videoLoop.get_video_map();
  std::map<std::string, VCL::Video>::iterator iter = videoMap.begin();

  VCL::Video::Codec vcl_codec = VCL::Video::Codec::H264;
  const std::string vcl_container = "mp4";

  while (iter != videoMap.end()) {
    auto video_enc = iter->second.get_encoded(vcl_container, vcl_codec);
    int size = video_enc.size();

    ASSERT_TRUE(!video_enc.empty());
    iter++;
  }
}

/**
 * Tests the working of the VideoLoop class
 * when a an operation pipeline is executed.
 * The resulting video being encoded should not be null.
 */
TEST_F(VideoTest, VideoLoopPipelineTest) {
  std::string _url = "http://localhost:5010/video";
  Json::Value _options;
  _options["format"] = "mp4";
  _options["text"] = "Video";
  _options["id"] = "caption";

  int ths = 100;

  int init = 10;
  int end = 100;
  int step = 5;

  std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                               "/video_test_VideoLoopPipelineTest_input.avi");
  copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());

  std::string vloop_name_vcl("videos_tests/vloop_vcl.mp4");
  {
    VCL::Video video_data(temp_video_input);
    video_data.store(vloop_name_vcl, VCL::Video::Codec::H264);
  }

  VideoLoop videoLoop;
  VCL::Video video_data(vloop_name_vcl);

  video_data.threshold(ths);
  video_data.interval(VCL::Video::FRAMES, init, end, step);
  video_data.remoteOperation(_url, _options);

  videoLoop.set_nrof_entities(1);

  videoLoop.enqueue(video_data);

  while (videoLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Video> videoMap = videoLoop.get_video_map();
  std::map<std::string, VCL::Video>::iterator iter = videoMap.begin();

  VCL::Video::Codec vcl_codec = VCL::Video::Codec::H264;
  const std::string vcl_container = "mp4";

  while (iter != videoMap.end()) {
    auto video_enc = iter->second.get_encoded(vcl_container, vcl_codec);
    int size = video_enc.size();

    ASSERT_TRUE(!video_enc.empty());
    iter++;
  }
}

/**
 * Tests the working of the VideoLoop class
 * when a wrong url is provided for a remote operation.
 * The resulting video object should have an error message.
 */
TEST_F(VideoTest, VideoLoopTestError) {
  std::string _url = "http://localhost:5010/vide";
  Json::Value _options;
  _options["format"] = "mp4";
  _options["text"] = "Video";
  _options["id"] = "caption";

  std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                               "/video_test_VideoLoopTestError_input.avi");
  copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());

  std::string vloop_name_vcl("videos_tests/vloop_vcl.mp4");
  {
    VCL::Video video_data(temp_video_input);
    video_data.store(vloop_name_vcl, VCL::Video::Codec::H264);
  }

  VideoLoop videoLoop;
  VCL::Video video_data(vloop_name_vcl);

  video_data.remoteOperation(_url, _options);

  videoLoop.set_nrof_entities(1);

  videoLoop.enqueue(video_data);

  while (videoLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Video> videoMap = videoLoop.get_video_map();
  std::map<std::string, VCL::Video>::iterator iter = videoMap.begin();

  ASSERT_TRUE(iter->second.get_query_error_response() != "");
}

/**
 * Tests the working of the VideoLoop class
 * when a wrong url is provided for a synchronous remote operation.
 * The resulting video object should have an error message.
 */
TEST_F(VideoTest, VideoLoopSyncRemoteTestError) {
  std::string _url = "http://localhost:5010/vide";
  Json::Value _options;
  _options["format"] = "mp4";
  _options["text"] = "Video";
  _options["id"] = "caption";

  std::string temp_video_input(
      VDMS::VDMSConfig::instance()->get_path_tmp() +
      "/video_test_VideoLoopSyncRemoteTestError_input.avi");
  copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());

  std::string vloop_name_vcl("videos_tests/vloop_vcl.mp4");
  {
    VCL::Video video_data(temp_video_input);
    video_data.store(vloop_name_vcl, VCL::Video::Codec::H264);
  }

  VideoLoop videoLoop;
  VCL::Video video_data(vloop_name_vcl);

  video_data.syncremoteOperation(_url, _options);

  videoLoop.set_nrof_entities(1);

  videoLoop.enqueue(video_data);

  while (videoLoop.is_loop_running()) {
    continue;
  }

  std::map<std::string, VCL::Video> videoMap = videoLoop.get_video_map();
  std::map<std::string, VCL::Video>::iterator iter = videoMap.begin();

  ASSERT_TRUE(iter->second.get_query_error_response() != "");
}

TEST_F(VideoTest, KeyFrameExtractionSuccess) {
  try {
    VCL::VideoTest video_data(_video_path_mp4_h264);

    auto key_frame_list = video_data.get_key_frame_list();

    // We know that this video contains exactly four I-frames.
    // Changing the video will fail this test. If the functionality
    // is to be tested with other videos, either create a seperate test
    // or update the assertion below accordingly.
    ASSERT_TRUE(key_frame_list.size() == 4);

  } catch (VCL::Exception e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

TEST_F(VideoTest, KeyFrameExtractionFailure) {
  VCL::KeyFrameList key_frame_list;
  try {
    VCL::VideoTest video_data(_video_path_mp4_h264);

    key_frame_list = video_data.get_key_frame_list();

  } catch (VCL::Exception e) {
    ASSERT_TRUE(key_frame_list.empty());
  }
}

TEST_F(VideoTest, KeyFrameDecodingSuccess) {
  try {
    VCL::VideoTest video_data(_video_path_mp4_h264);

    VCL::KeyFrameList key_frame_list;
    // The base here is wrong for all keyframes, but is does not matter as
    // h.264 seeking is based on time (frame_idx * 1/fps) and not base.
    key_frame_list.push_back({.idx = 155, .base = 495756});
    key_frame_list.push_back({.idx = 0, .base = 564});
    key_frame_list.push_back({.idx = 201, .base = 648600});
    key_frame_list.push_back({.idx = 99, .base = 319224});

    video_data.set_key_frame_list(key_frame_list);

    std::vector<unsigned> frame_query = {15, 30, 110, 150};
    int first_query_len = frame_query.size();

    std::vector<cv::Mat> mat_list = video_data.get_frames(frame_query);
    ASSERT_TRUE(mat_list.size() == frame_query.size());

    frame_query.clear();

    frame_query = {100, 120, 130};
    int second_query_len = frame_query.size();
    for (auto &m : video_data.get_frames(frame_query))
      mat_list.push_back(m);
    ASSERT_TRUE(mat_list.size() == (first_query_len + second_query_len));

    for (int i = 0; i < mat_list.size(); ++i) {

      std::string s = std::to_string(i);
      s.insert(s.begin(), 5 - s.length(), '0');
      std::string filename = "videos_tests/kf_frame_" + s;

      VCL::Image img(mat_list[i], false);
      img.store(filename, VCL::Format::PNG, false);
    }

  } catch (VCL::Exception e) {
    ASSERT_TRUE(false);
  }
}

TEST_F(VideoTest, CheckDecodedSequentialFrames) {
  std::string video_to_test = _video_path_mp4_h264;

  cv::VideoCapture testVideo(video_to_test);
  long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

  try {
    VCL::VideoTest video_data_kf(video_to_test);
    video_data_kf.get_key_frame_list();

    VCL::VideoTest video_data_ocv(video_to_test);

    for (int i = 0; i < test_frame_count; ++i) {

      const unsigned frame_idx = i;

      cv::Mat decoded_with_keyframe;
      decoded_with_keyframe = video_data_kf.get_frame(frame_idx);

      cv::Mat decoded_with_opencv;
      decoded_with_opencv = video_data_ocv.get_frame(frame_idx);

      EXPECT_TRUE(compare_mat_mat(decoded_with_keyframe, decoded_with_opencv));
    }
  } catch (VCL::Exception e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

TEST_F(VideoTest, CheckDecodedRandomFrames) {
  std::string video_to_test = _video_path_mp4_h264;

  cv::VideoCapture testVideo(video_to_test);
  long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

  /* initialize random seed: */
  srand(24); // (time(NULL));

  try {
    VCL::VideoTest video_data_kf(video_to_test);
    video_data_kf.get_key_frame_list();

    VCL::VideoTest video_data_ocv(video_to_test);

    for (int i = 0; i < test_frame_count * 2; ++i) {

      // generate random number between 0 and test_frame_count
      // every 2 calls.
      int frame_idx;
      if (i % 2 == 0)
        frame_idx = rand() % test_frame_count;
      else
        frame_idx =
            frame_idx + 4 < test_frame_count ? frame_idx + 4 : frame_idx;

      cv::Mat decoded_with_keyframe;
      decoded_with_keyframe = video_data_kf.get_frame(frame_idx);

      cv::Mat decoded_with_opencv;
      decoded_with_opencv = video_data_ocv.get_frame(frame_idx);

      EXPECT_TRUE(compare_mat_mat(decoded_with_keyframe, decoded_with_opencv));
    }
  } catch (VCL::Exception e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Create a Video object of MP4 format and point to an existing file.
 * Imitates the VDMS read then store capability. Should have the same
 * frames as an OpenCV video object.
 */
TEST_F(VideoTest, WriteFromFilePath) {
  // TODO: Remove the GTEST_SKIP() sentences when this test is fixed
  GTEST_SKIP() << "Skipping WriteFromFilePath test due to issue when "
               << "comparing the frames of the videos for non remote tests";
  try {
    std::string uname = VCL::create_unique(OUTPUT_VIDEO_DIR + "/videos", "mp4");
    {
      VCL::Video video_data(_video_path_mp4_h264, true);
      video_data.store(uname, VCL::Video::Codec::H264);
    }

    // OpenCV writing the video H264
    std::string write_output_ocv("videos_tests/write_test_ocv.mp4");
    {
      copy_video_to_temp(_video_path_mp4_h264, write_output_ocv, get_fourcc());
    }

    VCL::Video video_data(_video_path_mp4_h264);
    long input_frame_count = video_data.get_frame_count();

    cv::VideoCapture testVideo(write_output_ocv);
    long test_frame_count = testVideo.get(cv::CAP_PROP_FRAME_COUNT);

    ASSERT_EQ(input_frame_count, test_frame_count);

    for (int i = 0; i < input_frame_count; ++i) {
      cv::Mat input_frame = video_data.get_frame(i);
      cv::Mat test_frame;
      testVideo >> test_frame;

      if (test_frame.empty())
        break; // should not happen

      EXPECT_TRUE(compare_mat_mat(input_frame, test_frame));
    }

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Error scenario when added file path is not accessible.
 */
TEST_F(VideoTest, FilePathAccessError) {
  try {
    std::string write_output_vcl("videos_tests/write_test_vcl.mp4");
    copy_video_to_temp(_video_path_mp4_h264, write_output_vcl, get_fourcc());
    std::string uname = VCL::create_unique(OUTPUT_VIDEO_DIR + "/videos", "mp4");
    {
      VCL::Video video_data(write_output_vcl, true);
      video_data.store(uname, VCL::Video::Codec::H264);
    }

    if (std::remove(write_output_vcl.data()) != 0) {
      throw VCLException(ObjectEmpty,
                         "Error encountered while removing the file.");
    }

    VCL::Video video_data(write_output_vcl);
    ASSERT_THROW(video_data.get_frame_count(), VCL::Exception);

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates performing a remote operation
 * and then storing the video in VDMS with metadata.
 * Metadata check is performed by this test.
 */
TEST_F(VideoTest, SyncRemoteWriteWithMetadata) {
  std::string _url = "http://localhost:5010/video";
  Json::Value _options;
  _options["format"] = "mp4";
  _options["id"] = "metadata";
  _options["media_type"] = "video";
  _options["otype"] = "face";
  _options["ingestion"] = 1;

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_SyncRemoteWriteMD_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_SyncRemoteWriteMD_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string syncremote_name_vcl("videos_tests/syncremotemd_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.syncremoteOperation(_url, _options);
      video_data.store(syncremote_name_vcl, VCL::Video::Codec::H264);
      ASSERT_TRUE(video_data.get_ingest_metadata().size() > 0);
      for (auto metadata : video_data.get_ingest_metadata()) {
        ASSERT_STREQ(metadata["1"]["bbox"]["object"].asString().data(), "face");
      }
    }

    // Cleanup temp files.
    if (std::remove(temp_video_input.data()) != 0) {
    }
    if (std::remove(temp_video_test.data()) != 0) {
    }

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}

/**
 * Imitates performing a user defined operation
 * and then storing the video in VDMS with metadata.
 * Metadata check is performed by this test.
 */
TEST_F(VideoTest, UDFWriteWithMetadata) {
  Json::Value _options;
  _options["port"] = 5555;
  _options["id"] = "metadata";
  _options["media_type"] = "video";
  _options["otype"] = "face";
  _options["format"] = "mp4";

  try {

    std::string temp_video_input(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                 "/video_test_UDFWrite_input.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_input, get_fourcc());
    std::string temp_video_test(VDMS::VDMSConfig::instance()->get_path_tmp() +
                                "/video_test_UDFWrite_test.avi");
    copy_video_to_temp(_video_path_avi_xvid, temp_video_test, get_fourcc());

    std::string udf_name_vcl("videos_tests/udf_vcl.mp4");
    {
      VCL::Video video_data(temp_video_input); //
      video_data.userOperation(_options);
      video_data.store(udf_name_vcl, VCL::Video::Codec::H264);
      for (auto metadata : video_data.get_ingest_metadata()) {
        ASSERT_STREQ(metadata["1"]["bbox"]["object"].asString().data(), "face");
      }
    }

    // Cleanup temp files.
    if (std::remove(temp_video_input.data()) != 0) {
    }
    if (std::remove(temp_video_test.data()) != 0) {
    }

  } catch (VCL::Exception &e) {
    print_exception(e);
    ASSERT_TRUE(false);
  }
}