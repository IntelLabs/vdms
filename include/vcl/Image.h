/**
 * @file   Image.h
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
 * This file declares the C++ API for Image. NOTE: Operations on an Image are
 * delayed until the data is actually requested (in a store operation, or a get_*
 * operation such as get_cvmat)
 */

#pragma once

#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>

#include "Exception.h"
#include "utils.h"

namespace VCL {
    class ImageData;

    /**
     *  Uses the OpenCV Rect class to define an area in the image
     *    (starting x coordinate, starting y coordinate, height, width)
     */
    typedef cv::Rect Rectangle;

    class Image {
    private:
    /*  *********************** */
    /*        VARIABLES         */
    /*  *********************** */
        // Pointer to an ImageData object
        ImageData *_image;

    public:

        enum class Format{
            NONE_IMAGE = 0,
            JPG = 1,
            PNG = 2,
            TDB = 3
            };

    /*  *********************** */
    /*        CONSTRUCTORS      */
    /*  *********************** */
        /**
         *  Creates an Image object from the image id (where the
         *    image data can be found in the system)
         *
         *  @param image_id  The full path to the image
         */
        Image(const std::string &image_id);

        /**
         *  Creates an Image object from the OpenCV Mat
         *
         *  @param cv_img  An OpenCV Mat that contains an image
         */
        Image(const cv::Mat &cv_img);

        /**
         *  Creates an Image object from an encoded buffer
         *
         *  @param buffer  An encoded buffer that contains the image data
         *  @param size  Size of the encoded buffer
         *  @param flags  Flags specifying the color type of the encoded image,
         *    defaults to IMREAD_COLOR
         *  @see OpenCV documentation on imdecode for more information on flags
         */
        Image(void* buffer, long size, int flags=cv::IMREAD_ANYCOLOR);

        /**
         *  Creates an Image object from a buffer of raw pixel data
         *
         *  @param buffer  An buffer that contains the image data
         *  @param dimensions  An OpenCV Size object that contains the height
         *    and width of the image
         *  @param type  The OpenCV type of the image
         *  @see OpenCV documentation for more information on type and Size
         */
        // template <class T> Image(const T* buffer, cv::Size dimensions,
        Image(void* buffer, cv::Size dimensions,
            int cv_type);

        /**
         *  Creates a new Image object from an existing Image object
         *
         *  @param img  An existing Image object
         */
        Image(const Image &img);

        /**
         *  Sets an Image object equal to another Image object
         *
         *  @param img  An existing Image object
         */
        void operator=(const Image &img);

        ~Image();

    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */
        /**
         *  Gets the full path to the Image object
         *
         *  @return The string containing the full path to the Image
         */
        std::string get_image_id() const;

        /**
         *  Gets the dimensions of the image in pixels (width, height) using
         *    an OpenCV Size object
         *  @return The dimension of the image in pixels as an OpenCV Size object
         */
        cv::Size get_dimensions() const;

        /**
         *  Gets the format of the Image object
         *
         *  @return The Format of the Image object
         */
        VCL::Image::Format get_image_format() const;

        /**
         *  Gets the size of the image in pixels (height * width * channels)
         *
         *  @return The size of the image in pixels
         */
        long get_raw_data_size() const;

        /**
         *  Gets the OpenCV type of the image
         *
         *  @return The OpenCV type (CV_8UC3, etc)
         *  @see OpenCV documentation on types for more details
         */
        int get_image_type() const;

        /**
         *  Gets a specific area of the image, indicated by the Rectangle
         *    parameters and returns a new Image object
         *
         *  @param roi  The region of interest (starting x coordinate, starting
         *    y coordinate, height, width)
         *  @return A new Image object that is only the requested area
         */
        Image get_area(const Rectangle &roi) const;

        /**
         *  Gets an OpenCV Mat that contains the image data
         *
         *  @return An OpenCV Mat
         */
        cv::Mat get_cvmat() const;

