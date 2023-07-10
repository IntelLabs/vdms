/**
 * @file   TDBImage.cc
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

#include <iostream>
#include <stddef.h>
#include <string>

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "TDBImage.h"
#include "TDBObject.h"
#include "vcl/VCL.h"

using namespace VCL;

#define MAX_UCHAR 256

/*  *********************** */
/*        CONSTRUCTORS      */
/*  *********************** */
TDBImage::TDBImage() : TDBObject() {
  _img_height = 0;
  _img_width = 0;
  _img_channels = 0;
  _img_size = 0;

  _threshold = 0;

  set_num_dimensions(2);
  set_default_attributes();
  set_default_dimensions();

  _raw_data = NULL;
}

TDBImage::TDBImage(const std::string &image_id) : TDBObject(image_id) {
  _img_height = 0;
  _img_width = 0;
  _img_channels = 0;
  _img_size = 0;

  _threshold = 0;

  set_num_dimensions(2);
  set_default_attributes();
  set_default_dimensions();

  _raw_data = NULL;
}

TDBImage::TDBImage(const std::string &image_id, RemoteConnection &connection)
    : TDBObject(image_id, connection) {

  _img_height = 0;
  _img_width = 0;
  _img_channels = 0;
  _img_size = 0;

  _threshold = 0;

  set_num_dimensions(2);
  set_default_attributes();
  set_default_dimensions();

  _raw_data = NULL;
}

template <class T> TDBImage::TDBImage(T *buffer, long size) : TDBObject() {
  _img_height = 0;
  _img_width = 0;
  _img_channels = 0;
  _img_size = size;

  _threshold = 0;

  set_num_dimensions(2);
  set_default_attributes();
  set_default_dimensions();

  _raw_data = new unsigned char[size];
  std::memcpy(_raw_data, buffer, _img_size);
}

// OpenCV type CV_8UC1-4
template TDBImage::TDBImage(unsigned char *buffer, long size);
// OpenCV type CV_8SC1-4
template TDBImage::TDBImage(char *buffer, long size);
// OpenCV type CV_16UC1-4
template TDBImage::TDBImage(unsigned short *buffer, long size);
// OpenCV type CV_16SC1-4
template TDBImage::TDBImage(short *buffer, long size);
// OpenCV type CV_32SC1-4
template TDBImage::TDBImage(int *buffer, long size);
// OpenCV type CV_32FC1-4
template TDBImage::TDBImage(float *buffer, long size);
// OpenCV type CV_64FC1-4
template TDBImage::TDBImage(double *buffer, long size);

TDBImage::TDBImage(TDBImage &tdb) : TDBObject(tdb) {
  if (!tdb.has_data()) {
    try {
      tdb.read();
    } catch (VCL::Exception &e) {
      _raw_data = NULL;
    }
  }

  set_equal(tdb);
  set_image_data_equal(tdb);
  _raw_data = 0;

  if (tdb.has_data()) {
    uint64_t size = _img_height * _img_width * _img_channels;
    _raw_data = new unsigned char[size];
    std::memcpy(_raw_data, tdb._raw_data, size);
  }
}

void TDBImage::operator=(TDBImage &tdb) {
  unsigned char *temp = _raw_data;

  if (!tdb.has_data()) {
    try {
      tdb.read();
    } catch (VCL::Exception &e) {
      _raw_data = NULL;
    }
  }

  set_equal(tdb);
  set_image_data_equal(tdb);

  if (tdb.has_data()) {
    if (_raw_data != NULL)
      delete[] _raw_data;

    uint64_t array_size = _img_height * _img_width * _img_channels;
    _raw_data = new unsigned char[array_size];
    std::memcpy(_raw_data, tdb._raw_data, array_size);
  }
}

void TDBImage::set_image_data_equal(const TDBImage &tdb) {
  _img_height = tdb._img_height;
  _img_width = tdb._img_width;
  _img_channels = tdb._img_channels;
  _img_size = tdb._img_size;
  _threshold = tdb._threshold;
}

