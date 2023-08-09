/**
 * @file   Image.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2023 Intel Corporation
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
 * delayed until the data is actually requested (in a store operation, or a
 * get_* operation such as get_cvmat)
 */

#pragma once
#include <fstream>
#include <stdio.h>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/objdetect.hpp>

#include "Exception.h"
#include "RemoteConnection.h"
#include "TDBImage.h"
#include "utils.h"
#include <curl/curl.h>
#include <jsoncpp/json/value.h>
#include <zmq.hpp>

namespace VCL {

/**
 *  Uses the OpenCV Rect class to define an area in the image
 *    (starting x coordinate, starting y coordinate, height, width)
 */
typedef cv::Rect Rectangle;

class Image {
public:
  enum class Format { NONE_IMAGE = 0, JPG = 1, PNG = 2, TDB = 3, BIN = 4 };

  // enum class Storage { LOCAL = 0, AWS = 1 };

  /*  *********************** */
  /*        CONSTRUCTORS      */
  /*  *********************** */

  /**
   *  Creates an Image object from the image id (where the
   *    image data can be found in the system).
   *
   *  @param image_id  The full path to the image
   *  @param bucket_name  Optional parameter for bucket name if using AWS
   *  storage
   */
  Image(const std::string &image_id, std::string bucket_name = "");

  /**
   *  Creates an Image object from the OpenCV Mat
   *
   *  @param cv_img  An OpenCV Mat that contains an image
   *  @param copy  Deep copy if true, shallow copy if false
   */
  Image(const cv::Mat &cv_img, bool copy = true);

  /**
   *  Creates an OpenCV Image object from an encoded buffer
   *
   *  @param buffer  An encoded buffer that contains the image data
   *  @param size  Size of the encoded buffer
   *  @param flags  Flags specifying the color type of the encoded image,
   *    defaults to IMREAD_COLOR
   *  @see OpenCV documentation on imdecode for more information on flags
   */
  Image(void *buffer, long size, char raw_binary_file = 0,
        int flags = cv::IMREAD_ANYCOLOR);

  /**
   *  Creates a TDB Image object from a buffer of raw pixel data
   *
   *  @param buffer  A buffer that contains the image data
   *  @param dimensions  An OpenCV Size object that contains the height
   *    and width of the image
   *  @param type  The OpenCV type of the image
   *  @see OpenCV documentation for more information on type and Size
   */
  Image(void *buffer, cv::Size dimensions, int cv_type);

  /**
   *  Creates a new Image object from an existing Image object
   *
   *  @param img  An existing Image object
   *  @param copy Makes a deep copy if true, a shallow copy otherwise
   */
  Image(const Image &img, bool copy = true);

  /**
   *  Move constructor, needed to avoid copies of the arrays.
   *  noexcept is needed to let vectors grow and call the move
   *  instead of copy constructor.
   *
   *  @param img  An rvalue Image object
   */
  Image(Image &&img) noexcept;

  /**
   *  Assigns an Image object to this Image object by performing a deep
   *  copy operation
   *
   *  @param img  An existing Image object
   */
  Image &operator=(const Image &img);

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
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return The dimension of the image in pixels as an OpenCV Size object
   */
  cv::Size get_dimensions(bool performOp = true);

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
  long get_raw_data_size();

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
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return A new Image object that is only the requested area
   */
  Image get_area(const Rectangle &roi, bool performOp = true) const;

  /**
   *  Gets an OpenCV Mat that contains the image data
   *
   *  @param copy Specify if a deep copy of the cvmat will be made before
   *              returning the cvmat object.
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return An OpenCV Mat
   */
  cv::Mat get_cvmat(bool copy = true, bool performOp = true);

  /**
   *  Gets the raw image data
   *
   *  @param  buffer  A buffer (of any type) that will contain the image
   *     data when the function ends
   *  @param  performOp Specify if operations should be performed first. Default
   * is true.
   *  @param  buffer_size  The pixel size of the image (length of
   *     the buffer, not bytes)
   */
  void get_raw_data(void *buffer, long buffer_size, bool performOp = true);

  /**
   *  Gets encoded image data in a buffer
   *
   *  @param format  The Format the Image should be encoded as
   *  @param params  Optional parameters
   *  @return  A vector containing the encoded image
   *  @see OpenCV documentation for imencode for more details

/**
*  Gets encoded image data in a buffer
*
*  @param format  The Format the Image should be encoded as
*  @param params  Optional parameters
*  @return  A vector containing the encoded image
*  @see OpenCV documentation for imencode for more details
*/
  std::vector<unsigned char>
  get_encoded_image(VCL::Image::Format format,
                    const std::vector<int> &params = std::vector<int>());

  /**
   *  Gets encoded image data in a buffer in a async way
   *
   *  @param format  The Format the Image should be encoded as
   *  @param params  Optional parameters
   *  @param enc_img_vec Vector of operated images
   *  @return  A vector containing the encoded image
   *  @see OpenCV documentation for imencode for more details
   */
  std::vector<unsigned char>
  get_encoded_image_async(VCL::Image::Format format,
                          const std::vector<int> &params = std::vector<int>());

