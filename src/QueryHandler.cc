#include <string>
#include <fstream>
#include "QueryHandler.h"
#include "chrono/Chrono.h"

#include "jarvis.h"
#include "util.h"

#include "AthenaConfig.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

#define ATHENA_IM_TAG           "AT:IMAGE"
#define ATHENA_IM_NAME_PROP     "name"
#define ATHENA_IM_PATH_PROP     "imgPath"

#define ATHENA_COL_TAG          "AT:COLLECTION"
#define ATHENA_COL_NAME_PROP    "name"
#define ATHENA_COL_EDGE_TAG     "collection_tag"

using namespace athena;

static uint32_t STATIC_IDENTIFIER = 0;

// TODO This will be later replaced by a real logger
std::ofstream GENERIC_LOGGER("log.log", std::fstream::app);


QueryHandler::QueryHandler(Jarvis::Graph *db, std::mutex *mtx)
    : _pmgd_qh(db, mtx)
{
    _rs_cmds["AddEntity"]  = new AddEntity();
    _rs_cmds["Connect"]    = new AddConnection();
    _rs_cmds["FindEntity"] = new FindEntity();
    _rs_cmds["AddImage"]   = new AddImage();
    _rs_cmds["FindImage"]  = new FindImage();
}

void QueryHandler::process_connection(comm::Connection *c)
{
    CommandHandler handler(c);

    try {
        while (true) {
            protobufs::queryMessage response;
            process_query(handler.get_command(), response );
            handler.send_response(response);
        }
    } catch (comm::ExceptionComm e) {
        print_exception(e);
    }
    delete c;
}

void QueryHandler::process_query(protobufs::queryMessage proto_query,
                                 protobufs::queryMessage& proto_res)
{
    Json::FastWriter fastWriter;

    try {
        Json::Value json_responses;
        Json::Value root;
        Json::Reader reader;

        bool parsingSuccessful = reader.parse( proto_query.json().c_str(),
                root );

        if ( !parsingSuccessful ) {
            std::cout << "Error parsing: " << std::endl;
            std::cout << proto_query.json() << std::endl;
            Json::Value error;
            error["return"] = "Server error - parsing";
            json_responses.append(error);
        }

        // TODO REMOVE THIS:
        Json::StyledWriter swriter;
        GENERIC_LOGGER << swriter.write(root) << std::endl;

        // define a vector of commands
        // TODO WE NEED TO DELETE EACH ELEMENT AFTER DONE WITH THIS VECTOR!!!
        std::vector<pmgd::protobufs::Command *> cmds;
        unsigned group_count = 0;
        //this command to start a new transaction
        pmgd::protobufs::Command cmdtx;
        //this the protobuf of a new TxBegin
        cmdtx.set_cmd_id(pmgd::protobufs::Command::TxBegin);
        cmdtx.set_cmd_grp_id(group_count); //give it an ID
        cmds.push_back(&cmdtx); //push the creating command to the vector

        unsigned blob_count = 0;

        //iterate over the list of the queries
        for (int j = 0; j < root.size(); j++) {
            const Json::Value& query = root[j];
            assert (query.getMemberNames().size() == 1);
            std::string cmd = query.getMemberNames()[0];
            ++group_count;

            if (_rs_cmds.end() == _rs_cmds.find(cmd)) {
                Json::Value error;
                error["error"] = "Command not found: " + cmd;
                proto_res.set_json(fastWriter.write(error));
            }

            if (_rs_cmds[cmd]->need_blob()) {
                assert (proto_query.blobs().size() >= blob_count);
                std::string blob = proto_query.blobs(blob_count);
                _rs_cmds[cmd]->construct_protobuf(cmds, query, blob,
                        group_count);
                blob_count++;
            }
            else {
                _rs_cmds[cmd]->construct_protobuf(cmds, query, "",
                        group_count);
            }
        }
        ++group_count;

        pmgd::protobufs::Command cmdtxend;
        // Commit here doesn't change anything. Just indicates end of TX
        cmdtxend.set_cmd_id(pmgd::protobufs::Command::TxCommit);
        cmdtxend.set_cmd_grp_id(group_count);
        cmds.push_back(&cmdtxend);
        // execute the queries using the PMGDQueryHandler object

        // std::cout << "Queries: " << std::endl;
        // for (auto ele2 : cmds) {
        //     std::cout << ele2->DebugString() << std::endl;
        // }

        std::vector<std::vector<pmgd::protobufs::CommandResponse *>>
            pmgd_responses = _pmgd_qh.process_queries(cmds, group_count + 1);

        // std::cout << "Responses: " << std::endl;
        // for (auto ele : pmgd_responses) {
        //     for (auto ele2 : ele) {
        //         std::cout << ele2->DebugString() << std::endl;
        //     }
        // }

        // Make sure there were no errors
        if (pmgd_responses.size() != group_count + 1) {
            // TODO: This is where we will need request server rollback code.
            std::vector<pmgd::protobufs::CommandResponse *>& res = pmgd_responses.at(0);
            json_responses.append(RSCommand::construct_error_response(res[0]));
        }
        else {
            for (int j = 0; j < root.size(); j++) {
                std::string cmd = root[j].getMemberNames()[0];
                std::vector<pmgd::protobufs::CommandResponse *>& res =
                            pmgd_responses.at(j+1);
                json_responses.append( _rs_cmds[cmd]->construct_responses(
                                                        res,
                                                        &root[j],
                                                        proto_res) );
            }
        }
        proto_res.set_json(fastWriter.write(json_responses));

    } catch (VCL::Exception e) {
        print_exception(e);
        Json::Value error;
        error["error"] = "VCL Exception!";
        proto_res.set_json(fastWriter.write(error));
    } catch (Jarvis::Exception e) {
        print_exception(e);
        Json::Value error;
        error["error"] = "Jarvis Exception!";
        proto_res.set_json(fastWriter.write(error));
    }
}

