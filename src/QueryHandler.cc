#include <string>
#include "QueryHandler.h"
//map
using namespace athena;

QueryHandler::QueryHandler(Jarvis::Graph *db, std::mutex *mtx)
    : _pmgd_qh(db, mtx)
{
_rs_cmds.insert( std::pair<std::string,RSCommand*
    > ("AddNode", new AddNode()));

_rs_cmds.insert( std::pair<std::string,RSCommand*
        > ("AddEdge", new AddEdge()));
    std::cout<<_rs_cmds.size()<<"*****///\n";
 //initilaze the map

 }

void RSCommand::check_properties_type(pmgd::protobufs::Property *p,    const char * key , Json::Value val){

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
        //  p = e->add_properties();
        std::cout <<"here is the "<<key << "\t" << val << "\n";
    }



}


int AddNode::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds, const Json::Value json_Node, int txid){ //addNode command constructor
   pmgd::protobufs::Command* cmdadd = new  pmgd::protobufs::Command();

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_tx_id(txid);
    std::cout<<"AddNode-Protobuff"<<std::endl;
    //this->construct_protobuf(cmdadd,json_Node,txid);

    cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
    cmdadd->set_tx_id(txid);
    pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();
    if(json_Node.isMember("clientId"))
        an->set_identifier(json_Node["clientId"].asUInt());
      pmgd::protobufs::Node *n = an->mutable_node();

    if (json_Node.isMember("tag")){
        n->set_tag(json_Node["tag"].asCString());
    }
    if (json_Node.isMember("properties")){ //iterate over the properties of nodes
        std::cout<<" Properties"<<json_Node["properties"].size()<<"\n";
        const Json::Value node_properties = json_Node["properties"]; //take the sub-object of the properties

        for(  Json::ValueConstIterator itr = node_properties.begin() ; itr != node_properties.end() ; itr++ ){
            pmgd::protobufs::Property *p = n->add_properties();
    // Checking the properties
    check_properties_type(p, itr.key().asCString(), *itr);

        } //nodes properties
    }

    std::cout<<"******AddNode*****\n";
    cmds.push_back(cmdadd);
    return 1;

}

int AddEdge::construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds, const Json::Value json_Edge, int txid){
    pmgd::protobufs::Command* cmdedge =new pmgd::protobufs::Command();

    cmdedge->set_tx_id(txid);

    cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
    pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();
    if(json_Edge.isMember("clientId"))
        ae->set_identifier(json_Edge["clientId"].asUInt());

    pmgd::protobufs::Edge *e = ae->mutable_edge();
    if(json_Edge.isMember("sourceId"))
        e->set_src(json_Edge["sourceId"].asInt() );
    if(json_Edge.isMember("destId"))
        e->set_dst(json_Edge["destId"].asUInt());
    if(json_Edge.isMember("tag"))
        e->set_tag(json_Edge["tag"].asCString());
        if (json_Edge.isMember("properties")){ //iterate over the properties of nodes
            std::cout<<" Properties"<<json_Edge["properties"].size()<<"\n";
            const Json::Value edge_properties = json_Edge["properties"]; //take the sub-object of the properties

            for(  Json::ValueConstIterator itr = edge_properties.begin() ; itr != edge_properties.end() ; itr++ ){
                pmgd::protobufs::Property *p = e->add_properties();
        // Checking the properties
        check_properties_type(p, itr.key().asCString(), *itr);

            } //nodes properties
        }
     cmds.push_back(cmdedge);
    std::cout<<"******AddEdge*****\n";
    return 1;
}

Json::Value AddNode::send_response(){}

Json::Value AddEdge::send_response(){}





void QueryHandler::process_query(std::string json_query)
{
    try {
        std::cout<<"the converted String is " << json_query <<"\n";
        Json::Reader reader;
        Json::Value root;

        std::vector<pmgd::protobufs::Command *> cmds; //defien a vector of commands
        std::vector<pmgd::protobufs::CommandResponse *> responses; // a vector of responses to execute the commands
        int txid = 1; //a flag for this transaction
        pmgd::protobufs::Command cmdtx; //this command to start a new transaction
        cmdtx.set_cmd_id(pmgd::protobufs::Command::TxBegin); //this the protobuf of new TxBegin
        cmdtx.set_tx_id(txid); //give it an ID
        cmds.push_back(&cmdtx); //push the creating command to the vector
        //  std::string json_query=  cmd.json_query(); //commeted by Ragaad to test the parsing

        bool parsingSuccessful =  reader.parse(json_query, root); // reader can also read strings

        if ( !parsingSuccessful ) {
            std::cout << "Error parsing!" << std::endl;
            Json::Value error;
            error["return"] = "Server error - parsing";
            //  response.append(error);
            //return response;
        }
        else {
            std::cout<<"\nPasing successful and the root size is  "<<parsingSuccessful<<"\t"<<root.size()<<std::endl;
        //    const Json::Value& queries = root []; // array of characters
            //std::cout<<"Query Size is "<<queries.size(); //iterate over the query elements
            for (int j = 0; j < root.size(); j++){ //iterate over the list of the queries
                const Json::Value& query=root[j];
                assert (query.getMemberNames().size() == 1);
                std::string cmd = query.getMemberNames()[0];
                _rs_cmds[cmd]->construct_protobuf(cmds,query[cmd],1);
            }
        }
        //the vector is not called by reference
        //this is to push the TxEnd to the cmds vector
        pmgd::protobufs::Command cmdtxend;
        // Commit here doesn't change anything. Just indicates end of TX
        cmdtxend.set_cmd_id(pmgd::protobufs::Command::TxCommit);
        cmdtxend.set_tx_id(txid);
        cmds.push_back(&cmdtxend);
//        for(auto k=0; k<cmds.size(); k++) {
//            std::cout<<" the command number "<<cmds[k]->cmd_id()<<std::endl;
//        }

        _pmgd_qh.process_queries(cmds); // execute the queries using the PMGDQueryHandler object
    }
    catch (Jarvis::Exception e) {
        std::cout<<"Unexpected Thing Happened!!"<<std::endl;
    }
}

void QueryHandler::process_connection(comm::Connection *c)
{
    CommandHandler handler(c);

    try {
        while (true) {
            protobufs::queryMessage cmd = handler.get_command();
            process_query(cmd.json() ); // calling the test-process-query to build the protobuff

            // TODO Construct appropriate response here. Just a placeholder for now.
            protobufs::queryMessage resp;
            resp.set_json("{COOL RESPONSE}");
            handler.send_response(resp);
        }
    }
    catch (Jarvis::Exception e) {
        delete c;
    }
    catch (comm::ExceptionComm ce) {
        print_exception(ce);
        delete c;
    }
}
