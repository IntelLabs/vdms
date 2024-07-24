/**
 * @file   Video.h
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
 */

#pragma once

#include <list>
#include <memory> // For shared_ptr
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include "KeyFrame.h"
#include "vcl/Image.h"

#include "../utils/include/stats/SystemStats.h"
#include "Exception.h"
#include "VDMSConfigHelper.h"
#include "utils.h"
#include "zip.h"
#include <fstream>
#include <jsoncpp/json/reader.h>

#include "timers/TimerMap.h"

namespace VCL {

typedef cv::Rect Rectangle; // specify an ROI inside a video

class Video {

public:
  enum Codec { NOCODEC = 0, MJPG, XVID, H263, H264, AVC1 };

  std::string NOERRORSTRING = "";

  struct VideoSize {
    unsigned width;
    unsigned height;
    unsigned frame_count;
  };

  enum Unit { FRAMES = 0, SECONDS = 1 };

  enum OperationType {
    READ,
    WRITE,
    RESIZE,
    CROP,
    THRESHOLD,
    INTERVAL,
    SYNCREMOTEOPERATION,
    REMOTEOPERATION,
    USEROPERATION
  };

  RemoteConnection *_remote; // Remote connection (if one exists)
  TimerMap timers;
  std::vector<std::string> op_labels;

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
  Video(const std::string &video_id, bool no_blob = false);

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
  Video(void *buffer, long size);

  /**
   *  Sets an Video object equal to another Video object
   *
   *  @param video  A copy of an existing Video object. The parameter
   *                is passed by value to exploit copy-and-swap idiom
   *                to reduce code duplication (copy-constructor) and
   *                safer exception handling
   */
  Video &operator=(Video video);

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
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return The size of the Video in pixels
   */
  VideoSize get_size(bool performOp = true);

  /**
   *  Gets the dimensions (height and width) of the Video
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return The height and width of the Video as an OpenCV Size object
   */
  cv::Size get_frame_size(bool performOp = true);

  /**
   *  Gets number of frames in the video
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return Number of frames in the video
   */
  long get_frame_count(bool performOp = true);

  /**
   *  Gets frames per second.
   *
   *  @return Frames per second
   */
  float get_fps();

  /**
   *  Gets one specific frame from the video
   *
   *  If key frame information is stored for this video, both this
   *  function and key_frames() performs partial decoding on the video
   *  to exploit key frame information.
   *  @param performOp Specify if operations should be performed first. Default
   * is true.
   *  @return cv::Mat with the specified frame
   */
  cv::Mat get_frame(unsigned frame_num, bool performOp = true);

  /**
   *  Gets mutiple frames from the video
   *
   *  @return cv::Mat with the specified frame
   */
  std::vector<cv::Mat> get_frames(std::vector<unsigned> frame_list);

  /**
   *  Gets encoded Video data in a buffer
   *  Before calling this method, the store method must be called,
   *  as OpenCV only offers interfaces from encoding/decoding
   *  from/to files.
   *  @param container Video container format type, eg. mp4, in which the
   *  video should be encoded in
   *  @param vcl_codec The VCL codec, eg H264, in which the video is to be
   * encoded in
   */
  std::vector<unsigned char> get_encoded(std::string container,
                                         VCL::Video::Codec vcl_codec);

  /**
   *  Invokes key-frame generation on the video, if the video is encoded
   *  with a H264 encoder. Index, and byte offset/length of each key
   *  frame is stored. This operation is independent of other prior
   *  visual operations that may have been applied.
   *
   * @return List of KeyFrame objects
   */
  const KeyFrameList &get_key_frame_list();

  /**
   *  Gets the Codec as a fourcc array.
   *  @param _codec The VCL codec that is to be converted to fourcc
   */
  int get_video_fourcc(VCL::Video::Codec _codec);

  /**
   *  @return The error message if the query fails. Null if query is a success.
   */
  std::string get_query_error_response();

  /**
   *  @return The number of enqueued operations not executed yet
   */
  int get_enqueued_operation_count();

  /**
   *  @return The parameters sent by client for the remote operation
   */
  Json::Value get_remoteOp_params();

  /**
   *  @return The location of the temporary video file on which operations have
   * been perfromed
   */
  std::string get_operated_video_id();

