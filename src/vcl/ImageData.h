/**
 * @file   ImageData.h
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
 * This file declares the C++ API for ImageData. ImageData contains all of the
 *  messy details about the Image that aren't visible in Image.h/Image.cc. It
 *  keeps track of which operations to perform (and in what order), as well as
 *  the raw data if it is in a CV Mat, and a pointer to the TDB raw data if it is
 *  in TDB format.
 */

#pragma once

#include <memory>

#include "vcl/Image.h"
#include "TDBImage.h"

namespace VCL {

    class ImageData {

    /*  *********************** */
    /*        OPERATION         */
    /*  *********************** */
        enum OperationType { READ, WRITE, RESIZE, CROP, THRESHOLD,
                             FLIP, ROTATE };

        /**
         *  Provides a way to keep track of what operations should
         *   be performed on the data when it is needed
         *
         *  Operation is the base class, it keeps track of the format
         *   of the image data, defines a way to convert Format to
         *   a string, and defines a virtual function that overloads the
         *   () operator
         */
        class Operation {
        protected:
            /** The format of the image for this operation */
            Image::Format _format;

            /**
             *  Constructor, sets the format
             *
             *  @param format  The format for the operation
             *  @see Image.h for more details on Format
             */
            Operation(Image::Format format)
                : _format(format)
            {
            };

        public:
            /**
             *  Implemented by the specific operation, performs what
             *    the operation is supposed to do
             *
             *  @param img  A pointer to the current ImageData object
             */
            virtual void operator()(ImageData *img) = 0;

            virtual OperationType get_type() = 0;
        };

    /*  *********************** */
    /*       READ OPERATION     */
    /*  *********************** */
        /**
         *  Extends Operation, reads image from the file system
         */
        class Read : public Operation {
        private:
            /** The full path to the object to read */
            std::string _fullpath;

        public:
            /**
             *  Constructor, sets the format and path for reading
             *
             *  @param filename  The full path to read from
             *  @param format  The format to read the image from
             *  @see Image.h for more details on Image::Format
             */
            Read(const std::string& filename, Image::Format format);

            /**
             *  Reads an image from the file system (based on the format
             *    and file path indicated)
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);


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
            /** The full path of where to write the image */
            std::string _fullpath;
            /** The format the image used to be stored as */
            Image::Format _old_format;
            /** Whether to store the metadata */
            bool _metadata;

        public:
            /**
             *  Constructor, sets the formats and path for writing
             *
             *  @param filename  The full path to write to
             *  @param format  The format to store the image in
             *  @param old_format  The format the image was stored in
             *  @see Image.h for more details on Image::Format
             */
            Write(const std::string& filename, Image::Format format,
                Image::Format old_format, bool metadata);
            /**
             *  Writes an image to the file system (based on the format
             *    and file path indicated)
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);

            OperationType get_type() { return WRITE; };
        };

    /*  *********************** */
    /*       RESIZE OPERATION   */
    /*  *********************** */
        /**
         *  Extends Operation, resizes the image to the specified size
         */
         class Resize : public Operation {
         private:
            /** Gives the height and width to resize the image to */
            Rectangle _rect;

        public:
            /**
             *  Constructor, sets the size to resize to and the format
             *
             *  @param rect  Contains height and width to resize to
             *  @param format  The current format of the image data
             *  @see Image.h for more details on Image::Format and Rectangle
             */
            Resize(const Rectangle &rect, Image::Format format)
                : Operation(format),
                  _rect(rect)
            {
            };

            /**
             *  Resizes an image to the given dimensions
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);

            OperationType get_type() { return RESIZE; };
        };

    /*  *********************** */
    /*       CROP OPERATION     */
    /*  *********************** */
        /**
         *  Extends Operation, crops the image to the specified area
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
             *  @param format  The current format of the image data
             *  @see Image.h for more details on Image::Format and Rectangle
             */
            Crop(const Rectangle &rect, Image::Format format)
                : Operation(format),
                  _rect(rect)
            {
            };

            /**
             *  Crops the image to the given area
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);

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
             *  @param format  The current format of the image data
             *  @see Image.h for more details on Image::Format
             */
            Threshold(const int value, Image::Format format)
                : Operation(format),
                  _threshold(value)
            {
            };

            /**
             *  Performs the thresholding operation
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);

            OperationType get_type() { return THRESHOLD; };
        };

    /*  *********************** */
    /*    FLIP OPERATION   */
    /*  *********************** */
        /**  Extends Operation, performs a flip operation that
         */
        class Flip : public Operation {
        private:
            /** Minimum value pixels should be */
            int _code;

        public:
            /**
             *  Constructor, sets the flip code value.
             *
             *  @param code  Type of flipping operation
             *  @param format  The current format of the image data
             *  @see Image.h for more details on Image::Format
             */
            Flip(const int code, Image::Format format)
                : Operation(format),
                  _code(code)
            {
            };