//========= RSCommnand definitions =========

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
            GENERIC_LOGGER << "Operation not recognised: "
                           << type << std::endl;
        }
    }
}

Json::Value RSCommand::print_properties(const std::string &key,
                            const pmgd::protobufs::Property &p)
{
    Json::Value result;
    switch(p.type()) {
        case pmgd::protobufs::Property::BooleanType:
                result[key]= p.bool_value();
                break;

        case pmgd::protobufs::Property::IntegerType:
                 result[key]= (Json::Value::UInt64) p.int_value();
                break;
        case pmgd::protobufs::Property::StringType:


        case pmgd::protobufs::Property::TimeType:
                result[key]= p.string_value();

        break;
        case pmgd::protobufs::Property::FloatType:
                result[key] = p.float_value();
            break;
        default:
            printf(" Unknown\n");
    }

    return result[key];
}

void RSCommand::add_link(Json::Value& link, pmgd::protobufs::QueryNode *qn)
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

void RSCommand::set_property(pmgd::protobufs::Property *p,
        const char *key, Json::Value val)
{
    if (val.isObject()) {
        if (val.isMember("_date")) {
            p->set_type(pmgd::protobufs::Property::TimeType);
            p->set_key(key);
            p->set_string_value(val["_date"].asString());
        }

        if (val.isMember("_blob")) { //the blob value is read and stored as a string otherwose it is not shown in the graph.
            p->set_type(pmgd::protobufs::Property::StringType);
            p->set_key(key);
            p->set_string_value(val["_blob"].asString());
        }
    }
    else if (val.isString()) {
        p->set_type(pmgd::protobufs::Property::StringType);
        p->set_key(key);
        p->set_string_value(val.asString());
    }
    else if (((val).isInt())) {
        p->set_type(pmgd::protobufs::Property::IntegerType);
        p->set_key(key);
        p->set_int_value(val.asInt());
    }
    else if (((val).isBool())) {
        p->set_type(pmgd::protobufs::Property::BooleanType);
        p->set_key(key);
        p->set_bool_value(val.asBool());
    }
    else if (((val).isDouble())) {
        p->set_type(pmgd::protobufs::Property::FloatType);
        p->set_key(key);
        p->set_float_value(val.asDouble());
    }
}

Json::Value RSCommand::construct_error_response(pmgd::protobufs::CommandResponse *response)
{
    Json::Value json_responses;
    json_responses["status"] = response->error_code();
    json_responses["info"] = response->error_msg();
    return  json_responses;
}

