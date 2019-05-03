/**
 * @file   Image.cc
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

#include <stddef.h>

#include "vcl/Image.h"
#include "vcl/Exception.h"
#include "ImageData.h"

using namespace VCL;

    /*  *********************** */
    /*        CONSTRUCTORS      */
    /*  *********************** */

Image::Image(const std::string &image_id)
{
    _image = new ImageData(image_id);

    _image->read(image_id);
}

Image::Image(const cv::Mat &cv_img)
{
    if ( cv_img.empty() ) {
        throw VCLException(ObjectEmpty, "Image object is empty");
    }

    _image = new ImageData(cv_img);

}

Image::Image(cv::Mat &cv_img)
{
    if ( cv_img.empty() ) {
        throw VCLException(ObjectEmpty, "Image object is empty");
    }

    _image = new ImageData(cv_img);
}

Image::Image(void* buffer, long size, int flags)
{
    cv::Mat raw_data(cv::Size(size, 1), CV_8UC1, buffer);
    cv::Mat img = cv::imdecode(raw_data, flags);
    if ( img.empty() ) {
        throw VCLException(ObjectEmpty, "Image object is empty");
    }

    _image = new ImageData(img);
}

Image::Image(void* buffer, cv::Size dimensions, int cv_type)
    : _image(new ImageData(buffer, dimensions, cv_type))
{
}

Image::Image(const Image &img)
{
    _image = new ImageData(*img._image);
}

Image::Image(const Image &&img) noexcept
{
    _image = new ImageData(*img._image, false);
}

void Image::operator=(const Image &img)
{
    ImageData *temp = _image;
    _image = new ImageData(*img._image);
    delete temp;
}

Image::~Image()
{
    delete _image;
}

    /*  *********************** */
    /*        GET FUNCTIONS     */
    /*  *********************** */

std::string Image::get_image_id() const
{
    return _image->get_image_id();
}

cv::Size Image::get_dimensions() const
{
    return _image->get_dimensions();
}

Image::Format Image::get_image_format() const
{
    return _image->get_image_format();
}

long Image::get_raw_data_size() const
{
    return _image->get_size();
}

int Image::get_image_type() const
{
    return _image->get_type();
}


Image Image::get_area(const Rectangle &roi)
{
    Image img_copy(*this);
    *(img_copy._image) = _image->get_area(roi);

    return img_copy;
}

cv::Mat Image::get_cvmat(bool copy)
{
    cv::Mat mat = _image->get_cvmat();

    if (copy)
        return mat.clone();
    else
        return mat;
}

void Image::get_raw_data(void* buffer, long buffer_size ) const
{
    _image->get_buffer(buffer, buffer_size);
}


std::vector<unsigned char> Image::get_encoded_image(Image::Format format,
                const std::vector<int>& params) const
{
    return _image->get_encoded(format, params);
}

    /*  *********************** */
    /*        SET FUNCTIONS     */
    /*  *********************** */

void Image::set_compression(CompressionType comp)
{
    _image->set_compression(comp);
}

void Image::set_dimensions(cv::Size dims)
{
    _image->set_dimensions(dims);
}

void Image::set_image_type(int cv_type)
{
    _image->set_type(cv_type);
}

void Image::set_minimum_dimension(int dimension)
{
    _image->set_minimum(dimension);
}

    /*  *********************** */
    /*    IMAGE INTERACTIONS    */
    /*  *********************** */

void Image::store(const std::string &filename, Image::Format image_format,
    bool store_metadata)
{
    _image->write(filename, image_format, store_metadata);
    _image->perform_operations();
}

void Image::delete_image()
{
    _image->delete_object();

    delete _image;

    _image = new ImageData;
}

void Image::resize(int new_height, int new_width)
{
    _image->resize(new_height, new_width);
}

void Image::crop(const Rectangle &rect)
{
    _image->crop(rect);
}

void Image::threshold(int value)
{
    _image->threshold(value);
}

void Image::flip(int code)
{
    _image->flip(code);
}

void Image::rotate(float angle, bool keep_size)
{
    _image->rotate(angle, keep_size);
}
