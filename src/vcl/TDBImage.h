/**
 * @file   TDBImage.h
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
 * This file declares the C++ API for TDBImage
 */

#pragma once

#include <opencv2/core.hpp>

#include "TDBObject.h"
#include "vcl/Exception.h"
#include <tiledb/tiledb.h>

namespace VCL {

/**
 *  Uses the OpenCV Rect class to define an area in the image
 *    (starting x coordinate, starting y coordinate, height, width)
 */
typedef cv::Rect Rectangle;

class TDBImage : public TDBObject {

  /*  *********************** */
  /*        VARIABLES         */
  /*  *********************** */
private:
  // Image dimensions
  uint64_t _img_height, _img_width, _img_channels;
  long _img_size;

  // threshold value
  int _threshold;

  // raw data of the image
  unsigned char *_raw_data;
  std::vector<unsigned char> _full_array;

public:
  /*  *********************** */
  /*        CONSTRUCTORS      */
  /*  *********************** */
  /**
   *  Creates a empty TDBImage
   */
  TDBImage();

  /**
   *  Creates a TDBImage from an object id
   *
   *  @param image_id  The path of the TDBImage
   */
  TDBImage(const std::string &image_id);

  TDBImage(const std::string &image_id, RemoteConnection &connection);

  /**
   *  Creates a TDBImage from a buffer of raw data
   *
   *  @param buffer  The raw pixel data
   *  @param size  The length of the buffer
   */
  template <class T> TDBImage(T *buffer, long size);

  /**
   *  Creates a TDBImage object from an existing TDBImage
   *
   *  @param tdb  A reference to an existing TDBImage
   */
  TDBImage(TDBImage &tdb);

  /**
   *  Sets a TDBImage object equal to another TDBImage
   *
   *  @param tdb  A reference to an existing TDBImage
   */
  void operator=(TDBImage &tdb);

  ~TDBImage();

  /*  *********************** */
  /*        GET FUNCTIONS     */
  /*  *********************** */
  /**
   *  Gets the size of the image
   *
   *  @return The size of the image (height x width x channels)
   */
  long get_image_size();

  /**
   *  Gets the height of the image (number of rows)
   *
   *  @return The height of the image
   */
  int get_image_height();

  /**
   *  Gets the width of the image (number of columns)
   *
   *  @return The width of the image
   */
  int get_image_width();

  /**
   *  Gets the number of channels in the image. A color image has
   *    three channels (green, blue, red), a black and white image
   *    has one
   *
   *  @return The number of channels
   */
  int get_image_channels();

  /**
   *  Gets an OpenCV Mat that contains the image data
   *
   *  @return An OpenCV Mat
   */
  cv::Mat get_cvmat();

  /**
   *  Gets the raw data from the TDBImage
   *
   *  @param  buffer  A buffer (of any type) that will contain the raw
   *     data when the function ends
   *  @param  buffer_size  The length of buffer (not in bytes)
   */
  template <class T> void get_buffer(T *buffer, long buffer_size);

  /*  *********************** */
  /*        SET FUNCTIONS     */
  /*  *********************** */
  /**
   *  Sets the height (number of rows), width (number of columns), and
   *    number of channels in the image. Used when the metadata is not
   *    stored in TileDB
   *
   *  @param height  Height of the image
   *  @param width  Width of the image
   *  @param channels  Number of channels in the image
   */
  void set_image_properties(int height, int width, int channels);

  void set_configuration(RemoteConnection *remote);

  /*  *********************** */
  /*    TDBIMAGE INTERACTION  */
  /*  *********************** */
  /**
   *  Writes the raw data in the TDBImage to the given object id
   *
   *  @param image_id  The object id where the data is to be written
   *  @param  metadata  A flag indicating whether the metadata
   *    should be stored in TileDB or not. Defaults to true
   */
  void write(const std::string &image_id, bool metadata = true);

  /**
   *  Writes the data in the OpenCV Mat to a location specified
   *    by the existing TDBImage path variables
   *
   *  @param cv_img  The OpenCV Mat containing the image data
   *  @param  metadata  A flag indicating whether the metadata
   *    should be stored in TileDB or not. Defaults to true
   */
  void write(const cv::Mat &cv_img, bool metadata = true);

  /**
   *  Reads the raw data from the location specified by the existing
   *    TDBImage path variables
   */
  void read();

  /**
   *  Reads a subset of the raw data from the location specified
   *    by the existing TDBImage path variables
   *
   *  @param rect  A Rectangle structure containing the coordinates
   *    and size of the subset of data to be read (starting x coordinate,
   *    starting y coordinate, height, width)
   *  @see  Image.h for more details on Rectangle
   */
  void read(const Rectangle &rect);

