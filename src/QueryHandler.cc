#include <string>
#include "QueryHandler.h"
#include "chrono/Chrono.h"

#include "jarvis.h"
#include "util.h"

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

#define ATHENA_IM_TAG           "AT:IMAGE"
#define ATHENA_IM_NAME_PROP     "name"
#define ATHENA_IM_PATH_PROP     "imgPath"
#define ATHENA_STORAGE_PATH_TDB "/ssd_400/imagestdb/hls/"
#define ATHENA_STORAGE_PATH_PNG "/ssd_400/png/"
// #define ATHENA_STORAGE_PATH_PNG "/mnt/optane/png/"

using namespace athena;

QueryHandler::QueryHandler(Jarvis::Graph *db, std::mutex *mtx)
    : _pmgd_qh(db, mtx)
{
    _rs_cmds["AddEntity"]  = new AddNode();
    _rs_cmds["Connect"]  = new AddEdge();
    _rs_cmds["AddImage"] = new AddImage();
    _rs_cmds["FindEntity"] = new QueryNode();

}

void RSCommand::set_property(pmgd::protobufs::Property *p,
        const char * key , Json::Value val){

    if (val.isObject()){
        if(val.isMember("_date")){
            p->set_type(pmgd::protobufs::Property::TimeType);
            p->set_key(key);
            p->set_string_value(val["_date"].asString());

        }
        if(val.isMember("_blob")){ //the blob value is read and stored as a string otherwose it is not shown in the graph.
            p->set_type(pmgd::protobufs::Property::StringType);
            p->set_key(key);
            p->set_string_value(val["_blob"].asString());
        }
    }
    else if (val.isString()){
        p->set_type(pmgd::protobufs::Property::StringType);
        p->set_key(key);
        p->set_string_value(val.asString());
    }
    else if( ((val).isInt() )){
        p->set_type(pmgd::protobufs::Property::IntegerType);
        p->set_key(key);
        p->set_int_value(val.asInt());
    }
    else if( ((val).isBool() )){
        p->set_type(pmgd::protobufs::Property::BooleanType);
        p->set_key(key);
        p->set_bool_value(val.asBool());
    }
    else if( ((val).isDouble() )){
        p->set_type(pmgd::protobufs::Property::FloatType);
        p->set_key(key);
        p->set_float_value(val.asDouble());
    }
}

//addNode command constructor
int AddNode::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
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

    if(aNode.isMember("_ref"))
        an->set_identifier(aNode["_ref"].asUInt());
    pmgd::protobufs::Node *n = an->mutable_node();

    if (aNode.isMember("class")){
        n->set_tag(aNode["class"].asCString());
    }
    //iterate over the properties of nodes
    if (aNode.isMember("properties")){
        const Json::Value node_properties = aNode["properties"]; //take the sub-object of the properties

        for(  Json::ValueConstIterator itr = node_properties.begin() ; itr != node_properties.end() ; itr++ ){
            pmgd::protobufs::Property *p = n->add_properties();
            // Checking the properties
            set_property(p, itr.key().asCString(), *itr);

        } //nodes properties
    }

    cmds.push_back(cmdadd);
    return 1;
}

int AddEdge::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    Json::Value aEdge = jsoncmd["Connect"];

    pmgd::protobufs::Command* cmdedge =new pmgd::protobufs::Command();
    cmdedge->set_cmd_grp_id(grp_id);
    cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();

    if(aEdge.isMember("_ref"))
        ae->set_identifier(aEdge["_ref"].asUInt());

    pmgd::protobufs::Edge *e = ae->mutable_edge();

    if(aEdge.isMember("ref1"))
        e->set_src(aEdge["ref1"].asInt() );
    if(aEdge.isMember("ref2"))
        e->set_dst(aEdge["ref2"].asUInt());
    if(aEdge.isMember("class"))
        e->set_tag(aEdge["class"].asCString());

    if (aEdge.isMember("properties")){
        const Json::Value edge_properties = aEdge["properties"]; //take the sub-object of the properties

        for( Json::ValueConstIterator itr = edge_properties.begin() ;
                itr != edge_properties.end() ; itr++ ) {
            pmgd::protobufs::Property *p = e->add_properties();
            // Checking the properties
            set_property(p, itr.key().asCString(), *itr);
        } //nodes properties
    }
    cmds.push_back(cmdedge);
    return 1;
}

