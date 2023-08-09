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

#include "../../include/vcl/RemoteConnection.h"

using namespace VCL;

// CONSTRUCTOR
RemoteConnection::RemoteConnection() {
  // LogEntry(__FUNCTION__);
  _remote_connected = false;
  _aws_client = nullptr;
  _aws_sdk_options = nullptr;
}

// DESTRUCTOR
RemoteConnection::~RemoteConnection() {}

void RemoteConnection::start() {
  // LogEntry(__FUNCTION__);
  ConfigureAws();
}

void RemoteConnection::end() {
  // LogEntry(__FUNCTION__);
  ShutdownAws();
}

void RemoteConnection::ConfigureAws() {
  // LogEntry(__FUNCTION__);

  _aws_sdk_options = new Aws::SDKOptions();
  Aws::InitAPI(*_aws_sdk_options);

  Aws::Client::ClientConfiguration clientConfig;

  // TODO: proxy / override settings should be user configurable
  // use this block for AWS
  // clientConfig.proxyHost = "proxy-dmz.intel.com";
  // clientConfig.proxyPort = 912;
  // clientConfig.proxyScheme = Aws::Http::Scheme::HTTP;

  // use this override for MinIO
  clientConfig.endpointOverride = "http://127.0.0.1:9000";

  _aws_client = new Aws::S3::S3Client(clientConfig);
  _remote_connected = true;
}

// TODO make the log level configurable
// void RemoteConnection::SetLogLevelDebug() {
//   //_aws_sdk_options.loggingOptions.logLevel =
//   // Aws::Utils::Logging::LogLevel::Debug;
// }

void RemoteConnection::ShutdownAws() {
  // LogEntry(__FUNCTION__);
  Aws::ShutdownAPI(*_aws_sdk_options);
  _remote_connected = false;
}

// image file, takes path to store and vector<uchar> of data
// TODO: make the raw data a more efficient format?
void RemoteConnection::Write(const std::string &path,
                             std::vector<unsigned char> data) {
  if (_remote_connected) {
    write_s3(path, data);
  } else {
    std::cerr << "WRITE: The RemoteConnection has not been started"
              << std::endl;
  }
}

// video file (or any file on disk specified by full path)
// opens file, reads into memory, uploads to AWS
void RemoteConnection::Write(const std::string &filename) {
  if (_remote_connected) {
    write_s3(filename);
  } else {
    std::cerr << "WRITE: The RemoteConnection has not been started"
              << std::endl;
  }
}

void RemoteConnection::RetrieveFile(const std::string &filename) {
  if (_remote_connected) {
    retrieve_file(filename);
  } else {
    std::cerr << "WRITE: The RemoteConnection has not been started"
              << std::endl;
  }
}

std::vector<std::string>
RemoteConnection::ListFilesInFolder(const std::string &folder_name) {
  if (_remote_connected) {
    return get_file_list(folder_name);
  } else {
    std::cerr << "WRITE: The RemoteConnection has not been started"
              << std::endl;
    return std::vector<std::string>();
  }
}

std::vector<unsigned char> RemoteConnection::Read(const std::string &path) {
  if (_remote_connected) {
    return read_s3(path);
  } else {
    std::cerr << "READ: The RemoteConnection has not been started" << std::endl;
  }
  return std::vector<unsigned char>();
}

void RemoteConnection::Read_Video(const std::string &path) {
  if (_remote_connected) {
    read_s3_video(path);
  } else {
    std::cerr << "READ_Video: The RemoteConnection has not been started"
              << std::endl;
  }
}

void RemoteConnection::Remove_Object(const std::string &path) {
  if (_remote_connected) {
    return remove_s3_object(path);
  } else {
    std::cerr << "REMOVE: The RemoteConnection has not been started"
              << std::endl;
  }
}

//########Private S3 Functions########

void RemoteConnection::write_s3(const std::string &filename) {
  Aws::S3::Model::PutObjectRequest put_request;
  put_request.SetBucket(_bucket_name);
  put_request.SetKey(filename);

  std::shared_ptr<Aws::IOStream> inputData =
      Aws::MakeShared<Aws::FStream>("SampleAllocationTag", filename.c_str(),
                                    std::ios_base::in | std::ios_base::binary);

  if (!*inputData) {
    std::cerr << "Error unable to read file " << filename << std::endl;
    return;
  }

  put_request.SetBody(inputData);

  Aws::S3::Model::PutObjectOutcome outcome =
      _aws_client->PutObject(put_request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = outcome.GetError();
    std::cerr << "Error: PutObject: " << err.GetExceptionName() << ": "
              << err.GetMessage() << std::endl;
  } else {
    std::cout << "Added object '" << filename << "' to bucket: " << _bucket_name
              << std::endl;
  }
}

