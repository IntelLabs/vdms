/**
 * @file   RemoteConnection.cc
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
 * This file declares the C++ API for RemoteConnection, which allows users to 
 *  connect to different file systems. At the moment, S3 is enabled. 
 */

#include <fstream>
#include <vector>
#include <regex>

#ifdef S3_SUPPORT
    #include <aws/core/utils/StringUtils.h>
    #include <aws/s3/model/PutObjectRequest.h>
    #include <aws/s3/model/GetObjectRequest.h>
    #include <aws/s3/model/DeleteObjectRequest.h>
    #include <aws/s3/model/ListObjectsRequest.h>
    #include <aws/s3/model/Object.h>
#endif 

#include "vcl/RemoteConnection.h"

using namespace VCL;

RemoteConnection::RemoteConnection()
{    
    _remote = false;

    #ifdef S3_SUPPORT 
        _config.region = Aws::Utils::StringUtils::to_string("us-west-2");
        _config.requestTimeoutMs = 3000;
        _config.connectTimeoutMs = 3000;
    #endif
}

#ifdef S3_SUPPORT
RemoteConnection::RemoteConnection(const std::string &region)
{
    _remote = false;
    
    _config.region = Aws::Utils::StringUtils::to_string(region);
    _config.requestTimeoutMs = 400;
    _config.connectTimeoutMs = 3000;
}

RemoteConnection::RemoteConnection(const std::string &region, const std::string &id, 
    const std::string &key)
{
    _remote = false;
    
    _config.region = Aws::Utils::StringUtils::to_string(region);
    _config.requestTimeoutMs = 400;
    _config.connectTimeoutMs = 3000;

    set_s3_credentials(id, key);
}
#endif

RemoteConnection::RemoteConnection(const RemoteConnection &connection)
{    
    _remote = connection._remote;

    #ifdef S3_SUPPORT
        _options = connection._options;
        _config = connection._config;
        _credentials = connection._credentials;
    #endif
}

void RemoteConnection::operator=(const RemoteConnection &connection)
{    
    _remote = connection._remote;

    #ifdef S3_SUPPORT
        _options = connection._options;
        _config = connection._config;
        _credentials = connection._credentials;
    #endif
}

RemoteConnection::~RemoteConnection()
{
    if (!_remote)
        end();
}

void RemoteConnection::start()
{
    #ifdef S3_SUPPORT
        Aws::InitAPI(_options);
        _client = new Aws::S3::S3Client(_credentials, _config);
    #endif
    _remote = true;
}

void RemoteConnection::end()
{
    #ifdef S3_SUPPORT
        Aws::ShutdownAPI(_options);
        delete _client;
    #endif
    _remote = false;
}

void RemoteConnection::set_https_proxy(const std::string &host, const int port)
{
    #ifdef S3_SUPPORT
        _config.proxyScheme = Aws::Http::Scheme::HTTPS;
        _config.proxyHost = Aws::Utils::StringUtils::to_string(host);
        _config.proxyPort = port;
    #endif
}

void RemoteConnection::write(const std::string &path, std::vector<unsigned char> data)
{
    if (_remote) {
        #ifdef S3_SUPPORT
            write_s3(path, data);
        #else
            throw VCLException(UnsupportedSystem, 
                "The system specified by the path is not supported currently");
        #endif
    }
    else
        throw VCLException(SystemNotFound, "The RemoteConnection has not been started");
}

std::vector<char> RemoteConnection::read(const std::string &path)
{
    if ( _remote ) {
        #ifdef S3_SUPPORT
            return read_s3(path);
        #else
            throw VCLException(UnsupportedSystem, 
                "The system specified by the path is not supported currently");
        #endif
    }
    else
        throw VCLException(SystemNotFound, "The RemoteConnection has not been started");       
}

void RemoteConnection::remove_object(const std::string &path)
{
    if ( _remote ) {
        #ifdef S3_SUPPORT
            remove_s3_object(path);
        #else
            throw VCLException(UnsupportedSystem, 
                "The system specified by the path is not supported currently");
        #endif
    }
    else
        throw VCLException(SystemNotFound, "The RemoteConnection has not been started");       
}

long long RemoteConnection::get_object_size(const std::string &path)
{
    if ( _remote ) {
        #ifdef S3_SUPPORT
            return get_s3_object_size(path);
        #else
            throw VCLException(UnsupportedSystem, 
                "The system specified by the path is not supported currently");
        #endif
    }
    else
        throw VCLException(SystemNotFound, "The RemoteConnection has not been started");   
}


std::vector<std::string> RemoteConnection::split_path(const std::string &path)
{
    std::vector<std::string> divided_path;

    std::regex split(
        "^(([a-z0-9]+)(://))(([a-zA-Z0-9_-]+)(/))((([a-zA-Z0-9_-]+)(/))*([a-zA-Z0-9_-]+)(\\.[a-zA-Z0-9_-]+)*)");
    std::smatch match;
    if ( std::regex_search(path, match, split) ) {
        for ( auto element : match ) {
            divided_path.push_back(element);
        }
    }
    else
        throw VCLException(ObjectNotFound, path + " is formatted incorrectly");

    return divided_path;
}

#ifdef S3_SUPPORT
void RemoteConnection::set_s3_configuration(const std::string &region, 
    const long request_timeout, const long connect_timeout)
{
    _config.region = Aws::Utils::StringUtils::to_string(region);
    _config.requestTimeoutMs = request_timeout;
    _config.connectTimeoutMs = connect_timeout;
}

