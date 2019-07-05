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

#include <list>
#include <memory> // For shared_ptr
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "vcl/Image.h"
#include "vcl/KeyFrameParser.h"

#include "Exception.h"
#include "utils.h"

namespace VCL {

    typedef cv::Rect Rectangle; // spcifiy an ROI inside a video

    class Video {

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

        enum Unit { FRAMES = 0, SECONDS = 1 };

        enum OperationType { READ, WRITE, RESIZE, CROP, THRESHOLD, INTERVAL };

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
         *  @param video  A copy of an existing Video object. The parameter
         *                is passed by value to exploit copy-and-swap idiom
         *                to reduce code duplication (copy-constructor) and
         *                safer exception handling
         */
        Video& operator=(Video video);

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

        /**
         *  Invokes key-frame generation on the video, if the video is encoded
         *  with a H264 encoder. Index, and byte offset/length of each key
         *  frame is stored. This operation is independent of other prior
         *  visual operations that may have been applied.
         *
         * @return List of KeyFrame objects
         */
        const KeyFrameList& get_key_frame_list();

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
     *  @param width  Number of columns
     *  @param height Number of rows
     */
    void resize(int width, int height);

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
     *  Stores a Write Operation in the list of operations, performs the
     *  operations that are already in the list, and finally writes the
     *  video to the disk.
     *  This method will used when the video_id and codec are already defined.
     */
    void store();

    /**
     *  Deletes the Video file
     */
    void delete_video();

    private:

        class Operation;

        // Forward declaration of VideoTest class, that is used for the unit
        // test to accesss private methods of this class
        friend class VideoTest;

        // Full path to the video file.
        // It is called _video_id to keep it consistent with VCL::Image
        std::string _video_id;

        bool _flag_stored; // Flag to avoid unnecessary read/write

        std::vector<VCL::Image> _frames;

        VideoSize _size;

        float _fps;

        Codec _codec; // (h.264, etc).

        // List of key frames, filled only when KeyFrames operation is applied
        KeyFrameList _key_frame_list;

        std::list<std::shared_ptr<Operation>> _operations;

    /*  *********************** */
    /*        OPERATION         */
    /*  *********************** */

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
             *  @param img  A pointer to the current Video object
             */
            virtual void operator()(Video *video) = 0;

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
            void operator()(Video *video);

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
            void operator()(Video *video);

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
             *  @param video  A pointer to the current Video object
             */
            void operator()(Video *video);

            OperationType get_type() { return RESIZE; };
        };

    /*  *********************** */
    /*      INTERVAL OPERATION  */
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
             *  @param video  A pointer to the current Video object
             */
            void operator()(Video *video);

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
             *  @param video  A pointer to the current Video object
             */
            void operator()(Video *video);

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
             *  @param img  A pointer to the current Video object
             */
            void operator()(Video *video);

            OperationType get_type() { return THRESHOLD; };
        };

    protected:
    /*  *********************** */
    /*       UTILITIES          */
    /*  *********************** */
        /**
         *  Checks whether the video pointed by the current video_id has
         *  already been read.
         *
         * @return true if video was read, false otherwise
         */
        bool is_read(void);

        /**
         *  Performs the set of operations that have been requested
         *  on the Video
         */
        void perform_operations();

        /**
         * Swaps members of two Video objects, to be used by assignment
         * operator.
         */
        void swap(Video& rhs) noexcept;
};

} // namespace VCL
