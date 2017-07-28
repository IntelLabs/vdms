#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "protobuf/queryMessage.pb.h" // Protobuf implementation
#include "CommandHandler.h"
#include "PMGDQueryHandler.h" // to provide the database connection

// Json parsing files
#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>

using namespace pmgd;

namespace athena {
    // Instance created per worker thread to handle all transactions on a given
    // connection.
    //map

     class RSCommand{
     public:

         virtual int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds, const Json::Value root, int txid)=0;
         virtual Json::Value send_response()=0;

        protected:
             void check_properties_type(pmgd::protobufs::Property *p,     const char * , Json::Value  );

     };
     class AddNode: public RSCommand {

         int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds, const Json::Value root, int txid);
          Json::Value send_response();
     };
     class AddEdge: public RSCommand {
         int construct_protobuf(std::vector<pmgd::protobufs::Command*> &cmds,  const Json::Value root, int txid);
          Json::Value send_response();

     };



    class QueryHandler
    {
        PMGDQueryHandler _pmgd_qh;
        //map<string,RSCommannd*>
        std::unordered_map<std::string, RSCommand *> _rs_cmds;



    public:
        QueryHandler(Jarvis::Graph *db, std::mutex *mtx);
        void process_connection(comm::Connection *c);
        void process_query(std::string);




    };
};