Json::Value RSCommand::parse_response(
                pmgd::protobufs::CommandResponse* response)
{
    Json::Value json_responses;
    long nodeid = response->op_int_value();

    switch (response->r_type()) {

        case pmgd::protobufs::NodeID:
            if (response->error_code() == pmgd::protobufs::CommandResponse::Success)
                json_responses["status"] = pmgd::protobufs::CommandResponse::Success;

            else if (response->error_code() == pmgd::protobufs::CommandResponse::Exists)
                json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            else {
                json_responses["status"] = response->error_code();
                json_responses["info"] = response->error_msg();
            }
            break;

        case pmgd::protobufs::EdgeID:
            if (response->error_code() == pmgd::protobufs::CommandResponse::Success) {
                 if (response->error_code() == pmgd::protobufs::CommandResponse::Exists)
                    json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
                else
                    json_responses["status"] = response->error_code();
            }
            else {
                json_responses["status"] = response->error_code();
                json_responses["info"]   = response->error_msg();
            }
            break;

        case pmgd::protobufs::Cached:
            if (response->error_code() == pmgd::protobufs::CommandResponse::Success)
                json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            else {
                json_responses["status"] = response->error_code();
                json_responses["info"]   = response->error_msg();
            }
            break;

        case pmgd::protobufs::List:
            if (response->error_code() == pmgd::protobufs::CommandResponse::Success) {
                int cnt;
                Json::Value list;
                Json::Value result;
                auto mymap = response->prop_values();

                int count = 0;

                for (auto key:mymap) {
                    count=key.second.values().size();
                    break;
                }

                if (count > 0) {
                    for (int i = 0; i<count; ++i) {
                        Json::Value prop;

                        for (auto key : mymap) {
                            pmgd::protobufs::PropertyList &p = key.second;
                            prop[key.first] = print_properties(key.first.c_str(), p.values(i));
                        }

                        list.append(prop);
                    }
                    json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
                    json_responses["returned"] = Json::Int64 (response->op_int_value());
                    json_responses["entities"] = list;
                    list.clear();
               }
            }
            else {
                json_responses["status"]=response->error_code();
                json_responses["info"] =response->error_msg();
            }
            break;

    case pmgd::protobufs::Average:
        if (response->error_code() == pmgd::protobufs::CommandResponse::Success) {
            float average = response->op_float_value();
            json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            json_responses["average"] = double(average);
        }
        else {
            json_responses["status"] = response->error_code();
            json_responses["info"]   = response->error_msg();
        }
        break;

    case pmgd::protobufs::Sum:

        if (response->error_code() == pmgd::protobufs::CommandResponse::Success) {
            Json::Int64  sum = response->op_int_value();
            json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            json_responses["sum"]= sum;
        }
        else {
            json_responses["status"] = response->error_code();
            json_responses["info"]   = response->error_msg();
        }
        break;

    case pmgd::protobufs::Count:
        if (response->error_code() == pmgd::protobufs::CommandResponse::Success) {
            float count = response->op_int_value();
            json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            json_responses["count"] =float(count);
        }
        else {
            json_responses["status"] = response->error_code();
            json_responses["info"]   = response->error_msg();
        }
        break;

    default:
        json_responses["status"] = response->error_code();
        json_responses["info"]   = response->error_msg();
    }

    return  json_responses;
}

//========= AddEntity definitions =========

Json::Value AddEntity::construct_responses(
    std::vector<pmgd::protobufs::CommandResponse *>& response,
    Json::Value* query,
    protobufs::queryMessage &query_res)
{
    Json::Value addEntity;
    for (auto it : response) {
        addEntity[query->getMemberNames()[0]] = parse_response(it);
    }

    return addEntity;
}

int AddEntity::add_entity_body(pmgd::protobufs::AddNode *an,
        const Json::Value& json_node)
{
    if (json_node.isMember("_ref"))
        an->set_identifier( json_node["_ref"].asInt());
    else
        an->set_identifier( -1 );

    pmgd::protobufs::Node *n = an->mutable_node();

    if (json_node.isMember("class")) {
        n->set_tag(json_node["class"].asCString());
    }

    //iterate over the properties of nodes
    if (json_node.isMember("properties")) {

        const Json::Value node_properties = json_node["properties"]; //take the sub-object of the properties

        for (Json::ValueConstIterator itr = node_properties.begin();
                itr != node_properties.end(); itr++) {
            pmgd::protobufs::Property *p = n->add_properties();
            // Checking the properties
            set_property(p, itr.key().asCString(), *itr);
        } //nodes properties
    }

    return 0;
}