void RemoteConnection::write_s3(const std::string &path,
                                std::vector<unsigned char> data) {
  Aws::S3::Model::PutObjectRequest put_request;
  put_request.SetBucket(_bucket_name);
  put_request.SetKey(path);

  auto input_data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
  input_data->write(reinterpret_cast<char *>(data.data()), data.size());

  put_request.SetBody(input_data);
  Aws::S3::Model::PutObjectOutcome outcome =
      _aws_client->PutObject(put_request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = outcome.GetError();
    std::cerr << "Error: PutObject: " << err.GetExceptionName() << ": "
              << err.GetMessage() << std::endl;
  } else {
    std::cout << "Added object '" << path << "' to bucket: " << _bucket_name
              << std::endl;
  }
}

void RemoteConnection::read_s3_video(const std::string &file_path) {
  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(_bucket_name);
  request.SetKey(file_path);

  Aws::S3::Model::GetObjectOutcome outcome = _aws_client->GetObject(request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = outcome.GetError();
    std::cerr << "Error: GetObject: " << err.GetExceptionName() << ": "
              << err.GetMessage() << std::endl;
  } else {
    std::cout << "Successfully retrieved '" << file_path << "' from '"
              << _bucket_name << "'." << std::endl;

    auto &retrieved_file = outcome.GetResult().GetBody();
    std::ofstream output_file(file_path.c_str(),
                              std::ios::out | std::ios::binary);
    output_file << retrieved_file.rdbuf();
  }
}

std::vector<unsigned char>
RemoteConnection::read_s3(const std::string &file_path) {
  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(_bucket_name);
  request.SetKey(file_path);

  Aws::S3::Model::GetObjectOutcome outcome = _aws_client->GetObject(request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = outcome.GetError();
    std::cerr << "Error: GetObject: " << err.GetExceptionName() << ": "
              << err.GetMessage() << std::endl;
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
}

void RemoteConnection::retrieve_file(const std::string &file_path) {
  Aws::S3::Model::GetObjectRequest request;
  request.SetBucket(_bucket_name);
  request.SetKey(file_path);

  Aws::S3::Model::GetObjectOutcome outcome = _aws_client->GetObject(request);

  if (!outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = outcome.GetError();
    std::cerr << "Error: GetObject: " << err.GetExceptionName() << ": "
              << err.GetMessage() << std::endl;
  } else {
    std::cout << "Successfully retrieved '" << file_path << "' from '"
              << _bucket_name << "'." << std::endl;

    auto &retrieved_file = outcome.GetResult().GetBody();
    std::ofstream output_file(file_path.c_str(),
                              std::ios::out | std::ios::binary);
    output_file << retrieved_file.rdbuf();
  }
}

std::vector<std::string>
RemoteConnection::get_file_list(const std::string &path) {
  std::vector<std::string> results;

  Aws::S3::Model::ListObjectsRequest request;
  request.SetBucket(_bucket_name);
  request.SetPrefix(path);

  Aws::S3::Model::ListObjectsOutcome outcome =
      _aws_client->ListObjects(request);

  if (!outcome.IsSuccess()) {
    std::cerr << "Error: ListObjects: " << outcome.GetError().GetMessage()
              << std::endl;
  } else {
    Aws::Vector<Aws::S3::Model::Object> objects =
        outcome.GetResult().GetContents();

    for (Aws::S3::Model::Object &object : objects) {
      results.push_back(object.GetKey());
    }
  }

  return results;
}

void RemoteConnection::remove_s3_object(const std::string &file_path) {
  Aws::S3::Model::DeleteObjectRequest delete_request;

  delete_request.SetBucket(_bucket_name);
  delete_request.SetKey(file_path);

  auto delete_object_outcome = _aws_client->DeleteObject(delete_request);

  if (!delete_object_outcome.IsSuccess()) {
    const Aws::S3::S3Error &err = delete_object_outcome.GetError();
    std::cerr << "Error: DeleteObject: " << err.GetExceptionName() << ": "
              << err.GetMessage() << std::endl;
  }
}

// void RemoteConnection::LogEntry(std::string functionName) {
//   // std::cout << "Entering " << functionName << "()" << std::endl;
// }
