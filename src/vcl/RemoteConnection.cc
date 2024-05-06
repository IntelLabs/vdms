/**
 * @file   RemoteConnection.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2022-2023 Intel Corporation
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
 * This file declares the C++ API for RemoteConnection, which allows users to
 *  connect to different file systems. At the moment, S3 is enabled.
 */
#include <filesystem>

#include "../../include/VDMSConfigHelper.h"
#include "../../include/vcl/Exception.h"
#include "../../include/vcl/RemoteConnection.h"
#include "../../src/VDMSConfig.h"

using namespace VCL;
namespace fs = std::filesystem;

// CONSTRUCTOR
RemoteConnection::RemoteConnection() {
  _remote_connected = false;
  _aws_client = nullptr;
  _aws_sdk_options = nullptr;
}

// DESTRUCTOR
RemoteConnection::~RemoteConnection() {}

void RemoteConnection::start() {
  try {
    ConfigureAws();
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::start", ex.what());
  }
}

void RemoteConnection::end() {
  try {
    ShutdownAws();
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::end", ex.what());
  }
}

void RemoteConnection::ConfigureAws() {
  try {
    _aws_sdk_options = new Aws::SDKOptions();
    Aws::InitAPI(*_aws_sdk_options);

    Aws::Client::ClientConfiguration clientConfig;

    std::optional<std::string> value = std::nullopt;
    if (value = VDMS::VDMSConfig::instance()->get_proxy_host()) {
      clientConfig.proxyHost = *value;
    }

    std::optional<int> port_value = std::nullopt;
    if (port_value = VDMS::VDMSConfig::instance()->get_proxy_port()) {
      clientConfig.proxyPort = *port_value;
    }

    if (value = VDMS::VDMSConfig::instance()->get_proxy_scheme()) {
      if (*value == "http") {
        clientConfig.proxyScheme = Aws::Http::Scheme::HTTP;
      } else if (*value == "https") {
        clientConfig.proxyScheme = Aws::Http::Scheme::HTTPS;
      } else {
        std::cerr << "Error: Invalid scheme in the config file" << std::endl;
      }
    }

    // Use this property to set the endpoint for MinIO when the use_endpoint
    // value in the config file is equals to true and the storage type is equals
    // to AWS Format: "http://127.0.0.1:9000";
    if ((VDMS::VDMSConfig::instance()->get_storage_type() ==
         VDMS::StorageType::AWS) &&
        (VDMS::VDMSConfig::instance()->get_use_endpoint()) &&
        (VDMS::VDMSConfig::instance()->get_endpoint_override())) {
      value = VDMS::VDMSConfig::instance()->get_endpoint_override();
      clientConfig.endpointOverride = *value;
    }

    // Set AWS Logging level
    if (_aws_sdk_options) {
      _aws_sdk_options->loggingOptions.logLevel =
          VDMS::VDMSConfig::instance()->get_aws_log_level();
    }

    _aws_client = new Aws::S3::S3Client(clientConfig);
    _remote_connected = true;
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::ConfigureAws", ex.what());
  }
}

void RemoteConnection::ShutdownAws() {
  try {
    // LogEntry(__FUNCTION__);
    Aws::ShutdownAPI(*_aws_sdk_options);
    _remote_connected = false;
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::ShutdownAws", ex.what());
  }
}

