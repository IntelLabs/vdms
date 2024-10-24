/**
 * @file   Image.cc
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
 */

#include <chrono>
#include <fcntl.h>
#include <fstream>
#include <jpeglib.h>
#include <png.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "vcl/Exception.h"
#include "vcl/Image.h"

#include "../VDMSConfig.h"

using namespace VCL;

/*  *********************** */
/*        OPERATION         */
/*  *********************** */

/*  *********************** */
/*       READ OPERATION     */
/*  *********************** */

Image::Read::Read(const std::string &filename, VCL::Format format)
    : Operation(format), _fullpath(filename) {}

void Image::Read::operator()(Image *img) {

  if (_format == VCL::Format::TDB) {
    if (img->_tdb == NULL)
      throw VCLException(TileDBNotFound, "VCL::Format indicates image \
                stored in TDB format, but no data was found");

    img->_tdb->read();
    img->_height = img->_tdb->get_image_height();
    img->_width = img->_tdb->get_image_width();
    img->_channels = img->_tdb->get_image_channels();
  } else if (img->_storage == VDMS::StorageType::LOCAL) {
    if (_format == VCL::Format::BIN) {

      FILE *bin_file;
      bin_file = fopen(_fullpath.c_str(), "rb");
      if (bin_file != NULL) {
        // Prevent a possible memory leak
        if (nullptr != img->_bin) {
          free(img->_bin);
          img->_bin = nullptr;
        }

        img->_bin = (char *)malloc(sizeof(char) * img->_bin_size);
        if (nullptr != img->_bin)
          fread(img->_bin, 1, img->_bin_size, bin_file);
        fclose(bin_file);
      } else {
        throw VCLException(OpenFailed, _fullpath + " could not be written");
      }
    } else {
      cv::Mat img_read = cv::imread(_fullpath, cv::IMREAD_ANYCOLOR);
      img->shallow_copy_cv(img_read);
      if (img->_cv_img.empty())
        throw VCLException(ObjectEmpty,
                           _fullpath + " could not be read, object is empty");
    }
  } else //_type == AWS|MINIO
  {
    std::vector<unsigned char> data = img->_remote->Read(_fullpath);
    if (!data.empty())
      img->deep_copy_cv(cv::imdecode(cv::Mat(data), cv::IMREAD_ANYCOLOR));
    else
      throw VCLException(
          ObjectEmpty, _fullpath + " could not be read from RemoteConnection");
  }
}

/*  *********************** */
/*       WRITE OPERATION    */
/*  *********************** */

Image::Write::Write(const std::string &filename, VCL::Format format,
                    VCL::Format old_format, bool metadata)
    : Operation(format), _old_format(old_format), _metadata(metadata),
      _fullpath(filename) {}
// image1.jpg

void Image::Write::operator()(Image *img) {
  if (_format == VCL::Format::TDB) {
    if (img->_tdb == NULL) {
      if (img->_storage == VDMS::StorageType::LOCAL) {
        img->_tdb = new TDBImage(_fullpath);
        img->_tdb->set_compression(img->_compress);
      } else if (img->_storage == VDMS::StorageType::AWS) {
        img->_tdb = new TDBImage(_fullpath, *(img->_remote));
      } else {
        throw VCLException(
            UnsupportedSystem,
            "The system specified by the path is not supported currently");
      }
    }

    if (img->_tdb->has_data()) {
      if (img->_storage == VDMS::StorageType::LOCAL) {
        img->_tdb->set_configuration(img->_remote);
      }
      img->_tdb->write(_fullpath, _metadata);
    } else {
      img->_tdb->write(img->_cv_img, _metadata);
    }
  } else if (_format == VCL::Format::BIN) // TODO: Implement Remote
  {
    FILE *bin_file;
    bin_file = fopen(_fullpath.c_str(), "wb");
    if (bin_file != NULL) {
      fwrite(img->_bin, sizeof(char), img->_bin_size, bin_file);
      fclose(bin_file);
    } else {
      throw VCLException(OpenFailed, _fullpath + " could not be written");
    }
  } else {
    cv::Mat cv_img;
    if (_old_format == VCL::Format::TDB)
      cv_img = img->_tdb->get_cvmat();
    else
      cv_img = img->_cv_img;

    if (!cv_img.empty()) {
      if (img->_storage == VDMS::StorageType::LOCAL) {
        cv::imwrite(_fullpath, cv_img);
      } else {
        std::vector<unsigned char> data;
        std::string ext = "." + VCL::format_to_string(_format);
        cv::imencode(ext, cv_img, data);
        bool result = img->_remote->Write(_fullpath, data);
        if (!result) {
          throw VCLException(ObjectNotFound,
                             "Image: Path to the file was not found");
        }
      }
    } else
      throw VCLException(ObjectEmpty,
                         _fullpath + " could not be written, object is empty");
  }
}

/*  *********************** */
/*       RESIZE OPERATION   */
/*  *********************** */