  /**
   *  Resizes the image to the height and width specified in
   *    the Rectangle using bilinear interpolation
   *
   *  @param rect  A Rectangle structure containing height and
   *    width to resize the image to
   *  @see  Image.h for more details on Rectangle
   */
  void resize(const Rectangle &rect);

  /**
   *  Sets pixel values less than or equal to the specified
   *    value to zero
   *
   *  @param value  The threshold under which pixel values should
   *    be set to zero
   */
  void threshold(int value);

  /**
   *  Checks to see if the TDBImage is pointing to data
   *
   *  @return True if there is data, false if there is not
   */
  bool has_data();

  /**
   *  Deletes the object from TileDB
   *
   *  @param object_id  The object id where the data is written
   */
  void delete_image();

private:
  /*  *********************** */
  /*        GET FUNCTIONS     */
  /*  *********************** */

  /**
   *  Gets the coordinates in an array given the current tile
   *
   *  @param  subarray  An array in which the coordinates will be stored
   *  @param  current_column_tile  The current column tile
   *  @param  current_row_tile  The current row tile
   */
  void get_tile_coordinates(int64_t *subarray, int current_column_tile,
                            int current_row_tile);

  /**
   *  Used for resizing, gets the value of the calculated index
   *
   *  @param  image_buffer  A buffer to store the resized image in
   *  @param  index  The current index into the image_buffer
   *  @param  scale_r  The row to be used to calculate the index into
   *    the raw data
   *  @param  scale_c  The column to be used to calculate the index into
   *    the raw data
   */
  void get_index_value(unsigned char *image_buffer, int index, float scale_r,
                       float scale_c);

  /**
   *  Used for resizing, calculates the index in the raw data where the
   *    value found at [row, column] in the image can be found
   *
   *  @param  row  The row index
   *  @param  column  The column index
   *  @return  The index in the raw data where [row, column] is
   */
  long get_index(int row, int column) const;

  /**
   *  Used for resizing, calculates the height of the current tile (used
   *    when the image height is not the same as the array height)
   *
   *  @param  row  The row index
   *  @param  number_tiles  The number of row tiles in the image
   *  @return  The height of the current tile
   */
  int get_tile_height(int row, int number_tiles) const;

  /**
   *  Used for resizing, calculates the width of the current tile (used
   *    when the image width is not the same as the array width)
   *
   *  @param  column  The column index
   *  @param  number_tiles  The number of column tiles in the image
   *  @return  The width of the current tile
   */
  int get_tile_width(int column, int number_tiles) const;

  /*  *********************** */
  /*        SET FUNCTIONS     */
  /*  *********************** */
  /**
   *  Sets the names of the dimensions to the default of
   *    "height" and "width"
   */
  void set_default_dimensions();

  /**
   *  Sets the names of the attributes to the default of
   *    "pixel" if one attribute and "green", "blue", and
   *    "red" if two
   */
  void set_default_attributes();

  /**
   *  Sets the private members of one TDBImage object equal to
   *    another
   *
   *  @param tdb  Another TDBImage object
   */
  void set_image_data_equal(const TDBImage &tdb);

  /*  *********************** */
  /*      TDBIMAGE SETUP      */
  /*  *********************** */
  /**
   *  Determines the TileDB group and array name
   *    from the image id
   *
   *  @param  image_id  The full path of the image or the full path of
   *    where to store the image
   *  @return  The name of the TileDB array
   */
  std::string namespace_setup(const std::string &image_id);

  /*  *********************** */
  /*   METADATA INTERACTION   */
  /*  *********************** */

  /**
   *  Writes the metadata of the TDBImage
   *
   *  @param  array  The tiledb::Array to which data is being written
   */
  void write_image_metadata(tiledb::Array &array);

  /**
   *  Reads the metadata at the existing TDBImage path variables
   */
  void read_image_metadata();

  /*  *********************** */
  /*     DATA MANIPULATION    */
  /*  *********************** */

  /**
   *  Reads the specified subarray from the array at the existing
   *    TDBImage path variables (can be the full array)
   *
   *  @param  subarray  An array of the coordinates of the subarray
   *    to read
   */
  void read_from_tdb(std::vector<uint64_t> subarray);

  /*  *********************** */
  /*      MATH FUNCTIONS      */
  /*  *********************** */
  /**
   *  Linearly interpolates two data points
   *
   *  @param  x1  The first reference point
   *  @param  val1  The value at the first reference point
   *  @param  x2  The second reference point
   *  @param  val2  The value at the second reference point
   *  @param  x  The desired point
   *  @return  The value at the desired point
   */
  double linear_interpolation(double x1, double val1, double x2, double val2,
                              double x);
};
}; // namespace VCL
