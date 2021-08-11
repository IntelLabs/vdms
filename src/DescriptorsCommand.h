/**
 * @file   DescriptorsCommand.h
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
#include <unordered_map>

#include <jsoncpp/json/value.h>
#include <jsoncpp/json/json.h>

#include "QueryHandler.h" // to provide the database connection
#include "DescriptorsManager.h"
#include "tbb/concurrent_unordered_map.h"

namespace VDMS{

    typedef std::pair<std::vector<long>, std::vector<float>> IDDistancePair;

    // This class encapsulates common behavior of Descriptors-related cmds.
    class DescriptorsCommand : public RSCommand
    {
    protected:
        DescriptorsManager* _dm;

        // IDDistancePair is a pointer so that we can free its content
        // without having to use erase methods, which are not lock free
        // for this data structure in tbb
        tbb::concurrent_unordered_map<long, IDDistancePair*> _cache_map;

        // Will return the path to the set and the dimensions
        std::string get_set_path(PMGDQuery& query_tx,
                                 const std::string& set, int& dim);

        bool check_blob_size(const std::string& blob, const int dimensions,
                             const long n_desc);

    public:
        DescriptorsCommand(const std::string& cmd_name);

        virtual bool need_blob(const Json::Value& cmd) { return false; }

        virtual int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error) = 0;

        virtual Json::Value construct_responses(
                Json::Value& json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response,
                const std::string &blob) = 0;
    };

    class AddDescriptorSet: public DescriptorsCommand
    {
        std::string _storage_sets;

    public:
        AddDescriptorSet();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        Json::Value construct_responses(
                Json::Value& json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response,
                const std::string &blob);
    };

    class AddDescriptor: public DescriptorsCommand
    {
        long insert_descriptor(const std::string& blob,
                               const std::string& path,
                               int dim,
                               const std::string& label,
                               Json::Value& error);

    public:
        AddDescriptor();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& cmd) { return true; }

        Json::Value construct_responses(
                Json::Value& json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response,
                const std::string &blob);
    };

    class ClassifyDescriptor: public DescriptorsCommand
    {

    public:
        ClassifyDescriptor();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& cmd) { return true; }

        Json::Value construct_responses(
                Json::Value& json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response,
                const std::string &blob);

    };

    class FindDescriptor: public DescriptorsCommand
    {

    private:
      void convert_properties(Json::Value& entities, Json::Value& list);
      void populate_blobs(const std::string& set_path,
                          const Json::Value& results,
                          Json::Value& entities,
                          protobufs::queryMessage &query_res);

    public:
        FindDescriptor();

        int construct_protobuf(PMGDQuery& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);

        bool need_blob(const Json::Value& cmd);

        Json::Value construct_responses(
                Json::Value& json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response,
                const std::string &blob);

    };
  }
