/**
 * @file   PMGDQuery.cc
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

#include <string>
#include <iostream>
#include <cstring>

#include "PMGDQueryHandler.h"
#include "PMGDQuery.h"
#include "ExceptionsCommand.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>

using namespace VDMS;

// This is for internal reference of the transaction
#define REFERENCE_RANGE_START   20000

PMGDQuery::PMGDQuery(PMGDQueryHandler& pmgd_qh) :
    _pmgd_qh(pmgd_qh), _current_ref(REFERENCE_RANGE_START)
{
    _current_group = 0;
    //this command to start a new transaction
    PMGDCmd* cmdtx = new PMGDCmd;
    //this the protobuf of a new TxBegin
    cmdtx->set_cmd_id(PMGDCmd::TxBegin);
    cmdtx->set_cmd_grp_id(_current_group); //give it an ID
    _cmds.push_back(cmdtx); //push the creating command to the vector
}

PMGDQuery::~PMGDQuery()
{
    for (auto cmd : _cmds) {
        delete cmd;
    }

    for (unsigned i = 0; i < _pmgd_responses.size(); ++i) {
        for (unsigned j = 0; j < _pmgd_responses[i].size(); ++j) {
            if (_pmgd_responses[i][j] != NULL)
                delete _pmgd_responses[i][j];
        }
        _pmgd_responses[i].clear();
    }
}

Json::Value& PMGDQuery::run()
{
    add_group(); // will set _current_group correctly

    // End of the transaction
    PMGDCmd* cmdtxend = new PMGDCmd;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend->set_cmd_id(PMGDCmd::TxCommit);
    cmdtxend->set_cmd_grp_id(_current_group);
    _cmds.push_back(cmdtxend);

    // execute the queries using the PMGDQueryHandler object
    _pmgd_responses = _pmgd_qh.process_queries(_cmds, _current_group + 1);

    if (_pmgd_responses.size() != _current_group + 1) {
        _json_responses["status"] = -1;
        _json_responses["info"] = "PMGD Transacion Error";
        return _json_responses;
    }

    // Get rid of txbeg and txend
    for (int i = 1; i < _pmgd_responses.size() - 1; ++i) {
        auto vec_responses = _pmgd_responses[i];
        Json::Value arr;
        for (auto response : vec_responses) {
            arr.append(parse_response(response));
        }
        _json_responses.append(arr);
    }

    return _json_responses;
}

void PMGDQuery::add_link(const Json::Value& link, PMGDQueryNode* qn)
{
    PMGD::protobufs::LinkInfo *qnb = qn->mutable_link();
    if (link.isMember("ref")) {
        qnb->set_start_identifier(link["ref"].asInt());
    }

    if (link.isMember("direction")) {
        if (link["direction"]== "out")
            qnb->set_dir(PMGD::protobufs::LinkInfo::Outgoing);
        else if ( link["direction"] =="in" )
            qnb->set_dir(PMGD::protobufs::LinkInfo::Incoming);
        else if ( link["direction"] == "any" )
            qnb->set_dir(PMGD::protobufs::LinkInfo::Any);
    }

    if (link.isMember("unique"))
        qnb->set_nb_unique(link["unique"].asBool());

    if (link.isMember("class"))
         qnb->set_e_tag(link["class"].asCString());
}

void PMGDQuery::set_value(const std::string& key, const PMGDProp& p,
                          Json::Value& prop)
{
    switch(p.type()) {
        case PMGDProp::BooleanType:
            prop[key]= p.bool_value();
            break;

        case PMGDProp::IntegerType:
            prop[key]= (Json::Value::UInt64) p.int_value();
            break;

        case PMGDProp::StringType:
            prop[key]= p.string_value();
            break;

        case PMGDProp::TimeType:
            prop[key]= p.string_value();
            break;

        case PMGDProp::FloatType:
            prop[key] = p.float_value();
            break;

        default:
            throw ExceptionCommand(PMGDTransactiontError, "Type Error");
    }
}

void PMGDQuery::set_property(PMGDProp* p, const char* key, Json::Value val)
{
    if (val.isObject()) {
        if (val.isMember("_date")) {
            p->set_type(PMGDProp::TimeType);
            p->set_key(key);
            p->set_string_value(val["_date"].asString());
        }

        if (val.isMember("_blob")) {
            // the blob value is read and stored as a string
            p->set_type(PMGDProp::StringType);
            p->set_key(key);
            p->set_string_value(val["_blob"].asString());
        }
    }
    else if (val.isString()) {
        p->set_type(PMGDProp::StringType);
        p->set_key(key);
        p->set_string_value(val.asString());
    }
    else if (val.isInt()){
        p->set_type(PMGDProp::IntegerType);
        p->set_key(key);
        p->set_int_value(val.asInt());
    }
    else if (val.isBool()) {
        p->set_type(PMGDProp::BooleanType);
        p->set_key(key);
        p->set_bool_value(val.asBool());
    }
    else if (val.isDouble()) {
        p->set_type(PMGDProp::FloatType);
        p->set_key(key);
        p->set_float_value(val.asDouble());
    }
}

Json::Value PMGDQuery::construct_error_response(PMGDCmdResponse *response)
{
    Json::Value ret;
    ret["status"] = response->error_code();
    ret["info"]   = response->error_msg();
    return ret;
}

Json::Value PMGDQuery::parse_response(PMGDCmdResponse* response)
{
    Json::Value ret;
    int return_code = response->error_code();

    auto response_success_or_exists = [&return_code]() {
        return return_code == PMGDCmdResponse::Success &&
               return_code == PMGDCmdResponse::Exists;
    };

    auto response_success = [&return_code]() {
        return return_code == PMGDCmdResponse::Success;
    };

    switch (response->r_type()) {

        case PMGD::protobufs::NodeID:
            if (!response_success_or_exists()) {
                return construct_error_response(response);
            }
            break;

        case PMGD::protobufs::EdgeID:
            if (!response_success_or_exists()) {
                return construct_error_response(response);
            }
            break;

        case PMGD::protobufs::Cached:
            if (!response_success())
                return construct_error_response(response);
            break;

        case PMGD::protobufs::List:
            if (response_success()) {
                Json::Value list;
                auto& mymap = response->prop_values();

                assert(mymap.size() > 0);
                int count = mymap.begin()->second.values().size();

                if (count > 0) {
                    for (int i = 0; i < count; ++i) {
                        Json::Value prop;

                        for (auto& key : mymap) {
                            const PMGDPropList& p = key.second;
                            set_value(key.first, p.values(i), prop);
                        }

                        list.append(prop);
                    }
                    ret["returned"] = (Json::UInt64) response->op_int_value();
                    ret["entities"] = list;
               }
            }
            else {
                return construct_error_response(response);
            }
            break;

        case PMGD::protobufs::Average:
            if (response_success()) {
                assert(response->op_oneof_case() == PMGDCmdResponse::kOpFloatValue);
                double average = response->op_float_value();
                ret["average"] = double(average);
            }
            else {
                return construct_error_response(response);
            }
            break;

        case PMGD::protobufs::Sum:
            if (response_success()) {
                if (response->op_oneof_case() == PMGDCmdResponse::kOpFloatValue)
                    ret["sum"] = response->op_float_value();
                else
                    ret["sum"] = (Json::UInt64)response->op_int_value();
            }
            else {
                return construct_error_response(response);
            }
            break;

        case PMGD::protobufs::Count:
            if (response_success()) {
                ret["count"] = (Json::UInt64) response->op_int_value();
            }
            else {
                return construct_error_response(response);
            }
            break;

        default:
            return construct_error_response(response);
    }

    ret["status"] = PMGDCmdResponse::Success;
    return ret;
}

void PMGDQuery::set_operand(PMGDProp* p, const Json::Value& operand)
{
    if (operand.isInt()) {
        p->set_type(PMGDProp::IntegerType);
        p->set_int_value((operand.asInt()));
    }
    else if (operand.isBool()) {
        p->set_type(PMGDProp::BooleanType);
        p->set_bool_value((operand.asBool()));
    }
    else if (operand.isDouble()) {
        p->set_type(PMGDProp::FloatType);
        p->set_float_value((operand.asDouble()));
    }
    else if (operand.isString()) {
        p->set_type(PMGDProp::StringType);
        p->set_string_value((operand.asString()));
    }
    else {
        p->set_type(PMGDProp::NoValueType);
    }
}

void PMGDQuery::parse_query_constraints(const Json::Value& constraints,
                                       PMGDQueryNode* query_node)
{
    for (auto &key : constraints.getMemberNames()) {

        const Json::Value& predicate = constraints[key];

        // Will either have 2 or 4 arguments
        assert(predicate.isArray());
        assert(predicate.size() == 2 || predicate.size() == 4);

        PMGDPropPred::Op op;
        if (predicate.size() == 4) {
            if (strcmp(predicate[0].asCString(), ">") == 0 &&
                strcmp(predicate[2].asCString(), "<") == 0)
                op = PMGDPropPred::GtLt;
            else if (strcmp(predicate[0].asCString(), ">=") == 0 &&
                     strcmp(predicate[2].asCString(), "<") == 0)
                op = PMGDPropPred::GeLt;
            else if (strcmp(predicate[0].asCString(), ">" ) == 0 &&
                     strcmp(predicate[2].asCString(), "<=") == 0)
                op = PMGDPropPred::GtLe;
            else if (strcmp(predicate[0].asCString(), ">=") == 0 &&
                     strcmp(predicate[2].asCString(), "<=") == 0)
                op = PMGDPropPred::GeLe;
        }
        else {
            if (strcmp(predicate[0].asCString(), ">" ) == 0)
                op = PMGDPropPred::Gt;
            else if (strcmp(predicate[0].asCString(), ">=") == 0)
                op = PMGDPropPred::Ge;
            else if (strcmp(predicate[0].asCString(), "<") == 0)
                op = PMGDPropPred::Lt;
            else if (strcmp(predicate[0].asCString(), "<=") == 0)
                op = PMGDPropPred::Le;
            else if (strcmp(predicate[0].asCString(), "==") == 0)
                op = PMGDPropPred::Eq;
            else if(strcmp(predicate[0].asCString(), "!=") == 0)
                op = PMGDPropPred::Ne;
        }

        PMGDPropPred* pp = query_node->add_predicates();
        pp->set_key(key);  //assign the property predicate key
        pp->set_op(op);

        PMGDProp* p1 = pp->mutable_v1();
        p1->set_key(key);
        set_operand(p1, predicate[1]);

        if (predicate.size() == 4) {
            PMGDProp* p2 = pp->mutable_v2();
            p2->set_key(key);
            set_operand(p2, predicate[3]);
        }
    }
}

void PMGDQuery::get_response_type(const Json::Value& res_types,
            const std::string& response,
            PMGDQueryNode *query_node)
{
    for (auto response_key=0; response_key != res_types[response].size();
         response_key++) {
        std::string *r_key= query_node->add_response_keys();
        *r_key = res_types[response][response_key].asString();
    }
}

void PMGDQuery::parse_query_results (const Json::Value& result_type,
                                    PMGDQueryNode *query_node)
{
    for (auto response_type = result_type.begin();
            response_type != result_type.end(); response_type++) {

        if (response_type.key().asString() == "list") {
            query_node->set_r_type(PMGD::protobufs::List);
            get_response_type(result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "count") {
            query_node->set_r_type(PMGD::protobufs::Count);
            get_response_type (result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "sum") {
            query_node->set_r_type(PMGD::protobufs::Sum);
            get_response_type (result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "sort") {
            query_node->set_sort(true);
            std::string sort_key = result_type[response_type.key().asString()].asString();
            query_node->set_sort_key(sort_key);
        }
        else if (response_type.key().asString() == "limit") {
            int limit = result_type[response_type.key().asString()].asInt();
            query_node->set_limit(limit);
        }

        else if (response_type.key().asString() == "average") {
            query_node->set_r_type(PMGD::protobufs::Average);
            get_response_type(result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "EntityID") {
            query_node->set_r_type(PMGD::protobufs::NodeID);
        }

        else if (response_type.key().asString() == "ConnectionID") {
            query_node->set_r_type(PMGD::protobufs::EdgeID);
        }
    }
}

void PMGDQuery::AddNode(int ref,
                        const std::string& tag,
                        const Json::Value& props,
                        const Json::Value& constraints,
                        bool unique)
{
    PMGDCmd* cmdadd = new PMGDCmd();
    cmdadd->set_cmd_id(PMGDCmd::AddNode);
    cmdadd->set_cmd_grp_id(_current_group);
    PMGD::protobufs::AddNode *an = cmdadd->mutable_add_node();
    an->set_identifier(ref);

    PMGD::protobufs::Node *n = an->mutable_node();
    n->set_tag(tag.c_str());

    if(!props.isNull()) {
        for (auto m : props.getMemberNames()) {
            PMGDProp* p = n->add_properties();
            set_property(p, m.c_str(), props[m]);
        }
    }

    if(!constraints.isNull()) {
        PMGDQueryNode *qn = an->mutable_query_node();
        qn->set_identifier(-1);
        qn->set_tag(tag.c_str());
        qn->set_unique(unique);
        qn->set_p_op(PMGD::protobufs::And);
        qn->set_r_type(PMGD::protobufs::Count);
        parse_query_constraints(constraints, qn);
    }

    _cmds.push_back(cmdadd);
}

void PMGDQuery::AddEdge(int ident,
                        int src, int dst,
                        const std::string& tag,
                        const Json::Value& props)
{
    PMGDCmd* cmdedge = new PMGDCmd();
    cmdedge->set_cmd_grp_id(_current_group);
    cmdedge->set_cmd_id(PMGDCmd::AddEdge);
    PMGD::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();
    ae->set_identifier(ident);

    PMGD::protobufs::Edge *e = ae->mutable_edge();
    e->set_tag(tag.c_str());
    e->set_src(src);
    e->set_dst(dst);

    for (auto m : props.getMemberNames()) {
        PMGDProp* p = e->add_properties();
        set_property(p, m.c_str(), props[m]);
    }

    _cmds.push_back(cmdedge);
}

void PMGDQuery::QueryNode(int ref,
                          const std::string& tag,
                          const Json::Value& link,
                          const Json::Value& constraints,
                          const Json::Value& results,
                          bool unique)
{
    PMGDCmd* cmdquery = new PMGDCmd();
    cmdquery->set_cmd_id(PMGDCmd::QueryNode);
    cmdquery->set_cmd_grp_id(_current_group);

    PMGDQueryNode *qn = cmdquery->mutable_query_node();

    qn->set_identifier(ref);
    qn->set_tag(tag.c_str());
    qn->set_unique(unique);

    if (!link.isNull())
        add_link(link, qn);

    // TODO: We always assume AND, we need to change that
    qn->set_p_op(PMGD::protobufs::And);
    if (!constraints.isNull())
        parse_query_constraints(constraints, qn);

    if (!results.isNull())
        parse_query_results(results, qn);

    _cmds.push_back(cmdquery);
}
