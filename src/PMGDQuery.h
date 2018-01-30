/**
 * @file   PMGDQuery.h
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
    class PMGDQuery
    {
        std::vector<PMGDCommand* > _cmds;
        unsigned _group_count;
        unsigned _current_group;
        PMGDQueryHandler& _pmgd_qh;
        unsigned _current_ref;

        std::vector<std::vector<PMGDCmdResponse* >> _pmgd_responses;
        Json::Value _json_responses;

        void set_property(PMGDProp* p, const char* key, Json::Value val);
        void add_link(const Json::Value& link, pmgd::protobufs::QueryNode* qn);
        void parse_query_constraints(const Json::Value& constraints,
                                     pmgd::protobufs::QueryNode* query_node);

        void parse_query_results(const Json::Value& result_type,
                                 pmgd::protobufs::QueryNode* query_node);

        void set_operand(PMGDProp* p, const Json::Value& operand);

        void get_response_type(const Json::Value& result_type_array,
                               std::string response,
                               pmgd::protobufs::QueryNode* query_node);

        Json::Value parse_response(PMGDCmdResponse* response);

        Json::Value print_properties(const std::string& key,
                                     const PMGDProp& p);

        Json::Value construct_error_response(PMGDCmdResponse* response);

    public:
        PMGDQuery(PMGDQueryHandler& pmgd_qh);
        ~PMGDQuery();

        unsigned add_group()     { return ++_current_group; }
        unsigned current_group() { return _current_group; }
        unsigned get_available_reference() { return _current_ref++; }

        Json::Value& run();

        //This is a reference to avoid copies
        Json::Value& get_json_responses() {return _json_responses;}

        void AddNode(int ref,
                    const std::string& tag,
                    const Json::Value& props,
                    const Json::Value& constraints,
                    bool unique = false);

        void AddEdge(int ident,
                    int src, int dst,
                    const std::string& tag,
                    const Json::Value& props);

        void QueryNode(int ref,
                    const std::string& tag,
                    const Json::Value& link,
                    const Json::Value& constraints,
                    const Json::Value& results,
                    bool unique = false);
    };
}
