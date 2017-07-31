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
    _rs_cmds["AddNode"]  = new AddNode();
    _rs_cmds["AddEdge"]  = new AddEdge();
    _rs_cmds["AddImage"] = new AddImage();
}

void RSCommand::set_property(pmgd::protobufs::Property *p,
    const char * key , Json::Value val){

    if (val.isString()){
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
                                int txid)
{
    pmgd::protobufs::Command* cmdadd = new  pmgd::protobufs::Command();

    Json::Value aNode = jsoncmd["AddNode"];

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_tx_id(txid);

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_tx_id(txid);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();

    if(aNode.isMember("clientId"))
        an->set_identifier(aNode["clientId"].asUInt());
      pmgd::protobufs::Node *n = an->mutable_node();

    if (aNode.isMember("tag")){
        n->set_tag(aNode["tag"].asCString());
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
                                int txid)
{
    Json::Value aEdge = jsoncmd["AddEdge"];

    pmgd::protobufs::Command* cmdedge =new pmgd::protobufs::Command();
    cmdedge->set_tx_id(txid);
    cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();

    if(aEdge.isMember("clientId"))
        ae->set_identifier(aEdge["clientId"].asUInt());

    pmgd::protobufs::Edge *e = ae->mutable_edge();

    if(aEdge.isMember("sourceId"))
        e->set_src(aEdge["sourceId"].asInt() );
    if(aEdge.isMember("destId"))
        e->set_dst(aEdge["destId"].asUInt());
    if(aEdge.isMember("tag"))
        e->set_tag(aEdge["tag"].asCString());

    if (aEdge.isMember("properties")){
        std::cout<<" Properties"<<aEdge["properties"].size()<<"\n";
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

int AddImage::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& jsoncmd,
                                const std::string& blob,
                                int txid)
{
    Json::Value aImg = jsoncmd["AddImage"];
    Json::Value res;

    ChronoCpu ch("addImage");
    ch.tic();

    // Create PMGD cmd for AddNode
    pmgd::protobufs::Command* cmdadd = new pmgd::protobufs::Command();
    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_tx_id(txid);
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

// Json::Value AddNode::send_response(){}

// Json::Value AddEdge::send_response(){}

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

void QueryHandler::process_query(protobufs::queryMessage proto_query,
                                 protobufs::queryMessage& proto_res)
{
    Json::FastWriter fastWriter;

    try {
        std::vector<Json::Value> json_responses;
        Json::Value root;
        Json::Reader reader;

        bool parsingSuccessful = reader.parse( proto_query.json().c_str(),
                                               root );

        if ( !parsingSuccessful ) {
            std::cout << "Error parsing!" << std::endl;
            Json::Value error;
            error["return"] = "Server error - parsing";
            json_responses.push_back(error);
        }

            //defien a vector of commands
        std::vector<pmgd::protobufs::Command *> cmds;
        int txid = 1; //a flag for this transaction
            //this command to start a new transaction
        pmgd::protobufs::Command cmdtx;
            //this the protobuf of new TxBegin
        cmdtx.set_cmd_id(pmgd::protobufs::Command::TxBegin);
        cmdtx.set_tx_id(txid); //give it an ID
        cmds.push_back(&cmdtx); //push the creating command to the vector

        unsigned blob_count = 0, query_count = 0;

        //iterate over the list of the queries
        for (int j = 0; j < root.size(); j++) {
            const Json::Value& query = root[j];
            assert (query.getMemberNames().size() == 1);
            std::string cmd = query.getMemberNames()[0];
            query_count++;

            if (_rs_cmds[cmd]->need_blob()) {
                assert (proto_query.blobs().size() >= blob_count);
                std::string blob = proto_query.blobs(blob_count);
                _rs_cmds[cmd]->construct_protobuf(cmds, query, blob, j);
                blob_count++;
            }
            else{
                _rs_cmds[cmd]->construct_protobuf(cmds, query, "", j);
            }
        }
        //the vector is not called by reference
        //this is to push the TxEnd to the cmds vector
        pmgd::protobufs::Command cmdtxend;
        // Commit here doesn't change anything. Just indicates end of TX
        cmdtxend.set_cmd_id(pmgd::protobufs::Command::TxCommit);
        cmdtxend.set_tx_id(txid);
        cmds.push_back(&cmdtxend);

        // execute the queries using the PMGDQueryHandler object
        std::vector<std::vector<pmgd::protobufs::CommandResponse *>> pmgd_responses =
                            _pmgd_qh.process_queries(cmds, query_count);

        // //iterate over the list of the queries to generate responses
        // for (int j = 0; j < root.size(); j++){
        //     const Json::Value& query=root[j];
        //     assert (query.getMemberNames().size() == 1);
        //     std::string cmd = query.getMemberNames()[0];
        //     _rs_cmds[cmd]->construct_protobuf(cmds,query[cmd],1);
        // }

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
