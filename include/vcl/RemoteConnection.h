/**
 * @file   RemoteConnection.h
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

#pragma once

#include "Exception.h"

#ifdef S3_SUPPORT
    #include <aws/core/Aws.h>
    #include <aws/s3/S3Client.h>
    #include <aws/core/client/ClientConfiguration.h>
    #include <aws/core/auth/AWSCredentialsProvider.h>
#endif

namespace VCL {

    class RemoteConnection{
    public:
        /**
         *  Default constructor. Sets up a remote connection
         */
        RemoteConnection();

        #ifdef S3_SUPPORT
            /**
             *  S3 Constructor, sets S3 region to specified region
             *
             *  @param region  S3 region 
             */
            RemoteConnection(const std::string &region);

            /**
             *  S3 Constructor, sets S3 details as given
             *
             *  @param region  S3 region 
             *  @param id   S3 Access ID
             *  @param key  S3 Secret Key
             */
            RemoteConnection(const std::string &region, const std::string &id, 
                const std::string &key);
        #endif

       /**
         *  Copy Constructor, creates a new RemoteConnection based on 
         *   an existing one
         */         
        RemoteConnection(const RemoteConnection &connection);

        /**
         *  Sets one RemoteConnection equal to another
         *
         *  @param connection  An existing RemoteConnecton
         */
        void operator=(const RemoteConnection &connection);

        ~RemoteConnection();

        /**
         * Starts the remote connection
         */
        void start();

        /**
         * Ends the remote connection
         */
        void end();

        /**
         * Determines whether the RemoteConnection has been started
         *
         * @return True if the RemoteConnection has been started, false 
         *    otherwise
         */
        bool connected() { return _remote; };

        /**
         *  Sets https proxy information 
         *
         *  @param host  The proxy host (without https://)
         *  @param port  The proxy port
         */
        void set_https_proxy(const std::string &host, const int port);

        /**
         *  Write an object to a remote location
         *
         *  @param path  A string containing the path of where to write the object
         *  @param data  A vector containing the data to be written
         */
        void write(const std::string &path, std::vector<unsigned char> data);
        
        /**
         *  Read an object from a remote location
         *
         *  @param path  A string containing the path to the object to read
         *  @return  A vector containing the object that was read
         */
        std::vector<char> read(const std::string &path);

        /**
         *  Remove an object from a remote location
         *
         *  @param path  A string containing the path to the object to delete
         */
        void remove_object(const std::string &path);

        /**
         *  Get object metadata from a remote location
         *
         *  @param path  A string containing the path to the object
         */
        long long get_object_size(const std::string &path);        

        /**
         *  Split a path into parts
         *
         *  @param path  A string containing the path to the remote location
         *  @return  A vector containing the parts of the path
         *    [0] -- the full path
         *    [2] -- the scheme (s3, http, https)
         *    [5] -- the first part of the path (s3 bucket)
         *    [7] -- the remainder of the path (s3 key)
         */
        std::vector<std::string> split_path(const std::string &path);

        #ifdef S3_SUPPORT
            /**
             *  Sets S3 Client Configuration parameters
             *
             *  @param region  The S3 region to use
             *  @param result_timeout  The result timeout in ms
             *  @param connect_timeout  THe connection timeout in ms
             */
            void set_s3_configuration(const std::string &region, 
                const long result_timeout=3000, const long connect_timeout=3000);

            /**
             *  Sets S3 credentials if not set when creating the RemoteConnection
             *
             *  @param access  S3 Access ID
             *  @param key  S3 Secret Key
             */
            void set_s3_credentials(const std::string &access, const std::string &key);

            /**
             *  Gets the S3 region being used
             *
             *  @return A string contained the region
             */
            std::string get_s3_region();

            /**
             *  Gets the S3 result timeout
             *
             *  @return The result timeout in ms
             */
            long get_s3_result_timeout();

            /**
             *  Gets the S3 connect timeout
             *
             *  @return The connect timeout in ms
             */
            long get_s3_connect_timeout();

            /**
             *  Gets the S3 access ID
             *
             *  @return A string with the S3 access ID 
             */
            std::string get_s3_access_id();

            /**
             *  Gets the S3 secret key
             *
             *  @return A string with the S3 secret key 
             */
            std::string get_s3_secret_key();

            /**
             *  Write to S3
             *
             *  @param path  A string containing the path to write to, starting
             *      with s3://
             *  @param data  A vector containing the data to be written
             */
            void write_s3(const std::string &path, std::vector<unsigned char> data);
            
            /**
             *  Read from S3
             *
             *  @param path  A string containing the path to read from, starting
             *      with s3://
             *  @return  A vector containing the object that was read
             */
            std::vector<char> read_s3(const std::string &path);

            void read_to_file(const std::string &path, const std::string &local);

            /**
             *  Remove an object from S3
             *
             *  @param path  A string containing the path to the object to delete, 
             *     starting with s3://
             */
            void remove_s3_object(const std::string &path);

            /**
             *  Get object metadata from S3
             *
             *  @param path  A string containing the path to the object, 
             *     starting with s3://
             */
            long long get_s3_object_size(const std::string &path);

        #endif



    private:
        bool _remote;

        #ifdef S3_SUPPORT
            Aws::Auth::AWSCredentials _credentials;
            Aws::SDKOptions _options;
            Aws::Client::ClientConfiguration _config;
            Aws::S3::S3Client *_client;
        #endif
    };
}