            /**
             *  Performs the flip operation
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);

            OperationType get_type() { return FLIP; };
        };

    /*  *********************** */
    /*    ROTATE OPERATION   */
    /*  *********************** */
        /**  Extends Operation, performs a flip operation that
         */
        class Rotate : public Operation {
        private:
            /** Minimum value pixels should be */
            float _angle;
            bool _keep_size;

        public:
            /**
             *  Constructor, sets the flip code value.
             *
             *  @param format  The current format of the image data
             *  @see Image.h for more details on Image::Format
             */
            Rotate(const float angle, const bool keep_size, Image::Format format)
                : Operation(format),
                  _angle(angle),
                  _keep_size(keep_size)
            {
            };

            /**
             *  Performs the flip operation
             *
             *  @param img  A pointer to the current ImageData object
             */
            void operator()(ImageData *img);

            OperationType get_type() { return ROTATE; };
        };

    private:
    /*  *********************** */
    /*        VARIABLES         */
    /*  *********************** */
        // Image height and width
        uint _height, _width;

        // Type of image (OpenCV definition) and number of channels
        int _cv_type, _channels;

        // Maintains order of operations requested
        std::vector<std::shared_ptr<Operation>> _operations;

        // Image format and compression type
        Image::Format _format;
        CompressionType _compress;

        // Full path to image
        std::string _image_id;

        // Image data (OpenCV Mat or TDBImage)
        cv::Mat _cv_img;
        TDBImage *_tdb;

    public:
    /*  *********************** */
    /*        CONSTRUCTORS      */
    /*  *********************** */
        /**
         *  Default constructor, creates an empty ImageData object.
         *    Used when reading from the file system
         */
        ImageData();

       /**
         *  Creates an ImageData object from the OpenCV Mat,
         *  making a deep copy.
         *
         *  @param cv_img  An OpenCV Mat that contains an image
         */
        ImageData(const cv::Mat &cv_img, bool copy=true);

        /**
         *  Creates an ImageData object from the filename
         *
         *  @param image_id  A string indicating where the image is on disk
         */
        ImageData(const std::string &image_id);

       /**
         *  Creates an ImageData object from the given parameters
         *
         *  @param buffer  An buffer that contains the image data in raw pixels
         *  @param dimensions  An OpenCV Size object that contains the height
         *    and width of the image
         *  @param type  The OpenCV type of the image
         *  @see OpenCV documentation for more information on type and Size
         */
        ImageData(void* buffer, cv::Size dimensions,
            int cv_type);

        /**
         *  Creates an ImageData object from an existing ImageData object
         *
         *  @param img  A reference to an existing ImageData object
         */
        ImageData(const ImageData &img, bool copy=true);

        /**
         *  Sets an ImageData object equal to another ImageData object
         *
         *  @param img  A reference to an existing ImageData object
         */
        void operator=(const ImageData &img);

        ~ImageData();


    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */
        /**
         *  Gets the image id of the ImageData object
         *
         *  @return The string containing the full path to the ImageData object
         */
        std::string get_image_id() const;

        /**
         *  Gets the format of the ImageData object
         *
         *  @return The Image::Format of the ImageData object
         *  @see Image.h for more details on Image::Format
         */
        Image::Format get_image_format() const;

        /**
         *  Gets the OpenCV type of the image
         *
         *  @return The OpenCV type (CV_8UC3, etc)
         *  @see OpenCV documentation on types for more details
         */
        int get_type() const;

        /**
         *  Gets the dimensions (height and width) of the image
         *
         *  @return The height and width of the image as an OpenCV Size object
         */
        cv::Size get_dimensions();

        /**
         *  Gets the size of the image in pixels (height * width * channels)
         *
         *  @return The size of the image in pixels
         */
        long get_size();

        /**
         *  Gets the image data in a buffer
         *
         *  @param  buffer  A buffer (of any type) that will contain the image
         *     data when the function ends
         *  @param  buffer_size  The pixel size of the image (length of
         *     the buffer, not bytes)
         */
        void get_buffer(void* buffer, long buffer_size);

        /**
         *  Gets an OpenCV Mat that contains the image data
         *
         *  @return An OpenCV Mat
         */
        cv::Mat get_cvmat();

        /**
         *  Gets a specific area of the image, indicated by the Rectangle
         *    parameters
         *
         *  @param roi  The region of interest (starting x coordinate, starting
         *    y coordinate, height, width)
         *  @return ImageData of the area
         *  @see Image.h for more details about Rectangle
         */
        ImageData get_area(const Rectangle &roi);

        /**
         *  Gets encoded image data in a buffer
         *
         *  @param format  The Image::Format the image should be encoded as
         *  @param buffer  The buffer the encoded image will be stored in
         *  @param params  Optional parameters
         *  @see OpenCV documentation for imencode for more details
         */
        std::vector<unsigned char> get_encoded(Image::Format format,
            const std::vector<int>& params=std::vector<int>());


    /*  *********************** */
    /*        SET FUNCTIONS     */
    /*  *********************** */