TDBImage::~TDBImage() { delete[] _raw_data; }

/*  *********************** */
/*        GET FUNCTIONS     */
/*  *********************** */

long TDBImage::get_image_size() {
  if (_img_size == 0 && _name == "") {
    throw VCLException(TileDBNotFound, "No data in TileDB object yet");
  } else if (_img_size == 0 && _name != "") {
    read_image_metadata();
  }

  return _img_size;
}

int TDBImage::get_image_height() {
  if (_img_height == 0 && _name == "")
    throw VCLException(TileDBNotFound, "No data in TileDB object yet");
  else if (_img_height == 0 && _name != "")
    read_image_metadata();

  return _img_height;
}

int TDBImage::get_image_width() {
  if (_img_width == 0 && _name == "")
    throw VCLException(TileDBNotFound, "No data in TileDB object yet");
  else if (_img_width == 0 && _name != "")
    read_image_metadata();

  return _img_width;
}

int TDBImage::get_image_channels() {
  if (_img_channels == 0 && _name == "")
    throw VCLException(TileDBNotFound, "No data in TileDB object yet");
  else if (_img_channels == 0 && _name != "")
    read_image_metadata();

  return _img_channels;
}

cv::Mat TDBImage::get_cvmat() {
  if (_raw_data == NULL)
    read();

  unsigned char *buffer = new unsigned char[_img_size];

  std::memcpy(buffer, _raw_data, _img_size);

  cv::Mat img_clone;

  if (_img_channels == 1) {
    cv::Mat img(cv::Size(_img_width, _img_height), CV_8UC1, buffer);
    img_clone = img.clone();
  } else {
    cv::Mat img(cv::Size(_img_width, _img_height), CV_8UC3, buffer);
    img_clone = img.clone();
  }

  delete[] buffer;
  return img_clone;
}

template <class T> void TDBImage::get_buffer(T *buffer, long buffer_size) {
  if (buffer_size != get_image_size()) {
    std::cout << "buffer size not equal to image size\n";
    std::cout << "buffer size: " << buffer_size << std::endl;
    std::cout << "image size: " << _img_size << std::endl;
    throw VCLException(SizeMismatch,
                       buffer_size + " is not equal to " + _img_size);
  }

  if (_raw_data == NULL)
    read();

  std::memcpy(buffer, _raw_data, buffer_size);
}

template void TDBImage::get_buffer(unsigned char *buffer, long buffer_size);
template void TDBImage::get_buffer(char *buffer, long buffer_size);
template void TDBImage::get_buffer(unsigned short *buffer, long buffer_size);
template void TDBImage::get_buffer(short *buffer, long buffer_size);
template void TDBImage::get_buffer(int *buffer, long buffer_size);
template void TDBImage::get_buffer(float *buffer, long buffer_size);
template void TDBImage::get_buffer(double *buffer, long buffer_size);

/*  *********************** */
/*        SET FUNCTIONS     */
/*  *********************** */

void TDBImage::set_image_properties(int height, int width, int channels) {
  _img_height = height;
  _img_width = width;
  _img_channels = channels;
  _img_size = _img_height * _img_width * _img_channels;
}

void TDBImage::set_configuration(RemoteConnection *remote) {
  if (!remote->connected())
    throw VCLException(SystemNotFound, "Remote Connection not initialized");

  set_config(remote);
}

