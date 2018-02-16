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
    _rs_cmds["Connect"]  = new AddConnection();
    _rs_cmds["AddImage"] = new AddImage();
    _rs_cmds["FindEntity"] = new FindEntity();
}

void RSCommand::set_property(pmgd::protobufs::Property *p,
        const char * key , Json::Value val) {

    if (val.isObject()) {
        if (val.isMember("_date")) {
            p->set_type(pmgd::protobufs::Property::TimeType);
            p->set_key(key);
            p->set_string_value(val["_date"].asString());

        }
        if (val.isMember("_blob")){ //the blob value is read and stored as a string otherwose it is not shown in the graph.
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

//addNode command constructor
int AddEntity::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    pmgd::protobufs::Command* cmdadd = new  pmgd::protobufs::Command();

    Json::Value aNode = jsoncmd["AddEntity"];

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_cmd_grp_id(grp_id);

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_cmd_grp_id(grp_id);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();

    if (aNode.isMember("_ref"))
        an->set_identifier( aNode["_ref"].asUInt());
    pmgd::protobufs::Node *n = an->mutable_node();

    if (aNode.isMember("class")){
        n->set_tag(aNode["class"].asCString());
    }
    //iterate over the properties of nodes
    if (aNode.isMember("properties")) {
        const Json::Value node_properties = aNode["properties"]; //take the sub-object of the properties

        for (Json::ValueConstIterator itr = node_properties.begin() ; itr != node_properties.end() ; itr++ ) {
            pmgd::protobufs::Property *p = n->add_properties();
            // Checking the properties
            set_property(p, itr.key().asCString(), *itr);

        } //nodes properties
    }

    cmds.push_back(cmdadd);
    return 1;
}

int AddConnection::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    Json::Value aEdge = jsoncmd["Connect"];

    pmgd::protobufs::Command* cmdedge = new pmgd::protobufs::Command();
    cmdedge->set_cmd_grp_id(grp_id);
    cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();

    if (aEdge.isMember("_ref"))
        ae->set_identifier(aEdge["_ref"].asUInt());

    pmgd::protobufs::Edge *e = ae->mutable_edge();

    if (aEdge.isMember("ref1"))
        e->set_src(aEdge["ref1"].asInt() );
    if (aEdge.isMember("ref2"))
        e->set_dst(aEdge["ref2"].asUInt());
    if (aEdge.isMember("class"))
        e->set_tag(aEdge["class"].asCString());

    if (aEdge.isMember("properties")) {
        const Json::Value edge_properties = aEdge["properties"]; //take the sub-object of the properties

        for (Json::ValueConstIterator itr = edge_properties.begin() ;
                itr != edge_properties.end() ; itr++ ) {
            pmgd::protobufs::Property *p = e->add_properties();
            // Checking the properties
            set_property(p, itr.key().asCString(), *itr);
        } //nodes properties
    }
    cmds.push_back(cmdedge);
    return 1;
}

int FindEntity::build_Query_Node_protobuf ( pmgd::protobufs::Command* cmdquery , const Json::Value& aQuery)
{
    pmgd::protobufs::QueryNode *qn = cmdquery->mutable_query_node();

    // TODO: This is momentarily fix, this should be fixed at PMGD Handler.

    if (aQuery.isMember("_ref"))
        qn->set_identifier(aQuery["_ref"].asInt());
    else
        qn->set_identifier(-1);

    if (aQuery.isMember("class"))
        qn->set_tag(aQuery["class"].asCString());


        build_query_protobuf<pmgd::protobufs::QueryNode>(cmdquery, aQuery, qn);

 std::cout<<"New Implementation of QueryNode :^_^"<<std::endl;

    return 1;


   }
   template <class T> int FindEntity::parse_query_constraints(const Json::Value& aQuery, T* queryType){

     const Json::Value Predicates_array = aQuery;

    for (auto itr = Predicates_array.begin() ;
        itr != Predicates_array.end() ; itr++ ) {

    pmgd::protobufs::PropertyPredicate *pp = queryType->add_predicates();
    Json::Value predicate =*itr;

    std::string predicate_key=itr.key().asCString();
    pp->set_key(predicate_key);  //assign the property predicate key
    std::vector<std::string> operators;
    std::vector<Json::Value> Operands;

    pmgd::protobufs::Property *p1;
    pmgd::protobufs::Property *p2 ;

    for (int k=0; k<predicate.size(); k++) { //iterate over the key elements

        if ((predicate[k] == ">=")
                ||(predicate[k] =="<=")
                ||(predicate[k]=="==")
                ||(predicate[k]=="!=")
                ||(predicate[k]== "<")
                ||(predicate[k]== ">")
          )
            operators.push_back(predicate[k].asCString());
        else
            Operands.push_back(predicate[k]);
    }
    if ( operators.size() > 1 ) {

        if (operators[0 ] ==">" && operators[1] == "<")
            pp->set_op(pmgd::protobufs::PropertyPredicate::GtLt);
        else if (operators[0] == ">=" && operators[1] == "<")
            pp->set_op(pmgd::protobufs::PropertyPredicate::GeLt);
        else if (operators[0] == ">" && operators[1] == "<=")
            pp->set_op(pmgd::protobufs::PropertyPredicate::GtLe);
        else if (operators[0] == ">=" && operators[1] == "<=")
            pp->set_op(pmgd::protobufs::PropertyPredicate::GeLe);
    } //if two operations are involved

    else if (operators.size() == 1 ) {
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

    if (Operands.size() > 1) {//two operands per operator

        p1 = pp->mutable_v1();
        p2 = pp->mutable_v2();
        p1->set_key(predicate_key);
        p2->set_key(predicate_key);

        if (Operands[0].isInt() && Operands[1].isInt()) {
            p1->set_type(pmgd::protobufs::Property::IntegerType);
            p1->set_int_value((Operands[0].asInt()));
            p2->set_type(pmgd::protobufs::Property::IntegerType);
            p2->set_int_value((Operands[1].asInt()));
        }
        else if (Operands[0].isDouble() && Operands[1].isDouble()) {
            p1->set_type(pmgd::protobufs::Property::FloatType);
            p1->set_float_value((Operands[0].asDouble()));
            p2->set_type(pmgd::protobufs::Property::FloatType);
            p2->set_float_value((Operands[1].asDouble()));
        }
        else  if (Operands[0].isBool() && Operands[1].isBool()) {
            p1->set_type(pmgd::protobufs::Property::BooleanType);
            p1->set_bool_value((Operands[0].asBool()));
            p2->set_type(pmgd::protobufs::Property::BooleanType);
            p2->set_bool_value((Operands[1].asBool()));

        }
        else  if (Operands[0].isString() && Operands[1].isString()) {
            p1->set_type(pmgd::protobufs::Property::StringType);
            p1->set_string_value((Operands[0].asString()));
            p2->set_type(pmgd::protobufs::Property::BooleanType);
            p2->set_string_value((Operands[1].asString()));
        }
    }
    else if (Operands.size() == 1) {

        p1 = pp->mutable_v1();
        p1->set_key(predicate_key);

        if (Operands[0].isInt()) {
            p1->set_type(pmgd::protobufs::Property::IntegerType);
            p1->set_int_value((Operands[0].asInt()));
        }
        else  if (Operands[0].isBool()) {
            p1->set_type(pmgd::protobufs::Property::BooleanType);
            p1->set_bool_value((Operands[0].asBool()));
        }
        else if (Operands[0].isDouble()) {
            p1->set_type(pmgd::protobufs::Property::FloatType);
            p1->set_float_value((Operands[0].asDouble()));
        }
        else  if (Operands[0].isString()) {
            p1->set_type(pmgd::protobufs::Property::StringType);
            p1->set_string_value((Operands[0].asString()));
        }
    }
    else {
        p1->set_type(pmgd::protobufs::Property::NoValueType);
        p2->set_type(pmgd::protobufs::Property::NoValueType);
    }

    }

   return 1;
}



 template < class T > int  FindEntity::get_response_type( const Json::Value& result_type_array, std::string response, T* queryType) {

    for (auto response_key=0; response_key!=result_type_array[response].size(); response_key++){

        std::string *r_key= queryType->add_response_keys();

        *r_key = result_type_array[response][response_key].asString();

       }
return 1;
 }

template <class T> int FindEntity::parse_query_results (const Json::Value& result_type, T* queryType){

for (auto response_type =result_type.begin(); response_type!=result_type.end();response_type++)
{

    if (response_type.key().asString() == "list") {

        queryType->set_r_type(pmgd::protobufs::List);

        get_response_type <T> (result_type, response_type.key().asString(), queryType);

    }

    if (response_type.key().asString() == "count") {
        queryType->set_r_type(pmgd::protobufs::Count);

         get_response_type <T> (result_type, response_type.key().asString(), queryType);
    }

    if (response_type.key().asString() == "sum") {
        queryType->set_r_type(pmgd::protobufs::Sum);

         get_response_type <T> (result_type, response_type.key().asString(), queryType);

    }

    if (response_type.key().asString() == "average") {

        queryType->set_r_type(pmgd::protobufs::Average);

        get_response_type <T> (result_type, response_type.key().asString(), queryType);
    }

    if (response_type.key().asString() == "EntityID")
        queryType->set_r_type(pmgd::protobufs::NodeID);
    if (response_type.key().asString() == "ConnectionID")
        queryType->set_r_type(pmgd::protobufs::EdgeID);
}


}
template <class T > int FindEntity::build_query_protobuf(pmgd::protobufs::Command* ,
                                                       const Json::Value& aQuery ,
                                                       T* queryType )
{

    queryType->set_p_op(pmgd::protobufs::And);

    parse_query_constraints <T> (aQuery["constraints"], queryType);

    parse_query_results <T> (aQuery["results"], queryType);

return 1;


}

int FindEntity::build_Query_Neighbor_Node_protobuf (pmgd::protobufs::Command* cmdquery, const Json::Value& aQuery)
{
    pmgd::protobufs::QueryNeighbor *qnb = cmdquery->mutable_query_neighbor();
    pmgd::protobufs::QueryNode *qn = qnb->mutable_query_start_node();

     Json::Value link = aQuery["link"];
     if(link.isMember("ref")){
        qnb->set_start_identifier(link["ref"].asInt());

     }

    if(link.isMember("class"))
        qn->set_tag(link["class"].asCString());

    if( link.isMember("direction") ){
        if (link["direction"]=="out")
            qnb->set_dir(pmgd::protobufs::QueryNeighbor::Outgoing);

            else if( link["direction"]=="in" )
                qnb->set_dir(pmgd::protobufs::QueryNeighbor::Incoming);

            else if( link["direction"]=="any" )
                qnb->set_dir(pmgd::protobufs::QueryNeighbor::Any);
       }

    if (aQuery.isMember("_ref"))
        qn->set_identifier(aQuery["_ref"].asInt());
    else
        qn->set_identifier(-1);

    if (aQuery.isMember("class"))
        qnb->set_n_tag(aQuery["class"].asCString());

    if (aQuery.isMember("unique"))
      qnb->set_unique(aQuery["unique"].asBool());

     build_query_protobuf<pmgd::protobufs::QueryNeighbor>(cmdquery, aQuery, qnb);


return 1;
}


int FindEntity::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id)
{
    Json::Value aQuery = jsoncmd["FindEntity"];
    pmgd::protobufs::Command* cmdquery =new pmgd::protobufs::Command();
    cmdquery->set_cmd_grp_id(grp_id);

    if (aQuery.isMember("link")) {
        cmdquery->set_cmd_id(pmgd::protobufs::Command::QueryNeighbor);
        build_Query_Neighbor_Node_protobuf(cmdquery, aQuery);
    }
    else {
        cmdquery->set_cmd_id(pmgd::protobufs::Command::QueryNode);
        build_Query_Node_protobuf(cmdquery, aQuery);
    }

    cmds.push_back(cmdquery);

    return 1;
}

FindImage::FindImage()
{
}

Json::Value FindImage::construct_responses( std::vector<pmgd::protobufs::CommandResponse *>& response, Json::Value* query)
{
    Json::Value findImage;
    findImage["return"] = "success :)";

    return findImage;
}

int FindImage::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    if (jsoncmd.isMember("link")) {
        Json::Value link = jsoncmd["link"];

        pmgd::protobufs::Command* cmd = new pmgd::protobufs::Command();
        cmd->set_cmd_id(pmgd::protobufs::Command::QueryNode);
        cmd->set_cmd_grp_id(grp_id);

        pmgd::protobufs::QueryNode* qNode = new pmgd::protobufs::QueryNode();
        std::string entity   = link["entity"].asString();
        qNode->set_tag(entity);

        pmgd::protobufs::PropertyPredicate *pps =
                                qNode->add_predicates();

        std::string prop_id  = link["prop_id"].asString();
        pps->set_op(pmgd::protobufs::PropertyPredicate::Eq);
        pps->set_key(prop_id);

        // This is the same logic that Ragaa
        std::string prop_val = link["prop_value"].asString();
        pmgd::protobufs::Property* prop = new pmgd::protobufs::Property();
        Json::Value json_prop = link["prop_value"];
        set_property(prop, prop_id.c_str(), json_prop);

        pps->set_allocated_v1(prop);

        qNode->set_r_type(pmgd::protobufs::ResponseType::List);
        std::string* str_list = qNode->add_response_keys();
        *str_list = "imgPath";

        cmd->set_allocated_query_node(qNode);
        cmds.push_back(cmd);
    }

    return 0;
}

