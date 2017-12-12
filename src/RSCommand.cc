#include <string>
#include <iostream>

#include "QueryHandler.h"

using namespace athena;

RSCommand::RSCommand(const std::string& cmd_name):
    _cmd_name(cmd_name)
{
}

bool RSCommand::check_params(const Json::Value& cmd)
{
    std::map<std::string, int> valid = _valid_params_map;
    std::map<std::string, int> params_map;

    for (auto& param : cmd.getMemberNames()){
        params_map[param] += 1;
    }

    for (auto& param : params_map){
        auto it = valid.find(param.first);
        if ( it == valid.end() ) {
            return false;
        }
        valid[param.first] = 0;
    }

    for (auto& param : valid)
    {
        if (param.second > 1) {
            return false;
        }
    }

    return true;
}

void RSCommand::run_operations(VCL::Image& vclimg, const Json::Value& op)
{
    for (auto& operation : op) {
        std::string type = operation["type"].asString();
        if (type == "threshold") {
            vclimg.threshold(operation["value"].asInt());
        }
        else if (type == "resize") {
            vclimg.resize(operation["height"].asInt(),
                    operation["width" ].asInt());
        }
        else if (type == "crop") {
            vclimg.crop(VCL::Rectangle (
                        operation["x"].asInt(),
                        operation["y"].asInt(),
                        operation["height"].asInt(),
                        operation["width" ].asInt()));
        }
        else {
            // GENERIC_LOGGER << "Operation not recognised: "
            //                << type << std::endl;
        }
    }
}

Json::Value RSCommand::print_properties(const std::string &key,
                            const pmgdProp &p)
{
    Json::Value result;
    switch(p.type()) {
        case pmgdProp::BooleanType:
            result[key]= p.bool_value();
            break;

        case pmgdProp::IntegerType:
            result[key]= (Json::Value::UInt64) p.int_value();
            break;

        case pmgdProp::StringType:
            result[key]= p.string_value();
            break;

        case pmgdProp::TimeType:
            result[key]= p.string_value();
            break;

        case pmgdProp::FloatType:
            result[key] = p.float_value();
            break;

        default:
            // TODO, THROW
            std::cout << "RSCommand::print_properties: Unknown\n";
    }

    return result[key];
}

void RSCommand::add_link(const Json::Value& link, pmgd::protobufs::QueryNode *qn)
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

void RSCommand::set_property(pmgdProp *p,
        const char *key, Json::Value val)
{
    if (val.isObject()) {
        if (val.isMember("_date")) {
            p->set_type(pmgdProp::TimeType);
            p->set_key(key);
            p->set_string_value(val["_date"].asString());
        }

        if (val.isMember("_blob")) { //the blob value is read and stored as a string otherwose it is not shown in the graph.
            p->set_type(pmgdProp::StringType);
            p->set_key(key);
            p->set_string_value(val["_blob"].asString());
        }
    }
    else if (val.isString()) {
        p->set_type(pmgdProp::StringType);
        p->set_key(key);
        p->set_string_value(val.asString());
    }
    else if (val.isInt()){
        p->set_type(pmgdProp::IntegerType);
        p->set_key(key);
        p->set_int_value(val.asInt());
    }
    else if (val.isBool()) {
        p->set_type(pmgdProp::BooleanType);
        p->set_key(key);
        p->set_bool_value(val.asBool());
    }
    else if (val.isDouble()) {
        p->set_type(pmgdProp::FloatType);
        p->set_key(key);
        p->set_float_value(val.asDouble());
    }
}

Json::Value RSCommand::construct_error_response(pmgdCmdResponse *response)
{
    Json::Value ret;
    ret["status"] = response->error_code();
    ret["info"]   = response->error_msg();
    return ret;
}

Json::Value RSCommand::check_responses(
                            std::vector<pmgdCmdResponse *> &responses)
{
    bool flag_error = false;
    Json::Value ret;

    if (responses.size() == 0) {
        ret["status"] = pmgdCmdResponse::Error;
        ret["info"]   = "No responses!";
        return ret;
    }

    for (auto pmgd_res : responses) {
        ret = parse_response(pmgd_res);
        if (pmgd_res->error_code() != pmgdCmdResponse::Success
            &&
            pmgd_res->error_code() != pmgdCmdResponse::Exists)
        {
            ret["status"] = pmgd_res->error_code();
            ret["info"]   = pmgd_res->error_msg();

            flag_error = true;
            break;
        }
    }

    if (!flag_error) {
        ret["status"] = pmgdCmdResponse::Success;
    }

    return ret;
}