void Image::Resize::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      img->_tdb->resize(_rect);
      img->_height = img->_tdb->get_image_height();
      img->_width = img->_tdb->get_image_width();
      img->_channels = img->_tdb->get_image_channels();
    } else {
      if (!img->_cv_img.empty()) {
        cv::Mat cv_resized;
        cv::resize(img->_cv_img, cv_resized,
                   cv::Size(_rect.width, _rect.height));
        img->shallow_copy_cv(cv_resized);
      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*       CROP OPERATION     */
/*  *********************** */

void Image::Crop::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      img->_tdb->read(_rect);
      img->_height = img->_tdb->get_image_height();
      img->_width = img->_tdb->get_image_width();
      img->_channels = img->_tdb->get_image_channels();
    } else {
      if (!img->_cv_img.empty()) {
        if (img->_cv_img.rows < _rect.height + _rect.y ||
            img->_cv_img.cols < _rect.width + _rect.x)
          throw VCLException(SizeMismatch,
                             "Requested area is not within the image");
        cv::Mat roi_img(img->_cv_img, _rect);
        img->shallow_copy_cv(roi_img);
      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    THRESHOLD OPERATION   */
/*  *********************** */

void Image::Threshold::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB)
      img->_tdb->threshold(_threshold);
    else {
      if (!img->_cv_img.empty())
        cv::threshold(img->_cv_img, img->_cv_img, _threshold, _threshold,
                      cv::THRESH_TOZERO);
      else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    FLIP OPERATION        */
/*  *********************** */

void Image::Flip::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      // Not implemented
      throw VCLException(NotImplemented,
                         "Operation not supported for this format");
    } else {
      if (!img->_cv_img.empty()) {
        cv::Mat dst =
            cv::Mat(img->_cv_img.rows, img->_cv_img.cols, img->_cv_img.type());
        cv::flip(img->_cv_img, dst, _code);
        img->shallow_copy_cv(dst);
      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    ROTATE OPERATION      */
/*  *********************** */

void Image::Rotate::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      // Not implemented
      throw VCLException(NotImplemented,
                         "Operation not supported for this format");
    } else {
      if (!img->_cv_img.empty()) {

        if (_keep_size) {
          cv::Mat dst = cv::Mat(img->_cv_img.rows, img->_cv_img.cols,
                                img->_cv_img.type());

          cv::Point2f im_c(img->_cv_img.cols / 2., img->_cv_img.rows / 2.);
          cv::Mat r = cv::getRotationMatrix2D(im_c, _angle, 1.0);

          cv::warpAffine(img->_cv_img, dst, r, img->_cv_img.size());
          img->_cv_img = dst.clone();
        } else {

          cv::Point2f im_c((img->_cv_img.cols - 1) / 2.0,
                           (img->_cv_img.rows - 1) / 2.0);
          cv::Mat r = cv::getRotationMatrix2D(im_c, _angle, 1.0);
          // Bbox rectangle
          cv::Rect2f bbox =
              cv::RotatedRect(cv::Point2f(), img->_cv_img.size(), _angle)
                  .boundingRect2f();
          // Transformation Matrix
          r.at<double>(0, 2) += bbox.width / 2.0 - img->_cv_img.cols / 2.0;
          r.at<double>(1, 2) += bbox.height / 2.0 - img->_cv_img.rows / 2.0;

          cv::Mat dst;
          cv::warpAffine(img->_cv_img, dst, r, bbox.size());
          img->shallow_copy_cv(dst);
        }
      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    REMOTE OPERATION      */
/*  *********************** */

void Image::RemoteOperation::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      // Not implemented
      throw VCLException(NotImplemented,
                         "Operation not supported for this format");
    } else {
      if (!img->_cv_img.empty()) {
        img->set_remoteOp_params(_options, _url);
      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    SYNCREMOTE OPERATION  */
/*  *********************** */

size_t writeCallback(char *ip, size_t size, size_t nmemb, void *op) {
  ((std::string *)op)->append((char *)ip, size * nmemb);
  return size * nmemb;
}

void Image::SyncRemoteOperation::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      // Not implemented
      throw VCLException(NotImplemented,
                         "Operation not supported for this format");
    } else {
      if (!img->_cv_img.empty()) {

        std::string readBuffer;

        CURL *curl = NULL;

        CURLcode res;
        struct curl_slist *headers = NULL;
        curl_mime *form = NULL;
        curl_mimepart *field = NULL;

        curl = curl_easy_init();

        if (curl) {
          auto time_now = std::chrono::system_clock::now();
          std::chrono::duration<double> utc_time = time_now.time_since_epoch();

          VCL::Format img_format = img->get_image_format();
          std::string format = VCL::format_to_string(img_format);

          if (format == "" && _options.isMember("format")) {
            format = _options["format"].toStyledString().data();
            format.erase(std::remove(format.begin(), format.end(), '\n'),
                         format.end());
            format = format.substr(1, format.size() - 2);
          } else {
            format = "jpg";
          }

          std::string filePath =
              VDMS::VDMSConfig::instance()->get_path_tmp() + "/tempfile" +
              std::to_string(utc_time.count()) + "." + format;
          cv::imwrite(filePath, img->_cv_img);

          std::ofstream tsfile;

          auto opstart = std::chrono::system_clock::now();

          form = curl_mime_init(curl);

          field = curl_mime_addpart(form);
          curl_mime_name(field, "imageData");
          if (curl_mime_filedata(field, filePath.data()) != CURLE_OK) {
            if (std::remove(filePath.data()) != 0) {
            }
            throw VCLException(ObjectEmpty,
                               "Unable to create file for remoting");
          }

          field = curl_mime_addpart(form);
          curl_mime_name(field, "jsonData");
          if (curl_mime_data(field, _options.toStyledString().data(),
                             _options.toStyledString().length()) != CURLE_OK) {
            if (std::remove(filePath.data()) != 0) {
            }
            throw VCLException(ObjectEmpty, "Unable to create curl mime data");
          }

          // Post data
          if (curl_easy_setopt(curl, CURLOPT_URL, _url.data()) != CURLE_OK) {
            throw VCLException(UndefinedException, "CURL setup error with URL");
          }
          if (curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback) !=
              CURLE_OK) {
            throw VCLException(UndefinedException,
                               "CURL setup error with callback");
          }
          if (curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer) !=
              CURLE_OK) {
            throw VCLException(UndefinedException,
                               "CURL setup error with read buffer");
          }
          if (curl_easy_setopt(curl, CURLOPT_MIMEPOST, form) != CURLE_OK) {
            throw VCLException(UndefinedException,
                               "CURL setup error with form");
          }

          res = curl_easy_perform(curl);

          int http_status_code;
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_status_code);

          curl_easy_cleanup(curl);
          curl_mime_free(form);

          if (http_status_code != 200) {
            if (http_status_code == 0) {
              throw VCLException(ObjectEmpty, "Remote server is not running.");
            }
            if (http_status_code == 400) {
              throw VCLException(ObjectEmpty,
                                 "Invalid Request to the Remote Server.");
            } else if (http_status_code == 404) {
              throw VCLException(ObjectEmpty,
                                 "Invalid URL Request. Please check the URL.");
            } else if (http_status_code == 500) {
              throw VCLException(ObjectEmpty,
                                 "Exception occurred at the remote server. "
                                 "Please check your query.");
            } else if (http_status_code == 503) {
              throw VCLException(ObjectEmpty, "Unable to reach remote server");
            } else {
              throw VCLException(ObjectEmpty, "Remote Server error.");
            }
          }

          std::string delimiter = ":metadata:";

          size_t pos = 0;
          std::string token;
          std::string tmpBuffer = readBuffer;
          if ((pos = tmpBuffer.find(delimiter)) != std::string::npos) {
            readBuffer = tmpBuffer.substr(0, pos);
            tmpBuffer.erase(0, pos + delimiter.length());
            Json::Value message;
            Json::Reader reader;
            bool parsingSuccessful = reader.parse(tmpBuffer, message);
            if (!parsingSuccessful) {
              throw VCLException(ObjectEmpty, "Error parsing string.");
            }
            img->set_ingest_metadata(message["metadata"]);
          }
          // Decode the response
          std::vector<unsigned char> vectordata(readBuffer.begin(),
                                                readBuffer.end());
          cv::Mat data_mat(vectordata, true);

          if (data_mat.empty()) {
            throw VCLException(ObjectEmpty,
                               "Empty response from remote server");
          }

          cv::Mat decoded_mat(cv::imdecode(data_mat, 1));

          img->shallow_copy_cv(decoded_mat);

          if (std::remove(filePath.data()) != 0) {
          }
        }

      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  }
}

/*  *********************** */
/*    USER OPERATION        */
/*  *********************** */

void Image::UserOperation::operator()(Image *img) {
  try {
    if (_format == VCL::Format::TDB) {
      // Not implemented
      throw VCLException(NotImplemented,
                         "Operation not supported for this format");
    } else {
      if (!img->_cv_img.empty()) {

        std::string opfile;

        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::req);

        // This is setting a timeout for avoiding infinite loops
        socket.setsockopt(ZMQ_SNDTIMEO, 10000);// milliseconds
        socket.setsockopt(ZMQ_RCVTIMEO, 30000);

        std::string port = _options["port"].asString();
        std::string address = "tcp://127.0.0.1:" + port;

        socket.connect(address.data());

        auto time_now = std::chrono::system_clock::now();
        std::chrono::duration<double> utc_time = time_now.time_since_epoch();

        VCL::Format img_format = img->get_image_format();
        std::string format = VCL::format_to_string(img_format);

        if (format == "" && _options.isMember("format")) {
          format = _options["format"].toStyledString().data();
          format.erase(std::remove(format.begin(), format.end(), '\n'),
                       format.end());
          format = format.substr(1, format.size() - 2);
        } else {
          format = "jpg";
        }

        std::string filePath = VDMS::VDMSConfig::instance()->get_path_tmp() +
                               "/tempfile" + std::to_string(utc_time.count()) +
                               "." + format;
        cv::imwrite(filePath, img->_cv_img);

        _options["ipfile"] = filePath;
        _options["media_type"] = "image";

        std::string message_to_send = _options.toStyledString();

        int message_len = message_to_send.length();
        zmq::message_t ipfile(message_len);
        memcpy(ipfile.data(), message_to_send.data(), message_len);

        socket.send(ipfile, 0);
        std::string response;
        while (true) {
          zmq::message_t reply;
          socket.recv(&reply);

          response =
              std::string(static_cast<char *>(reply.data()), reply.size());

          break;
        }

        std::string delimiter = "metadata";
        size_t pos;
        if ((pos = response.find(delimiter)) != std::string::npos) {
          Json::Value message;
          Json::Reader reader;
          bool parsingSuccessful = reader.parse(response, message);
          if (!parsingSuccessful) {
            throw VCLException(ObjectEmpty, "Error parsing string.");
          }
          img->set_ingest_metadata(message["metadata"]);
        } else {
          if (response == "") {
            std::string errorMessage = "UserOperation error: Timeout, no response from the server";
            std::cout << errorMessage << std::endl;
            throw VCLException(SystemNotFound, errorMessage);
          }          
          opfile = response;
          std::ifstream rfile;
          rfile.open(opfile);         

          if (rfile) {
            rfile.close();
          } else {
            if (std::remove(filePath.data()) != 0) {
              throw VCLException(ObjectNotFound, "Unable to remove file");
            }
            throw VCLException(OpenFailed, "UDF Error with file: " + filePath + ". Response:" + opfile);
          }

          VCL::Image res_image(opfile);
          img->shallow_copy_cv(res_image.get_cvmat(true));

          if (std::remove(filePath.data()) != 0) {
            throw VCLException(ObjectNotFound, "Unable to remove file");
          }

          if (std::remove(opfile.data()) != 0) {
            throw VCLException(ObjectNotFound, "Unable to remove file");
          }
        }

      } else
        throw VCLException(ObjectEmpty, "Image object is empty");
    }
    img->_op_completed++;
  } catch (VCL::Exception e) {
    img->set_query_error_response(e.msg);
    print_exception(e);
    return;
  } catch (std::exception &e) {
    img->set_query_error_response(e.what());
    std::cerr << "UserOperation exception: " << e.what() << std::endl;
    return;
  } catch (...) {
    img->set_query_error_response("Unknown error");
    std::cerr << "Unknown error" << std::endl;
    return;
  }
}

/*  *********************** */
/*        CONSTRUCTORS      */
/*  *********************** */

Image::Image() {
  _channels = 0;
  _height = 0;
  _width = 0;
  _cv_type = CV_8UC3;

  _format = VCL::Format::NONE_IMAGE;
  _compress = CompressionType::LZ4;

  _tdb = nullptr;
  _image_id = "";
  _bin = nullptr;
  _bin_size = 0;
  _remote = nullptr;
  _op_completed = 0;
}

Image::Image(const std::string &image_id, std::string bucket_name) {
  _remote = nullptr;
  if (bucket_name.length() != 0) {
    VCL::RemoteConnection *connection = new VCL::RemoteConnection();
    connection->_bucket_name = bucket_name;
    set_connection(connection);
  }

  _channels = 0;
  _height = 0;
  _width = 0;
  _cv_type = CV_8UC3;
  _bin = nullptr;
  _bin_size = 0;

  std::string extension = get_extension(image_id);
  set_format(extension);
  _compress = CompressionType::LZ4;
  _image_id = create_fullpath(image_id, _format);

  if (_format == VCL::Format::TDB) {
    _tdb = new TDBImage(_image_id);
    _tdb->set_compression(_compress);
  } else
    _tdb = nullptr;

  read(image_id);

  _op_completed = 0;
}

Image::Image(const std::string &image_id, bool no_blob) {
  _remote = nullptr;

  _channels = 0;
  _height = 0;
  _width = 0;
  _cv_type = CV_8UC3;
  _bin = 0;
  _bin_size = 0;

  std::string extension = get_extension(image_id);
  set_format(extension);

  _compress = CompressionType::LZ4;

  _image_id = create_fullpath(image_id, _format);
  _no_blob = no_blob;

  _tdb = nullptr;
  read(image_id);

  _op_completed = 0;
}

Image::Image(const cv::Mat &cv_img, bool copy) {
  if (cv_img.empty()) {
    throw VCLException(ObjectEmpty, "Image object is empty");
  }

  _remote = nullptr;

  if (copy)
    deep_copy_cv(cv_img);
  else
    shallow_copy_cv(cv_img);

  _format = VCL::Format::NONE_IMAGE;
  _compress = CompressionType::LZ4;
  _image_id = "";
  _tdb = nullptr;
  _bin = nullptr;
  _bin_size = 0;

  _op_completed = 0;
}
Image::Image(const std::string &filename, const std::string &blob,
             VCL::Format input_format) {
  _channels = 0;
  _height = 0;
  _width = 0;
  _cv_type = CV_8UC3;

  _format = input_format;
  _compress = CompressionType::LZ4;

  _tdb = nullptr;
  _image_id = "";
  _bin = nullptr;
  _bin_size = 0;
  _remote = nullptr;
  _op_completed = 0;
}

Image::Image(void *buffer, long size, char binary_image_flag, int flags) {
  _bin = nullptr;
  _bin_size = 0;
  _remote = nullptr;

  _tdb = nullptr;
  // _bin = nullptr; Duplicated?
  if (size < 0) {
    std::cerr << "Error: Invalid buffer size." << std::endl;
    return;
  }

  _format = VCL::Format::NONE_IMAGE;

  set_data_from_encoded(buffer, size, binary_image_flag, flags);

  _compress = CompressionType::LZ4;
  _image_id = "";
  _op_completed = 0;
}

Image::Image(void *buffer, cv::Size dimensions, int cv_type) {
  _bin = nullptr;
  _bin_size = 0;
  _remote = nullptr;

  _height = dimensions.height;
  _width = dimensions.width;
  _cv_type = cv_type;
  _channels = (cv_type / 8) + 1;

  _format = VCL::Format::TDB;
  _compress = CompressionType::LZ4;
  _image_id = "";

  set_data_from_raw(buffer, _height * _width * _channels);
  _tdb->set_compression(_compress);

  _op_completed = 0;
}

Image::Image(const Image &img, bool copy) {
  _bin = nullptr;
  _bin_size = 0;
  _remote = nullptr;

  _height = img._height;
  _width = img._width;
  _cv_type = img._cv_type;
  _channels = img._channels;

  _format = img._format;
  _compress = img._compress;
  _image_id = img._image_id;
  _no_blob = img._no_blob;

  if (!(img._cv_img).empty()) {
    if (copy) {
      deep_copy_cv(img._cv_img);
    } else {
      shallow_copy_cv(img._cv_img);
    }
  }

  if (img._tdb != NULL)
    _tdb = new TDBImage(*img._tdb);
  else
    _tdb = NULL;

  int start;
  if (img._operations.size() > 0) {
    std::shared_ptr<Operation> front = img._operations.front();
    if (front->get_type() == OperationType::READ) {
      start = 1;
      cv::Mat img_read = cv::imread(img._image_id, cv::IMREAD_ANYCOLOR);
      shallow_copy_cv(img_read);
    } else
      start = 0;

    for (int i = start; i < img._operations.size(); ++i) {
      op_labels.push_back(img.op_labels[i]);
      _operations.push_back(img._operations[i]);
    }
  }

  _op_completed = img._op_completed;
  remoteOp_params = img.remoteOp_params;
}

Image::Image(Image &&img) noexcept {
  _bin = nullptr;
  _bin_size = 0;
  _remote = nullptr;
  _no_blob = img._no_blob;

  _format = img._format;
  _compress = img._compress;
  _image_id = img._image_id;
  _tdb = img._tdb;
  _operations = std::move(img._operations);
  shallow_copy_cv(img._cv_img);
  op_labels = img.op_labels;

  img._tdb = NULL;

  _op_completed = img._op_completed;
  remoteOp_params = img.remoteOp_params;
}

Image &Image::operator=(const Image &img) {

  TDBImage *temp = _tdb;
  _bin = nullptr;
  _bin_size = 0;
  if (!(img._cv_img).empty())
    deep_copy_cv(img._cv_img);
  else {
    _channels = img._channels;

    _height = img._height;
    _width = img._width;

    _cv_type = img._cv_type;
  }

  _format = img._format;
  _compress = img._compress;
  _image_id = img._image_id;
  _no_blob = img._no_blob;

  if (img._tdb != NULL) {
    _tdb = new TDBImage(*img._tdb);
  } else
    _tdb = NULL;

  int start;

  _operations.clear();
  _operations.shrink_to_fit();

  if (img._operations.size() > 0) {
    std::shared_ptr<Operation> front = img._operations.front();
    if (front->get_type() == OperationType::READ) {
      start = 1;
      cv::Mat img_read = cv::imread(img._image_id, cv::IMREAD_ANYCOLOR);
      shallow_copy_cv(img_read);
    } else
      start = 0;

    for (int i = start; i < img._operations.size(); ++i) {
      op_labels.push_back(img.op_labels[i]);
      _operations.push_back(img._operations[i]);
    }
  }

  _op_completed = img._op_completed;
  remoteOp_params = img.remoteOp_params;

  delete temp;

  return *this;
}

Image::~Image() {
  _operations.clear();
  _operations.shrink_to_fit();
  if (_tdb) {
    delete _tdb;
    _tdb = nullptr;
  }

  if (_bin) {
    free(_bin);
    _bin = nullptr;
  }
}

/*  *********************** */
/*        GET FUNCTIONS     */
/*  *********************** */
void Image::save_image(const std::string &fullpath, const std::string &blob) {
  if (_storage == VDMS::StorageType::LOCAL) {

    FILE *bin_file = fopen(fullpath.c_str(), "wb");
    if (bin_file != NULL) {
      // Write the blob data to the binary file
      fwrite(blob.data(), sizeof(char), blob.size(), bin_file);
      fclose(bin_file); // Close the file when done
    } else {
      // Handle the case where the file couldn't be opened
      std::cerr << "Failed to open " << fullpath << " for writing."
                << std::endl;
    }
  } else {
    // store the image in AWS
    std::vector<unsigned char> data(blob.begin(), blob.end());
    _remote->Write(fullpath, data);
  }
}
std::string Image::get_image_id() const { return _image_id; }

bool Image::is_blob_not_stored() const { return _no_blob; }

cv::Size Image::get_dimensions(bool performOp) {
  //  TODO: iterate over operations themsevles to determine
  //        image size, rather than performing the operations.
  if (_operations.size() > 0 && performOp)
    perform_operations();
  return cv::Size(_width, _height);
}

VCL::Format Image::get_image_format() const { return _format; }

long Image::get_raw_data_size() {
  if (_height == 0) {
    if (_format == VCL::Format::TDB) {
      if (_tdb == NULL)
        throw VCLException(TileDBNotFound, "VCL::Format indicates image \
                    stored in TDB format, but no data was found");
      return _tdb->get_image_size();
    } else {
      std::shared_ptr<Operation> op = _operations.front();
      (*op)(this);
      _operations.erase(_operations.begin());
    }
  }

  return long(_height) * long(_width) * _channels;
}

int Image::get_image_type() const { return _cv_type; }

Image Image::get_area(const Rectangle &roi, bool performOp) const {
  Image area(*this);

  if (area._format == VCL::Format::TDB && area._operations.size() == 1) {
    if (area._tdb == NULL)
      throw VCLException(TileDBNotFound, "VCL::Format indicates image \
                stored in TDB format, but no data was found");
    area._operations.pop_back();
  }

  std::shared_ptr<Operation> op = std::make_shared<Crop>(roi, area._format);

  area.op_labels.push_back("get_area");
  area._operations.push_back(op);

  if (performOp)
    area.perform_operations();

  area._height = roi.height;
  area._width = roi.width;

  return area;
}

cv::Mat Image::get_cvmat(bool copy, bool performOp) {
  if (performOp)
    perform_operations();

  cv::Mat mat = (_format == VCL::Format::TDB) ? _tdb->get_cvmat() : _cv_img;

  if (copy)
    return mat.clone();
  else
    return mat;
}

void Image::get_raw_data(void *buffer, long buffer_size, bool performOp) {
  if (performOp)
    perform_operations();

  switch (_cv_type % 8) {
  case 0:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<unsigned char *>(buffer));
    else
      _tdb->get_buffer(static_cast<unsigned char *>(buffer), buffer_size);
    break;
  case 1:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<char *>(buffer));
    else
      _tdb->get_buffer(static_cast<char *>(buffer), buffer_size);
    break;
  case 2:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<unsigned short *>(buffer));
    else
      _tdb->get_buffer(static_cast<unsigned short *>(buffer), buffer_size);
    break;
  case 3:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<short *>(buffer));
    else
      _tdb->get_buffer(static_cast<short *>(buffer), buffer_size);
    break;
  case 4:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<int *>(buffer));
    else
      _tdb->get_buffer(static_cast<int *>(buffer), buffer_size);
    break;
  case 5:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<float *>(buffer));
    else
      _tdb->get_buffer(static_cast<float *>(buffer), buffer_size);
    break;
  case 6:
    if (_format != VCL::Format::TDB)
      copy_to_buffer(static_cast<double *>(buffer));
    else
      _tdb->get_buffer(static_cast<double *>(buffer), buffer_size);
    break;
  default:
    throw VCLException(UnsupportedFormat, _cv_type + " is not a \
                supported type");
    break;
  }
}