int AddEntity::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    pmgd::protobufs::Command* cmdadd = new  pmgd::protobufs::Command();

    Json::Value json_node = jsoncmd["AddEntity"];

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_cmd_grp_id(grp_id);

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_cmd_grp_id(grp_id);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();

    if( json_node.isMember("constraints")) {

        pmgd::protobufs::QueryNode *qn = an->mutable_query_node();

        if (json_node.isMember("_ref"))
            qn->set_identifier( json_node["_ref"].asInt());
        else
            qn->set_identifier(-1);

        if (json_node.isMember("class"))
            qn->set_tag(json_node["class"].asCString());
        if (json_node.isMember("unique"))
            qn->set_unique(json_node["unique"].asBool());

        qn->set_p_op(pmgd::protobufs::And);
        parse_query_constraints(json_node["constraints"], qn);
    }

    add_entity_body(an,json_node);

    cmds.push_back(cmdadd);
    return 0;
}

//========= AddConnection definitions =========

int AddConnection::construct_protobuf(
        std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    Json::Value json_edge = jsoncmd["Connect"];

    pmgd::protobufs::Command* cmdedge = new pmgd::protobufs::Command();
    cmdedge->set_cmd_grp_id(grp_id);
    cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();

    if (json_edge.isMember("_ref"))
        ae->set_identifier(json_edge["_ref"].asUInt());

    pmgd::protobufs::Edge *e = ae->mutable_edge();

    if (json_edge.isMember("ref1"))
        e->set_src(json_edge["ref1"].asInt());
    if (json_edge.isMember("ref2"))
        e->set_dst(json_edge["ref2"].asUInt());
    if (json_edge.isMember("class"))
        e->set_tag(json_edge["class"].asCString());

    if (json_edge.isMember("properties")) {
        const Json::Value edge_properties = json_edge["properties"]; //take the sub-object of the properties

        for (Json::ValueConstIterator itr = edge_properties.begin() ;
                itr != edge_properties.end() ; itr++ ) {
            pmgd::protobufs::Property *p = e->add_properties();
            // Checking the properties
            set_property(p, itr.key().asCString(), *itr);
        } //nodes properties
    }

     cmds.push_back(cmdedge);
    return 0;
}

Json::Value AddConnection::construct_responses(
    std::vector<pmgd::protobufs::CommandResponse*>& response,
    Json::Value* query,
    protobufs::queryMessage &query_res)
{
    Json::Value addEdge;
    for (auto it : response) {
        addEdge[query->getMemberNames()[0]] = parse_response(it);
    }
    return addEdge;
}

//========= FindEntity definitions =========

void RSCommand::set_operand(pmgd::protobufs::Property* p, Json::Value operand)
{
    if (operand.isInt()) {
        p->set_type(pmgd::protobufs::Property::IntegerType);
        p->set_int_value((operand.asInt()));
    }
    else if (operand.isBool()) {
        p->set_type(pmgd::protobufs::Property::BooleanType);
        p->set_bool_value((operand.asBool()));
    }
    else if (operand.isDouble()) {
        p->set_type(pmgd::protobufs::Property::FloatType);
        p->set_float_value((operand.asDouble()));
    }
    else if (operand.isString()) {
        p->set_type(pmgd::protobufs::Property::StringType);
        p->set_string_value((operand.asString()));
    }
    else {
        p->set_type(pmgd::protobufs::Property::NoValueType);
    }
}