Json::Value RSCommand::parse_response(pmgdCmdResponse* response)
{
    Json::Value ret;
    // We down-cast from uint64 to int
    int return_val  = response->op_int_value();
    int return_code = response->error_code();

    bool flag_error = false;

    switch (response->r_type()) {

        case pmgd::protobufs::NodeID:
            if (return_code != pmgdCmdResponse::Success &&
                return_code != pmgdCmdResponse::Exists ) {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::EdgeID:
            if (return_code != pmgdCmdResponse::Success &&
                return_code != pmgdCmdResponse::Exists ) {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Cached:
            if (return_code != pmgdCmdResponse::Success)
                flag_error = true;
            break;

        case pmgd::protobufs::List:
            if (return_code == pmgdCmdResponse::Success) {
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
                            const pmgdPropList &p = key.second;
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
            if (return_code == pmgdCmdResponse::Success) {
                float average = response->op_float_value();
                ret["average"] = double(average);
            }
            else {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Sum:

            if (return_code == pmgdCmdResponse::Success) {
                // We down-cast from uint64 to int64
                Json::Int64 sum = response->op_int_value();
                ret["sum"]= sum;
            }
            else {
                flag_error = true;
            }
            break;

        case pmgd::protobufs::Count:
            if (return_code == pmgdCmdResponse::Success) {
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
        ret["status"] = pmgdCmdResponse::Success;
    }

    return ret;
}

void RSCommand::set_operand(pmgdProp* p, const Json::Value& operand)
{
    if (operand.isInt()) {
        p->set_type(pmgdProp::IntegerType);
        p->set_int_value((operand.asInt()));
    }
    else if (operand.isBool()) {
        p->set_type(pmgdProp::BooleanType);
        p->set_bool_value((operand.asBool()));
    }
    else if (operand.isDouble()) {
        p->set_type(pmgdProp::FloatType);
        p->set_float_value((operand.asDouble()));
    }
    else if (operand.isString()) {
        p->set_type(pmgdProp::StringType);
        p->set_string_value((operand.asString()));
    }
    else {
        p->set_type(pmgdProp::NoValueType);
    }
}

void RSCommand::parse_query_constraints(const Json::Value& constraints,
                                       pmgd::protobufs::QueryNode* query_node)
{
    for (auto &key : constraints.getMemberNames()) {

        pmgdPropPred *pp = query_node->add_predicates();
        const Json::Value &predicate = constraints[key];

        assert(predicate.size() > 1);

        pp->set_key(key);  //assign the property predicate key
        std::vector<std::string> operators;
        std::vector<Json::Value> operands;

        pmgdProp *p1;
        pmgdProp *p2 ;

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
                pp->set_op(pmgdPropPred::GtLt);
            else if (operators[0] == ">=" && operators[1] == "<")
                pp->set_op(pmgdPropPred::GeLt);
            else if (operators[0] == ">" && operators[1] == "<=")
                pp->set_op(pmgdPropPred::GtLe);
            else if (operators[0] == ">=" && operators[1] == "<=")
                pp->set_op(pmgdPropPred::GeLe);
        } //if two operations are involved
        else if (operators.size() == 1) {
            if (operators[0] == ">" )
                pp->set_op(pmgdPropPred::Gt);
            else if (operators[0] == ">=")
                pp->set_op(pmgdPropPred::Ge);
            else if (operators[0] == "<")
                pp->set_op(pmgdPropPred::Lt);
            else if (operators[0] == "<=")
                pp->set_op(pmgdPropPred::Le);
            else if (operators[0] == "==")
                pp->set_op(pmgdPropPred::Eq);
            else if(operators[0] == "!=")
                pp->set_op(pmgdPropPred::Ne);
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

void RSCommand::get_response_type(const Json::Value& result_type_array,
            std::string response,
            pmgd::protobufs::QueryNode *query_node)
{
    for (auto response_key=0; response_key!=result_type_array[response].size();
            response_key++) {
        std::string *r_key= query_node->add_response_keys();
        *r_key = result_type_array[response][response_key].asString();
    }
}

void RSCommand::parse_query_results (const Json::Value& result_type,
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
            query_node->set_sort (true);
            std::string sort_key = result_type[response_type.key().asString()].asString();
            query_node->set_sort_key(sort_key);
        }
        else if (response_type.key().asString() == "limit") {
            int limit =  result_type[response_type.key().asString()].asInt();
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

pmgd::protobufs::Command* RSCommand::AddNode(int grp_id,
                                            int ref,
                                            const std::string &tag,
                                            const Json::Value& props,
                                            Json::Value constraints,
                                            bool unique)
{
    pmgd::protobufs::Command* cmdadd = new pmgd::protobufs::Command();
    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_cmd_grp_id(grp_id);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();
    an->set_identifier(ref);

    pmgd::protobufs::Node *n = an->mutable_node();
    n->set_tag(tag.c_str());

    if(!props.isNull()) {
        for (auto m : props.getMemberNames()) {
            pmgdProp *p = n->add_properties();
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

    return cmdadd;
}

pmgd::protobufs::Command* RSCommand::AddEdge(int grp_id, int ident,
                                      int src, int dst,
                                      const std::string &tag,
                                      Json::Value& props)
{
    pmgd::protobufs::Command* cmdedge = new pmgd::protobufs::Command();
    cmdedge->set_cmd_grp_id(grp_id);
    cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();
    ae->set_identifier(ident);

    pmgd::protobufs::Edge *e = ae->mutable_edge();
    e->set_tag(tag.c_str());
    e->set_src(src);
    e->set_dst(dst);

    for (auto m : props.getMemberNames()) {
        pmgdProp *p = e->add_properties();
        set_property(p, m.c_str(), props[m]);
    }

    return cmdedge;
}

pmgd::protobufs::Command* RSCommand::QueryNode(int grp_id, int ref,
                                      const std::string& tag,
                                      Json::Value& link,
                                      Json::Value& constraints,
                                      Json::Value& results,
                                      bool unique)
{
    pmgd::protobufs::Command* cmdquery = new pmgd::protobufs::Command();
    cmdquery->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    cmdquery->set_cmd_grp_id(grp_id);

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

    return cmdquery;
}