int QueryNode::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id){
        Json::Value aQuery = jsoncmd["FindEntity"];
    pmgd::protobufs::Command* cmdquery =new pmgd::protobufs::Command();
    cmdquery->set_cmd_id(pmgd::protobufs::Command::QueryNode);
    cmdquery->set_cmd_grp_id(grp_id);
    pmgd::protobufs::QueryNode *qn = cmdquery->mutable_query_node();

    if(aQuery.isMember("_ref"))
        qn->set_identifier(aQuery["_ref"].asUInt());
    if(aQuery.isMember("class"))
        qn->set_tag(aQuery["class"].asCString());
    qn->set_p_op(pmgd::protobufs::And);
    const Json::Value Predicates_array = aQuery["constraints"];

    for( Json::ValueConstIterator itr = Predicates_array.begin() ;
            itr != Predicates_array.end() ; itr++ ) {

        pmgd::protobufs::PropertyPredicate *pp = qn->add_predicates();
        Json::Value predicate =*itr;

        std::string predicate_key=itr.key().asCString();
        pp->set_key(predicate_key);  //assign the property predicate key
        std::vector<std::string> operators;
        std::vector<Json::Value> Operands;
        pmgd::protobufs::Property *p1;
        pmgd::protobufs::Property *p2 ;

        for(int k=0; k<predicate.size();k++) //iterate over the key elements
        {

            if((predicate[k]==">=")
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
        if(operators.size()>1){

            if(operators[0]==">" && operators[1]=="<")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GtLt);
            else if(operators[0]==">=" && operators[1]=="<")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GeLt);
            else if(operators[0]==">" && operators[1]=="<=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GtLe);
            else if(operators[0]==">=" && operators[1]=="<=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::GeLe);

        } //if two operations are involved
        else if(operators.size()==1){
            if(operators[0]==">")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Gt);
            else if(operators[0] ==">=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Ge);
            else if(operators[0] =="<")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Lt);
            else if(operators[0] =="<=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Le);
            else if(operators[0] =="==")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Eq);
            else if(operators[0] =="!=")
                pp->set_op(pmgd::protobufs::PropertyPredicate::Ne);

        }


        if(Operands.size()>1)//two operand per operator
        {   p1 = pp->mutable_v1();
            p2 = pp->mutable_v2();
            p1->set_key(predicate_key);

            p2->set_key(predicate_key);
            if(Operands[0].isInt() && Operands[1].isInt()){
                p1->set_type(pmgd::protobufs::Property::IntegerType);
                p1->set_int_value((Operands[0].asInt()));
                p2->set_type(pmgd::protobufs::Property::IntegerType);
                p2->set_int_value((Operands[1].asInt()));

            }
            else if(Operands[0].isDouble() && Operands[1].isDouble()){
                p1->set_type(pmgd::protobufs::Property::FloatType);
                p1->set_float_value((Operands[0].asDouble()));
                p2->set_type(pmgd::protobufs::Property::FloatType);
                p2->set_float_value((Operands[1].asDouble()));

            }
            else  if(Operands[0].isBool() && Operands[1].isBool()){
                p1->set_type(pmgd::protobufs::Property::BooleanType);
                p1->set_bool_value((Operands[0].asBool()));
                p2->set_type(pmgd::protobufs::Property::BooleanType);
                p2->set_bool_value((Operands[1].asBool()));

            }

            else  if(Operands[0].isString() && Operands[1].isString()){
                p1->set_type(pmgd::protobufs::Property::StringType);
                p1->set_string_value((Operands[0].asString()));
                p2->set_type(pmgd::protobufs::Property::BooleanType);
                p2->set_string_value((Operands[1].asString()));

            }
        }
        else if(Operands.size()==1)
        {    p1 = pp->mutable_v1();
            p1->set_key(predicate_key);


            if(Operands[0].isInt() ){
                p1->set_type(pmgd::protobufs::Property::IntegerType);
                p1->set_int_value((Operands[0].asInt()));
            }
            else  if(Operands[0].isBool() ){
                p1->set_type(pmgd::protobufs::Property::BooleanType);
                p1->set_bool_value((Operands[0].asBool()));
            }
            else if(Operands[0].isDouble() ){
                p1->set_type(pmgd::protobufs::Property::FloatType);
                p1->set_float_value((Operands[0].asDouble()));
            }
            else  if(Operands[0].isString()){
                p1->set_type(pmgd::protobufs::Property::StringType);
                p1->set_string_value((Operands[0].asString()));
            }
        }

        else{
            p1->set_type(pmgd::protobufs::Property::NoValueType);
            p2->set_type(pmgd::protobufs::Property::NoValueType);
        }

    }

    Json::Value type = aQuery["results"];
    for(auto response_type =type.begin(); response_type!=type.end();response_type++){

        if(response_type.key().asString()=="list")
        {
            qn->set_r_type(pmgd::protobufs::List);
            for(auto response_key=0; response_key!=type[response_type.key().asString()].size(); response_key++){

                std::string *r_key= qn->add_response_keys();
                *r_key = type[response_type.key().asString()][response_key].asString();
            }

        }
        if(response_type.key().asString()=="count"){
            qn->set_r_type(pmgd::protobufs::Count);
            for(auto response_key=0; response_key!=type[response_type.key().asString()].size(); response_key++){
                std::string *r_key= qn->add_response_keys();
                *r_key = type[response_type.key().asString()][response_key].asString();
            }

        }
        if(response_type.key().asString()=="sum"){

            for(auto response_key=0; response_key!=type[response_type.key().asString()].size(); response_key++){

                std::string *r_key= qn->add_response_keys();
                *r_key = type[response_type.key().asString()][response_key].asString();
            }
        qn->set_r_type(pmgd::protobufs::Sum);
        }
        if(response_type.key().asString()=="average"){

            qn->set_r_type(pmgd::protobufs::Average);
            for(auto response_key=0; response_key!=type[response_type.key().asString()].size(); response_key++)
            {

                std::string *r_key= qn->add_response_keys();
                *r_key = type[response_type.key().asString()][response_key].asString();
            }

        }
        if(response_type.key().asString()=="EntityID")
            qn->set_r_type(pmgd::protobufs::NodeID);
        if(response_type.key().asString()=="ConnectionID")
            qn->set_r_type(pmgd::protobufs::EdgeID);

    }
    cmds.push_back(cmdquery);
    return 1;

}

int AddImage::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id)
{
    Json::Value aImg = jsoncmd["AddImage"];
    Json::Value res;

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

    // TODO: IMPLEMENT LOGIC FOR CATEGORY
    // if (aImg.isMember("category")) {
    //     // will get the JL node for that category.
    //     // here we check if the category is defined,
    //     // if not, we create a new category.
    //     // std::cout << aImg["category"]["name"].asString()
    //     //           << std::endl;

    //     std::string cat = aImg["category"]["name"].asString();

    //     Jarvis::NodeIterator i = demodb.db()->get_nodes(cat.c_str());

    //     if (i) {
    //         // ChronoCpu addEdge("addEdge");
    //         // addEdge.tic();
    //         Jarvis::Edge &e = demodb.db()->add_edge(im_node, *i,
    //                         "AT:IMAGE_EDGE");
    //         // addEdge.tac();
    //         // addEdge.printLastTime_us();
    //     }
    //     else {
    //         // ChronoCpu addNode("addNode");
    //         // addNode.tic();
    //         Jarvis::Node &cat_node = demodb.db()->add_node(cat.c_str());
    //         // addNode.tac();
    //         // addNode.printLastTime_ms();

    //         std::cout <<  " { \"warning\": \"No such category, adding\"}\n";
    //         Jarvis::Edge &e = demodb.db()->add_edge(im_node, cat_node,
    //                         "AT:IMAGE_EDGE");
    //     }
    // }


    // TODO: IMPLEMENT LOGIC FOR LINK
    // if (aImg.isMember("link")) {
    //     Json::Value link = aImg["link"];
    //     // will get the JL node for that the entity,
    //     // which can be a generic entity.
    //     // For the HLS case, this entity can be a Patient.
    //     // Some unique ID must be given in that case.

    //     std::string entity   = link["entity"].asString();
    //     std::string prop_id  = link["prop_id"].asString();
    //     std::string prop_val = link["prop_value"].asString();

    //     // Here we need a swtich based on the type? absolutely horrible

    //     Jarvis::PropertyPredicate pps1(prop_id.c_str(),
    //                                 Jarvis::PropertyPredicate::Eq,
    //                                 prop_val);

    //     Jarvis::NodeIterator i = demodb.db()->get_nodes(entity.c_str(),
    //                                                    pps1);

    //     if (i) {
    //         Jarvis::Edge &e = demodb.db()->add_edge(im_node, *i, "AT:IMAGE_EDGE");
    //     }
    //     else {
    //         Json::Value error;
    //         error["addImage"] = "error: No such entity";
    //         std::cout << "No such entity: " << prop_val << std::endl;
    //         response.append(error);
    //         return;
    //     }
    // }

    ChronoCpu ch_ops("addImage");
    ch_ops.tic();

    VCL::Image vclimg((void*)blob.data(), blob.size());

    if (aImg.isMember("operation")) {
        (vclimg, aImg["operation"]);
    }

    ChronoCpu write_time("write_time");

    std::string img_root = ATHENA_STORAGE_PATH_TDB;
    VCL::ImageFormat vcl_format = VCL::TDB;

    if (aImg.isMember("format")) {
        std::string format = aImg["format"].asString();

        if (format == "png") {
            vcl_format = VCL::PNG;
            img_root = ATHENA_STORAGE_PATH_PNG;
        }
        else if (format == "tdb") {
            vcl_format = VCL::TDB;
            img_root = ATHENA_STORAGE_PATH_TDB;
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

    write_time.tic();
    vclimg.store(file_name, vcl_format);
    write_time.tac();

    ch_ops.tac();

    cmds.push_back(cmdadd);

    ch.tac();

    Json::Value addImage;
    addImage["return"] = "success :)";
    addImage["name"] = aImg["name"].asString();

    // if (aImg.isMember("timing")) {
    //     Json::Value timing;
    //     timing["addImage[us]"]  = ch.getAvgTime_us();
    //     timing["imageTotal[%]"] = ch_ops.getAvgTime_us()*100 /
    //                               ch.getAvgTime_us();
    //     timing["imageTotal[us]"] = ch_ops.getAvgTime_us();
    //     timing["write_image[us]"] = write_time.getAvgTime_us();

    //     addImage["timing"] = timing;
    // }

    // res["addImage"] = addImage;
    // response.append(res);
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

std::string print_properties( const std::string &key, const pmgd::protobufs::Property &p)
{
     std::string s;
     std::string s1=key;
     std::string s2;
    switch(p.type()) {
        case pmgd::protobufs::Property::BooleanType:
            //printf("key: %s, value: %d\n", key.c_str(), p.bool_value());
                s2=std::to_string(p.bool_value());


            break;

        case pmgd::protobufs::Property::IntegerType:
            //printf("key: %s, value: %ld\n", key.c_str(), p.int_value());
            s2=std::to_string(p.int_value());


            break;
        case pmgd::protobufs::Property::StringType:
        case pmgd::protobufs::Property::TimeType:
            //printf("key: %s, value: %s\n", key.c_str(), p.string_value().c_str());
                s2=p.string_value();
            // *s=key.c_str() +p.string_value();

            //s2=p.string_value().asString();
            //    s=strcat(s1,s2);

            break;
        case pmgd::protobufs::Property::FloatType:
        //    printf("key: %s, value: %lf\n", key.c_str(), p.float_value());
                s2=std::to_string(p.float_value());
            break;
        default:
            printf(" Unknown\n");
    }
    s="  "+s1+"    " +s2
    ;
    return s;
}


Json::Value  QueryHandler::construct_responses(std::vector<pmgd::protobufs::CommandResponse *> response){
    Json::Value json_responses;
    for (auto it : response) {
            if (it->r_type() == pmgd::protobufs::NodeID) {
            long nodeid = it->op_int_value();
            Json::Value AddNode;

            if(it->error_code()!=pmgd::protobufs::CommandResponse::Success){
                //AddNode["Node_ID"]=int(nodeid);
                AddNode["return"] = it->error_msg();

            }
            else{
                //AddNode["Entity_ID"]=int(nodeid);
                AddNode["return"] = "Success: " +it->error_msg();
            }
            Json::Value node;
            node["AddEntity"]=AddNode;
            json_responses.append(node);


        }
        if (it->r_type() == pmgd::protobufs::EdgeID) {
            Json::Value AddEdge;
            long edgeid = it->op_int_value();
            //AddEdge["Connection_ID"]=int(edgeid);
            if(it->error_code()!=pmgd::protobufs::CommandResponse::Success){

                AddEdge["return"] = it->error_msg();}
            else
                AddEdge["return"] = "Success "+it->error_msg();
                Json::Value edge;
                edge["Connection"]=AddEdge;
                json_responses.append(edge);

        }
        if (it->r_type() == pmgd::protobufs::List) {
            Json::Value List;
            std::string list = "";


            auto mymap = it->prop_values();
            for(auto m_it : mymap) {
                // Assuming string for now
                pmgd::protobufs::PropertyList &p = m_it.second;

                for (int i = 0; i < p.values_size(); ++i) {
                    list= list+print_properties(m_it.first.c_str(), p.values(i));

                    //List["value"]="";//std::string(*list)"";
                    List["return"] = list;

                }

            }
            Json::Value node;
            node["List"]=List;
            json_responses.append(node);
        } //List

        if (it->r_type() == pmgd::protobufs::Average) {
            Json::Value Average;
            float average = it->op_float_value();

            Average["return"] =    double(average);

            Json::Value node;
            node["Average"]=Average;
            json_responses.append(node);


        }

        if (it->r_type() == pmgd::protobufs::Sum) {

            Json::Value Sum;
            float sum = it->op_float_value();
                Sum["return"]=float(sum);

            Json::Value node;
            node["Sum"]=Sum;
            json_responses.append(node);
                    }
        if (it->r_type() == pmgd::protobufs::Count) {
            Json::Value Count;
            float count = it->op_int_value();
            Count["return"] =float(count);
            Json::Value node;
            node["Count"]=count;
            json_responses.append(node);

        }

        printf("\n");
    }
return  json_responses;
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
            std::cout << "Error parsing!" << std::endl;
            Json::Value error;
            error["return"] = "Server error - parsing";
            json_responses.append(error);
        }

        //defien a vector of commands
        std::vector<pmgd::protobufs::Command *> cmds;
        unsigned group_count = 0;
        //this command to start a new transaction
        pmgd::protobufs::Command cmdtx;
        //this the protobuf of new TxBegin
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
            else{
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
        std::vector<std::vector<pmgd::protobufs::CommandResponse *>> pmgd_responses =
            _pmgd_qh.process_queries(cmds, group_count + 1);
            for (int i = 0; i < group_count; ++i) {
                std::vector<pmgd::protobufs::CommandResponse *> &response = pmgd_responses[i];


            json_responses_root.append(construct_responses(response));//append the result of the response to the response_root json


            }
            Json::StyledWriter Writer;
            Json_output = Writer.write(json_responses_root);
            std::cout<<Json_output;
            proto_res.set_json(Json_output);

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
