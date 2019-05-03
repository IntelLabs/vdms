/**
 * @file   VideoData.cc
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

#include <fstream>
#include <algorithm>

#include <iostream> // cout

#include "vcl/VCL.h"
#include "VideoData.h"

using namespace VCL;

Video::Codec VideoData::Read::read_codec(char* fourcc)
{
    std::string codec(fourcc);
    std::transform(codec.begin(), codec.end(), codec.begin(), ::tolower);

    if (codec == "mjpg")
        return Video::Codec::MJPG;
    else if (codec == "xvid")
        return Video::Codec::XVID;
    else if (codec == "u263")
        return Video::Codec::H263;
    else if (codec == "avc1" || codec == "x264")
        return Video::Codec::H264;
    else
        throw VCLException(UnsupportedFormat, codec + " is not supported");
}

void VideoData::Read::operator()(VideoData *video)
{
    cv::VideoCapture inputVideo(video->get_video_id());

    video->_fps = static_cast<float>(inputVideo.get(CV_CAP_PROP_FPS));
    video->_frame_count  = static_cast<int>(
                            inputVideo.get(CV_CAP_PROP_FRAME_COUNT));
    video->_frame_width  = static_cast<int>(
                            inputVideo.get(CV_CAP_PROP_FRAME_WIDTH));
    video->_frame_height =  static_cast<int>(
                            inputVideo.get(CV_CAP_PROP_FRAME_HEIGHT));

    // Get Codec Type- Int form
    int ex = static_cast<int>(inputVideo.get(CV_CAP_PROP_FOURCC));
    char fourcc[] = {(char)((ex & 0XFF)),
                     (char)((ex & 0XFF00) >> 8),
                     (char)((ex & 0XFF0000) >> 16),
                     (char)((ex & 0XFF000000) >> 24),
                     0};

    video->_codec = read_codec(fourcc);

    std::vector<VCL::Image>& frames = video->get_frames();
    frames.clear();

    // Read all the frames
    // TODO, If interval operation is specified, it make sense to
    // apply this here as well, as will prevent copying some frames.

    while(true) {

        cv::Mat mat_frame;
        inputVideo >> mat_frame; // Read frame

        if (mat_frame.empty())
            break;

        frames.push_back(VCL::Image(mat_frame));
    }

    inputVideo.release();
}

    /*  *********************** */
    /*       WRITE OPERATION    */
    /*  *********************** */

int VideoData::Write::get_fourcc()
{
    switch(_codec)
    {
        case Video::Codec::MJPG:
            return CV_FOURCC('M','J','P','G');
        case Video::Codec::XVID:
            return CV_FOURCC('X', 'V', 'I', 'D');
        case Video::Codec::H263:
            return CV_FOURCC('U', '2', '6', '3');
        case Video::Codec::H264:
            return CV_FOURCC('X', '2', '6', '4');
        case Video::Codec::AVC1:
            return CV_FOURCC('A', 'V', 'C', '1');
        default:
            throw VCLException(UnsupportedFormat, std::to_string((int)_codec) +
                               " is not a valid format");
    }
}

void VideoData::Write::operator()(VideoData *video)
{
    cv::VideoWriter outputVideo(
                    _outname,
                    get_fourcc(),
                    video->_fps,
                    cv::Size(video->_frame_width, video->_frame_height));

    if (!outputVideo.isOpened()) {
        throw VCLException(OpenFailed,
                "Could not open the output video for write");
    }

    std::vector<VCL::Image>& frames = video->get_frames();

    for (auto& frame : frames) {
        outputVideo << frame.get_cvmat(false);
    }
    outputVideo.release();

    video->_video_id = _outname;
    video->_codec    = _codec;
    video->_flag_stored = true;
}

    /*  *********************** */
    /*       RESIZE OPERATION   */
    /*  *********************** */

void VideoData::Resize::operator()(VideoData *video)
{
    std::vector<VCL::Image>& frames = video->get_frames();

    for (auto& frame : frames) {
        // VCL::Image expect the params (h,w) (contrary to openCV convention)
        frame.resize(_size.height, _size.width);
    }

    video->_frame_width  = _size.width;
    video->_frame_height = _size.height;
}

    /*  *********************** */
    /*       CROP OPERATION     */
    /*  *********************** */

void VideoData::Crop::operator()(VideoData *video)
{
    std::vector<VCL::Image>& frames = video->get_frames();

    for (auto& frame : frames) {
        frame.crop(_rect);
    }

    video->_frame_width  = _rect.width;
    video->_frame_height = _rect.height;
}

    /*  *********************** */
    /*    THRESHOLD OPERATION   */
    /*  *********************** */

void VideoData::Threshold::operator()(VideoData *video)
{
    std::vector<VCL::Image>& frames = video->get_frames();

    for (auto& frame : frames) {
        frame.threshold(_threshold);
    }
}

    /*  *********************** */
    /*   INTERVAL Operation     */
    /*  *********************** */