  /**
   *  Executes the operations in the operation vector
   *
   *  @return  -1 if operation should run on a separate thread otherwise 0
   */
  int execute_operation();

  /**
   *  @return Size of the operations vector
   */
  int get_enqueued_operation_count();

  /**
   *  @return Number of operations completed
   */
  int get_op_completed();

  /**
   *  @return Parameters required to run a remote operation
   */
  Json::Value get_remoteOp_params();

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

  /**
   *  Updates the number of operations completed
   */
  void update_op_completed();

  /**
   *  Set parameters to run a remote operation
   */
  void set_remoteOp_params(Json::Value options, std::string url);

  void set_connection(RemoteConnection *remote);

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
             bool store_metadata = true);

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
   *  Performs a remote operation on the image.
   *
   *  @param url  Remote url
   *  @param options  operation options
   */
  void syncremoteOperation(std::string url, Json::Value options);

  /**
   *  Performs a remote operation on the image.
   *
   *  @param url  Remote url
   *  @param options  operation options
   */
  void remoteOperation(std::string url, Json::Value options);

  /**
   *  Performs a user defined operation on the image.
   *
   *  @param options  operation options
   */
  void userOperation(Json::Value options);

  /**
   *  Checks to see if the Image has a depth associated with it.
   *    Currently returns false, as we do not support depth camera
   *    input yet.
   */
  bool has_depth() { return false; };

  /**
   *  Deletes the Image as well as removes file from system if
   *    it exists
   */
  void delete_image();
  /*  *********************** */
  /*      COPY FUNCTIONS      */
  /*  *********************** */
  /**
   *  Copies (deep copy) an OpenCV Mat into the Image OpenCV Mat
   *
   *  @param cv_img  An existing OpenCV Mat
   */
  void deep_copy_cv(const cv::Mat &cv_img);

  /**
   *  Copies (shallow copy) an OpenCV Mat into the Image OpenCV Mat
   *
   *  @param cv_img  An existing OpenCV Mat
   */
  void shallow_copy_cv(const cv::Mat &cv_img);

  /**
   *  Copies the Image OpenCV Mat into a buffer
   *
   *  @param buffer  The buffer that will contain the image
   *    data
   */
  template <class T> void copy_to_buffer(T *buffer);

  std::string format_to_string(Image::Format format);

private:
  /**
   *  Default constructor, creates an empty Image object.
   *    Used when reading from the file system
   */
  Image();

  // Forward declaration of Operation class, to be used of _operations
  // list
  class Operation;

  // Forward declaration of ImageTest class, that is used for the unit
  // test to accesss private methods of this class
  friend class ImageTest;

  /*  *********************** */
  /*        VARIABLES         */
  /*  *********************** */
  // Image height and width
  uint _height, _width;

  // Type of image (OpenCV definition) and number of channels
  int _cv_type, _channels;

  // Maintains order of operations requested
  std::vector<std::shared_ptr<Operation>> _operations;

  // Count of operations executed
  int _op_completed;

  // Parameters required for remote operations
  Json::Value remoteOp_params;

  // Remote connection (if one exists)
  RemoteConnection *_remote;

  // Image format and compression type
  Format _format;
  CompressionType _compress;
  Storage _storage = Storage::LOCAL;

  // Full path to image
  std::string _image_id;

  // Image data (OpenCV Mat or TDBImage)
  cv::Mat _cv_img;
  TDBImage *_tdb;
  char *_bin;
  long _bin_size;

  /*  *********************** */
  /*      UTIL FUNCTIONS      */
  /*  *********************** */

  /**
   *  Performs the set of operations that have been requested
   *    on the Image
   */
  void perform_operations();

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

  /*  *********************** */
  /*        OPERATION         */
  /*  *********************** */

  enum class OperationType {
    READ,
    WRITE,
    RESIZE,
    CROP,
    THRESHOLD,
    FLIP,
    ROTATE,
    SYNCREMOTEOPERATION,
    REMOTEOPERATION,
    USEROPERATION
  };

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
    Format _format;

    /**
     *  Constructor, sets the format
     *
     *  @param format  The format for the operation
     *  @see Image.h for more details on Format
     */
    Operation(Format format) : _format(format){};

  public:
    /**
     *  Implemented by the specific operation, performs what
     *    the operation is supposed to do
     *
     *  @param img  A pointer to the current Image object
     */
    virtual void operator()(Image *img) = 0;

    virtual OperationType get_type() const = 0;
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
     *  @see Image.h for more details on ::Format
     */
    Read(const std::string &filename, Format format);

    /**
     *  Reads an image from the file system (based on the format
     *    and file path indicated)
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::READ; };
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
    Format _old_format;
    /** Whether to store the metadata */
    bool _metadata;