int Image::get_enqueued_operation_count() { return _operations.size(); }

int Image::get_op_completed() { return _op_completed; }

Json::Value Image::get_remoteOp_params() { return remoteOp_params; }

std::string Image::get_query_error_response() { return _query_error_response; }

std::vector<Json::Value> Image::get_ingest_metadata() {
  return _ingest_metadata;
}

std::vector<unsigned char>
Image::get_encoded_image(VCL::Format format, const std::vector<int> &params) {
  if (format == VCL::Format::BIN) {
    std::ifstream bin_image(_image_id, std::ios::in | std::ifstream::binary);
    if (!bin_image) {
      std::cerr << "Error opening image file: " << _image_id << std::endl;
    }
    long file_size = bin_image.tellg();

    bin_image.seekg(0, std::ios::end);
    file_size = bin_image.tellg() - file_size;
    std::vector<unsigned char> buffer(file_size, 0);
    bin_image.seekg(0, std::ios::beg);
    bin_image.read((char *)&buffer[0], file_size);
    bin_image.close();
    return buffer;

  }

  else {
    perform_operations();

    std::string extension = "." + format_to_string(format);

    if (_cv_img.empty()) {
      if (_tdb == NULL)
        throw VCLException(ObjectEmpty, "No data to encode");
      else {
        cv::Mat img = _tdb->get_cvmat();
        shallow_copy_cv(img);
      }
    }

    std::vector<unsigned char> buffer;
    cv::imencode(extension, _cv_img, buffer, params);
    return buffer;
  }
}