AddImage::AddImage()
{
    _storage_tdb = AthenaConfig::instance()
                ->get_string_value("tiledb_database", DEFAULT_TDB_PATH);
    _storage_png = AthenaConfig::instance()
                ->get_string_value("png_database", DEFAULT_PNG_PATH);
}


Json::Value AddImage::construct_responses(std::vector<pmgd::protobufs::CommandResponse *>& cmds, Json::Value* query)
{
    for(auto pmgd_res : cmds) {
        if (pmgd_res->error_code() != 0){
            Json::Value err;
            err["return"] = "err :(";

            return err;
        }
    }

    Json::Value addImage;
    addImage["return"] = "success :)";

    return addImage;
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
    if (aImg.isMember("name")) {
        std::string im_name = aImg["name"].asString();

        pmgd::protobufs::Property *p = n->add_properties();
        p->set_type(pmgd::protobufs::Property::StringType);
        p->set_key(ATHENA_IM_NAME_PROP);
        p->set_string_value(im_name);
    }

    ChronoCpu ch_ops("addImage");
    ch_ops.tic();

    VCL::Image vclimg((void*)blob.data(), blob.size());

    if (aImg.isMember("operation")) {
        (vclimg, aImg["operation"]);
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

    if (aImg.isMember("log")) {
        std::cout << file_name << std::endl;
        std::cout << "Timing: " << ch.getAvgTime_us() << ", "; // Total
        std::cout << ch_ops.getAvgTime_us() << ", "; // Total Ops
        std::cout << wr_ch.getAvgTime_us() << ", ";  // Total Write

        std::cout << ch_ops.getAvgTime_us()*100 /
                     ch.getAvgTime_us() << "\%" << std::endl; // % ops to total
    }
}

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

Json::Value print_properties(const std::string &key,
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

Json::Value AddEntity::construct_responses( std::vector<pmgd::protobufs::CommandResponse *>& response, Json::Value* query)
{
    Json::Value addEntity;
    for (auto it : response) {
        addEntity[query->getMemberNames()[0]] = parse_response(it);
   }
   return addEntity;
}

Json::Value AddConnection::construct_responses( std::vector<pmgd::protobufs::CommandResponse*>&  response, Json::Value* query)
{
    Json::Value addEdge;
    for (auto it : response) {
        addEdge[query->getMemberNames()[0]] = parse_response(it);
    }
   return addEdge;
}

Json::Value FindEntity::construct_responses( std::vector<pmgd::protobufs::CommandResponse *>& response, Json::Value* query)
{
    Json::Value findEntity;

    for (auto it : response) {
        findEntity[query->getMemberNames()[0]] = parse_response(it);
    }
    return findEntity;
}

Json::Value  RSCommand::parse_response
        (pmgd::protobufs::CommandResponse* response)
{
    Json::Value json_responses;
    if (response->r_type() == pmgd::protobufs::NodeID) {

        long nodeid = response->op_int_value();

        if (response->error_code()!=pmgd::protobufs::CommandResponse::Success) {
            json_responses["Status"] = response->error_msg();
        }
       else {
            json_responses["Status"] = "success";
        }

    }

    else if (response->r_type() == pmgd::protobufs::EdgeID)
    {

        long edgeid = response->op_int_value();
        if (response->error_code()!=pmgd::protobufs::CommandResponse::Success) {

            json_responses["Status"] = response->error_msg();
        }
        else
            json_responses["Status"] = "success";

    }

    else if (response->r_type() == pmgd::protobufs::List) {
         int cnt;

        Json::Value list;
        Json::Value result;
        auto mymap = response->prop_values();

        int count=0;

        for (auto key:mymap) {
            count=key.second.values().size();
            break;
        }

        if (count >0) {

            for (int i=0; i<count; ++i) {

            Json::Value prop;

            for (auto key : mymap) {
                pmgd::protobufs::PropertyList &p = key.second;
                prop[key.first]=print_properties(key.first.c_str(), p.values(i));
            }

            list.append(prop);
          }
        json_responses["Status"] = "success";
        json_responses["list"] = list;
        list.clear();
       }
    }

    else if (response->r_type() == pmgd::protobufs::Average) {

            float average = response->op_float_value();
            json_responses["Status"] = "success";
            json_responses["average"]=double(average);

    }
    else if (response->r_type() == pmgd::protobufs::Sum) {

             float sum = response->op_float_value();
             json_responses["Status"]="success";
             json_responses["sum"]= float(sum);

    }
    else if (response->r_type() == pmgd::protobufs::Count) {

            float count = response->op_int_value();
            json_responses["Status"]="success";
            json_responses["count"] =float(count);

    }
return  json_responses;
}



void QueryHandler::process_query (protobufs::queryMessage proto_query,
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
            std::cout << "Error parsing!" << std::endl;
            Json::Value error;
            error["return"] = "Server error - parsing";
            json_responses.append(error);
        }

        //defien a vector of commands
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
         Json::Value json_responses_root;
        // execute the queries using the PMGDQueryHandler object

        std::vector<std::vector<pmgd::protobufs::CommandResponse *>>
            pmgd_responses = _pmgd_qh.process_queries(cmds, group_count + 1);

        for (int j = 0; j < root.size(); j++) {
            std::vector<pmgd::protobufs::CommandResponse *> response = pmgd_responses[j+1];//j+1 to execulde the TXBegin command
            Json::Value* query = &root[j];
            std::string cmd = query->getMemberNames()[0];

            json_responses.append( _rs_cmds[cmd]->construct_responses(response, query));
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