/*  *********************** */
/*    TDBIMAGE INTERACTION  */
/*  *********************** */
void TDBImage::write(const std::string &image_id, bool metadata) {
  if (_raw_data == NULL)
    throw VCLException(ObjectEmpty, "No data to be written");

  std::string array_name = namespace_setup(image_id);

  std::vector<unsigned char> num_values;
  if (_num_attributes == 1 && _img_channels == 3)
    num_values.push_back(3);
  else
    num_values.push_back(1);
  set_schema_dense(array_name, num_values);

  tiledb::Array array(_ctx, array_name, TILEDB_WRITE);

  tiledb::Query write_query(_ctx, array, TILEDB_WRITE);

  write_query.set_layout(TILEDB_ROW_MAJOR);

  if (_num_attributes == 1) {
    write_image_metadata(array);
    std::vector<uint64_t> subarray = {1, _img_height, 0, _img_width - 1};
    write_query.set_subarray(subarray);
    write_query.set_data_buffer(_attributes[0], _raw_data,
                                _img_height * _img_width * _img_channels);
  } else {
    size_t buffer_size = _img_height * _img_width;
    unsigned char *blue_buffer = new unsigned char[buffer_size];
    unsigned char *green_buffer = new unsigned char[buffer_size];
    unsigned char *red_buffer = new unsigned char[buffer_size];

    int count = 0;
    for (unsigned int i = 0; i < buffer_size; ++i) {
      blue_buffer[i] = _raw_data[count];
      green_buffer[i] = _raw_data[count + 1];
      red_buffer[i] = _raw_data[count + 2];
    }
    write_query.set_data_buffer(_attributes[0], blue_buffer, buffer_size);
    write_query.set_data_buffer(_attributes[1], green_buffer, buffer_size);
    write_query.set_data_buffer(_attributes[2], red_buffer, buffer_size);
  }

  write_query.submit();
  write_query.finalize();
  array.close();
}

void TDBImage::write(const cv::Mat &cv_img, bool metadata) {
  if (_group == "")
    throw VCLException(ObjectNotFound, "Object path is not defined");
  if (_name == "")
    throw VCLException(ObjectNotFound, "Object name is not defined");

  std::string array_name = _group + _name;
  if (tiledb::Object::object(_ctx, array_name).type() !=
      tiledb::Object::Type::Invalid)
    tiledb::Object::remove(_ctx, array_name);

  set_dimension_lowerbounds(std::vector<uint64_t>{0, 0});
  set_dimension_upperbounds(std::vector<uint64_t>{(uint64_t)(cv_img.rows + 1),
                                                  (uint64_t)(cv_img.cols)});

  _img_height = cv_img.rows;
  _img_width = cv_img.cols;
  _img_channels = cv_img.channels();
  _img_size = _img_height * _img_width * _img_channels;

  std::vector<unsigned char> num_values;
  if (_num_attributes == 1 && _img_channels == 3)
    num_values.push_back(3);
  else
    num_values.push_back(1);
  set_schema_dense(array_name, num_values);

  tiledb::Array array(_ctx, array_name, TILEDB_WRITE);

  write_image_metadata(array);

  tiledb::Query write_query(_ctx, array);
  write_query.set_layout(TILEDB_ROW_MAJOR);

  std::vector<uint64_t> subarray = {1, _img_height, 0, _img_width - 1};
  write_query.set_subarray(subarray);

  size_t buffer_size = _img_height * _img_width * _img_channels;
  _raw_data = new unsigned char[buffer_size];

  if (_num_attributes == 1) {
    std::memcpy(_raw_data, cv_img.data, buffer_size);
    write_query.set_data_buffer(_attributes[0], _raw_data, buffer_size);
  } else {
    std::vector<cv::Mat> channels(3);
    cv::split(cv_img, channels);
    size_t size = _img_height * _img_width;
    unsigned char *blue_buffer = new unsigned char[size];
    unsigned char *green_buffer = new unsigned char[size];
    unsigned char *red_buffer = new unsigned char[size];

    const unsigned char *bp;
    for (unsigned int i = 0; i < _img_height; ++i) {
      bp = channels[0].ptr<unsigned char>(i);
      unsigned char *b = &blue_buffer[i * _img_width];
      std::memcpy(b, bp, _img_width);
    }

    const unsigned char *gp;
    for (int i = 0; i < _img_height; ++i) {
      gp = channels[1].ptr<unsigned char>(i);
      unsigned char *g = &green_buffer[i * _img_width];
      std::memcpy(g, gp, _img_width);
    }

    const unsigned char *rp;
    for (unsigned int i = 0; i < _img_height; ++i) {
      rp = channels[2].ptr<unsigned char>(i);
      unsigned char *r = &red_buffer[i * _img_width];
      std::memcpy(r, rp, _img_width);
    }
    write_query.set_data_buffer(_attributes[0], blue_buffer, buffer_size);
    write_query.set_data_buffer(_attributes[1], green_buffer, buffer_size);
    write_query.set_data_buffer(_attributes[2], red_buffer, buffer_size);

    delete[] blue_buffer;
    delete[] green_buffer;
    delete[] red_buffer;
  }

  write_query.submit();
  write_query.finalize();
  array.close();
}