std::vector<unsigned char>
Image::get_encoded_image_async(VCL::Format format,
                               const std::vector<int> &params) {

  // When data is stored in raw binary format, read data from file
  if (format == VCL::Format::BIN) {
    std::ifstream bin_image(_image_id, std::ios::in | std::ifstream::binary);
    long file_size = bin_image.tellg();
    bin_image.seekg(0, std::ios::end);
    file_size = bin_image.tellg() - file_size;
    std::vector<unsigned char> buffer(file_size, 0);
    bin_image.seekg(0, std::ios::beg);
    bin_image.read((char *)&buffer[0], file_size);
    bin_image.close();
    return buffer;

  }

  else {
    std::string extension = "." + format_to_string(format);

    if (_cv_img.empty()) {
      if (_tdb == NULL)
        throw VCLException(ObjectEmpty, "No data to encode");
      else {
        cv::Mat img = _tdb->get_cvmat();
        shallow_copy_cv(img);
      }
    }

    std::vector<unsigned char> buffer;
    cv::imencode(extension, _cv_img, buffer, params);
    return buffer;
  }
}

/*  *********************** */
/*        SET FUNCTIONS     */
/*  *********************** */

void Image::set_data_from_raw(void *buffer, long size) {
  switch (_cv_type % 8) {
  case 0:
    _tdb = new TDBImage(static_cast<unsigned char *>(buffer), size);
    break;
  case 1:
    _tdb = new TDBImage(static_cast<char *>(buffer), size);
    break;
  case 2:
    _tdb = new TDBImage(static_cast<unsigned short *>(buffer), size);
    break;
  case 3:
    _tdb = new TDBImage(static_cast<short *>(buffer), size);
    break;
  case 4:
    _tdb = new TDBImage(static_cast<int *>(buffer), size);
    break;
  case 5:
    _tdb = new TDBImage(static_cast<float *>(buffer), size);
    break;
  case 6:
    _tdb = new TDBImage(static_cast<double *>(buffer), size);
    break;
  default:
    throw VCLException(UnsupportedFormat, _cv_type + " is not a \
                supported type");
    break;
  }
}

