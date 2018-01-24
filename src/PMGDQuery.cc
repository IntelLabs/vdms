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

#include "PMGDQueryHandler.h" // to provide the database connection
#include "PMGDQuery.h" // to provide the database connection

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>

using namespace athena;

PMGDQuery::PMGDQuery(PMGDQueryHandler& pmgd_qh) :
    _pmgd_qh(pmgd_qh)
{
    _current_group = 0;
    //this command to start a new transaction
    PMGDCommand* cmdtx = new PMGDCommand;
    //this the protobuf of a new TxBegin
    cmdtx->set_cmd_id(PMGDCommand::TxBegin);
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
    PMGDCommand* cmdtxend = new PMGDCommand;
    // Commit here doesn't change anything. Just indicates end of TX
    cmdtxend->set_cmd_id(PMGDCommand::TxCommit);
    cmdtxend->set_cmd_grp_id(_current_group);
    _cmds.push_back(cmdtxend);

    // execute the queries using the PMGDQueryHandler object
    _pmgd_responses = _pmgd_qh.process_queries(_cmds, _current_group + 1);

    if (_pmgd_responses.size() != _current_group + 1) {
        // TODO: This is where we will need request server rollback code.
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

void PMGDQuery::add_link(const Json::Value& link, pmgd::protobufs::QueryNode *qn)
{
    pmgd::protobufs::LinkInfo *qnb = qn->mutable_link();
    if (link.isMember("ref")) {
        qnb->set_start_identifier(link["ref"].asInt());
    }
    if (link.isMember("direction")) {
        if (link["direction"]== "out")
            qnb->set_dir(pmgd::protobufs::LinkInfo::Outgoing);
        else if ( link["direction"] =="in" )
            qnb->set_dir(pmgd::protobufs::LinkInfo::Incoming);
        else if ( link["direction"] == "any" )
            qnb->set_dir(pmgd::protobufs::LinkInfo::Any);
    }
    if (link.isMember("unique"))
        qnb->set_nb_unique(link["unique"].asBool());
    if (link.isMember("class"))
         qnb->set_e_tag(link["class"].asCString());
}

Json::Value PMGDQuery::print_properties(const std::string &key,
                            const PMGDProp &p)
{
    Json::Value result;
    switch(p.type()) {
        case PMGDProp::BooleanType:
            result[key]= p.bool_value();
            break;

        case PMGDProp::IntegerType:
            result[key]= (Json::Value::UInt64) p.int_value();
            break;

        case PMGDProp::StringType:
            result[key]= p.string_value();
            break;

        case PMGDProp::TimeType:
            result[key]= p.string_value();
            break;

        case PMGDProp::FloatType:
            result[key] = p.float_value();
            break;

        default:
            // TODO, THROW
            std::cout << "RSCommand::print_properties: Unknown\n";
    }

    return result[key];
}

void PMGDQuery::set_property(PMGDProp *p,
        const char *key, Json::Value val)
{
    if (val.isObject()) {
        if (val.isMember("_date")) {
            p->set_type(PMGDProp::TimeType);
            p->set_key(key);
            p->set_string_value(val["_date"].asString());
        }

        if (val.isMember("_blob")) { //the blob value is read and stored as a string otherwose it is not shown in the graph.
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
    // We down-cast from uint64 to int
    int return_val  = response->op_int_value();
    int return_code = response->error_code();

    bool flag_error = false;

    switch (response->r_type()) {

        case pmgd::protobufs::NodeID:
            if (return_code != PMGDCmdResponse::Success &&
                return_code != PMGDCmdResponse::Exists ) {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::EdgeID:
            if (return_code != PMGDCmdResponse::Success &&
                return_code != PMGDCmdResponse::Exists ) {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Cached:
            if (return_code != PMGDCmdResponse::Success)
                flag_error = true;
            break;

        case pmgd::protobufs::List:
            if (return_code == PMGDCmdResponse::Success) {
                Json::Value list;
                auto& mymap = response->prop_values();

                int count = 0;

                for (auto& key : mymap) {
                    count = key.second.values().size();
                    break;
                }

                if (count > 0) {
                    for (int i = 0; i < count; ++i) {
                        Json::Value prop;

                        for (auto& key : mymap) {
                            const PMGDPropList &p = key.second;
                            prop[key.first] = print_properties(
                                                key.first.c_str(),
                                                p.values(i));
                        }

                        list.append(prop);
                    }
                    ret["returned"] = return_val;
                    ret["entities"] = list;
               }
            }
            else {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Average:
            if (return_code == PMGDCmdResponse::Success) {
                float average = response->op_float_value();
                ret["average"] = double(average);
            }
            else {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Sum:

            if (return_code == PMGDCmdResponse::Success) {
                // We down-cast from uint64 to int64
                Json::Int64 sum = response->op_int_value();
                ret["sum"]= sum;
            }
            else {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Count:
            if (return_code == PMGDCmdResponse::Success) {
                ret["count"] = return_val;
            }
            else {
                flag_error = true;
            }
            break;

        default:
            flag_error = true;
    }

    if (flag_error) {
        return construct_error_response(response);
    }
    else {
        ret["status"] = PMGDCmdResponse::Success;
    }

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
                                       pmgd::protobufs::QueryNode* query_node)
{
    for (auto &key : constraints.getMemberNames()) {

        PMGDPropPred *pp = query_node->add_predicates();
        const Json::Value &predicate = constraints[key];

        assert(predicate.size() > 1);

        pp->set_key(key);  //assign the property predicate key
        std::vector<std::string> operators;
        std::vector<Json::Value> operands;

        PMGDProp *p1;
        PMGDProp *p2 ;

        //iterate over the key elements
        for (int k=0; k < predicate.size(); k++) {
            if ((predicate[k] == ">=") ||
                (predicate[k] == "<=") ||
                (predicate[k] == "==") ||
                (predicate[k] == "!=") ||
                (predicate[k] == "<") ||
                (predicate[k] == ">")
              )
                operators.push_back(predicate[k].asCString());
            else
                operands.push_back(predicate[k]);
        }
        if (operators.size() > 1) {
            if (operators[0 ] ==">" && operators[1] == "<")
                pp->set_op(PMGDPropPred::GtLt);
            else if (operators[0] == ">=" && operators[1] == "<")
                pp->set_op(PMGDPropPred::GeLt);
            else if (operators[0] == ">" && operators[1] == "<=")
                pp->set_op(PMGDPropPred::GtLe);
            else if (operators[0] == ">=" && operators[1] == "<=")
                pp->set_op(PMGDPropPred::GeLe);
        } //if two operations are involved
        else if (operators.size() == 1) {
            if (operators[0] == ">" )
                pp->set_op(PMGDPropPred::Gt);
            else if (operators[0] == ">=")
                pp->set_op(PMGDPropPred::Ge);
            else if (operators[0] == "<")
                pp->set_op(PMGDPropPred::Lt);
            else if (operators[0] == "<=")
                pp->set_op(PMGDPropPred::Le);
            else if (operators[0] == "==")
                pp->set_op(PMGDPropPred::Eq);
            else if(operators[0] == "!=")
                pp->set_op(PMGDPropPred::Ne);
        }

        p1 = pp->mutable_v1();
        p1->set_key(key);
        set_operand(p1, operands[0]);

        if (operands.size() > 1) { //two operands per operator

            p2 = pp->mutable_v2();
            p2->set_key(key);
            set_operand(p2, operands[1]);
        }
    }
}

void PMGDQuery::get_response_type(const Json::Value& result_type_array,
            std::string response,
            pmgd::protobufs::QueryNode *query_node)
{
    for (auto response_key=0; response_key!=result_type_array[response].size();
            response_key++) {
        std::string *r_key= query_node->add_response_keys();
        *r_key = result_type_array[response][response_key].asString();
    }
}

void PMGDQuery::parse_query_results (const Json::Value& result_type,
                                    pmgd::protobufs::QueryNode *query_node)
{
    for (auto response_type =result_type.begin();
            response_type!=result_type.end(); response_type++) {

        if (response_type.key().asString() == "list") {
            query_node->set_r_type(pmgd::protobufs::List);
            get_response_type(result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "count") {
            query_node->set_r_type(pmgd::protobufs::Count);
            get_response_type (result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "sum") {
            query_node->set_r_type(pmgd::protobufs::Sum);
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

            query_node->set_r_type(pmgd::protobufs::Average);
            get_response_type(result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "EntityID")
            query_node->set_r_type(pmgd::protobufs::NodeID);

        else if (response_type.key().asString() == "ConnectionID")
            query_node->set_r_type(pmgd::protobufs::EdgeID);
    }
}

void PMGDQuery::AddNode(int ref,
                        const std::string &tag,
                        const Json::Value& props,
                        const Json::Value& constraints,
                        bool unique)
{
    PMGDCommand* cmdadd = new PMGDCommand();
    cmdadd->set_cmd_id(PMGDCommand::AddNode);
    cmdadd->set_cmd_grp_id(_current_group);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();
    an->set_identifier(ref);

    pmgd::protobufs::Node *n = an->mutable_node();
    n->set_tag(tag.c_str());

    if(!props.isNull()) {
        for (auto m : props.getMemberNames()) {
            PMGDProp *p = n->add_properties();
            set_property(p, m.c_str(), props[m]);
        }
    }

    if(!constraints.isNull()) {
        pmgd::protobufs::QueryNode *qn = an->mutable_query_node();
        qn->set_identifier(-1);
        qn->set_tag(tag.c_str());
        qn->set_unique(unique);
        qn->set_p_op(pmgd::protobufs::And);
        qn->set_r_type(pmgd::protobufs::Count);
        parse_query_constraints(constraints, qn);
    }

    _cmds.push_back(cmdadd);
}

void PMGDQuery::AddEdge(int ident,
                        int src, int dst,
                        const std::string &tag,
                        const Json::Value& props)
{
    PMGDCommand* cmdedge = new PMGDCommand();
    cmdedge->set_cmd_grp_id(_current_group);
    cmdedge->set_cmd_id(PMGDCommand::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();
    ae->set_identifier(ident);

    pmgd::protobufs::Edge *e = ae->mutable_edge();
    e->set_tag(tag.c_str());
    e->set_src(src);
    e->set_dst(dst);

    for (auto m : props.getMemberNames()) {
        PMGDProp *p = e->add_properties();
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
    PMGDCommand* cmdquery = new PMGDCommand();
    cmdquery->set_cmd_id(PMGDCommand::QueryNode);
    cmdquery->set_cmd_grp_id(_current_group);

    pmgd::protobufs::QueryNode *qn = cmdquery->mutable_query_node();

    qn->set_identifier(ref);
    qn->set_tag(tag.c_str());
    qn->set_unique(unique);

    if (!link.isNull())
        add_link(link, qn);

    // TODO: We always assume AND, we need to change that
    qn->set_p_op(pmgd::protobufs::And);
    if (!constraints.isNull())
        parse_query_constraints(constraints, qn);

    if (!results.isNull())
        parse_query_results(results, qn);

    _cmds.push_back(cmdquery);
}
