#include <string>
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

using namespace athena;

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
    Json::StyledWriter sWriter;

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

        for (int j = 0; j < root.size(); j++) {

            std::string cmd = root[j].getMemberNames()[0];

            std::vector<pmgd::protobufs::CommandResponse *>& res =
                        pmgd_responses.at(j+1);

            json_responses.append( _rs_cmds[cmd]->construct_responses(
                                                    res,
                                                    &root[j],
                                                    proto_res) );
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
    std::string type = op["type"].asString();
    if (type == "threshold"){
        vclimg.threshold(op["value"].asInt());
    }
    if (type == "resize"){
        vclimg.resize(op["height"].asInt(),
                op["width" ].asInt());
    }
    if (type == "crop"){
        vclimg.crop(VCL::Rectangle (
                    op["x"].asInt(),
                    op["y"].asInt(),
                    op["height"].asInt(),
                    op["width" ].asInt()));
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

Json::Value RSCommand::parse_response(
                pmgd::protobufs::CommandResponse* response)
{
    Json::Value json_responses;
    long nodeid = response->op_int_value();

    switch (response->r_type()) {

        case pmgd::protobufs::NodeID:
            if (response->error_code()== pmgd::protobufs::CommandResponse::Success)
                json_responses["status"] = pmgd::protobufs::CommandResponse::Success;

            else if (response->error_code() == pmgd::protobufs::CommandResponse::Exists)
                json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            else {
                json_responses["status"] = response->error_code();
                json_responses["info"] = response->error_msg();
            }
            break;

        case pmgd::protobufs::EdgeID:
            if (response->error_code()== pmgd::protobufs::CommandResponse::Success) {
                 if (response->error_code() == pmgd::protobufs::CommandResponse::Exists)
                    json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
                else
                    json_responses["status"] = response->error_code();
            }
            else {
                json_responses["status"]=response->error_code();
                json_responses["info"] =response->error_msg();
            }
            break;

        case pmgd::protobufs::Cached:
            if (response->error_code()== pmgd::protobufs::CommandResponse::Success)
                json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            else {
                json_responses["status"]=response->error_code();
                json_responses["info"] =response->error_msg();
            }
            break;

        case pmgd::protobufs::List :
            if (response->error_code()== pmgd::protobufs::CommandResponse::Success) {
                int cnt;
                Json::Value list;
                Json::Value result;
                auto mymap = response->prop_values();

                int count=0;

                for (auto key:mymap) {
                    count=key.second.values().size();
                    break;
                }

                if (count > 0) {
                    for (int i=0; i<count; ++i) {
                        Json::Value prop;

                        for (auto key : mymap) {
                            pmgd::protobufs::PropertyList &p = key.second;
                            prop[key.first]=print_properties(key.first.c_str(), p.values(i));
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
        if (response->error_code()==pmgd::protobufs::CommandResponse::Success) {
            float average = response->op_float_value();
            json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            json_responses["average"]=double(average);
        }
        else {
            json_responses["status"]=response->error_code();
            json_responses["info"] =response->error_msg();
        }
        break;

    case pmgd::protobufs::Sum:

        if (response->error_code()==pmgd::protobufs::CommandResponse::Success) {
            Json::Int64  sum = response->op_int_value();
            json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            json_responses["sum"]= sum;
        }
        else {
            json_responses["status"]=response->error_code();
            json_responses["info"] =response->error_msg();
        }
        break;

    case pmgd::protobufs::Count:
        if (response->error_code()==pmgd::protobufs::CommandResponse::Success) {
            float count = response->op_int_value();
            json_responses["status"] = pmgd::protobufs::CommandResponse::Success;
            json_responses["count"] =float(count);
        }
        else {
            json_responses["status"]=response->error_code();
            json_responses["info"] =response->error_msg();
        }
        break;

    default:
           json_responses["status"]=response->error_code();
           json_responses["info"] =response->error_msg();
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

        FindEntity query; // to call the parse constraints block
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
        query.parse_query_constraints(json_node["constraints"], qn);

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

void FindEntity::set_operand(pmgd::protobufs::Property* p, Json::Value operand)
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

int FindEntity::parse_query_constraints(const Json::Value& predicates_array, pmgd::protobufs::QueryNode* query_node)
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

int FindEntity::get_response_type(const Json::Value& result_type_array,
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

int FindEntity::parse_query_results (const Json::Value& result_type,
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

int FindEntity::build_query_protobuf(pmgd::protobufs::Command*,
                                     const Json::Value& _query,
                                     pmgd::protobufs::QueryNode *query_node )
{
    query_node->set_p_op(pmgd::protobufs::And);
    parse_query_constraints(_query["constraints"], query_node);
    parse_query_results (_query["results"], query_node);
    return 0;
}


int FindEntity::construct_protobuf(
    std::vector<pmgd::protobufs::Command*> &cmds,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id)
{
    Json::Value _query = jsoncmd["FindEntity"];
    pmgd::protobufs::Command* cmdquery = new pmgd::protobufs::Command();
    cmdquery->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    cmdquery->set_cmd_grp_id(grp_id);

    pmgd::protobufs::QueryNode *qn = cmdquery->mutable_query_node();

    // TODO: This is momentarily fix, this should be fixed at PMGD Handler.
    // I think this was already fixed and nobody removed this.

    if (_query.isMember("_ref"))
        qn->set_identifier(_query["_ref"].asInt());
    else
        qn->set_identifier(-1);

    if (_query.isMember("class"))
        qn->set_tag(_query["class"].asCString());

    if (_query.isMember("unique"))
        qn->set_unique(_query["unique"].asBool());

    if (_query.isMember("link")) {
        pmgd::protobufs::LinkInfo *qnb = qn->mutable_link();
        Json::Value link = _query["link"];
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

    build_query_protobuf(cmdquery, _query, qn);

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

    // Adds AT:IMAGE node
    pmgd::protobufs::Node *n = an->mutable_node();
    n->set_tag(ATHENA_IM_TAG);

    // Will add the "name" property to the image
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

    if (aImg.isMember("operation")) {
        run_operations(vclimg, aImg["operation"]);
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

    ch.tac();

    Json::Value addImage;
    addImage["return"] = "success :)";
    addImage["name"] = aImg["name"].asString();

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
    Json::Value addImage;
    bool flag_error = false;
    for(auto pmgd_res : responses) {
        if (pmgd_res->error_code() != 0){
            addImage["return"] = "ERROR";
            addImage["code"] = pmgd_res->error_code();
            addImage["message"] = pmgd_res->error_msg();
            flag_error = true;
        }
    }

    if (!flag_error) {
        addImage["status"] = "Success";
    }

    Json::Value ret;
    ret["AddImage"] = addImage;

    return ret;
}

//========= FindImage definitions =========

int FindImage::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    Json::Value fImg = jsoncmd["FindImage"];
    // Create PMGD cmd for QueryNode
    pmgd::protobufs::Command* cmdq = new pmgd::protobufs::Command();
    cmdq->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    cmdq->set_cmd_grp_id(grp_id);

    pmgd::protobufs::QueryNode* qNode = new pmgd::protobufs::QueryNode();
    qNode->set_tag(ATHENA_IM_TAG);
    qNode->set_r_type(pmgd::protobufs::List);
    std::string *r_img = qNode->add_response_keys();
    *r_img = ATHENA_IM_PATH_PROP;

    // Will add the "name" property to the image
    if (fImg.isMember("properties")) {
        Json::Value props = fImg["properties"];
        pmgd::protobufs::PropertyPredicate* pps = qNode->add_predicates();

        for (auto m : props.getMemberNames()) {
            std::string * r_key = qNode->add_response_keys();
            *r_key = m;
            pps->set_key(m);
            pps->set_op(pmgd::protobufs::PropertyPredicate::Eq);
            pmgd::protobufs::Property *p = new pmgd::protobufs::Property();
            set_property(p, m.c_str(), props[m]);
            pps->set_allocated_v1(p);
        }
    }
    cmdq->set_allocated_query_node(qNode);
    cmds.push_back(cmdq);

    // if (fImg.isMember("link")) {
    //     Json::Value link = fImg["link"];

    //     pmgd::protobufs::Command* cmd = new pmgd::protobufs::Command();
    //     cmd->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    //     cmd->set_cmd_grp_id(grp_id);

    //     pmgd::protobufs::QueryNode* qNode = new pmgd::protobufs::QueryNode();
    //     std::string entity   = link["entity"].asString();
    //     qNode->set_tag(entity);

    //     pmgd::protobufs::PropertyPredicate *pps = qNode->add_predicates();

    //     std::string prop_id  = link["prop_id"].asString();
    //     pps->set_op(pmgd::protobufs::PropertyPredicate::Eq);
    //     pps->set_key(prop_id);

    //     // This is the same logic that Ragaa
    //     std::string prop_val = link["prop_value"].asString();
    //     pmgd::protobufs::Property* prop = new pmgd::protobufs::Property();
    //     Json::Value json_prop = link["prop_value"];
    //     set_property(prop, prop_id.c_str(), json_prop);

    //     pps->set_allocated_v1(prop);

    //     qNode->set_r_type(pmgd::protobufs::ResponseType::List);
    //     std::string* str_list = qNode->add_response_keys();
    //     *str_list = "imgPath";

    //     cmd->set_allocated_query_node(qNode);
    //     cmds.push_back(cmd);
    // }

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

    bool flag_error = false;

    for (auto pmgd_res : responses) {
        if (pmgd_res->error_code() != 0) {
            findImage["return"]  = "ERROR";
            findImage["message"] = pmgd_res->error_msg();

            flag_error = true;
            break;
        }

        if (pmgd_res->r_type() != pmgd::protobufs::List) {
            continue;
        }

        int matches = pmgd_res->op_int_value();

        for (auto key : pmgd_res->prop_values()) {
            assert(matches == key.second.values().size());
            break;
        }

        if (matches > 0) {

            for (int i = 0; i < matches; ++i) {
                Json::Value prop;
                for (auto key : pmgd_res->prop_values()) {
                    // Check Type, wait for Ragaad's code
                    prop[key.first] = key.second.values(i).string_value();

                    if (key.first == ATHENA_IM_PATH_PROP) {

                        std::string im_path = prop[key.first].asString();
                        try {
                            VCL::Image vclimg(im_path);

                            if (fImg.isMember("operation")) {
                                run_operations(vclimg, fImg["operation"]);
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
                            findImage["return"]  = "ERROR";
                            findImage["message"] = "Not Found!";
                            flag_error = true;
                            break;
                        }
                    }
                }
                props_array.append(prop);
            }
        }
        else {
            findImage["return"]  = "ERROR";
            findImage["message"] = "Not Found!";
            flag_error = true;
            break;
        }
    }

    if (!flag_error) {
        findImage["return"] = "Success";
        findImage["properties"] = props_array;
    }

    Json::Value ret;
    ret["FindImage"] = findImage;

    return ret;
}
