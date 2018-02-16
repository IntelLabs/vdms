#pragma once
#include <string>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "VCL.h"

#include "protobuf/queryMessage.pb.h" // Protobuf implementation
#include "CommandHandler.h"
#include "PMGDQueryHandler.h" // to provide the database connection

// Json parsing files
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/writer.h>
#include <jsoncpp/json/json.h>

namespace athena {
    // Helper classes for handling various JSON commands.
class RSCommand
{
protected:
     void set_property(pmgd::protobufs::Property *p,
                                    const char * prop_name,
                                    Json::Value );
    public:

    virtual int construct_protobuf(
                            std::vector<pmgd::protobufs::Command*> &cmds,
                            const Json::Value& root,
                            const std::string& blob,
                            int txid) = 0;

    void run_operations(VCL::Image& vclimg, const Json::Value& op);

    virtual bool need_blob() { return false; }

    virtual  Json::Value parse_response(pmgd::protobufs::CommandResponse *);
    virtual  Json::Value construct_responses(  std::vector<pmgd::protobufs::CommandResponse *>&, Json::Value* )=0;
};

    // Low-level API
class AddEntity : public RSCommand
{
    public:
        int construct_protobuf( std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid);

        Json::Value construct_responses( std::vector<pmgd::protobufs::CommandResponse *>&, Json::Value*);
};

class AddConnection : public RSCommand
{
    public:
        int construct_protobuf( std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid);
         Json::Value construct_responses( std::vector<pmgd::protobufs::CommandResponse *>&, Json::Value*);
};

class FindEntity : public RSCommand
{
    public:
        int construct_protobuf( std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid);
        int build_Query_Node_protobuf(pmgd::protobufs::Command*, const Json::Value& root);

        int build_Query_Neighbor_Node_protobuf(pmgd::protobufs::Command*, const Json::Value& root);


        template <class T> int build_query_protobuf(pmgd::protobufs::Command*, const Json::Value& root, T* queryType);

        template <class T> int parse_query_constraints(const Json::Value& root, T* queryType);

        template < class T > int  get_response_type( const Json::Value& result_type_array, std::string response, T* queryType);

        template <class T> int parse_query_results (const Json::Value& result_type, T* queryType);

         Json::Value construct_responses( std::vector<pmgd::protobufs::CommandResponse *>&, Json::Value*);

};

    // High-level API
class AddImage: public RSCommand
{
    const std::string DEFAULT_TDB_PATH = "./tdb_database";
    const std::string DEFAULT_PNG_PATH = "./png_database";

    std::string _storage_tdb;
    std::string _storage_png;

    public:
        AddImage();

        int construct_protobuf( std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid);
        bool need_blob() { return true; }

       Json::Value construct_responses( std::vector<pmgd::protobufs::CommandResponse *>&, Json::Value*);
 };

class FindImage: public RSCommand
{

    public:

        FindImage();

        int construct_protobuf( std::vector<pmgd::protobufs::Command*> &cmds,
                                const Json::Value& root,
                                const std::string& blob,
                                int txid);
        bool need_blob() { return false; }

        Json::Value construct_responses( std::vector<pmgd::protobufs::CommandResponse *>&, Json::Value*);
};

    // Instance created per worker thread to handle all transactions on a given
    // connection.
class QueryHandler
{
    PMGDQueryHandler _pmgd_qh;
    std::unordered_map<std::string, RSCommand *> _rs_cmds;

     std::string Json_output;

    public:
        QueryHandler(Jarvis::Graph *db, std::mutex *mtx);
        void process_connection(comm::Connection *c);
        void process_query(protobufs::queryMessage proto_query,
                           protobufs::queryMessage& response);

    };

};
