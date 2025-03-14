/**
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2025 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <fstream>
#include <iostream>
#include <mutex>
#include <stdlib.h> /* system, NULL, EXIT_FAILURE */
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include <jsoncpp/json/writer.h>

#include "QueryHandlerNeo4j.h"
#include "QueryHandlerTester.h"
#include "VDMSConfig.h"
#include "Server.h"

TEST(Neo4JHandlerTest, addEntity) {

Json::FastWriter fastWriter;

Json::Value subobj;
Json::Value md_q;
Json::Value add_md;
std::string label = "testlabel_md";
std::string prop_name = "testprop_name";
std::string prop_value = "testprop_value";
std::string cypher_q;
std::string mdAdd;

cypher_q = "CREATE (VDMSNODE:" + label + "{" + prop_name + ":" + "\"" +
           prop_value + "\"" + "})";
md_q["cypher"] = cypher_q;
md_q["target_data_type"] = "md_only";

add_md["NeoAdd"] = md_q;

std::string singleAddMd = fastWriter.write(add_md);
mdAdd += "[" + singleAddMd + "]";

VDMS::VDMSConfig::init("server/config-tests.json"); //Setup JSOn for this

VDMS::QueryHandlerNeo4j::init();
VDMS::QueryHandlerNeo4j qh_base;
VDMS::QueryHandlerNeo4jTester query_handler(qh_base);

VDMS::protobufs::queryMessage proto_query;
proto_query.set_json(mdAdd);
proto_query.add_blobs();

VDMS::protobufs::queryMessage response;
query_handler.pq(proto_query, response);

Json::Reader json_reader;
Json::Value json_response;
json_reader.parse(response.json(), json_response);

subobj = json_response[0];
ASSERT_EQ(subobj["metadata_res"], Json::Value::null);

VDMS::VDMSConfig::destroy();
}

TEST(Neo4JHandlerTest, addEntityThenFind) {

Json::FastWriter fastWriter;


//Add Logic (mostly same as above)
Json::Value subobj;
Json::Value md_q;
Json::Value add_md;
std::string label = "testlabel_md_addfind";
std::string prop_name = "testprop_add_name";
std::string prop_value = "testprop_add_value";
std::string cypher_q;
std::string mdAdd;

cypher_q = "CREATE (VDMSNODE:" + label + "{" + prop_name + ":" + "\"" +
           prop_value + "\"" + "})";
md_q["cypher"] = cypher_q;
md_q["target_data_type"] = "md_only";

add_md["NeoAdd"] = md_q;

std::string singleAddMd = fastWriter.write(add_md);
mdAdd += "[" + singleAddMd + "]";

VDMS::VDMSConfig::init("server/config-tests.json"); //Setup JSOn for this

VDMS::QueryHandlerNeo4j::init();
VDMS::QueryHandlerNeo4j qh_base;
VDMS::QueryHandlerNeo4jTester query_handler(qh_base);

VDMS::protobufs::queryMessage proto_query;
proto_query.set_json(mdAdd);
proto_query.add_blobs();

VDMS::protobufs::queryMessage response;
query_handler.pq(proto_query, response);

Json::Reader json_reader;
Json::Value json_response;
json_reader.parse(response.json(), json_response);

subobj = json_response[0];
ASSERT_EQ(subobj["metadata_res"], Json::Value::null);

//---- find


Json::Value md_find_q;
Json::Value find_md;
std::string find_label = "testlabel_md_addfind";
std::string ret_prop_name = "testprop_add_name";
std::string find_cypher_q;
std::string findMd;
Json::Value metadata_obj_list;
Json::Value ind_metadata;

find_cypher_q = "MATCH (VDMSNODE:" + find_label + ") RETURN (VDMSNODE." + ret_prop_name +")";
md_find_q["cypher"] = find_cypher_q;
md_find_q["target_data_type"] = "md_only";

find_md ["NeoFind"] = md_find_q;


std::cout << find_md;

std::string findMdString = fastWriter.write(find_md);
findMd += "[" + findMdString + "]";

VDMS::protobufs::queryMessage proto_query_find;
proto_query_find.set_json(findMd);
proto_query_find.add_blobs();

VDMS::protobufs::queryMessage find_response;
query_handler.pq(proto_query_find, find_response);

Json::Reader json_reader_find;
Json::Value json_response_find;
json_reader_find.parse(find_response.json(), json_response_find);
subobj = json_response_find[0];
metadata_obj_list = subobj["metadata_res"];
ind_metadata = metadata_obj_list[0];
ASSERT_EQ(ind_metadata["(VDMSNODE." + ret_prop_name + ")"],prop_value);
VDMS::VDMSConfig::destroy();
}