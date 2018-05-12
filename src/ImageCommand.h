/**
 * @file   ImageCommand.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once
#include <string>
#include <mutex>
#include <vector>
#include "VCL.h"

#include "RSCommand.h"
#include "ExceptionsCommand.h"

namespace VDMS {

// Helper classes for handling various JSON commands.

    class ImageCommand: public RSCommand
    {

    protected:
        void enqueue_operations(VCL::Image& img, const Json::Value& op);

    public:

        ImageCommand(const std::string &cmd_name);

        virtual int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error) = 0;

        virtual bool need_blob(const Json::Value& cmd) { return false; }
    };

    class AddImage: public ImageCommand
    {
        const std::string DEFAULT_TDB_PATH = "images/tdb/database";
        const std::string DEFAULT_PNG_PATH = "images/png_database";
        const std::string DEFAULT_JPG_PATH = "images/jpg_database";

        std::string _storage_tdb;
        std::string _storage_png;
        std::string _storage_jpg;

    public:
        AddImage();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& cmd) { return true; }
    };

    class UpdateImage: public ImageCommand
    {
    public:
        UpdateImage();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    class FindImage: public ImageCommand
    {
    public:
        FindImage();
        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

}; // namespace VDMS