int RSCommand::parse_query_constraints(const Json::Value& predicates_array, pmgd::protobufs::QueryNode* query_node)
{
    for (auto itr = predicates_array.begin() ;
           itr != predicates_array.end() ; itr++ ) {
        pmgd::protobufs::PropertyPredicate *pp = query_node->add_predicates();
        Json::Value predicate =*itr;

        std::string predicate_key=itr.key().asCString();
        pp->set_key(predicate_key);  //assign the property predicate key
        std::vector<std::string> operators;
        std::vector<Json::Value> operands;

        pmgd::protobufs::Property *p1;
        pmgd::protobufs::Property *p2 ;

        for (int k=0; k < predicate.size(); k++) { //iterate over the key elements
            if ((predicate[k] == ">=")
                    ||(predicate[k] =="<=")
                    ||(predicate[k]=="==")
                    ||(predicate[k]=="!=")
                    ||(predicate[k]== "<")
                    ||(predicate[k]== ">")
              )
                operators.push_back(predicate[k].asCString());
            else
                operands.push_back(predicate[k]);
        }
        if (operators.size() > 1) {
            if (operators[0 ] ==">" && operators[1] == "<")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GtLt);
            else if (operators[0] == ">=" && operators[1] == "<")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GeLt);
            else if (operators[0] == ">" && operators[1] == "<=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GtLe);
            else if (operators[0] == ">=" && operators[1] == "<=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GeLe);
        } //if two operations are involved
        else if (operators.size() == 1) {
            if (operators[0] == ">" )
                pp->set_op(pmgd::protobufs::PropertyPredicate::Gt);
            else if (operators[0] == ">=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Ge);
            else if (operators[0] == "<")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Lt);
            else if (operators[0] == "<=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Le);
            else if (operators[0] == "==")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Eq);
            else if(operators[0] == "!=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Ne);
        }

        p1 = pp->mutable_v1();
        p1->set_key(predicate_key);

        set_operand(p1, operands[0]);

        if (operands.size() > 1) { //two operands per operator

            p2 = pp->mutable_v2();
            p2->set_key(predicate_key);
            set_operand( p2, operands[1]);

        }
    }

   return 0;
}

int RSCommand::get_response_type(const Json::Value& result_type_array,
            std::string response,
            pmgd::protobufs::QueryNode *query_node)
{
    for (auto response_key=0; response_key!=result_type_array[response].size();
            response_key++) {
        std::string *r_key= query_node->add_response_keys();
        *r_key = result_type_array[response][response_key].asString();
    }

    return 0;
}

int RSCommand::parse_query_results (const Json::Value& result_type,
                                    pmgd::protobufs::QueryNode *query_node)
{
    for (auto response_type =result_type.begin(); response_type!=result_type.end(); response_type++) {

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

        else if (response_type.key().asString() == "average") {

            query_node->set_r_type(pmgd::protobufs::Average);
            get_response_type(result_type, response_type.key().asString(), query_node);
        }

        else if (response_type.key().asString() == "EntityID")
            query_node->set_r_type(pmgd::protobufs::NodeID);

        else if (response_type.key().asString() == "ConnectionID")
            query_node->set_r_type(pmgd::protobufs::EdgeID);
    }

    return 0;
}

int RSCommand::build_query_protobuf(pmgd::protobufs::Command* cmd,
                                     const Json::Value& query,
                                     pmgd::protobufs::QueryNode *query_node )
{
    // TODO: We always assume AND, we need to change that
    query_node->set_p_op(pmgd::protobufs::And);
    parse_query_constraints(query["constraints"], query_node);
    parse_query_results (query["results"], query_node);
    return 0;
}


int FindEntity::construct_protobuf(
    std::vector<pmgd::protobufs::Command*> &cmds,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id)
{
    Json::Value find_ent = jsoncmd["FindEntity"];
    pmgd::protobufs::Command* cmdquery = new pmgd::protobufs::Command();
    cmdquery->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    cmdquery->set_cmd_grp_id(grp_id);

    pmgd::protobufs::QueryNode *qn = cmdquery->mutable_query_node();

    if (find_ent.isMember("_ref"))
        qn->set_identifier(find_ent["_ref"].asInt());
    else
        qn->set_identifier(-1);

    if (find_ent.isMember("class"))
        qn->set_tag(find_ent["class"].asCString());

    if (find_ent.isMember("unique"))
        qn->set_unique(find_ent["unique"].asBool());

    if (find_ent.isMember("link")) {
        add_link(find_ent["link"], qn);
    }

    build_query_protobuf(cmdquery, find_ent, qn);

    cmds.push_back(cmdquery);

    return 0;
}

Json::Value FindEntity::construct_responses(
    std::vector<pmgd::protobufs::CommandResponse*>& response,
    Json::Value* query,
    protobufs::queryMessage &query_res)
{
    Json::Value findEntity;
    for (auto it : response)
        findEntity[query->getMemberNames()[0]] = parse_response(it);

    return findEntity;
}

//========= AddImage definitions =========

AddImage::AddImage()
{
    _storage_tdb = AthenaConfig::instance()
                ->get_string_value("tiledb_database", DEFAULT_TDB_PATH);
    _storage_png = AthenaConfig::instance()
                ->get_string_value("png_database", DEFAULT_PNG_PATH);
}

int AddImage::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    Json::Value aImg = jsoncmd["AddImage"];

    ChronoCpu ch("addImage");
    ch.tic();

    // Create PMGD cmd for AddNode
    pmgd::protobufs::Command* cmdadd = new pmgd::protobufs::Command();
    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_cmd_grp_id(grp_id);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();

    // TODO: THIS STATIC IDENTIFIER IS HORRIBLE
    uint32_t id_node = STATIC_IDENTIFIER++;
    an->set_identifier(id_node);

    // Adds AT:IMAGE node
    pmgd::protobufs::Node *n = an->mutable_node();
    n->set_tag(ATHENA_IM_TAG);

    if (aImg.isMember("properties")) {
        Json::Value props = aImg["properties"];

        for (auto m : props.getMemberNames()) {
            pmgd::protobufs::Property *p = n->add_properties();
            set_property(p, m.c_str(), props[m]);
        }
    }

    ChronoCpu ch_ops("operations");
    ch_ops.tic();

    VCL::Image vclimg((void*)blob.data(), blob.size());

    if (aImg.isMember("operations")) {
        run_operations(vclimg, aImg["operations"]);
    }

    ChronoCpu wr_ch("wr_ch");

    std::string img_root = _storage_tdb;
    VCL::ImageFormat vcl_format = VCL::TDB;

    if (aImg.isMember("format")) {
        std::string format = aImg["format"].asString();

        if (format == "png") {
            vcl_format = VCL::PNG;
            img_root = _storage_png;
        }
        else if (format == "tdb") {
            vcl_format = VCL::TDB;
            img_root = _storage_tdb;
        }
        else {
            std::cout << "Format Not Implemented" << std::endl;
            Json::Value error;
            error["Format"] = format + " Not implemented";
            // response.append(error);
            // return;
        }
    }

    std::string file_name = vclimg.create_name(img_root, vcl_format);

    pmgd::protobufs::Property *p = n->add_properties();
    p->set_type(pmgd::protobufs::Property::StringType);
    p->set_key(ATHENA_IM_PATH_PROP);
    p->set_string_value(file_name);

    wr_ch.tic();
    vclimg.store(file_name, vcl_format);
    wr_ch.tac();

    ch_ops.tac();

    cmds.push_back(cmdadd);

    if (aImg.isMember("link")) {
        Json::Value& link = aImg["link"];

        if (link.isMember("ref")) {
            pmgd::protobufs::Command* cmd = new pmgd::protobufs::Command();
            cmd->set_cmd_id(pmgd::protobufs::Command::AddEdge);
            cmd->set_cmd_grp_id(grp_id);

            pmgd::protobufs::AddEdge* adde = cmd->mutable_add_edge();
            pmgd::protobufs::Edge* edge = adde->mutable_edge();

            if (link.isMember("direction") && link["direction"] == "in") {
                edge->set_src(link["ref"].asUInt());
                edge->set_dst(id_node);
            }
            else {
                edge->set_src(id_node);
                edge->set_dst(link["ref"].asUInt());
            }

            if (link.isMember("class"))
                edge->set_tag(link["class"].asString());
            else
                edge->set_tag("AT:IMG_LINK");

            cmds.push_back(cmd);
        }
    }

    if (aImg.isMember("collections")) {
        Json::Value collections = aImg["collections"];

        for (auto col : collections) {
            pmgd::protobufs::Command* cmd = new pmgd::protobufs::Command();
            cmd->set_cmd_id(pmgd::protobufs::Command::AddNode);
            cmd->set_cmd_grp_id(grp_id);

            pmgd::protobufs::AddNode* addn = cmd->mutable_add_node();

            uint32_t collection_id = STATIC_IDENTIFIER++;
            addn->set_identifier(collection_id);

            pmgd::protobufs::Node* node = addn->mutable_node();
            node->set_tag(ATHENA_COL_TAG);

            pmgd::protobufs::Property* p;
            p = node->add_properties();

            p->set_type(pmgd::protobufs::Property::StringType);
            p->set_key(ATHENA_COL_NAME_PROP);
            p->set_string_value(col.asString());

            pmgd::protobufs::QueryNode* qn = addn->mutable_query_node();
            qn->set_unique(true);
            qn->set_tag(ATHENA_COL_TAG);
            qn->set_p_op(pmgd::protobufs::And);

            pmgd::protobufs::PropertyPredicate* pps = qn->add_predicates();
            pps->set_op(pmgd::protobufs::PropertyPredicate::Eq);

            p = pps->mutable_v1();
            p->set_type(pmgd::protobufs::Property::StringType);
            p->set_key(ATHENA_COL_NAME_PROP);
            p->set_string_value(col.asString());

            cmds.push_back(cmd);

            // Add the edge

            pmgd::protobufs::Command* cmd_edge
                                    = new pmgd::protobufs::Command();
            cmd_edge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
            cmd_edge->set_cmd_grp_id(grp_id);

            pmgd::protobufs::AddEdge* adde = cmd_edge->mutable_add_edge();
            pmgd::protobufs::Edge* edge = adde->mutable_edge();
            edge->set_src(collection_id);
            edge->set_dst(id_node);
            edge->set_tag(ATHENA_COL_EDGE_TAG);

            cmds.push_back(cmd_edge);
        }
    }

    ch.tac();

    if (aImg.isMember("log") && aImg["log"].asBool()) {
        std::cout << file_name << std::endl;
        std::cout << "Timing: " << ch.getAvgTime_us() << ", "; // Total
        std::cout << ch_ops.getAvgTime_us() << ", "; // Total Ops
        std::cout << wr_ch.getAvgTime_us() << ", ";  // Total Write
        std::cout << ch_ops.getAvgTime_us()*100 /
                     ch.getAvgTime_us() << "\%" << std::endl; // % ops to total
    }
}

