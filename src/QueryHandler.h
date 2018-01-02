#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "VCL.h"

#include "CommandHandler.h"
#include "PMGDQueryHandler.h" // to provide the database connection
#include "PMGDTransaction.h"

// Json parsing files
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/json.h>

namespace athena {

typedef ::google::protobuf::RepeatedPtrField<std::string> BlobArray;

// Helper classes for handling various JSON commands.
    class RSCommand
    {
    protected:

        const std::string _cmd_name;
        std::map<std::string, int> _valid_params_map;

        virtual Json::Value check_responses(Json::Value &responses);

        void run_operations(VCL::Image &vclimg, const Json::Value &op);

    public:

        RSCommand(const std::string& cmd_name);

        bool check_params(const Json::Value& cmd);

        virtual bool need_blob() { return false; }

        virtual int construct_protobuf(
                                PMGDTransaction& tx,
                                const Json::Value& root,
                                const std::string& blob,
                                int grp_id) = 0;

        virtual Json::Value construct_responses(
            Json::Value &json_responses,
            const Json::Value &json,
            protobufs::queryMessage &response) = 0;
    };

    class AddEntity : public RSCommand
    {
    public:
        AddEntity();
        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id);

        Json::Value construct_responses(
            Json::Value &json_responses,
            const Json::Value &json,
            protobufs::queryMessage &response);
    };

    class Connect : public RSCommand
    {
    public:
        Connect();
        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id);

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    class FindEntity : public RSCommand
    {
    public:
        FindEntity();
        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id);

        Json::Value construct_responses(
            Json::Value &json_responses,
            const Json::Value &json,
            protobufs::queryMessage &response);
    };

    class AddImage: public RSCommand
    {
        const std::string DEFAULT_TDB_PATH = "./tdb_database";
        const std::string DEFAULT_PNG_PATH = "./png_database";
        const std::string DEFAULT_JPG_PATH = "./jpg_database";

        std::string _storage_tdb;
        std::string _storage_png;
        std::string _storage_jpg;

    public:
        AddImage();

        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id);

        bool need_blob() { return true; }

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    class FindImage: public RSCommand
    {
    public:
        FindImage();
        int construct_protobuf(PMGDTransaction& tx,
                               const Json::Value& root,
                               const std::string& blob,
                               int grp_id);

        Json::Value construct_responses(
                Json::Value &json_responses,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    // Instance created per worker thread to handle all transactions on a given
    // connection.
    class QueryHandler
    {
        PMGDQueryHandler _pmgd_qh;
        std::unordered_map<std::string, RSCommand *> _rs_cmds;

        bool syntax_checker(const Json::Value &root);

    public:
        QueryHandler(Jarvis::Graph *db, std::mutex *mtx);
        ~QueryHandler();
        void process_connection(comm::Connection *c);
        void process_query(protobufs::queryMessage& proto_query,
                           protobufs::queryMessage& response);
    };
};