void Image::set_data_from_encoded(void *buffer, long size,
                                  char binary_image_flag, int flags) {
  // with raw binary files, we simply copy the data and do not encode
  if (binary_image_flag) {
    _bin_size = size;
    if (_bin) {
      free(_bin);
      _bin = nullptr;
    }
    _bin = (char *)malloc(sizeof(char) * size);
    memcpy(_bin, buffer, size);
  } else {
    cv::Mat raw_data(cv::Size(size, 1), CV_8UC1, buffer);
    cv::Mat img = cv::imdecode(raw_data, flags);
    if (img.empty()) {
      throw VCLException(ObjectEmpty, "Image object is empty");
    }

    //
    // We can safely make a shallow-copy here, as cv::Mat uses a reference
    // counter to keep track of the references
    //
    shallow_copy_cv(img);
  }
}

void Image::set_compression(CompressionType comp) { _compress = comp; }

void Image::set_dimensions(cv::Size dims) {
  _height = dims.height;
  _width = dims.width;

  if (_format == VCL::Format::TDB) {
    if (_tdb == NULL)
      throw VCLException(TileDBNotFound, "VCL::Format indicates image \
                stored in TDB format, but no data was found");
    _tdb->set_image_properties(_height, _width, _channels);
  }
}

void Image::set_format(const std::string &extension) {
  if (extension == "jpg")
    _format = VCL::Format::JPG;
  else if (extension == "png")
    _format = VCL::Format::PNG;
  else if (extension == "tdb")
    _format = VCL::Format::TDB;
  else if (extension == "bin")
    _format = VCL::Format::BIN;
  else
    throw VCLException(UnsupportedFormat, extension + " is not a \
            supported format");
}