Json::Value AddImage::construct_responses(
    std::vector<pmgd::protobufs::CommandResponse *> &responses,
    Json::Value *json,
    protobufs::queryMessage &query_res)
{
    Json::Value ret;
    bool flag_error = false;
    Json::Value addImage;

    for(auto pmgd_res : responses) {
        // std::cout << "status: " << pmgd_res->error_code() << std::endl;
        // std::cout << "info: " << pmgd_res->error_msg() << std::endl;
        // std::cout << "pmgd_err_code: " << pmgd_res->error_code() << std::endl;
        if (pmgd_res->error_code() != pmgd::protobufs::CommandResponse::Success
            &&
            pmgd_res->error_code() != pmgd::protobufs::CommandResponse::Exists)
            {

            addImage["status"] = pmgd::protobufs::CommandResponse::Error;
            addImage["info"] = pmgd_res->error_msg();
            addImage["pmgd_err_code"] = pmgd_res->error_code();
            flag_error = true;
            break;
        }
    }
    // std::cout << "+++++++++++" << std::endl;

    if (!flag_error) {
        addImage["status"] = pmgd::protobufs::CommandResponse::Success;
    }

    ret["AddImage"] = addImage;

    return ret;
}

//========= FindImage definitions =========

int FindImage::construct_protobuf(
    std::vector<pmgd::protobufs::Command*> &cmds,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id)
{
    Json::Value find_img = jsoncmd["FindImage"];
    pmgd::protobufs::Command* cmdq = new pmgd::protobufs::Command();
    cmdq->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    cmdq->set_cmd_grp_id(grp_id);

    pmgd::protobufs::QueryNode* qn = cmdq->mutable_query_node();

    if (find_img.isMember("_ref"))
        qn->set_identifier(find_img["_ref"].asInt());
    else
        qn->set_identifier(-1);

    qn->set_tag(ATHENA_IM_TAG);

    if (find_img.isMember("unique"))
        qn->set_unique(find_img["unique"].asBool());

    if (find_img.isMember("link")) {
        add_link(find_img["link"], qn);
    }

    // We need the path for the image.
    std::string *r_img = qn->add_response_keys();
    *r_img = ATHENA_IM_PATH_PROP;

    // Will add the "name" property to the image
    // This is cover inside build_query_proto
    if (find_img.isMember("properties")) {
        Json::Value props = find_img["properties"];
        pmgd::protobufs::PropertyPredicate* pps = qn->add_predicates();

        for (auto m : props.getMemberNames()) {
            std::string *r_key = qn->add_response_keys();
            *r_key = m;
            pps->set_key(m);
            pps->set_op(pmgd::protobufs::PropertyPredicate::Eq);
            pmgd::protobufs::Property *p = new pmgd::protobufs::Property();
            set_property(p, m.c_str(), props[m]);
            pps->set_allocated_v1(p);
        }
    }

    if (find_img.isMember("collections")) {
        Json::Value collections = find_img["collections"];

        for (auto col : collections) {
            // Do stuff with the collections
            // Here we will need and/or etc.
        }
    }

    // Will overwrite whatever build_query_protobuf() sets,
    // TODO: CHECK HOW TO HANDLE THIS CASE BETTER FOR FINDIMAGE
    qn->set_r_type(pmgd::protobufs::List);

    build_query_protobuf(NULL, find_img, qn);

    cmds.push_back(cmdq);

    return 0;
}

