/**
 * @file   Video.cc
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
#include "VideoData.h"

using namespace VCL;

Video::Video()
{
    _video = new VideoData();
}

Video::Video(const std::string& fileName)
{
    _video = new VideoData(fileName);
}

Video::Video(void* buffer, long size)
{
    _video = new VideoData(buffer, size);
}

Video::Video(const Video &video)
{
    _video = new VideoData(*video._video);
}

Video::~Video()
{
    delete _video;
}

void Video::operator=(const Video &vid)
{
    delete _video;
    _video = new VideoData(*vid._video);
}

    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */

std::string Video::get_video_id() const
{
    return _video->get_video_id();
}

Video::Codec Video::get_codec() const
{
    return _video->get_codec();
}

cv::Mat Video::get_frame(unsigned frame_number)
{
    return _video->get_frame(frame_number);
}

long Video::get_frame_count()
{
    return _video->get_frame_count();
}

float Video::get_fps()
{
    return _video->get_fps();
}

cv::Size Video::get_frame_size()
{
    return _video->get_frame_size();
}

Video::VideoSize Video::get_size()
{
    return _video->get_size();
}

std::vector<unsigned char> Video::get_encoded()
{
    return _video->get_encoded();
}

    /*  *********************** */
    /*        SET FUNCTIONS     */
    /*  *********************** */

void Video::set_video_id(const std::string &video_id)
{
    _video->set_video_id(video_id);
}

void Video::set_codec(Video::Codec codec)
{
    _video->set_codec(codec);
}

void Video::set_dimensions(const cv::Size& dimensions)
{
    _video->set_dimensions(dimensions);
}

    /*  *********************** */
    /*   Video INTERACTION      */
    /*  *********************** */

void Video::resize(int new_height, int new_width)
{
    _video->resize(new_height, new_width);
}

void Video::interval(Video::Unit u, int start, int stop, int step)
{
    _video->interval(u, start, stop, step);
}

void Video::crop(const Rectangle &rect)
{
    _video->crop(rect);
}

void Video::threshold(int value)
{
    _video->threshold(value);
}

void Video::store(const std::string &video_id, Video::Codec video_codec)
{
    _video->write(video_id, video_codec);
}

void Video::delete_video()
{
    std::string fname = _video->get_video_id();
    if (exists(fname)) {
        std::remove(fname.c_str());
    }
}