void Image::set_image_type(int cv_type) {
  _cv_type = cv_type;

  _channels = (cv_type / 8) + 1;
}

void Image::set_minimum_dimension(int dimension) {
  if (_format == VCL::Format::TDB) {
    if (_tdb == NULL)
      throw VCLException(TileDBNotFound, "VCL::Format indicates image \
                stored in TDB format, but no data was found\n");
    _tdb->set_minimum(dimension);
  }
}

void Image::set_remoteOp_params(Json::Value options, std::string url) {
  remoteOp_params["options"] = options;
  remoteOp_params["url"] = url;
}

void Image::set_query_error_response(std::string response_error) {
  _query_error_response = response_error;
}

void Image::set_ingest_metadata(Json::Value metadata) {
  _ingest_metadata.push_back(metadata);
}

void Image::update_op_completed() { _op_completed++; }

void Image::set_connection(RemoteConnection *remote) {
  if (!remote) {
    throw VCLException(SystemNotFound, "Invalid remote connection");
  }

  if (!remote->connected())
    remote->start();

  if (!remote->connected()) {
    throw VCLException(SystemNotFound, "No remote connection started");
  }

  _remote = remote;
  _storage = VDMS::StorageType::AWS;

  if (_tdb != NULL) {
    _tdb->set_configuration(remote);
  }
}