void RemoteConnection::set_s3_credentials(const std::string &access, const std::string &key)
{
    _credentials.SetAWSAccessKeyId(Aws::Utils::StringUtils::to_string(access));
    _credentials.SetAWSSecretKey(Aws::Utils::StringUtils::to_string(key));
}

std::string RemoteConnection::get_s3_region()
{
    return (_config.region).c_str();
}

long RemoteConnection::get_s3_result_timeout()
{
    return _config.requestTimeoutMs;
}

long RemoteConnection::get_s3_connect_timeout()
{
    return _config.connectTimeoutMs;
}

std::string RemoteConnection::get_s3_access_id()
{
    return (_credentials.GetAWSAccessKeyId()).c_str();
}

std::string RemoteConnection::get_s3_secret_key()
{
    return (_credentials.GetAWSSecretKey()).c_str();
}

void RemoteConnection::write_s3(const std::string &path, std::vector<unsigned char> data)
{
    std::vector<std::string> divided_path = split_path(path);
    Aws::String bucket_name = Aws::Utils::StringUtils::to_string(divided_path[5]);
    Aws::String key_name = Aws::Utils::StringUtils::to_string(divided_path[7]);

    Aws::S3::Model::PutObjectRequest object_request;
    object_request.WithBucket(bucket_name).WithKey(key_name);

    auto input_data = Aws::MakeShared<Aws::StringStream>("PutObjectInputStream");
    input_data->write(reinterpret_cast<char*>(data.data()), data.size());

    object_request.SetBody(input_data);

    auto put_object_outcome = _client->PutObject(object_request);

    if ( !put_object_outcome.IsSuccess() ) {
        std::string err = (put_object_outcome.GetError().GetMessage()).c_str();
        throw VCLException(OperationFailed, err);
    }
}

std::vector<char> RemoteConnection::read_s3(const std::string &path)
{
    std::vector<std::string> divided_path = split_path(path);
    Aws::String bucket_name = Aws::Utils::StringUtils::to_string(divided_path[5]);
    Aws::String key_name = Aws::Utils::StringUtils::to_string(divided_path[7]);

    Aws::S3::Model::GetObjectRequest object_request;
    object_request.WithBucket(bucket_name).WithKey(key_name);

    auto get_object_outcome = _client->GetObject(object_request);

    if(get_object_outcome.IsSuccess()) {
        std::stringstream stream;
        stream << get_object_outcome.GetResult().GetBody().rdbuf();
        std::string str_stream = stream.str();
        std::vector<char> data(str_stream.begin(), str_stream.end());
        return data;
    }
    else {
        std::string err = (get_object_outcome.GetError().GetMessage()).c_str();
        throw VCLException(OperationFailed, err);
    }
}

void RemoteConnection::read_to_file(const std::string &path, const std::string &local)
{
    std::vector<std::string> divided_path = split_path(path);
    Aws::String bucket_name = Aws::Utils::StringUtils::to_string(divided_path[5]);
    Aws::String key_name = Aws::Utils::StringUtils::to_string(divided_path[7]);
    std::cout << bucket_name << "  " << key_name << std::endl;

    Aws::S3::Model::GetObjectRequest object_request;
    object_request.WithBucket(bucket_name).WithKey(key_name);

    auto get_object_outcome = _client->GetObject(object_request);

    if (get_object_outcome.IsSuccess()) {
        Aws::OFStream local_file;
        local_file.open(local.c_str(), std::ios::out | std::ios::binary);
        local_file << get_object_outcome.GetResult().GetBody().rdbuf();
    }
    else {
        std::string err = (get_object_outcome.GetError().GetMessage()).c_str();
        throw VCLException(OperationFailed, err);
    }
}

void RemoteConnection::remove_s3_object(const std::string &path)
{
    std::vector<std::string> divided_path = split_path(path);
    Aws::String bucket_name = Aws::Utils::StringUtils::to_string(divided_path[5]);
    Aws::String key_name = Aws::Utils::StringUtils::to_string(divided_path[7]);

    Aws::S3::Model::DeleteObjectRequest object_request;
    object_request.WithBucket(bucket_name).WithKey(key_name);

    auto delete_object_outcome = _client->DeleteObject(object_request);

    if ( !delete_object_outcome.IsSuccess() ) {
        std::string err = (delete_object_outcome.GetError().GetMessage()).c_str();
        throw VCLException(OperationFailed, err);
    }
}

long long RemoteConnection::get_s3_object_size(const std::string &path)
{
    std::vector<std::string> divided_path = split_path(path);
    Aws::String bucket_name = Aws::Utils::StringUtils::to_string(divided_path[5]);
    Aws::String key_name = Aws::Utils::StringUtils::to_string(divided_path[7]);

    long long size = 0;

    Aws::S3::Model::ListObjectsRequest objects_request;
    objects_request.WithBucket(bucket_name).WithPrefix(key_name);

    auto list_objects_outcome = _client->ListObjects(objects_request);

    if (list_objects_outcome.IsSuccess())
    {
        Aws::Vector<Aws::S3::Model::Object> object_list =
            list_objects_outcome.GetResult().GetContents();

        for (auto const &s3_object : object_list)
            size += s3_object.GetSize();
        return size;
    }
    else
    {
        std::string err = list_objects_outcome.GetError().GetMessage().c_str();
        throw VCLException(OperationFailed, err);  
    }
}
#endif