  public:
    /**
     *  Constructor, sets the formats and path for writing
     *
     *  @param filename  The full path to write to
     *  @param format  The format to store the image in
     *  @param old_format  The format the image was stored in
     *  @see Image.h for more details on ::Format
     */
    Write(const std::string &filename, Format format, Format old_format,
          bool metadata);
    /**
     *  Writes an image to the file system (based on the format
     *    and file path indicated)
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::WRITE; };
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
     *  @see Image.h for more details on ::Format and Rectangle
     */
    Resize(const Rectangle &rect, Format format)
        : Operation(format), _rect(rect){};

    /**
     *  Resizes an image to the given dimensions
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::RESIZE; };
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
     *  @see Image.h for more details on ::Format and Rectangle
     */
    Crop(const Rectangle &rect, Format format)
        : Operation(format), _rect(rect){};

    /**
     *  Crops the image to the given area
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::CROP; };
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
     *  @see Image.h for more details on ::Format
     */
    Threshold(const int value, Format format)
        : Operation(format), _threshold(value){};

    /**
     *  Performs the thresholding operation
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::THRESHOLD; };
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
     *  @see Image.h for more details on ::Format
     */
    Flip(const int code, Format format) : Operation(format), _code(code){};

    /**
     *  Performs the flip operation
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::FLIP; };
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
     *  @see Image.h for more details on Format
     */
    Rotate(float angle, bool keep_size, Format format)
        : Operation(format), _angle(angle), _keep_size(keep_size){};

    /**
     *  Performs the flip operation
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::ROTATE; };
  };

  /*  *********************** */
  /*    SYNC OPERATION   */
  /*  *********************** */
  /**  Extends Operation, performs a remote operation that
   */
  class SyncRemoteOperation : public Operation {
  private:
    /** Minimum value pixels should be */
    std::string _url;
    Json::Value _options;

  public:
    /**
     *  Constructor, sets the flip code value.
     *
     *  @param url  The current format of the image data
     *  @param options
     *  @see Image.h for more details on Format
     */
    SyncRemoteOperation(std::string url, Json::Value options, Format format)
        : Operation(format), _url(url), _options(options){};

    /**
     *  Performs the remote operation
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const {
      return OperationType::SYNCREMOTEOPERATION;
    };
  };

  /*  *********************** */
  /*    REMOTE OPERATION   */
  /*  *********************** */
  /**  Extends Operation, performs a remote operation that
   */
  class RemoteOperation : public Operation {
  private:
    /** Minimum value pixels should be */
    std::string _url;
    Json::Value _options;

  public:
    /**
     *  Constructor, sets the flip code value.
     *
     *  @param url  The current format of the image data
     *  @param options
     *  @see Image.h for more details on Format
     */
    RemoteOperation(std::string url, Json::Value options, Format format)
        : Operation(format), _url(url), _options(options){};

    /**
     *  Performs the remote operation
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::REMOTEOPERATION; };
  };

  /*  *********************** */
  /*    USER OPERATION   */
  /*  *********************** */
  /**  Extends Operation, performs a user operation that
   */
  class UserOperation : public Operation {
  private:
    /** Minimum value pixels should be */
    Json::Value _options;

  public:
    /**
     *  Constructor, sets the flip code value.
     *
     *  @param options
     *  @see Image.h for more details on Format
     */
    UserOperation(Json::Value options, Format format)
        : Operation(format), _options(options){};

    /**
     *  Performs the user operation
     *
     *  @param img  A pointer to the current Image object
     */
    void operator()(Image *img);

    OperationType get_type() const { return OperationType::USEROPERATION; };
  };

  /*  *********************** */
  /*    IMAGE INTERACTIONS    */
  /*  *********************** */

  /**
   *  Stores a Read Operation in the list of operations
   *    to perform
   *
   *  @param image_id  The full path to the image to be read
   */
  void read(const std::string &image_id);

  /*  *********************** */
  /*        SET FUNCTIONS     */
  /*  *********************** */

  /**
   *  Sets the Image object to contain raw pixel data
   *    from a buffer of raw pixel data (stored in a TDB object)
   *
   *  @param buffer  The buffer containing the raw pixel data
   *  @param size  The size of the buffer
   */
  void set_data_from_raw(void *buffer, long size);

  /**
   *  Sets the Image object to contain raw pixel data
   *    from an encoded image buffer (stored in a CV Mat)
   *
   *  @param buffer  The buffer containing the encoded pixel data
   */
  void set_data_from_encoded(void *buffer, long size, char raw_binary_file = 0,
                             int flags = cv::IMREAD_ANYCOLOR);

  /**
   *  Sets the format of the Image object
   *
   *  @param extension  A string containing the file system
   *    extension corresponding to the desired Image::Format
   *  @see Image.h for more details on Image::Format
   */
  void set_format(const std::string &extension);
};
}; // namespace VCL