// image file, takes path to store and vector<uchar> of data
// TODO: make the raw data a more efficient format?
bool RemoteConnection::Write(const std::string &path,
                             std::vector<unsigned char> data) {
  try {
    if (_remote_connected) {
      return write_s3(path, data);
    } else {
      std::cerr << "WRITE: The RemoteConnection has not been started"
                << std::endl;
      return false;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::Write", ex.what());
  }
  return false;
}

// video file (or any file on disk specified by full path)
// opens file, reads into memory, uploads to AWS
bool RemoteConnection::Write(const std::string &filename) {
  try {
    if (_remote_connected) {
      return write_s3(filename);
    } else {
      std::cerr << "WRITE: The RemoteConnection has not been started"
                << std::endl;
      return false;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::Write", ex.what());
  }
  return false;
}

bool RemoteConnection::RetrieveFile(const std::string &filename) {
  try {
    if (_remote_connected) {
      return retrieve_file(filename);
    } else {
      std::cerr << "WRITE: The RemoteConnection has not been started"
                << std::endl;
      return false;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::RetrieveFile", ex.what());
  }
  return false;
}

std::vector<std::string>
RemoteConnection::ListFilesInFolder(const std::string &folder_name) {
  try {
    if (_remote_connected) {
      return get_file_list(folder_name);
    } else {
      std::cerr << "WRITE: The RemoteConnection has not been started"
                << std::endl;
      return std::vector<std::string>();
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::ListFilesInFolder", ex.what());
  }
  return std::vector<std::string>();
}

std::vector<unsigned char> RemoteConnection::Read(const std::string &path) {
  try {
    if (_remote_connected) {
      return read_s3(path);
    } else {
      std::cerr << "READ: The RemoteConnection has not been started"
                << std::endl;
    }
    return std::vector<unsigned char>();
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::Read", ex.what());
  }
  return std::vector<unsigned char>();
}

bool RemoteConnection::Read_Video(const std::string &path) {
  try {
    bool result = false;
    if (_remote_connected) {
      result = read_s3_video(path);
    } else {
      std::cerr << "READ_Video: The RemoteConnection has not been started"
                << std::endl;
      result = false;
    }

    return result;
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::Read_Video", ex.what());
  }

  return false;
}

bool RemoteConnection::Remove_Object(const std::string &path) {
  try {
    if (_remote_connected) {
      return remove_s3_object(path);
    } else {
      std::cerr << "REMOVE: The RemoteConnection has not been started"
                << std::endl;
      return false;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::Remove_Object", ex.what());
  }
  return false;
}

// ########Private S3 Functions########

bool RemoteConnection::write_s3(const std::string &filename) {
  try {
    if (fs::is_directory(filename)) {
      std::cerr << "Upload to S3 failed: " << filename
                << " is a directory instead of a regular file." << std::endl;
      return false;
    }
    Aws::S3::Model::PutObjectRequest put_request;
    put_request.SetBucket(_bucket_name);
    put_request.SetKey(filename);

    std::shared_ptr<Aws::IOStream> inputData = Aws::MakeShared<Aws::FStream>(
        "SampleAllocationTag", filename.c_str(),
        std::ios_base::in | std::ios_base::binary);

    if (!*inputData) {
      std::cerr << "Error unable to read file " << filename << std::endl;
      return false;
    }

    put_request.SetBody(inputData);

    Aws::S3::Model::PutObjectOutcome outcome =
        _aws_client->PutObject(put_request);

    if (!outcome.IsSuccess()) {
      const Aws::S3::S3Error &err = outcome.GetError();
      std::cerr << "Error: PutObject: " << err.GetExceptionName() << ": "
                << err.GetMessage() << ". Bucket: " << _bucket_name
                << ", key: " << filename << std::endl;
      return false;
    } else {
      std::cout << "Added object '" << filename
                << "' to bucket: " << _bucket_name << std::endl;
      return true;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::write_s3", ex.what());
  }
  return false;
}

bool RemoteConnection::write_s3(const std::string &path,
                                std::vector<unsigned char> data) {
  try {
    Aws::S3::Model::PutObjectRequest put_request;
    put_request.SetBucket(_bucket_name);
    put_request.SetKey(path);

    auto input_data =
        Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
    input_data->write(reinterpret_cast<char *>(data.data()), data.size());

    put_request.SetBody(input_data);
    Aws::S3::Model::PutObjectOutcome outcome =
        _aws_client->PutObject(put_request);

    if (!outcome.IsSuccess()) {
      const Aws::S3::S3Error &err = outcome.GetError();
      std::cerr << "Error: PutObject: " << err.GetExceptionName() << ": "
                << err.GetMessage() << std::endl;
      return false;
    } else {
      std::cout << "Added object '" << path << "' to bucket: " << _bucket_name
                << std::endl;
      return true;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::write_s3", ex.what());
  }
  return false;
}

bool RemoteConnection::read_s3_video(const std::string &file_path) {
  try {
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(_bucket_name);
    request.SetKey(file_path);

    Aws::S3::Model::GetObjectOutcome outcome = _aws_client->GetObject(request);

    if (!outcome.IsSuccess()) {
      const Aws::S3::S3Error &err = outcome.GetError();
      std::cerr << "Error: GetObject: " << err.GetExceptionName() << ": "
                << err.GetMessage() << " Key: " << file_path << std::endl;
      return false;
    } else {
      std::cout << "Successfully retrieved '" << file_path << "' from '"
                << _bucket_name << "'." << std::endl;

      auto &retrieved_file = outcome.GetResult().GetBody();
      std::ofstream output_file(file_path.c_str(),
                                std::ios::out | std::ios::binary);
      output_file << retrieved_file.rdbuf();
      return true;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::read_s3_video", ex.what());
  }
  return false;
}

std::vector<unsigned char>
RemoteConnection::read_s3(const std::string &file_path) {
  try {
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(_bucket_name);
    request.SetKey(file_path);

    Aws::S3::Model::GetObjectOutcome outcome = _aws_client->GetObject(request);

    if (!outcome.IsSuccess()) {
      const Aws::S3::S3Error &err = outcome.GetError();
      std::cerr << "Error: GetObject: " << err.GetExceptionName() << ": "
                << err.GetMessage() << " Key: " << file_path << std::endl;
      return std::vector<unsigned char>();
    } else {
      std::cout << "Successfully retrieved '" << file_path << "' from '"
                << _bucket_name << "'." << std::endl;

      std::stringstream stream;
      stream << outcome.GetResult().GetBody().rdbuf();
      std::string str_stream = stream.str();
      std::vector<unsigned char> data(str_stream.begin(), str_stream.end());
      return data;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::read_s3", ex.what());
  }
  return std::vector<unsigned char>();
}

bool RemoteConnection::retrieve_file(const std::string &file_path) {
  try {
    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(_bucket_name);
    request.SetKey(file_path);

    Aws::S3::Model::GetObjectOutcome outcome = _aws_client->GetObject(request);

    if (!outcome.IsSuccess()) {
      const Aws::S3::S3Error &err = outcome.GetError();
      std::cerr << "Error: GetObject: " << err.GetExceptionName() << ": "
                << err.GetMessage() << std::endl;
      return false;
    } else {
      std::cout << "Successfully retrieved '" << file_path << "' from '"
                << _bucket_name << "'." << std::endl;

      auto &retrieved_file = outcome.GetResult().GetBody();
      std::ofstream output_file(file_path.c_str(),
                                std::ios::out | std::ios::binary);
      output_file << retrieved_file.rdbuf();
      return true;
    }
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::retrieve_file", ex.what());
  }
  return false;
}

std::vector<std::string>
RemoteConnection::get_file_list(const std::string &path) {
  std::vector<std::string> results;
  try {

    Aws::S3::Model::ListObjectsRequest request;
    request.SetBucket(_bucket_name);
    request.SetPrefix(path);

    Aws::S3::Model::ListObjectsOutcome outcome =
        _aws_client->ListObjects(request);

    if (!outcome.IsSuccess()) {
      std::string error_message =
          "Error in get_file_list(): " + outcome.GetError().GetMessage();
      throw VCLException(ObjectNotFound, error_message);
    } else {
      Aws::Vector<Aws::S3::Model::Object> objects =
          outcome.GetResult().GetContents();

      for (Aws::S3::Model::Object &object : objects) {
        results.push_back(object.GetKey());
      }
    }
    return results;

  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::get_file_list", ex.what());
  }
  return results;
}

bool RemoteConnection::remove_s3_object(const std::string &file_path) {
  try {
    Aws::S3::Model::DeleteObjectRequest delete_request;

    delete_request.SetBucket(_bucket_name);
    delete_request.SetKey(file_path);

    auto delete_object_outcome = _aws_client->DeleteObject(delete_request);

    if (!delete_object_outcome.IsSuccess()) {
      const Aws::S3::S3Error &err = delete_object_outcome.GetError();
      std::cerr << "Error: DeleteObject: " << err.GetExceptionName() << ": "
                << err.GetMessage() << std::endl;
      return false;
    }

    return true;
  } catch (VCL::Exception &ex) {
    print_exception(ex);
  } catch (std::exception &ex) {
    printErrorMessage("RemoteConnection::remove_s3_object", ex.what());
  }
  return false;
}

void RemoteConnection::printErrorMessage(const std::string &functionName,
                                         const std::string &errorMessage) {
  try {
    std::string message = "Exception ocurred in " + functionName + "().";
    if (errorMessage != "") {
      message += " Error: " + errorMessage;
    }
    std::cout << message << std::endl;
  } catch (std::exception &ex) {
    std::cout << "Exception ocurred in RemoteConnection::printErrorMessage()."
              << " Error: " << ex.what() << std::endl;
  }
}