void TDBImage::read() {
  if (_raw_data == NULL) {
    if (_img_height == 0)
      read_image_metadata();

    // {start row, end row, start col, end col}
    std::vector<uint64_t> subarray = {1, _img_height, 0, _img_width - 1};
    read_from_tdb(subarray);
  }
}

void TDBImage::read(const Rectangle &rect) {
  if (_raw_data == NULL) {

    if (_img_height == 0)
      read_image_metadata();

    if (_img_height < rect.height + rect.y || _img_width < rect.width + rect.x)
      throw VCLException(SizeMismatch,
                         "Requested area is not within the image");

    _img_height = rect.height;
    _img_width = rect.width;
    _img_size = _img_height * _img_width * _img_channels;

    std::vector<uint64_t> subarray;

    subarray.push_back(rect.x);               // start row
    subarray.push_back(rect.x + rect.height); // end row
    subarray.push_back(rect.y);               // start column
    subarray.push_back(rect.y + rect.width);  // end column

    read_from_tdb(subarray);
  }
}

void TDBImage::resize(const Rectangle &rect) {
  if (_raw_data == NULL)
    read();

  int r, c;

  int data_index = 0;
  unsigned char *image_buffer =
      new unsigned char[rect.height * rect.width * _img_channels];
  memset(image_buffer, 0, rect.height * rect.width * _img_channels);

  float row_ratio = _img_height / float(rect.height);
  float column_ratio = _img_width / float(rect.width);

  for (r = 0; r < rect.height; ++r) {
    float scale_r = (r + 0.5) * row_ratio - 0.5;

    for (c = 0; c < rect.width; ++c) {
      float scale_c = (c + 0.5) * column_ratio - 0.5;

      data_index = rect.width * r * _img_channels + c * _img_channels;

      get_index_value(image_buffer, data_index, scale_r, scale_c);
    }
  }

  _img_height = rect.height;
  _img_width = rect.width;
  _img_size = _img_height * _img_width * _img_channels;
  std::vector<uint64_t> values = {_img_height + 1, _img_width};
  set_dimension_upperbounds(values);

  _raw_data = new unsigned char[_img_size];
  std::memcpy(_raw_data, image_buffer, _img_size);

  delete[] image_buffer;
}

void TDBImage::threshold(int value) {
  if (_raw_data == NULL) {
    _threshold = value;
    read();
  } else {
    int length = _img_height * _img_width * _img_channels;

    for (int i = 0; i < length; ++i) {
      if (_raw_data[i] <= value)
        _raw_data[i] = 0;
    }
  }
}

bool TDBImage::has_data() {
  if (_raw_data == NULL)
    return false;
  else
    return true;
}

void TDBImage::delete_image() {
  delete _raw_data;
  _raw_data = NULL;
  delete_object();
}

/*  *********************** */
/*   PRIVATE GET FUNCTIONS  */
/*  *********************** */
void TDBImage::get_tile_coordinates(int64_t *subarray, int current_row_tile,
                                    int current_column_tile) {
  int row_start = current_row_tile * _tile_dimension[0];
  int column_start = current_column_tile * _tile_dimension[1];
  int row_end = row_start + _tile_dimension[0];
  int column_end = column_start + _tile_dimension[1];

  if (row_end > _img_height)
    row_end = (_img_height - row_start) + row_start;

  if (column_end > _img_width)
    column_end = (_img_width - column_start) + column_start;

  subarray[0] = row_start;
  subarray[1] = row_end;
  subarray[2] = column_start;
  subarray[3] = column_end;
}

