#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "VCL.h"

#include "CommandHandler.h"
#include "PMGDQueryHandler.h" // to provide the database connection

// Json parsing files
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/json.h>

namespace athena {

// Helper classes for handling various JSON commands.
    class RSCommand
    {
    protected:
        void set_property(pmgd::protobufs::Property *p,
                                        const char * prop_name,
                                        Json::Value);

        virtual Json::Value parse_response(pmgd::protobufs::CommandResponse *);

        Json::Value print_properties(const std::string &key,
                             const pmgd::protobufs::Property &p);

        typedef ::google::protobuf::RepeatedPtrField<std::string> BlobArray;

    public:
        virtual int construct_protobuf(
                                std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid) = 0;

        void run_operations(VCL::Image& vclimg, const Json::Value& op);

        virtual bool need_blob() { return false; }

        virtual Json::Value construct_responses(
            std::vector<pmgd::protobufs::CommandResponse *> &cmds,
            Json::Value *json,
            protobufs::queryMessage &response) = 0;
     };

    class AddEntity : public RSCommand
    {
    public:
        int add_entity_body(pmgd::protobufs::AddNode*,
                            const Json::Value& root);

        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        Json::Value construct_responses(
            std::vector<pmgd::protobufs::CommandResponse *>&,
            Json::Value*,
            protobufs::queryMessage &response);
    };

    class AddConnection : public RSCommand
    {
    public:
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        Json::Value construct_responses(
                std::vector<pmgd::protobufs::CommandResponse *>&,
                Json::Value*,
                protobufs::queryMessage &response);
    };

    class FindEntity : public RSCommand
    {
    protected:
        void set_operand(pmgd::protobufs::Property* p1, Json::Value);

        int build_query_protobuf(pmgd::protobufs::Command*,
                                 const Json::Value& root,
                                 pmgd::protobufs::QueryNode *queryType);

        int get_response_type(const Json::Value& result_type_array,
                              std::string response,
                              pmgd::protobufs::QueryNode *queryType);

        int parse_query_results(const Json::Value& result_type,
                                pmgd::protobufs::QueryNode* queryType);

    public:
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        int parse_query_constraints(const Json::Value& root, pmgd::protobufs::QueryNode* queryType);

        Json::Value construct_responses(
            std::vector<pmgd::protobufs::CommandResponse *>&,
            Json::Value*,
            protobufs::queryMessage &response);
    };

    class AddImage: public RSCommand
    {
        const std::string DEFAULT_TDB_PATH = "./tdb_database";
        const std::string DEFAULT_PNG_PATH = "./png_database";

        std::string _storage_tdb;
        std::string _storage_png;

    public:
        AddImage();

        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        bool need_blob() { return true; }

        Json::Value construct_responses(
                std::vector<pmgd::protobufs::CommandResponse*> &cmds,
                Json::Value *json,
                protobufs::queryMessage &response);
    };

    class FindImage: public RSCommand
    {
    public:
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        bool need_blob() { return false; }

        Json::Value construct_responses(
                std::vector<pmgd::protobufs::CommandResponse *> &cmds,
                Json::Value *json,
                protobufs::queryMessage &response);
    };

    // Instance created per worker thread to handle all transactions on a given
    // connection.
    class QueryHandler
    {
        PMGDQueryHandler _pmgd_qh;
        std::unordered_map<std::string, RSCommand *> _rs_cmds;

    public:
        QueryHandler(Jarvis::Graph *db, std::mutex *mtx);
        void process_connection(comm::Connection *c);
        void process_query(protobufs::queryMessage proto_query,
                           protobufs::queryMessage& response);
    };
};
