/**
 * @file   Video.h
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
 * @section DESCRIPTION
 *
 */

#pragma once

#include <string>

#include <opencv2/core.hpp>

#include "Exception.h"
#include "utils.h"

namespace VCL {

    class VideoData; // To hide the Video implementation details

    typedef cv::Rect Rectangle; // spcifiy an ROI inside a video

    class Video {

    private:

        VideoData *_video; // Pointer to a VideoData object

    public:

        enum  Codec { NOCODEC = 0,
                      MJPG,
                      XVID,
                      H263,
                      H264,
                      AVC1 };

        struct VideoSize {  unsigned width;
                            unsigned height;
                            unsigned frame_count; };

        enum  Unit { FRAMES = 0, SECONDS = 1 };

    /*  *********************** */
    /*        CONSTRUCTORS      */
    /*  *********************** */

        /**
         *  Default constructor, creates an empty Video object.
         *    Used when reading from the file system
         */
        Video();

        /**
         *  Creates an Video object from the filename
         *
         *  @param video_id  A string indicating where the Video is on disk
         */
        Video(const std::string &video_id);

        /**
         *  Creates an Video object from an existing Video object
         *
         *  @param video  A reference to an existing Video object
         */
        Video(const Video &video);

        /**
         *  Creates an Video object from buffer
         *
         *  A path must be specified for the video to be written in disk
         *  before reading the buffer.
         *  This is because OpenCV does not offer API for encoding/decoding
         *  from/to memory.
         */
        Video(void* buffer, long size);

        /**
         *  Sets an Video object equal to another Video object
         *
         *  @param video  A reference to an existing Video object
         */
        void operator=(const Video &video);

        ~Video();

    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */

        /**
         *  Gets the path set on the Video object
         *
         *  @return The string containing the path to the Video object
         */
        std::string get_video_id() const;

        /**
         *  Gets the codec used.
         *
         *  @return Codec
         */
        Codec get_codec() const;

        /**
         *  Gets the size of the Video in pixels (height * width * channels)
         *
         *  @return The size of the Video in pixels
         */
        VideoSize get_size();

         /**
         *  Gets the dimensions (height and width) of the Video
         *
         *  @return The height and width of the Video as an OpenCV Size object
         */
        cv::Size get_frame_size();

        /**
         *  Gets number of frames in the video
         *
         *  @return Number of frames in the video
         */
        long get_frame_count();

        /**
         *  Gets frames per second.
         *
         *  @return Frames per second
         */
        float get_fps();

        /**
         *  Gets one specific frame from the video
         *
         *  @return cv::Mat with the specified frame
         */
        cv::Mat get_frame(unsigned frame_num);

        /**
         *  Gets encoded Video data in a buffer
         *  Before calling this method, the store method must be called,
         *  as OpenCV only offers interfaces from encoding/decoding
         *  from/to files.
         *
         */
        std::vector<unsigned char> get_encoded();

    /*  *********************** */
    /*        SET FUNCTIONS     */
    /*  *********************** */

        /**
         *  Sets the file system location of where the Video
         *    can be found
         *
         *  @param Video_id  The full path to the Video location, including extension (container)
         */
        void set_video_id(const std::string &video_id);

        /**
         *  Sets the codec used for writing the video to file.
         *
         *  @param codec  supported codec
         */
        void set_codec(Codec codec);

        /**
         *  Sets the height and width of the Video
         *
         *  @param dimensions  The height and width of the Video
         *  @see OpenCV documentation for more details on Size
         */
        void set_dimensions(const cv::Size& dimensions);

    /*  *********************** */
    /*    Video INTERACTIONS    */
    /*  *********************** */

    /**
     *  Resizes the Video to the given size. This operation is not
     *    performed until the data is needed (ie, store is called or
     *    one of the get_ functions such as get_cvmat)
     *
     *  @param new_height Number of rows
     *  @param new_width  Number of columns
     */
    void resize(int new_height, int new_width);

    /**
     *  Crops the Video to the area specified. This operation is not
     *    performed until the data is needed (ie, store is called or
     *    one of the get_ functions such as get_cvmat)
     *
     *  @param rect  The region of interest (starting x coordinate,
     *    starting y coordinate, height, width) the video should be
     *    cropped to
     */
    void crop(const Rectangle &rect);

    /**
     *  Performs a thresholding operation on the Video. Discards the pixel
     *    value if it is less than or equal to the threshold and sets that
     *    pixel to zero. This operation is not performed until the data
     *    is needed (ie, store is called or one of the get_ functions
     *    such as get_cvmat)
     *
     *  @param value  The threshold value
     */
    void threshold(int value);

    /**
     *  Modifies the number of frames in the video.
     *  Frames 0 to start-1 are dropped.
     *  Frames stop to last are dropped.
     *  Step-1 frames are dropped in between.
     *  Note: Only FRAMES as united is supported.
     *
     *  @param unit  Unit used for specifying rest of params
     *  @param start New Starting frame
     *  @param stop  New End frame
     *  @param step  Decimation for frames
     */
    void interval(Unit u, int start, int stop, int step = 1);

    /**
     *  Writes the Video to the system at the given location and in
     *    the given format
     *
     *  @param video_id  Full path to where the video should be written
     *  @param video_codec  Codec in which to write the video
     */
    void store(const std::string &video_id, Codec video_codec);

    /**
     *  Deletes the Video file
     */
    void delete_video();
};

} // namespace VCL