void TDBImage::get_index_value(unsigned char *image_buffer, int index,
                               float scale_r, float scale_c) {
  int column_left = floor(scale_c);
  int column_right = floor(scale_c + 1);
  int row_top = floor(scale_r);
  int row_bottom = floor(scale_r + 1);

  if (column_left < 0)
    column_left = 0;
  if (column_right > _img_width - 1)
    column_right = _img_width - 1;

  if (row_top < 0)
    row_top = 0;
  if (row_bottom > _img_height - 1)
    row_bottom = _img_height - 1;

  long top_left_index = get_index(row_top, column_left) * _img_channels;
  long top_right_index = get_index(row_top, column_right) * _img_channels;
  long bottom_left_index = get_index(row_bottom, column_left) * _img_channels;
  long bottom_right_index = get_index(row_bottom, column_right) * _img_channels;

  for (int x = 0; x < _img_channels; ++x) {
    unsigned char top_left = _raw_data[top_left_index + x];
    unsigned char top_right = _raw_data[top_right_index + x];
    unsigned char bottom_left = _raw_data[bottom_left_index + x];
    unsigned char bottom_right = _raw_data[bottom_right_index + x];

    double top = linear_interpolation(column_left, top_left, column_right,
                                      top_right, scale_c);
    double bottom = linear_interpolation(column_left, bottom_left, column_right,
                                         bottom_right, scale_c);
    double middle =
        linear_interpolation(row_top, top, row_bottom, bottom, scale_r);

    // we want the middle of the pixel
    unsigned char pixel_value = floor(middle + 0.5);
    image_buffer[index + x] = pixel_value;
  }
}

long TDBImage::get_index(int row, int column) const {
  int tile_width = get_tile_width(column, _img_width / _tile_dimension[1]);
  int tile_height = get_tile_height(row, _img_height / _tile_dimension[0]);

  long tile_size = tile_width * tile_height;

  long current_tile_row = row % long(_tile_dimension[0]);
  long current_row_tile = row / long(_tile_dimension[0]);
  long current_column_tile = column / long(_tile_dimension[1]);
  long current_tile_column = column % long(_tile_dimension[1]);

  long full_row_tile = _img_width * _tile_dimension[0];

  long row_index =
      current_row_tile * full_row_tile + current_tile_row * tile_width;
  long column_index = current_column_tile * tile_size + current_tile_column;

  return row_index + column_index;
}

int TDBImage::get_tile_height(int row, int number_tiles) const {
  int tile_height = int(_tile_dimension[0]);

  if (row / _tile_dimension[0] == (unsigned int)number_tiles)
    tile_height = _img_height - (number_tiles)*_tile_dimension[0];

  return tile_height;
}

int TDBImage::get_tile_width(int column, int number_tiles) const {
  int tile_width = int(_tile_dimension[1]);

  if (column / _tile_dimension[1] == (unsigned int)number_tiles)
    tile_width = _img_width - (number_tiles)*_tile_dimension[1];

  return tile_width;
}

/*  *********************** */
/*   PRIVATE SET FUNCTIONS  */
/*  *********************** */
void TDBImage::set_default_dimensions() {
  _dimension_names.push_back("height");
  _dimension_names.push_back("width");
}

void TDBImage::set_default_attributes() {
  _attributes.clear();
  switch (_num_attributes) {
  case 1: {
    _attributes.push_back("pixel");
    break;
  }
  case 3: {
    _attributes.push_back("blue");
    _attributes.push_back("green");
    _attributes.push_back("red");
    break;
  }
  }
}

/*  *********************** */
/*      TDBIMAGE SETUP      */
/*  *********************** */
std::string TDBImage::namespace_setup(const std::string &image_id) {
  size_t pos = get_path_delimiter(image_id);

  _group = get_group(image_id, pos);
  _name = get_name(image_id, pos);

  return _group + _name;
}

