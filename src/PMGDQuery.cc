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
    _pmgd_qh(pmgd_qh), _current_ref(REFERENCE_RANGE_START),
    _readonly(true)
{
    _current_group_id = 0;
    //this command to start a new transaction
    PMGDCmd* cmdtx = new PMGDCmd;
    //this the protobuf of a new TxBegin
    cmdtx->set_cmd_id(PMGDCmd::TxBegin);
    cmdtx->set_cmd_grp_id(_current_group_id); //give it an ID
    _cmds.push_back(cmdtx); //push the creating command to the vector
}

PMGDQuery::~PMGDQuery()
{
    for (auto cmd : _cmds) {
        delete cmd;
    }
}

Json::Value& PMGDQuery::run()
{
    add_group(); // will set _current_group_id correctly

    // End of the transaction
    PMGDCmd* cmdtxend = new PMGDCmd;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend->set_cmd_id(PMGDCmd::TxCommit);
    cmdtxend->set_cmd_grp_id(_current_group_id);
    _cmds.push_back(cmdtxend);

    // execute the queries using the PMGDQueryHandler object
    std::vector<std::vector<PMGDCmdResponse* >> _pmgd_responses;
    _pmgd_responses = _pmgd_qh.process_queries(_cmds, _current_group_id + 1, _readonly);

    if (_pmgd_responses.size() != _current_group_id + 1) {
        if (_pmgd_responses.size() == 1 && _pmgd_responses[0].size() == 1) {
            _json_responses["status"] = -1;
            _json_responses["info"] = _pmgd_responses[0][0]->error_msg();
            return _json_responses;
        }
        _json_responses["status"] = -1;
        _json_responses["info"] = "PMGDQuery: PMGD Transacion Error";
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

    for (auto& group : _pmgd_responses) {
        for (auto ptr : group) {
            delete ptr;
        }
        group.clear();
    }

    return _json_responses;
}

void PMGDQuery::add_link(const Json::Value& link, PMGDQueryNode* qn)
{
    PMGD::protobufs::LinkInfo *qnl = qn->mutable_link();

    qnl->set_start_identifier(link["ref"].asInt());
    qnl->set_dir(PMGD::protobufs::LinkInfo::Any);

    if (link.isMember("direction")) {
        const std::string& direction = link["direction"].asString();

        if (direction == "out")
            qnl->set_dir(PMGD::protobufs::LinkInfo::Outgoing);
        else if ( direction == "in")
            qnl->set_dir(PMGD::protobufs::LinkInfo::Incoming);
    }

    if (link.isMember("unique"))
        qnl->set_nb_unique(link["unique"].asBool());
    else
        qnl->set_nb_unique(false);

    if (link.isMember("class"))
         qnl->set_e_tag(link["class"].asString());
}

void PMGDQuery::set_value(const std::string& key, const PMGDProp& p,
                          Json::Value& prop)
{
    switch(p.type()) {
        case PMGDProp::BooleanType:
            prop[key] = p.bool_value();
            break;

        case PMGDProp::IntegerType:
            prop[key] = (Json::Value::Int64) p.int_value();
            break;

        case PMGDProp::StringType:
            prop[key] = p.string_value();
            break;

        case PMGDProp::TimeType:
            prop[key] = p.time_value();
            break;

        case PMGDProp::FloatType:
            prop[key] = p.float_value();
            break;

        default:
            throw ExceptionCommand(PMGDTransactiontError, "Type Error");
    }
}

void PMGDQuery::set_property(PMGDProp* p, const std::string& key,
                             const Json::Value& val)
{
    p->set_key(key);

    switch (val.type()) {
        case Json::intValue:
        case Json::uintValue:
            p->set_type(PMGDProp::IntegerType);
            p->set_int_value(val.asInt64());
            break;

        case Json::booleanValue:
            p->set_type(PMGDProp::BooleanType);
            p->set_bool_value(val.asBool());
            break;

        case Json::realValue:
            p->set_type(PMGDProp::FloatType);
            p->set_float_value(val.asDouble());
            break;

        case Json::stringValue:
            p->set_type(PMGDProp::StringType);
            p->set_string_value(val.asString());
            break;

        case Json::objectValue:
            if (val.isMember("_date")) {
                p->set_type(PMGDProp::TimeType);
                p->set_time_value(val["_date"].asString());
            }
            else if (val.isMember("_blob")) {
                // the blob value is read and stored as a string
                p->set_type(PMGDProp::StringType);
                p->set_string_value(val["_blob"].asString());
            }
            else {
                printf("%s\n", key.c_str());
                throw ExceptionCommand(PMGDTransactiontError,
                                       "Object Type Error");
            }
            break;

        default:
            printf("%s\n", key.c_str());
            throw ExceptionCommand(PMGDTransactiontError,
                                   "Object Type Error");
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
                Json::Value list(Json::arrayValue);
                auto& mymap = response->prop_values();

                // assert(mymap.size() > 0);

                uint64_t count = response->op_int_value();

                for (uint64_t i = 0; i < count; ++i) {
                    Json::Value prop;

                    for (auto& key : mymap) {
                        const PMGDPropList& p = key.second;
                        set_value(key.first, p.values(i), prop);
                    }

                    list.append(prop);
                }

                // if count <= 0, we return an empty list (json array)
                ret["returned"] = (Json::UInt64) count;
                if (response->node_edge())
                    ret["entities"] = list;
                else
                    ret["connections"] = list;
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

void PMGDQuery::parse_query_constraints(const Json::Value& constraints,
                                        PMGDQueryConstraints* qn)
{
    for (auto it = constraints.begin(); it != constraints.end(); ++it) {

        const Json::Value& predicate = *it;
        const std::string& key = it.key().asString();

        // Will either have 2 or 4 arguments as verified when parsing
        // JSON
        if (predicate.size() == 2 && predicate[1].isArray()) {

            // This will make the entire query OR,
            // not sure if it is right.
            qn->set_p_op(PMGD::protobufs::Or);

            const std::string& pred1 = predicate[0].asString();

            PMGDPropPred::Op op;

            if (pred1 == ">")
                op = PMGDPropPred::Gt;
            else if (pred1 == ">=")
                op = PMGDPropPred::Ge;
            else if (pred1 == "<")
                op = PMGDPropPred::Lt;
            else if (pred1 == "<=")
                op = PMGDPropPred::Le;
            else if (pred1 == "==")
                op = PMGDPropPred::Eq;
            else if (pred1 == "!=")
                op = PMGDPropPred::Ne;

            for (auto& value : predicate[1]) {

                PMGDPropPred* pp = qn->add_predicates();
                pp->set_key(key);  //assign the property predicate key
                pp->set_op(op);
                PMGDProp* p1 = pp->mutable_v1();
                set_property(p1, key, value);
            }

        }
        else if (predicate.size() == 2) {

            PMGDPropPred* pp = qn->add_predicates();
            pp->set_key(key);  //assign the property predicate key

            PMGDProp* p1 = pp->mutable_v1();
            set_property(p1, key, predicate[1]);

            const std::string& pred1 = predicate[0].asString();

            if (pred1 == ">")
                pp->set_op(PMGDPropPred::Gt);
            else if (pred1 == ">=")
                pp->set_op(PMGDPropPred::Ge);
            else if (pred1 == "<")
                pp->set_op(PMGDPropPred::Lt);
            else if (pred1 == "<=")
                pp->set_op(PMGDPropPred::Le);
            else if (pred1 == "==")
                pp->set_op(PMGDPropPred::Eq);
            else if (pred1 == "!=")
                pp->set_op(PMGDPropPred::Ne);
        }
        else {

            PMGDPropPred* pp = qn->add_predicates();
            pp->set_key(key);  //assign the property predicate key

            PMGDProp* p1 = pp->mutable_v1();
            set_property(p1, key, predicate[1]);

            const std::string& pred1 = predicate[0].asString();

            PMGDProp* p2 = pp->mutable_v2();
            set_property(p2, key, predicate[3]);

            const std::string& pred2 = predicate[2].asString();

            if (pred1 == ">" && pred2 == "<")
                pp->set_op(PMGDPropPred::GtLt);
            else if (pred1 == ">=" && pred2 == "<")
                pp->set_op(PMGDPropPred::GeLt);
            else if (pred1 == ">"  && pred2 == "<=")
                pp->set_op(PMGDPropPred::GtLe);
            else if (pred1 == ">=" && pred2 == "<=")
                pp->set_op(PMGDPropPred::GeLe);

        }
    }
}

void PMGDQuery::get_response_type(const Json::Value& res, PMGDQueryResultInfo *qn)
{
    for (auto it = res.begin(); it != res.end(); it++) {
        std::string *r_key= qn->add_response_keys();
        *r_key = (*it).asString();
    }
}

void PMGDQuery::parse_query_results(const Json::Value& results,
                                    PMGDQueryResultInfo *qn)
{
    for (auto it = results.begin(); it != results.end(); it++) {
        const std::string& key = it.key().asString();

        if (key == "list") {
            qn->set_r_type(PMGD::protobufs::List);
            get_response_type(*it, qn);
        }
        else if (key == "count") {
            qn->set_r_type(PMGD::protobufs::Count);
        }
        else if (key == "sum") {
            qn->set_r_type(PMGD::protobufs::Sum);
            get_response_type(*it, qn);
        }
        else if (key == "sort") {
            qn->set_sort(true);
            std::string *sort_key= qn->mutable_sort_key();

            if ((*it).isObject()) {
                *sort_key = (*it)["key"].asString();
                if ((*it).isMember("order")) {
                    qn->set_descending((*it)["order"] == "descending" ?
                                        true : false);
                }
                else {
                    // Default is False (i.e. result in ascending order)
                    qn->set_descending(false);
                }
            }
            else {
                *sort_key = (*it).asString();
                qn->set_descending(false);
            }
        }
        else if (key == "limit") {
            int limit = (*it).asUInt();
            qn->set_limit(limit);
        }
        else if (key == "average") {
            qn->set_r_type(PMGD::protobufs::Average);
            get_response_type(*it, qn);
        }
    }
}

void PMGDQuery::AddNode(int ref,
                        const std::string& tag,
                        const Json::Value& props,
                        const Json::Value& constraints)
{
    _readonly = false;

    PMGDCmd* cmdadd = new PMGDCmd();
    cmdadd->set_cmd_id(PMGDCmd::AddNode);
    cmdadd->set_cmd_grp_id(_current_group_id);
    PMGD::protobufs::AddNode *an = cmdadd->mutable_add_node();
    an->set_identifier(ref);

    PMGD::protobufs::Node *n = an->mutable_node();
    n->set_tag(tag);

    for (auto it = props.begin(); it != props.end(); ++it) {
        PMGDProp* p = n->add_properties();
        set_property(p, it.key().asString(), *it);
    }

    if(!constraints.isNull()) {
        PMGDQueryNode *qn = an->mutable_query_node();
        qn->set_identifier(ref); // Use the same ref to cache if node exists.
        PMGDQueryConstraints *qc = qn->mutable_constraints();
        qc->set_tag(tag);
        qc->set_unique(true);
        qc->set_p_op(PMGD::protobufs::And);
        parse_query_constraints(constraints, qc);
        PMGDQueryResultInfo *qr = qn->mutable_results();
        qr->set_r_type(PMGD::protobufs::NodeID); // Since PMGD returns ids.
    }

    _cmds.push_back(cmdadd);
}

void PMGDQuery::UpdateNode(int ref,
                        const std::string& tag,
                        const Json::Value& props,
                        const Json::Value& remove_props,
                        const Json::Value& constraints,
                        bool unique)
{
    _readonly = false;

    PMGDCmd* cmdupdate = new PMGDCmd();
    cmdupdate->set_cmd_id(PMGDCmd::UpdateNode);
    cmdupdate->set_cmd_grp_id(_current_group_id);
    PMGD::protobufs::UpdateNode *un = cmdupdate->mutable_update_node();
    un->set_identifier(ref);

    for (auto it = props.begin(); it != props.end(); ++it) {
        PMGDProp* p = un->add_properties();
        set_property(p, it.key().asString(), *it);
    }

    for (auto it = remove_props.begin(); it != remove_props.end(); it++) {
        std::string *r_key= un->add_remove_props();
        *r_key = (*it).asString();
    }

    if(!constraints.isNull()) {
        PMGDQueryNode *qn = un->mutable_query_node();
        qn->set_identifier(ref < 0 ? get_available_reference() : ref);
        PMGDQueryConstraints *qc = qn->mutable_constraints();
        qc->set_tag(tag);
        qc->set_unique(unique);
        qc->set_p_op(PMGD::protobufs::And);
        parse_query_constraints(constraints, qc);
        PMGDQueryResultInfo *qr = qn->mutable_results();
        qr->set_r_type(PMGD::protobufs::NodeID); // Since PMGD returns ids.
    }

    _cmds.push_back(cmdupdate);
}

void PMGDQuery::AddEdge(int ident,
                        int src, int dst,
                        const std::string& tag,
                        const Json::Value& props)
{
    _readonly = false;

    PMGDCmd* cmdedge = new PMGDCmd();
    cmdedge->set_cmd_grp_id(_current_group_id);
    cmdedge->set_cmd_id(PMGDCmd::AddEdge);
    PMGD::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();
    ae->set_identifier(ident);

    PMGD::protobufs::Edge *e = ae->mutable_edge();
    e->set_tag(tag);
    e->set_src(src);
    e->set_dst(dst);

    for (auto it = props.begin(); it != props.end(); ++it) {
        PMGDProp* p = e->add_properties();
        set_property(p, it.key().asString(), *it);
    }

    _cmds.push_back(cmdedge);
}

void PMGDQuery::UpdateEdge(int ref, int src_ref, int dest_ref,
                        const std::string& tag,
                        const Json::Value& props,
                        const Json::Value& remove_props,
                        const Json::Value& constraints,
                        bool unique)
{
    _readonly = false;

    PMGDCmd* cmdupdate = new PMGDCmd();
    cmdupdate->set_cmd_id(PMGDCmd::UpdateEdge);
    cmdupdate->set_cmd_grp_id(_current_group_id);
    PMGD::protobufs::UpdateEdge *ue = cmdupdate->mutable_update_edge();
    ue->set_identifier(ref);

    for (auto it = props.begin(); it != props.end(); ++it) {
        PMGDProp* p = ue->add_properties();
        set_property(p, it.key().asString(), *it);
    }

    for (auto it = remove_props.begin(); it != remove_props.end(); it++) {
        std::string *r_key= ue->add_remove_props();
        *r_key = (*it).asString();
    }

    if(!constraints.isNull()) {
        PMGDQueryEdge *qe = ue->mutable_query_edge();
        qe->set_identifier(ref < 0 ? get_available_reference() : ref);
        qe->set_src_node_id(src_ref);
        qe->set_dest_node_id(dest_ref);
        PMGDQueryConstraints *qc = qe->mutable_constraints();
        qc->set_tag(tag);
        qc->set_unique(unique);
        qc->set_p_op(PMGD::protobufs::And);
        parse_query_constraints(constraints, qc);
        PMGDQueryResultInfo *qr = qe->mutable_results();
        qr->set_r_type(PMGD::protobufs::EdgeID); // Since PMGD returns ids.
    }

    _cmds.push_back(cmdupdate);
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
    cmdquery->set_cmd_grp_id(_current_group_id);

    PMGDQueryNode *qn = cmdquery->mutable_query_node();

    qn->set_identifier(ref);

    PMGDQueryConstraints *qc = qn->mutable_constraints();
    qc->set_tag(tag);
    qc->set_unique(unique);

    if (!link.isNull()) {
        add_link(link, qn);
    }

    // TODO: We always assume AND, we need to change that
    qc->set_p_op(PMGD::protobufs::And);
    if (!constraints.isNull())
        parse_query_constraints(constraints, qc);

    PMGDQueryResultInfo *qr = qn->mutable_results();
    if (!results.isNull())
        parse_query_results(results, qr);

    _cmds.push_back(cmdquery);
}

void PMGDQuery::QueryEdge(int ref, int src_ref, int dest_ref,
                          const std::string& tag,
                          const Json::Value& constraints,
                          const Json::Value& results,
                          bool unique)
{
    PMGDCmd* cmdquery = new PMGDCmd();
    cmdquery->set_cmd_id(PMGDCmd::QueryEdge);
    cmdquery->set_cmd_grp_id(_current_group_id);

    PMGDQueryEdge *qn = cmdquery->mutable_query_edge();

    qn->set_identifier(ref);
    qn->set_src_node_id(src_ref);
    qn->set_dest_node_id(dest_ref);

    PMGDQueryConstraints *qc = qn->mutable_constraints();
    qc->set_tag(tag);
    qc->set_unique(unique);

    // TODO: We always assume AND, we need to change that
    qc->set_p_op(PMGD::protobufs::And);
    if (!constraints.isNull())
        parse_query_constraints(constraints, qc);

    PMGDQueryResultInfo *qr = qn->mutable_results();
    if (!results.isNull())
        parse_query_results(results, qr);

    _cmds.push_back(cmdquery);
}