Json::Value FindImage::construct_responses(
    std::vector<pmgd::protobufs::CommandResponse *> &responses,
    Json::Value *json,
    protobufs::queryMessage &query_res)
{
    Json::Value findImage;
    Json::Value props_array;
    Json::Value fImg = (*json)["FindImage"];
    Json::Value ret;

    bool flag_error = false;

    if (responses.size() == 0) {
        findImage["status"]  = pmgd::protobufs::CommandResponse::Error;
        findImage["info"] = "Not Found!";
        flag_error = true;
        ret["FindImage"] = findImage;
        return ret;
    }

    for (auto pmgd_res : responses) {

        if (pmgd_res->error_code() != 0) {
            findImage["status"] = pmgd::protobufs::CommandResponse::Error;
            findImage["info"] = pmgd_res->error_msg();
            findImage["pmgd_err_code"] = pmgd_res->error_code();

            flag_error = true;
            break;
        }

        // This list will be the one with the imgPath information
        if (pmgd_res->r_type() != pmgd::protobufs::List) {
            continue;
        }

        findImage = parse_response(pmgd_res);

        int matches = pmgd_res->op_int_value();

        for (auto key : pmgd_res->prop_values()) {
            assert(matches == key.second.values().size());
            break;
        }

        for (auto& ent : findImage["entities"]) {

            std::string im_path = ent[ATHENA_IM_PATH_PROP].asString();
            try {
                VCL::Image vclimg(im_path);

                if (fImg.isMember("operations")) {
                    run_operations(vclimg, fImg["operations"]);
                }

                std::vector<unsigned char> img_enc;
                img_enc = vclimg.get_encoded_image(VCL::PNG);
                if (!img_enc.empty()) {
                    std::string img_str((const char*)
                                        img_enc.data(),
                                        img_enc.size());

                    query_res.add_blobs(img_str);
                }
            } catch (VCL::Exception e) {
                print_exception(e);
                findImage["status"]  = pmgd::protobufs::CommandResponse::Success;
                findImage["info"] = "VCL Exception";
                flag_error = true;
                break;
            }
        }
    }

    if (!flag_error) {
        findImage["status"] = pmgd::protobufs::CommandResponse::Success;
    }

    // In case no properties asked by the user
    // TODO: This is more like a hack. I don;t like it
    bool empty_flag = false;

    for (auto& ent : findImage["entities"]) {
        ent.removeMember(ATHENA_IM_PATH_PROP);
        if (ent.getMemberNames().size() == 0) {
            empty_flag = true;
            break;
        }
    }

    if (empty_flag) {
        findImage.removeMember("entities");
    }

    ret["FindImage"] = findImage;

    return ret;
}