/*  *********************** */
/*    IMAGE INTERACTIONS    */
/*  *********************** */

void Image::perform_operations() {
  try {
    for (int x = 0; x < _operations.size(); ++x) {
      std::string op_name = op_labels[x];
      timers.add_timestamp(op_name);
      std::shared_ptr<Operation> op = _operations[x];
      if (op == NULL)
        throw VCLException(ObjectEmpty, "Nothing to be done");
      (*op)(this);
      timers.add_timestamp(op_name);
    }

  } catch (cv::Exception &e) {
    throw VCLException(OpenCVError, e.what());
  }

  _operations.clear();
}

int Image::execute_operation() {
  std::shared_ptr<Operation> op = _operations[_op_completed];
  if (op == NULL)
    throw VCLException(ObjectEmpty, "Nothing to be done");

  if ((*op).get_type() != VCL::Image::OperationType::REMOTEOPERATION) {
    (*op)(this);
    if (this->get_query_error_response() == "") {
      return 0;
    } else {
      return -2;
    }
  } else {
    (*op)(this);
    if (this->get_query_error_response() == "") {
      return -1;
    } else {
      return -2;
    }
  }
}

void Image::read(const std::string &image_id) {
  _image_id = create_fullpath(image_id, _format);
  op_labels.push_back("read");
  _operations.push_back(std::make_shared<Read>(_image_id, _format));
}

