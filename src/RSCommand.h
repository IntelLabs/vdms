#pragma once
#include <string>
#include <vector>
#include <unordered_map>

#include "PMGDQuery.h"
#include "protobuf/queryMessage.pb.h"

// Json parsing files
#include <jsoncpp/json/value.h>

#define PARAM_MANDATORY 2
#define PARAM_OPTIONAL  1

#define ATHENA_COL_TAG          "AT:COLLECTION"
#define ATHENA_COL_NAME_PROP    "name"
#define ATHENA_COL_EDGE_TAG     "collection_tag"

namespace athena {

    static uint32_t STATIC_IDENTIFIER = 0;

// Helper classes for handling various JSON commands.
    class RSCommand
    {
    protected:

        const std::string _cmd_name;
        std::map<std::string, int> _valid_params_map;

        template <typename T>
        T get_value(const Json::Value& json, const std::string& key, const T& def);

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

        bool check_params(const Json::Value& cmd, Json::Value& error);

        virtual bool need_blob() { return false; }

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
    public:
        AddEntity();
        int construct_protobuf(PMGDQuery& query,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id,
                               Json::Value& error);
    };

    class Connect : public RSCommand
    {
    public:
        Connect();
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
    };

}; // namespace athena