        std::string format_to_string(Image::Format format);

        /**
         *  Sets the file system location of where the image
         *    can be found
         *
         *  @param image_id  The full path to the image location
         */
        void set_image_id(const std::string &image_id);

        /**
         *  Sets the format of the ImageData object
         *
         *  @param extension  A string containing the file system
         *    extension corresponding to the desired Image::Format
         *  @see Image.h for more details on Image::Format
         */
        void set_format(const std::string &extension);

        /**
         *  Sets the type of the ImageData object using OpenCV types
         *
         *  @param cv_type  The OpenCV type of the object
         *  @see OpenCV documentation on types for more details
         */
        void set_type(int cv_type);

        /**
         *  Sets the type of compression to be used when compressing
         *    the TDBImage
         *
         *  @param comp  The compression type
         *  @see Image.h for details on CompressionType
         */
        void set_compression(CompressionType comp);

        /**
         *  Sets the height and width of the image
         *
         *  @param dimensions  The height and width of the image
         *  @see OpenCV documentation for more details on Size
         */
        void set_dimensions(cv::Size dimensions);

        /**
         *  Sets the Image object to contain raw pixel data
         *    from a buffer of raw pixel data (stored in a TDB object)
         *
         *  @param buffer  The buffer containing the raw pixel data
         *  @param size  The size of the buffer
         */
        void set_data_from_raw(void* buffer, long size);

        /**
         *  Sets the Image object to contain raw pixel data
         *    from an encoded image buffer (stored in a CV Mat)
         *
         *  @param buffer  The buffer containing the encoded pixel data
         */
        void set_data_from_encoded(const std::vector<unsigned char> &buffer);

        void set_minimum(int dimension);

    /*  *********************** */
    /*   IMAGEDATA INTERACTION  */
    /*  *********************** */
        /**
         *  Performs the set of operations that have been requested
         *    on the ImageData
         */
        void perform_operations();

        /**
         *  Stores a Read Operation in the list of operations
         *    to perform
         *
         *  @param image_id  The full path to the image to be read
         */
        void read(const std::string &image_id);

        /**
         *  Stores a Write Operation in the list of operations
         *    to perform
         *
         *  @param image_id  The full path to where the image should
         *    be written
         *  @param image_format  The Image::Format to write the image in
         *  @param store_metadata  A flag to indicate whether to store the
         *    metadata in TileDB or not. Defaults to true
         *  @see Image.h for more details on Image::Format
         */
        void write(const std::string &image_id, Image::Format image_format,
            bool store_metadata=true);

        // void remove(const std::string &image_id);

        /**
         *  Stores a Resize Operation in the list of operations
         *    to perform
         *
         *  @param rows  The number of rows in the resized image
         *  @param columns  The number of columns in the resized image
         */
        void resize(int rows, int columns);

        /**
         *  Stores a Crop Operation in the list of operations
         *    to perform
         *
         *  @param rect  The region of interest (starting x coordinate,
         *    starting y coordinate, height, width) the image should be
         *    cropped to
         *  @see Image.h for more details about Rectangle
         */
        void crop(const Rectangle &rect);

        /**
         *  Stores a Threshold Operation in the list of operations
         *    to perform
         *
         *  @param value  The threshold value
         */
        void threshold(int value);

        /**
         *  Stores a Flip Operation in the list of operations
         *    to perform
         *
         *  @param code  The code of flip
         */
        void flip(int code);

        /**
         *  Rotates the image
         *
         *  @param angle  Angle of rotation
         * @param keep_resize  Specifies if the image will be resized after
         *                      the rotation, or size will be kept.
         */
        void rotate(float angle, bool keep_size);

        /**
         *  Deletes the ImageData as well as removes file from system if
         *    it exists
         */
        void delete_object();

    private:
    /*  *********************** */
    /*      COPY FUNCTIONS      */
    /*  *********************** */
        /**
         *  Copies (deep copy) an OpenCV Mat into the ImageData OpenCV Mat
         *
         *  @param cv_img  An existing OpenCV Mat
         */
        void deep_copy_cv(const cv::Mat &cv_img);

        /**
         *  Copies (shallow copy) an OpenCV Mat into the ImageData OpenCV Mat
         *
         *  @param cv_img  An existing OpenCV Mat
         */
        void shallow_copy_cv(const cv::Mat &cv_img);

        /**
         *  Copies the ImageData OpenCV Mat into a buffer
         *
         *  @param buffer  The buffer that will contain the image
         *    data
         */
        template <class T> void copy_to_buffer(T* buffer);

    /*  *********************** */
    /*      UTIL FUNCTIONS      */
    /*  *********************** */

        /**
         *  Creates full path to Image with appropriate extension based
         *    on the Image::Format
         *
         *  @param filename The path to the Image object
         *  @param format  The Image::Format of the Image object
         *  @return Full path to the object including extension
         */
        std::string create_fullpath(const std::string &filename,
            Image::Format format);


    };

}
