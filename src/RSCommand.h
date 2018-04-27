/**
 * @file   RSCommand.h
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
#include <vector>
#include <atomic>
#include <unordered_map>

#include "PMGDQuery.h"
#include "protobuf/queryMessage.pb.h"

// Json parsing files
#include <jsoncpp/json/value.h>

namespace VDMS {

// Helper classes for handling various JSON commands.
    class RSCommand
    {
    protected:

        const std::string _cmd_name;
        std::map<std::string, int> _valid_params_map;

        template <typename T>
        T get_value(const Json::Value& json, const std::string& key,
                    T def = T());

        void add_link(PMGDQuery& query, const Json::Value& link,
                      int node_ref, const std::string tag);

        virtual Json::Value check_responses(Json::Value& responses);

    public:

        enum ErrorCode {
            Success = 0,
            Error   = -1,
            Empty   = 1,
            Exists  = 2,
            NotUnique  = 3
        };

        RSCommand(const std::string& cmd_name);

        virtual bool need_blob(const Json::Value& cmd) { return false; }

        virtual int construct_protobuf(
                                PMGDQuery& query,
                                const Json::Value& root,
                                const std::string& blob,
                                int grp_id,
                                Json::Value& error) = 0;

        virtual Json::Value construct_responses(
            Json::Value& json_responses,
            const Json::Value& json,
            protobufs::queryMessage &response);
    };

    class AddEntity : public RSCommand
    {
    private:
        const std::string DEFAULT_BLOB_PATH = "blobs/";

        std::string _storage_blob;

    public:
        AddEntity();
        int construct_protobuf(PMGDQuery& query,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& jsoncmd);
    };

    class UpdateEntity : public RSCommand
    {
    public:
        UpdateEntity();
        int construct_protobuf(PMGDQuery& query,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        Json::Value construct_responses(
            Json::Value& json_responses,
            const Json::Value& json,
            protobufs::queryMessage &response);
    };

    class AddConnection : public RSCommand
    {
    public:
        AddConnection();
        int construct_protobuf(PMGDQuery& query,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);
    };

    class FindEntity : public RSCommand
    {
    public:
        FindEntity();
        int construct_protobuf(PMGDQuery& query,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        Json::Value construct_responses(
            Json::Value& json_responses,
            const Json::Value& json,
            protobufs::queryMessage &response);
    };

}; // namespace VDMS