        /**
         *  Gets the raw image data
         *
         *  @param  buffer  A buffer (of any type) that will contain the image
         *     data when the function ends
         *  @param  buffer_size  The pixel size of the image (length of
         *     the buffer, not bytes)
         */
        void get_raw_data(void* buffer, long buffer_size) const;

        /**
         *  Gets encoded image data in a buffer
         *
         *  @param format  The Format the Image should be encoded as
         *  @param params  Optional parameters
         *  @return  A vector containing the encoded image
         *  @see OpenCV documentation for imencode for more details
         */
        std::vector<unsigned char> get_encoded_image(VCL::Image::Format format,
                const std::vector<int>& params=std::vector<int>()) const;

    /*  *********************** */
    /*        SET FUNCTIONS     */
    /*  *********************** */

        /**
         *  Sets the type of compression to be used when compressing. Currently
         *    applicable only to TileDB
         *
         *  @param comp  The compression type
         */
        void set_compression(CompressionType comp);

        /**
         *  Sets the size of the image in pixels (width, height) using
         *    an OpenCV Size object
         *
         *  @param dims  The dimensions of the image in OpenCV Size format
         *  @see OpenCV documentation on Size for more details
         */
        void set_dimensions(cv::Size dims);

        /**
         *  Sets the OpenCV type of the image
         *
         *  @param The OpenCV type (CV_8UC3, etc)
         *  @see OpenCV documentation on types for more details
         */
        void set_image_type(int cv_type);

        void set_minimum_dimension(int dimension);

    /*  *********************** */
    /*    IMAGE INTERACTIONS    */
    /*  *********************** */
        /**
         *  Writes the Image to the system at the given location and in
         *    the given format
         *
         *  @param image_id  Full path to where the image should be written
         *  @param image_format  Format in which to write the image
         *  @param store_metadata  Flag to indicate whether to store the
         *    image metadata. Defaults to true (assuming no other metadata
         *    storage)
         */
        void store(const std::string &image_id, VCL::Image::Format image_format,
            bool store_metadata=true);

        /**
         *  Deletes the Image
         */
        void delete_image();

        /**
         *  Resizes the Image to the given size. This operation is not
         *    performed until the data is needed (ie, store is called or
         *    one of the get_ functions such as get_cvmat)
         *
         *  @param new_height  Number of rows
         *  @param new_width  Number of columns
         */
        void resize(int new_height, int new_width);

        /**
         *  Crops the Image to the area specified. This operation is not
         *    performed until the data is needed (ie, store is called or
         *    one of the get_ functions such as get_cvmat)
         *
         *  @param rect  The region of interest (starting x coordinate,
         *    starting y coordinate, height, width) the image should be
         *    cropped to
         */
        void crop(const Rectangle &rect);

        /**
         *  Performs a thresholding operation on the Image. Discards the pixel
         *    value if it is less than or equal to the threshold and sets that
         *    pixel to zero. This operation is not performed until the data
         *    is needed (ie, store is called or one of the get_ functions
         *    such as get_cvmat)
         *
         *  @param value  The threshold value
         */
        void threshold(int value);

        /**
         *  Flips the image either vertically, horizontally, or both, depending
         *  on code, following OpenCV convention:
         *  0 means flip vertically
         *  positive value means flip horizontally
         *  negative value means flip both vertically and horizontally
         *
         *  @param code  Specificies vertical, horizontal, or both.
         */
        void flip(int code);

        /**
         *  Rotates the image following the angle provided as parameter.
         *
         *  @param angle  Specificies the angle of rotation
         *  @param keep_resize  Specifies if the image will be resized after
         *                      the rotation, or size will be kept.
         */
        void rotate(float angle, bool keep_size);

        /**
         *  Checks to see if the Image has a depth associated with it.
         *    Currently returns false, as we do not support depth camera
         *    input yet.
         */
        bool has_depth() { return false; };
    };

};
