/**
 * @file   VideoData.h
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
 * This file declares the C++ API for VideoData. VideoData contains all of the
 *  messy details about the Video that aren't visible in Video.h/Video.cc. It
 *  keeps track of which operations to perform (and in what order), as well as
 *  the raw data if it is in a CV Mat, and a pointer to the TDB raw data if it is
 *  in TDB format.
 */

#pragma once

#include <memory> // For shared_ptr

#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "vcl/VCL.h"

namespace VCL {

    class VideoData {

    /*  *********************** */
    /*        OPERATION         */
    /*  *********************** */

        enum OperationType { READ, WRITE, RESIZE, CROP, THRESHOLD, INTERVAL };

        /**
         *  Provides a way to keep track of what operations should
         *   be performed on the data when it is needed
         *
         *  Operation is the base class, it keeps track of the format
         *   of the Video data, defines a way to convert Format to
         *   a string, and defines a virtual function that overloads the
         *   () operator
         */
        class Operation {

        public:

            /**
             *  Implemented by the specific operation, performs what
             *    the operation is supposed to do
             *
             *  @param img  A pointer to the current VideoData object
             */
            virtual void operator()(VideoData *video) = 0;

            virtual OperationType get_type() = 0;
        };

    /*  *********************** */
    /*       READ OPERATION     */
    /*  *********************** */

        /**
         *  Extends Operation, reads Video from the file system
         */
        class Read : public Operation {

            Video::Codec read_codec(char* fourcc);

        public:

            /**
             *  Reads an Video from the file system (based on specified path)
             *
             */
            void operator()(VideoData *video);

            OperationType get_type() { return READ; };
        };

    /*  *********************** */
    /*       WRITE OPERATION    */
    /*  *********************** */
        /**
         *  Extends Operation, writes to the file system in the specified
         *    format
         */
        class Write : public Operation {
        private:
            std::string  _outname;
            Video::Codec _codec;

            int get_fourcc();

        public:

            Write(std::string outname, Video::Codec codec) :
                _outname(outname), _codec(codec)
            {
            }

            /**
             *  Writes an Video to the file system.
             *
             */
            void operator()(VideoData *video);

            OperationType get_type() { return WRITE; };
        };

    /*  *********************** */
    /*       RESIZE OPERATION   */
    /*  *********************** */
        /**
         *  Extends Operation, resizes the Video to the specified size
         */
         class Resize : public Operation {
         private:
            /** Gives the height and width to resize the Video to */
            cv::Size _size;

            public:
            /**
             *  Constructor, sets the size to resize to and the format
             *
             *  @param size  Struct that contains w and h
             */
            Resize(const cv::Size &size) : _size(size)
            {
            };

            /**
             *  Resizes an Video to the given dimensions
             *
             *  @param video  A pointer to the current VideoData object
             */
            void operator()(VideoData *video);

            OperationType get_type() { return RESIZE; };
        };

                    /*  *********************** */
                    /*      Interval Operation  */
                    /*  *********************** */

        class Interval : public Operation {
        private:
            int _start;
            int _stop;
            int _step;
            Video::Unit _u;

        public:
            /**
             *  Constructor, sets the size to resize to and the format
             *
             *  @param u  Unit used for interval operation
             *  @param start First frame
             *  @param stop  Last frame
             *  @param step  Number of frames to be skipped in between.
             */
            Interval(Video::Unit u, const int start , const int stop, int step)
                : _u(u),
                  _start(start),
                  _stop(stop),
                  _step(step)
            {
            };

            /**
             *  Resizes an Video to the given dimensions
             *
             *  @param video  A pointer to the current VideoData object
             */
            void operator()(VideoData *video);

            OperationType get_type() { return INTERVAL; };
        };

    /*  *********************** */
    /*       CROP OPERATION     */
    /*  *********************** */

        /**
         *  Extends Operation, crops the Video to the specified area
         */
         class Crop : public Operation {
         private:
            /** Gives the dimensions and coordinates of the desired area */
            Rectangle _rect;

            public:
            /**
             *  Constructor, sets the area to crop to and the format
             *
             *  @param rect  Contains dimensions and coordinates of
             *    desired area
             */
            Crop(const Rectangle &rect )
                : _rect(rect)
            {
            };

            /**
             *  Crops the Video to the given area
             *
             *  @param video  A pointer to the current VideoData object
             */
            void operator()(VideoData *video);

            OperationType get_type() { return CROP; };
        };

