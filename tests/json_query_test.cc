#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>
#include <stdlib.h>     /* system, NULL, EXIT_FAILURE */

#include "gtest/gtest.h"
#include <jsoncpp/json/writer.h>

#include "jarvis.h"
#include "AthenaConfig.h"
#include "QueryHandlerTester.h"

using namespace athena;
using namespace Jarvis;
using namespace std;

TEST(QueryHandler, addTest){

    Json::StyledWriter writer;

    std::ifstream ifile;
    int fsize;
    char * inBuf;
    ifile.open("newAPI.json", std::ifstream::in);
    ifile.seekg(0, std::ios::end);
    fsize = (int)ifile.tellg();
    ifile.seekg(0, std::ios::beg);
    inBuf = new char[fsize];
    ifile.read(inBuf, fsize);
    std::string json_query = std::string(inBuf);
    ifile.close();
    delete[] inBuf;

    Json::Reader reader;
    Json::Value root;
    Json::Value parsed;
    reader.parse(json_query, root);
    int in_node_num  = 0, out_node_num  = 0;
    int in_edge_num  = 0, out_edge_num  = 0;
    int in_query_num = 0, out_query_num = 0;
    int in_props = 0, out_props = 0;
    int success=0;
    bool list_found_before = false, average_found_before = false;
    bool count_found_before =false , sum_found_before =false;
    bool list_found_after = false , average_found_after = false;
    bool count_found_after =false , sum_found_after =false;
    double average_value=0;
    int count_value = 4342;

    for (int j = 0; j < root.size(); j++) {
        const Json::Value& query = root[j];
        assert (query.getMemberNames().size() == 1);
        std::string cmd = query.getMemberNames()[0];

        if (cmd=="AddEntity")
          in_node_num++;

        else if (cmd == "Connect")
          in_edge_num++;

        else if (cmd == "FindEntity") {
          in_query_num++;
          if ( query[cmd]["results"].isMember("list") )
            list_found_before=true;

          if ( query[cmd]["results"].isMember("average") )
            average_found_before=true;

          if ( query[cmd]["results"].isMember("sum") )
            sum_found_before=true;

          if ( query[cmd]["results"].isMember("count") ) {
            count_found_before=true;
          }
        }
        else if (query.isMember("properties"))
          in_props=query["properties"].size();
    }

    // This is needed every time we run, if not the test will fail
    // int i = system("rm -r jsongraph");
    Graph db("jsongraph", Graph::Create);

    mutex dblock;

    AthenaConfig::init("config-tests.json");

    QueryHandler qh_base(&db, &dblock);
    QueryHandlerTester query_handler(qh_base);

    protobufs::queryMessage proto_query;
    proto_query.set_json(json_query);
    protobufs::queryMessage response;

    query_handler.pq(proto_query, response );
    // std::string response_output= *(response.release_json());

    reader.parse(response.json().c_str(), parsed);
    // std::cout << writer.write(parsed) << std::endl;

    for (int j = 0; j < parsed.size(); j++) {
        const Json::Value& query = parsed[j];
        ASSERT_EQ(query.getMemberNames().size(),1);
        std::string cmd = query.getMemberNames()[0];

        if (cmd=="AddEntity")
            out_node_num++;
        if (cmd=="Connect")
            out_edge_num++;
        if (cmd =="FindEntity")
            out_query_num++;

        if ( query[cmd]["status"] == 0)
            success++;

        if (query[cmd].isMember("list"))
            list_found_after = true;

        if (query[cmd].isMember("average") ) {
            average_found_after = true;
            average_value = query[cmd]["average"].asDouble();
        }

        if (query[cmd].isMember("sum"))
            sum_found_after = true;

        if (query[cmd].isMember("count")){
            count_found_after = true;
            count_value = query[cmd]["count"].asInt();
       }

     }

    int total_success = out_node_num + out_query_num + out_edge_num;
    //google tests to double check the read and parsed values.
    EXPECT_EQ(in_node_num, out_node_num) << "Not enough nodes found";
    EXPECT_EQ(in_edge_num, out_edge_num) << "Not enough edges found";
    EXPECT_EQ(in_query_num, out_query_num) << "Not enough queries found";
    EXPECT_EQ(success, total_success) << "Not enough queries found";
    // EXPECT_EQ(list_found_before, list_found_after) <<"Wrong list operation!!";
    EXPECT_EQ(average_found_before, average_found_after) <<
                  "Wrong average operation!!";
    EXPECT_EQ(sum_found_before, sum_found_after) << "Wrong sum operation!!";
    EXPECT_EQ(count_found_before, count_found_after) <<
                  "Wrong count operation!!";
    // EXPECT_EQ(count_value, 4) <<"Wrong count value!!";
    // EXPECT_EQ(average_value, 69.25) <<"Wrong average value!!";

    // EXPECT_EQ(average_value, 69.25) <<"Wrong average value !!";
}
