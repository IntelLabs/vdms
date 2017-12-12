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

typedef ::google::protobuf::RepeatedPtrField<std::string> BlobArray;
typedef pmgd::protobufs::CommandResponse pmgdCmdResponse;
typedef pmgd::protobufs::PropertyPredicate pmgdPropPred;
typedef pmgd::protobufs::PropertyList pmgdPropList;
typedef pmgd::protobufs::Property pmgdProp;

// Helper classes for handling various JSON commands.
    class RSCommand
    {
    protected:

        const std::string _cmd_name;
        std::map<std::string, int> _valid_params_map;

        virtual Json::Value parse_response(pmgd::protobufs::CommandResponse *);

        virtual Json::Value check_responses(
                              std::vector<pmgdCmdResponse *> &responses);

        void run_operations(VCL::Image &vclimg, const Json::Value &op);

        // An AddNode with either constraints or unique is a ConditionalAddNode
        pmgd::protobufs::Command* AddNode(int grp_id,
                                      int ref,
                                      const std::string &tag,
                                      const Json::Value& props,
                                      Json::Value constraints = Json::Value(),
                                      bool unique = false);

        pmgd::protobufs::Command* AddEdge(int grp_id, int ident,
                                      int src, int dst,
                                      const std::string &tag,
                                      Json::Value& props);

        pmgd::protobufs::Command* QueryNode(int grp_id, int ref,
                                      const std::string& tag,
                                      Json::Value& link,
                                      Json::Value& constraints,
                                      Json::Value& results,
                                      bool unique = false);

    private:

        void set_property(pmgd::protobufs::Property *p,
                          const char *prop_name,
                          Json::Value);

        Json::Value print_properties(const std::string &key,
                                     const pmgd::protobufs::Property &p);

        void add_link(const Json::Value& link, pmgd::protobufs::QueryNode *qn);

        void set_operand(pmgd::protobufs::Property* p1, const Json::Value&);

        void get_response_type(const Json::Value& result_type_array,
                              std::string response,
                              pmgd::protobufs::QueryNode *queryType);

        void parse_query_results(const Json::Value& result_type,
                                pmgd::protobufs::QueryNode* queryType);

        void parse_query_constraints(const Json::Value& root, pmgd::protobufs::QueryNode* queryType);

    public:

        RSCommand(const std::string& cmd_name);

        bool check_params(const Json::Value& cmd);

        static Json::Value construct_error_response(pmgdCmdResponse *response);

        virtual bool need_blob() { return false; }

        virtual int construct_protobuf(
                                std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid) = 0;

        virtual Json::Value construct_responses(
            std::vector<pmgd::protobufs::CommandResponse *> &cmds,
            const Json::Value &json,
            protobufs::queryMessage &response) = 0;
    };

    class AddEntity : public RSCommand
    {
    public:
        AddEntity();
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        Json::Value construct_responses(
            std::vector<pmgd::protobufs::CommandResponse *>&,
            const Json::Value &json,
            protobufs::queryMessage &response);
    };

    class Connect : public RSCommand
    {
    public:
        Connect();
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        Json::Value construct_responses(
                std::vector<pmgd::protobufs::CommandResponse *>&,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    class FindEntity : public RSCommand
    {
    public:
        FindEntity();
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        Json::Value construct_responses(
            std::vector<pmgd::protobufs::CommandResponse *>&,
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

        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        bool need_blob() { return true; }

        Json::Value construct_responses(
                std::vector<pmgd::protobufs::CommandResponse*> &cmds,
                const Json::Value &json,
                protobufs::queryMessage &response);
    };

    class FindImage: public RSCommand
    {
    public:
        FindImage();
        int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                               const Json::Value& root,
                               const std::string& blob,
                               int txid);

        Json::Value construct_responses(
                std::vector<pmgd::protobufs::CommandResponse *> &cmds,
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