    /*  *********************** */
    /*    THRESHOLD OPERATION   */
    /*  *********************** */

        /**  Extends Operation, performs a thresholding operation that
         *     discards the pixel value if it is less than or equal to the
         *     threshold and sets that pixel to 0
         */
        class Threshold : public Operation {
        private:
            /** Minimum value pixels should be */
            int _threshold;

        public:
            /**
             *  Constructor, sets the threshold value and format
             *
             *  @param value  Minimum value pixels should be
             */
            Threshold(const int value)
                : _threshold(value)
            {
            };

            /**
             *  Performs the thresholding operation
             *
             *  @param img  A pointer to the current VideoData object
             */
            void operator()(VideoData *video);

            OperationType get_type() { return THRESHOLD; };
        };

    private:

        // Full path to the video file.
        // It is called _video_id to keep it consistent with VCL::Image
        std::string _video_id;

        bool _flag_stored; // Flag to avoid unnecessary read/write

        std::vector<VCL::Image> _frames;

        unsigned _frame_count;
        unsigned _frame_width;
        unsigned _frame_height;

        float _fps;

        Video::Codec _codec; // (h.264, etc).

        std::vector<std::shared_ptr<Operation>> _operations;

    protected:

        /**
         *  Get a reference to the vector of frames.
         *
         *  @return Reference to vector of frames (as VCL::Image).
         */
        std::vector<VCL::Image>& get_frames() { return _frames; }

        /**
         *  Stores a Read Operation in the list of operations
         *    to perform
         *
         *  @param Video_id  The full path to the Video to be read
         */
        void read(const std::string &video_id );

        /**
         *  Performs the set of operations that have been requested
         *  on the VideoData
         */
        void perform_operations();

    public:

    /*  *********************** */
    /*        CONSTRUCTORS      */
    /*  *********************** */

        /**
         *  Default constructor, creates an empty VideoData object.
         *    Used when reading from the file system
         */
        VideoData();
        /**
         *  Creates an VideoData object from the filename
         *
         *  @param video_id  A string indicating where the Video is on disk
         */
        VideoData(const std::string& video_id);

        /**
         *  Creates an VideoData object from buffer
         *
         *  A path must be specified for the video to be written in disk
         *  before reading the buffer.
         *  This is because OpenCV does not offer API for encoding/decoding
         *  from/to memory.
         */
        VideoData(void*  buffer, long size);

        /**
         *  Creates an VideoData object from an existing VideoData object
         *
         *  @param video  A reference to an existing VideoData object
         */
        VideoData(const VideoData& video);

        /**
         *  Sets an VideoData object equal to another VideoData object
         *
         *  @param video  A reference to an existing VideoData object
         */
        void operator=(const VideoData &video);

    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */

        /**
         *  Gets the path set on the VideoData object
         *
         *  @return The string containing the path to the VideoData object
         */
        std::string get_video_id() const { return _video_id; }

        /**
         *  Gets the codec used.
         *
         *  @return Video::Codec
         */
        Video::Codec get_codec() const { return _codec; };

        /**
         *  Gets the size of the Video in pixels (height * width * channels)
         *
         *  @return The size of the Video in pixels
         */
        Video::VideoSize get_size();

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
        float get_fps() { return _fps; };

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
        void set_video_id(const std::string &Video_id);

        /**
         *  Sets the codec used for writing the video to file.
         *
         *  @param codec  supported codec
         */
        void set_codec(Video::Codec codec);

        /**
         *  Sets the height and width of the Video
         *
         *  @param dimensions  The height and width of the Video
         *  @see OpenCV documentation for more details on Size
         */
        void set_dimensions(const cv::Size& dimensions);

    /*  *********************** */
    /*   VideoData INTERACTION  */
    /*  *********************** */

        void resize(int rows, int columns);

        void interval(Video::Unit u, int start, int stop, int step);

        void crop(const Rectangle &rect);

        void threshold(int value);

        /**
         *  Stores a Write Operation in the list of operations to perform.
         *  This method updates the video_id and codec specified.
         *
         *  @param video_id  The path to where the Video should be written
         *  @param video_codec Supported codec to be used
         *  @see Video.h for more details on Video::Codec
         */
        void write(const std::string &video_id,  Video::Codec video_codec);

        /**
         *  Stores a Write Operation in the list of operations to perform.
         *  This method will used the video_id and codec already defined.
         */
        void write();
    };

} // namespace VCL
