#include "gtest/gtest.h"

#include "QueryHandler.h"
#include "AthenaConfig.h"

#include <string>
#include <fstream>
#include <iostream>
#include <mutex>
#include <vector>

using namespace athena;
using namespace Jarvis;

std::string singleAddImage(" \
        { \
            \"AddImage\": { \
                \"category\": { \
                    \"name\": \"BrainScans\" \
                }, \
                \"operation\": { \
                    \"width\": 512, \
                    \"type\": \"resize\", \
                    \"height\": 512 \
                }, \
                \"log\": false, \
                \"properties\": { \
                    \"name\": \"brain_0\", \
                    \"doctor\": \"Dr. Strange Love\" \
                }, \
                \"format\": \"png\" \
            } \
        } \
    ");

TEST(AddImage, simpleAdd)
{
    std::string addImg;
    addImg += "[" + singleAddImage + "]";

    int i = system("rm -r simpleAdd_db");
    Graph db("simpleAdd_db", Graph::Create);
    AthenaConfig::init("./addImage-config.json");

    std::mutex mu;
    QueryHandler query_handler(&db, &mu);

    protobufs::queryMessage proto_query;
    proto_query.set_json(addImg);

    std::string image;
    std::ifstream file("test_images/brain.png",
                    std::ios::in | std::ios::binary | std::ios::ate);

    image.resize(file.tellg());

    file.seekg(0, std::ios::beg);
    if( !file.read(&image[ 0 ], image.size()))
        std::cout << "error" << std::endl;

    proto_query.add_blobs(image);

    protobufs::queryMessage response;
    query_handler.process_query(proto_query, response);

    Json::Reader json_reader;
    Json::Value json_response;

    // std::cout << response.json() << std::endl;
    json_reader.parse(response.json(), json_response);

    EXPECT_EQ(json_response[0]["AddImage"]["status"].asString(),
            "Success");
}

TEST(AddImage, simpleAddx10)
{
    int total_images = 10;
    std::string string_query("[");

    for (int i = 0; i < total_images; ++i) {
        string_query += singleAddImage;
        if (i != total_images - 1)
            string_query += ",";
    }
    string_query += "]";

    int i = system("rm -r simpleAddx10_db");
    Graph db("simpleAddx10_db", Graph::Create);

    AthenaConfig::init("./addImage-config.json");

    std::mutex mu;
    QueryHandler query_handler(&db, &mu);

    protobufs::queryMessage proto_query;
    proto_query.set_json(string_query);

    std::string image;
    std::ifstream file("test_images/brain.png",
                    std::ios::in | std::ios::binary | std::ios::ate);

    image.resize(file.tellg());

    file.seekg(0, std::ios::beg);
    if( !file.read(&image[ 0 ], image.size()))
        std::cout << "error" << std::endl;

    for (int i = 0; i < total_images; ++i) {
        proto_query.add_blobs(image);
    }

    protobufs::queryMessage response;
    query_handler.process_query(proto_query, response);

    Json::Reader json_reader;
    Json::Value json_response;

    // std::cout << response.json() << std::endl;
    json_reader.parse(response.json(), json_response);

    for (int i = 0; i < total_images; ++i) {
        EXPECT_EQ(json_response[i]["AddImage"]["status"].asString(),
                "Success");
    }
}