  /**
   *  @return The metadata to be added based on UDF/Remote Operation response
   */
  std::vector<Json::Value> get_ingest_metadata();

  /**
   *  Checks if a blob is stored for the video or not
   *
   *  @return True if blob is stored
   */
  bool is_blob_not_stored() const;

  /*  *********************** */
  /*        SET FUNCTIONS     */
  /*  *********************** */

  /**
   *  Sets the file system location of where the Video
   *    can be found
   *
   *  @param Video_id  The full path to the Video location, including extension
   * (container)
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
  void set_dimensions(const cv::Size &dimensions);

  /**
   *  Set key frame information associated with the underlying video
   *  stream.
   *
   *  @param[in] key_frames list of key frames
   */
  void set_key_frame_list(KeyFrameList &key_frames);

  /**
   * Sets the RemoteConnection if AWS storage is being used
   *
   */
  void set_connection(RemoteConnection *remote);

  /**
   *  Sets the _query_error_response message when an exception occurs
   *
   *  @param error_msg Error message to be sent to the client.
   */
  void set_query_error_response(std::string error_msg);

  /**
   *  Sets the remote parameters that a remote operation will require
   *
   *  @param options encapsulated parameters for a specific remote operation.
   *  @param url remote API url
   */
  void set_remoteOp_params(Json::Value options, std::string url);

  /**
   *  Sets the location of the temporary video file on which operations have
   * been perfromed
   *
   *  @param filename location of the temporary video file
   */
  void set_operated_video_id(std::string filename);

  /**
   *  Sets the metadata to be ingested based on UDF/Remote operation
   *
   *  @param metadata metadata to be ingested
   */
  void set_ingest_metadata(Json::Value metadata);

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
   *  Performs a synchronous remote operation on the video.
   *
   *  @param url  Remote url
   *  @param options  operation options
   */
  void syncremoteOperation(std::string url, Json::Value options);

  /**
   *  Performs a asynchronous remote operation on the video.
   *
   *  @param url  Remote url
   *  @param options  operation options
   */
  void remoteOperation(std::string url, Json::Value options);

  /**
   *  Performs a user defined operation on the video.
   *
   *  @param options  operation options
   */
  void userOperation(Json::Value options);

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

  /**
   *  Initiates execution of the enqueued operation. Called by the VideoLoop.
   *  @param isRemote If the operation to be executed is a remote operation.
   * Default is false.
   */
  int execute_operations(bool isRemote = false);

private:
  class Operation;

  // Forward declaration of VideoTest class, that is used for the unit
  // test to accesss private methods of this class
  friend class VideoTest;

  // Full path to the video file.
  // It is called _video_id to keep it consistent with VCL::Image
  std::string _video_id;

  // Full path to the temporary video file on which operations are performed.
  std::string _operated_video_id;

  // No blob stored. The file path is stored instead
  // and is accessed locally or over the network.
  bool _no_blob = false;

  // Query Error response
  std::string _query_error_response = "";

  bool _flag_stored; // Flag to avoid unnecessary read/write

  VideoSize _size;

  float _fps;

  Codec _codec; // (h.264, etc).

  // Pointer to key frame decoder object, allocated when key frames
  // are set, and used whenever frames are decoded using key-frame
  // information
  std::unique_ptr<VCL::KeyFrameDecoder> _key_frame_decoder;

  // List of key frames, filled only when KeyFrames operation is applied
  KeyFrameList _key_frame_list;

  std::list<std::shared_ptr<Operation>> _operations;

  VDMS::StorageType _storage = VDMS::StorageType::LOCAL;

  // Remote operation parameters sent by the client
  Json::Value remoteOp_params;

  std::vector<Json::Value> _ingest_metadata;

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
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    virtual void operator()(Video *video, cv::Mat &frame,
                            std::string args = "") = 0;

    virtual OperationType get_type() = 0;

    /**
     *  Implemented by the Resize and Crop operations.
     *  Used to set the size of the video writer object.
     *
     */
    virtual cv::Size get_video_size() {
      cv::Size size;
      return size;
    };
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
    Resize(const cv::Size &size) : _size(size){};