void Image::store(const std::string &image_id, VCL::Format image_format,
                  bool store_metadata) {
  op_labels.push_back("store");
  _operations.push_back(
      std::make_shared<Write>(create_fullpath(image_id, image_format),
                              image_format, _format, store_metadata));
  perform_operations();
}

bool Image::delete_image() {
  if (_tdb != NULL)
    _tdb->delete_image();

  if (exists(_image_id)) {
    std::remove(_image_id.c_str());
  } else if (_remote != NULL) {
    bool result = _remote->Remove_Object(_image_id);
    return result;
  }

  return true;
}

void Image::resize(int new_height, int new_width) {
  _operations.push_back(std::make_shared<Resize>(
      Rectangle(0, 0, new_width, new_height), _format));
  op_labels.push_back("resize");
}

void Image::crop(const Rectangle &rect) {
  if (_format == Format::TDB && _operations.size() == 1) {
    if (_tdb == NULL)
      throw VCLException(TileDBNotFound, "VCL::Format indicates image \
                stored in TDB format, but no data was found");
    _operations.pop_back();
  }

  _operations.push_back(std::make_shared<Crop>(rect, _format));
  op_labels.push_back("crop");
}

void Image::threshold(int value) {
  _operations.push_back(std::make_shared<Threshold>(value, _format));
  op_labels.push_back("threshold");
}

void Image::flip(int code) {
  _operations.push_back(std::make_shared<Flip>(code, _format));
  op_labels.push_back("flip");
}

void Image::rotate(float angle, bool keep_size) {
  _operations.push_back(std::make_shared<Rotate>(angle, keep_size, _format));
  op_labels.push_back("rotate");
}

void Image::syncremoteOperation(std::string url, Json::Value options) {
  _operations.push_back(
      std::make_shared<SyncRemoteOperation>(url, options, _format));
  op_labels.push_back("sync_remote");
}

void Image::remoteOperation(std::string url, Json::Value options) {
  _operations.push_back(
      std::make_shared<RemoteOperation>(url, options, _format));
  op_labels.push_back("remote_op");
}

void Image::userOperation(Json::Value options) {
  _operations.push_back(std::make_shared<UserOperation>(options, _format));
  op_labels.push_back("user_op");
}

/*  *********************** */
/*      COPY FUNCTIONS      */
/*  *********************** */

void Image::deep_copy_cv(const cv::Mat &cv_img) {
  _channels = cv_img.channels();

  _height = cv_img.rows;
  _width = cv_img.cols;

  _cv_type = cv_img.type();

  _cv_img = cv_img.clone(); // deep copy
}

void Image::shallow_copy_cv(const cv::Mat &cv_img) {
  _channels = cv_img.channels();

  _height = cv_img.rows;
  _width = cv_img.cols;
  _cv_type = cv_img.type();
  _cv_img = cv_img; // shallow copy
}

template <class T> void Image::copy_to_buffer(T *buffer) {

  static_assert(std::is_integral<T>::value || std::is_floating_point<T>::value,
                "Cannot copy from T");

  int index = 0;

  int rows = _height;
  int columns = _width;

  if (_cv_img.isContinuous()) {
    columns *= rows;
    rows = 1;
  }

  for (int i = 0; i < rows; ++i) {
    for (int j = 0; j < columns; ++j) {
      if (_channels == 1)
        buffer[index] = T(_cv_img.at<unsigned char>(i, j));
      else {
        cv::Vec3b colors = _cv_img.at<cv::Vec3b>(i, j);
        for (int x = 0; x < _channels; ++x) {
          buffer[index + x] = T(colors.val[x]);
        }
      }
      index += _channels;
    }
  }
}

/*  *********************** */
/*      UTIL FUNCTIONS      */
/*  *********************** */

std::string Image::create_fullpath(const std::string &filename,
                                   VCL::Format format) {
  if (filename == "")
    throw VCLException(ObjectNotFound, "Location to write object is undefined");

  std::string extension = get_extension(filename);
  std::string ext = VCL::format_to_string(format);

  if (ext.compare(extension) == 0 || ext == "")
    return filename;
  else
    return filename + "." + ext;
}
