/**
 * @file   VideoCommand.h
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

    class VideoCommand: public RSCommand
    {

    protected:
        void enqueue_operations(VCL::Video& video, const Json::Value& op);

    public:

        VideoCommand(const std::string &cmd_name);

        virtual int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error) = 0;

        virtual bool need_blob(const Json::Value& cmd) { return false; }
    };

    class AddVideo: public VideoCommand
    {
         const std::string DEFAULT_VIDEO_PATH = "videos/database";


         std::string _storage_video;


    public:
        AddVideo();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& cmd) { return true; }
    };

    class UpdateVideo: public VideoCommand
    {
    public:
        UpdateVideo();

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

    class FindVideo: public VideoCommand
    {
    public:
        FindVideo();
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

   class AddFrame: public VideoCommand
    {

    public:
        AddFrame();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& cmd) { return true; }
        std::string _storage_video;
    };
 class FindFrame: public VideoCommand
    {
    public:
        FindFrame();
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