void VideoData::Interval::operator()(VideoData *video)
{
    if (_u != Video::Unit::FRAMES)
        throw VCLException(UnsupportedOperation,
                "Only Unit::FRAMES supported for interval operation");

    std::vector<VCL::Image>& frames = video->get_frames();
    unsigned nframes = frames.size();

    if (_start >= nframes)
        throw VCLException(SizeMismatch,
                "Start Frame cannot be greater than number of frames");

    if (_stop >= nframes)
        throw VCLException(SizeMismatch,
                "End Frame cannot be greater than number of frames");

    std::vector<VCL::Image> interval_vector;

    for (int i = _start; i < _stop; i += _step) {
        interval_vector.push_back(frames[i]);
    }

    frames.insert(frames.begin(), interval_vector.begin(),
                                  interval_vector.end());
    frames.erase(frames.begin() + interval_vector.size(), frames.end());

    video->_fps /= _step;
    video->_frame_count = interval_vector.size();
}

    /*  *********************** */
    /*        CONSTRUCTORS      */
    /*  *********************** */

VideoData::VideoData() :
    _frame_count(0),
    _fps(0),
    _frame_width(0),
    _frame_height(0),
    _video_id(""),
    _flag_stored(true),
    _codec(Video::Codec::NOCODEC)
{
}

VideoData::VideoData(const std::string &video_id) :
    VideoData()
{
    _video_id = video_id;
    read(_video_id); // push the read operation
}

VideoData::VideoData(void* buffer, long size) :
    VideoData()
{
    std::string uname = create_unique("/tmp/", "vclvideoblob");
    std::ofstream outfile(uname, std::ofstream::binary);

    if (outfile.is_open()) {
        outfile.write((char*)buffer, size);
        outfile.close();
    }
    else
        throw VCLException(OpenFailed, "Cannot create temporary file");

    _video_id = uname;
    read(_video_id); // push the read operation
}

VideoData::VideoData(const VideoData &video) : VideoData()
{
    _video_id = video._video_id;

    _frame_width  = video._frame_width;
    _frame_height = video._frame_height;
    _frame_count  = video._frame_count;

    _fps   = video._fps;
    _codec = video._codec;

    _video_id = video.get_video_id();
    _codec    = video.get_codec();

    _flag_stored = video._flag_stored;

    _frames     = video._frames;
    _operations = video._operations;

    for (int i = 0; i < video._operations.size(); ++i)
        _operations.push_back(video._operations[i]);
}

void VideoData::operator=(const VideoData &video)
{
}

    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */

cv::Mat VideoData::get_frame(unsigned frame_number)
{
    perform_operations();
    if (frame_number >= _frame_count)
        throw VCLException(OutOfBounds, "Frame requested is out of bounds");

    return _frames.at(frame_number).get_cvmat();
}

long VideoData::get_frame_count()
{
    perform_operations();
    return _frame_count;
}

cv::Size VideoData::get_frame_size()
{
    perform_operations();
    cv::Size dims((int) _frame_width,
                  (int) _frame_height);
    return dims;
}

Video::VideoSize VideoData::get_size()
{
    perform_operations();
    Video::VideoSize vsize;
    vsize.width        = _frame_width;
    vsize.height       = _frame_height;
    vsize.frame_count  = _frame_count;

    return vsize;
}

std::vector<unsigned char> VideoData::get_encoded()
{
    if (_flag_stored == false)
        throw VCLException(ObjectEmpty, "Object not written");

    std::ifstream ifile(_video_id, std::ifstream::in);
    ifile.seekg(0, std::ios::end);
    size_t encoded_size = (long)ifile.tellg();
    ifile.seekg(0, std::ios::beg);

    std::vector<unsigned char> encoded(encoded_size);

    ifile.read((char*)encoded.data(), encoded_size);
    ifile.close();

    return encoded;
}

    /*  *********************** */
    /*        SET FUNCTIONS     */
    /*  *********************** */

void VideoData::set_video_id(const std::string &video_id)
{
    _video_id = video_id;
}

void VideoData::set_codec(Video::Codec codec)
{
    _codec = codec;
}

void VideoData::set_dimensions(const cv::Size& dimensions)
{
    _frame_height = dimensions.height;
    _frame_width  = dimensions.width;
}

    /*  *********************** */
    /*   VideoData INTERACTION  */
    /*  *********************** */

void VideoData::perform_operations()
{
    try
    {
        for (int x = 0; x < _operations.size(); ++x) {
            std::shared_ptr<Operation> op = _operations[x];
            if ( op == NULL )
                throw VCLException(ObjectEmpty, "Nothing to be done");
            (*op)(this);
        }
    } catch( cv::Exception& e ) {
        throw VCLException(OpenCVError, e.what());
    }

    _operations.clear();
}

void VideoData::read(const std::string &video_id)
{
    _video_id = video_id;
    _operations.push_back(std::make_shared<Read>());
}

void VideoData::resize(int width, int height)
{
    _flag_stored = false;
    _operations.push_back(std::make_shared<Resize>(cv::Size(width, height)));
}

void VideoData::interval(Video::Unit u, int start, int stop, int step)
{
    _flag_stored = false;
    _operations.push_back(std::make_shared<Interval>(u, start, stop, step));
}

void VideoData::crop(const Rectangle &rect)
{
    _flag_stored = false;
    _operations.push_back(std::make_shared<Crop>(rect));
}

void VideoData::threshold(int value)
{
    _flag_stored = false;
    _operations.push_back(std::make_shared<Threshold>(value));
}

void VideoData::write(const std::string& out_name, Video::Codec codec)
{
    // out_name cannot be assigned to _video_id here as the read operation
    // may be pending and the input file name is needed for the read.
    _operations.push_back(std::make_shared<Write>(out_name, codec));
    perform_operations();
}

void VideoData::write()
{
    write(_video_id, _codec);
}
