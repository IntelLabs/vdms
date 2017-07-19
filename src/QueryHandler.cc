#include "QueryHandler.h"


#include<string>

using namespace athena;

QueryHandler::QueryHandler(Jarvis::Graph *db, std::mutex *mtx)
	: _pmgd_qh(db, mtx)
{
}


void QueryHandler::construct_protobuf(std::vector<pmgd::protobufs::Command*>  &cmds, Json::Value root,int txid)
{
	int node_index=0, edge_index=0;
	const Json::Value& queries = root ["Query"]; // array of characters
	std::cout<<"Query Size is "<<queries.size(); //iterate over the query elements
	for (int j = 0; j < queries.size(); j++){ //iterate over the list of the queries
		const Json::Value& query=queries[j];
		for(int i=0;i< query.getMemberNames().size();i++) //iterate over the individual query
		{
			const Json::Value json_Node= queries[j]["AddNode"]; //the json Node
			const Json::Value json_Edge= queries[j]["AddEdge"]; //the json Edge
			std::string cmd = query.getMemberNames()[i];
			if (cmd == "Query"){
				Json::Value error;
				error["return"] = "Query not supported";
			}

			if (cmd == "AddNode"){
				pmgd::protobufs::Command* cmdadd = new  pmgd::protobufs::Command();
				cmdadd->set_cmd_id(pmgd::protobufs::Command::AddNode);
				cmdadd->set_tx_id(txid);
				pmgd::protobufs::AddNode *an = cmdadd->mutable_add_node();
				if(json_Node.isMember("clientId"))
					an->set_identifier(json_Node["clientId"].asUInt());
				pmgd::protobufs::Node *n = an->mutable_node();

				if (json_Node.isMember("Tag")){
					n->set_tag(json_Node["Tag"].asCString());
				}
				if (json_Node.isMember("properties")){ //iterate over the properties of nodes
					std::cout<<" Properties"<<json_Node["properties"].size()<<"\n";
					const Json::Value node_properties = json_Node["properties"]; //take the sub-object of the properties

					for(  auto itr = node_properties.begin() ; itr != node_properties.end() ; itr++ ){
						pmgd::protobufs::Property *p = n->add_properties();

						if ((*itr).isString()){
							p->set_type(pmgd::protobufs::Property::StringType);
							p->set_key(itr.key().asCString() );
							p->set_string_value((*itr).asString());
						}

						else if( ((*itr).isInt() )){
							p->set_type(pmgd::protobufs::Property::IntegerType);
							p->set_key(itr.key().asCString() );
							p->set_int_value((*itr).asInt());
						}

						else if( ((*itr).isBool() )){
							p->set_type(pmgd::protobufs::Property::BooleanType);
							p->set_key(itr.key().asCString() );
							p->set_int_value((*itr).asBool());
						}

						else if( ((*itr).isDouble() )){
							p->set_type(pmgd::protobufs::Property::FloatType);
							p->set_key(itr.key().asCString() );
							p->set_int_value((*itr).asDouble());
						}
						else
							printf( "unknown type=[%d]", itr.key().type() );
					} //nodes properties
				}

				cmds.push_back(cmdadd);

			}

			if (cmd == "AddEdge"){


				pmgd::protobufs::Command* cmdedge =new pmgd::protobufs::Command();
				cmdedge->set_tx_id(txid);

				cmdedge->set_cmd_id(pmgd::protobufs::Command::AddEdge);
				pmgd::protobufs::AddEdge *ae = cmdedge->mutable_add_edge();
				if(json_Edge.isMember("clientId"))
					ae->set_identifier(json_Edge["clientId"].asUInt());
				pmgd::protobufs::Edge *e = ae->mutable_edge();

				if(json_Edge.isMember("src")){
					e->set_src(json_Edge["src"].asInt() );
				}
				if(json_Edge.isMember("dst")){
					e->set_dst(json_Edge["dst"].asUInt());
				}
				if(json_Edge.isMember("tag") ||(json_Edge.isMember("Tag"))){
					e->set_tag(json_Edge["tag"].asCString());
				}
				if (json_Edge.isMember("properties")){ //iterate over the properties of nodes
					const Json::Value properties = json_Edge["properties"]; //take the sub-object of the properties

					for(  auto itr = properties.begin() ; itr != properties.end() ; itr++ ){
						//checking the property value type in the follwoing if statement:
						pmgd::protobufs::Property *p = e->add_properties();
						if ((*itr).isString()){
							p->set_type(pmgd::protobufs::Property::TimeType);
							p->set_key(itr.key().asCString());
							p->set_time_value((*itr).asString());
						}
						else if( ((*itr).isInt() )){
							p->set_type(pmgd::protobufs::Property::IntegerType);
							p->set_key(itr.key().asCString() );
							p->set_int_value((*itr).asInt());
						}

						else if( ((*itr).isBool() )){
							p->set_type(pmgd::protobufs::Property::BooleanType);
							p->set_key(itr.key().asCString() );
							p->set_int_value((*itr).asBool());
						}

						else if( ((*itr).isDouble() )){
							p->set_type(pmgd::protobufs::Property::FloatType);
							p->set_key(itr.key().asCString() );
							p->set_int_value((*itr).asDouble());
							//  p = e->add_properties();
							std::cout<<"here is the "<<itr.key().asCString()<<"\t"<<*itr<<"\n";
						}
					} // edge properties

				}

				std::cout<<"the edge id is "<< cmdedge->cmd_id()<<" Edge ID ***********\n";
				std::cout<<" the actual pmgd::protobufs::Command::Addedge is"<<pmgd::protobufs::Command::AddEdge<<std::endl;
				edge_index++;
				std::cout<<"Edge is added"<<std::endl;
				cmds.push_back(cmdedge);
			}//AddEdge
			else if( cmd=="queryNode"){
				std::cout<<"Query Nodes"<<std::endl;

			}
			else
				std::cout<<"This case is not covered\n";

		}
	}


}



void QueryHandler::process_query( std::string json_query) //overlaoded for testing purposes
{
	try {

		//----------Ragaad Test----------------

		std::cout<<"the converted String is "<<json_query<<"\n";
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
		else{
			std::cout<<"\nPasing successful and the root size is  "<<parsingSuccessful<<"\t"<<root.size()<<std::endl;
			construct_protobuf( cmds, root,txid); //this a function to bulid the protobuf fpr each json object
			std::cout<<"parsing and protobufs converting Successful:"<<parsingSuccessful<<std::endl;
			//checking the

		}
		//the vector is not called by reference
		//this is to push the TxEnd to the cmds vector
		pmgd::protobufs::Command cmdtxend;
		// Commit here doesn't change anything. Just indicates end of TX
		cmdtxend.set_cmd_id(pmgd::protobufs::Command::TxCommit);
		cmdtxend.set_tx_id(txid);
		cmds.push_back(&cmdtxend);
		for(auto k=0; k<cmds.size(); k++)
		{
			std::cout<<" the command number "<<cmds[k]->cmd_id()<<std::endl;
		}

		_pmgd_qh.process_queries(cmds); //execute the queries using the PMGDQueryHandler object
	}
	catch (Jarvis::Exception e) {
		std::cout<<"Unexpected Thing Happened!!"<<std::endl;
	}
}

void QueryHandler::process_connection(comm::Connection *c)
{
	CommandHandler handler(c);

	try {
		protobufs::queryMessage cmd = handler.get_command();
		//  std::cout << cmd.json_query() << std::endl;
		process_query(cmd.json_query() ); //calling the test-process-query to build the protobuff
	}
	catch (Jarvis::Exception e) {
		//  _dblock->unlock();
		delete c;
	}
}
