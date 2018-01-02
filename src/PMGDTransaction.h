#pragma once
#include <string>

#include "CommandHandler.h"
#include "PMGDQueryHandler.h" // to provide the database connection

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>

namespace athena {

typedef pmgd::protobufs::Command PMGDCommand;
typedef pmgd::protobufs::CommandResponse PMGDCmdResponse;
typedef pmgd::protobufs::PropertyPredicate PMGDPropPred;
typedef pmgd::protobufs::PropertyList PMGDPropList;
typedef pmgd::protobufs::Property PMGDProp;

    /* This class takes care of the transaction and conversion
        from Protobuf data structures used by PMGD to Json structures
        used by the QueryHandler
    */
    class PMGDTransaction
    {
        std::vector<PMGDCommand *> _cmds;
        unsigned _group_count;
        unsigned _current_group;
        PMGDQueryHandler& _pmgd_qh;

        std::vector<std::vector<PMGDCmdResponse *>> _pmgd_responses;
        Json::Value _json_responses;

        void set_property(PMGDProp *p, const char *key, Json::Value val);
        void add_link(const Json::Value& link, pmgd::protobufs::QueryNode *qn);
        void parse_query_constraints(const Json::Value& constraints,
                                       pmgd::protobufs::QueryNode* query_node);

        void parse_query_results(const Json::Value& result_type,
                                    pmgd::protobufs::QueryNode *query_node);

        void set_operand(PMGDProp* p, const Json::Value& operand);

        void get_response_type(const Json::Value& result_type_array,
            std::string response,
            pmgd::protobufs::QueryNode *query_node);

        Json::Value parse_response(PMGDCmdResponse* response);

        Json::Value print_properties(const std::string &key, const PMGDProp &p);

        Json::Value construct_error_response(PMGDCmdResponse *response);

    public:
        PMGDTransaction(PMGDQueryHandler& pmgd_qh);
        ~PMGDTransaction();

        unsigned add_group()     {return ++_current_group;}
        unsigned current_group() {return _current_group;}

        Json::Value& run();

        //This is a reference to avoid copies
        Json::Value& get_json_responses() {return _json_responses;}

        void AddNode(int ref,
                    const std::string &tag,
                    const Json::Value& props,
                    Json::Value constraints = Json::Value(),
                    bool unique = false);

        void AddEdge(int ident,
                    int src, int dst,
                    const std::string &tag,
                    Json::Value& props);

        void QueryNode(int ref,
                    const std::string& tag,
                    Json::Value& link,
                    Json::Value& constraints,
                    Json::Value& results,
                    bool unique = false);
    };
}