    /**
     *  Resizes an Video to the given dimensions
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

    OperationType get_type() { return RESIZE; };

    cv::Size get_video_size() { return _size; }
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
    Interval(Video::Unit u, const int start, const int stop, int step)
        : _u(u), _start(start), _stop(stop), _step(step){};

    /**
     *  Resizes an Video to the given dimensions
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

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
    Crop(const Rectangle &rect) : _rect(rect){};

    /**
     *  Crops the Video to the given area
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

    OperationType get_type() { return CROP; };

    cv::Size get_video_size() { return _rect.size(); }
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
    Threshold(const int value) : _threshold(value){};

    /**
     *  Performs the thresholding operation
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

    OperationType get_type() { return THRESHOLD; };
  };

  /*  *********************** */
  /*    SYNCREMOTE OPERATION   */
  /*  *********************** */
  /**  Extends Operation, performs a synchronous remote operation
   */
  class SyncRemoteOperation : public Operation {
  private:
    std::string _url;
    Json::Value _options;

  public:
    /**
     *
     *  Constructor, sets the remote url and client options
     *
     *  @param url remote server url
     *  @param options client parameters for the operation
     */
    SyncRemoteOperation(std::string url, Json::Value options)
        : _url(url), _options(options){};

    /**
     *  Performs the remote operation
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

    OperationType get_type() { return SYNCREMOTEOPERATION; };
  };

  /*  *********************** */
  /*    REMOTE OPERATION   */
  /*  *********************** */
  /**  Extends Operation, performs an asynchronous remote operation
   */
  class RemoteOperation : public Operation {
  private:
    std::string _url;
    Json::Value _options;

  public:
    /**
     *
     *  Constructor, sets the remote url and client options
     *
     *  @param url remote server url
     *  @param options client parameters for the operation
     */
    RemoteOperation(std::string url, Json::Value options)
        : _url(url), _options(options){};

    /**
     *  Performs the remote operation
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

    OperationType get_type() { return REMOTEOPERATION; };
  };

  /*  *********************** */
  /*    USER DEFINED OPERATION   */
  /*  *********************** */
  /**  Extends Operation, performs a udf
   */
  class UserOperation : public Operation {
  private:
    Json::Value _options;

  public:
    /**
     *
     *  Constructor, sets the client options
     *
     *  @param options client parameters for the operation
     */
    UserOperation(Json::Value options) : _options(options){};

    /**
     *  Performs the remote operation
     *
     *  @param video A pointer to the current Video object
     *  @param frame The frame on which the operation will be performed
     *  @param args  Any additional parameters required by the operation
     */
    void operator()(Video *video, cv::Mat &frame, std::string args = NULL);

    OperationType get_type() { return USEROPERATION; };
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
   *  Sets video attributes such as frame count, height, width
   *  @param vname path to the video file
   */
  void initialize_video_attributes(std::string vname);

  /**
   *  Performs the set of operations that have been requested
   *  on the Video
   *  @param is_store Is the function called to perform a write to the data
   * store
   *  @param store_id File name to be used for the video stored in the data
   * store
   */
  void perform_operations(bool is_store = false, std::string store_id = "");

  /**
   *  Checks if sufficient memory is available to perform the
   *  Video operation
   * @param VideoSize struct containing the width, height, and frame
   * count of the video
   */
  bool check_sufficient_memory(const struct VideoSize &size);

  /**
   * Swaps members of two Video objects, to be used by assignment
   * operator.
   * @param rhs The video from which the attributes are to be swapped with.
   */
  void swap(Video &rhs) noexcept;

  /**
   * Get the format of the video file.
   * @param video_id Path of the video file
   */
  std::string get_video_format(char *video_id);

  /**
   *  Set size of the video writer object
   *  @param op_count Current operation number
   */
  void set_video_writer_size(int op_count);

  /**
   *  Store a video to the data store
   *  @param id Input video path
   *  @param store_id path to the file location where the video should be stored
   *  @param fname path to the temporary file location
   */
  void store_video_no_operation(std::string id, std::string store_id,
                                std::string fname);

  /**
   * Perform operations in a frame-by-frame manner on the video.
   * @param id source video file path
   * @param op_count index of the current operation being executed
   * @param fname path to the temporary file location
   */
  int perform_single_frame_operations(std::string id, int op_count,
                                      std::string fname);
};

} // namespace VCL