/*  *********************** */
/*   METADATA INTERACTION   */
/*  *********************** */
void TDBImage::write_image_metadata(tiledb::Array &array) {
  std::vector<unsigned char> metadata;

  metadata.emplace_back((unsigned char)_img_channels / MAX_UCHAR);
  metadata.emplace_back((unsigned char)_img_channels % MAX_UCHAR);
  metadata.emplace_back((unsigned char)(_img_height / MAX_UCHAR));
  metadata.emplace_back((unsigned char)(_img_height % MAX_UCHAR));
  metadata.emplace_back((unsigned char)(_img_width / MAX_UCHAR));
  metadata.emplace_back((unsigned char)(_img_width % MAX_UCHAR));

  std::vector<uint64_t> subarray = {0, 0, 0, 1};

  tiledb::Query md_write(_ctx, array);
  md_write.set_subarray(subarray);
  md_write.set_layout(TILEDB_ROW_MAJOR);
  md_write.set_data_buffer(_attributes[_num_attributes - 1], metadata);

  md_write.submit();
}

void TDBImage::read_image_metadata() {
  std::vector<uint64_t> metadata(3);

  std::string md_name = _group + _name;
  std::vector<uint64_t> subarray = {0, 0, 0, 1};
  std::string attr = _attributes[_num_attributes - 1];
  read_metadata(md_name, subarray, metadata, attr);

  _img_height = metadata[1];
  _img_width = metadata[2];
  _img_channels = metadata[0];

  _img_size = _img_height * _img_width * _img_channels;

  set_dimension_lowerbounds(std::vector<uint64_t>{0, 0});
  set_dimension_upperbounds(
      std::vector<uint64_t>{(_img_height + 1), (_img_width)});
}

/*  *********************** */
/*     DATA MANIPULATION    */
/*  *********************** */
void TDBImage::read_from_tdb(std::vector<uint64_t> subarray) {
  std::string array_name = _group + _name;

  set_from_schema(array_name);

  tiledb::Array array(_ctx, array_name, TILEDB_READ);

  tiledb::Query read_query(_ctx, array, TILEDB_READ);
  read_query.set_layout(TILEDB_ROW_MAJOR);
  read_query.set_subarray(subarray);

  if (_num_attributes == 1) {
    int buffer_size = _img_height * _img_width * _img_channels;
    _raw_data = new unsigned char[buffer_size];

    read_query.set_data_buffer(_attributes[0], _raw_data, buffer_size);
    read_query.submit();
  }

  else {
    int buffer_size = _img_height * _img_width;
    _raw_data = new unsigned char[buffer_size * _img_channels];
    unsigned char *blue_buffer = new unsigned char[buffer_size];
    unsigned char *green_buffer = new unsigned char[buffer_size];
    unsigned char *red_buffer = new unsigned char[buffer_size];

    read_query.set_data_buffer(_attributes[0], blue_buffer, buffer_size);
    read_query.set_data_buffer(_attributes[1], green_buffer, buffer_size);
    read_query.set_data_buffer(_attributes[2], red_buffer, buffer_size);

    read_query.submit();

    int count = 0;
    for (int i = 0; i < buffer_size; ++i) {
      _raw_data[count] = blue_buffer[i];
      _raw_data[count + 1] = green_buffer[i];
      _raw_data[count + 2] = red_buffer[i];
      count += 3;
    }

    delete[] blue_buffer;
    delete[] green_buffer;
    delete[] red_buffer;
  }

  array.close();
}

/*  *********************** */
/*      MATH FUNCTIONS      */
/*  *********************** */
double TDBImage::linear_interpolation(double x1, double val1, double x2,
                                      double val2, double x) {

  if (x1 == x2)
    return val1;

  double value = val2 - val1;
  double multiply = x - x1;
  double divide = x2 - x1;

  return val1 + (value / divide * multiply);
}
