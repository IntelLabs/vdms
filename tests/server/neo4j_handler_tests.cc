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

void _generate_desc_linear_increase(int d, int nb, float *xb, float init) {
    float val = init;
    for (int i = 1; i <= nb * d; ++i) {
        xb[i - 1] = val;
        if (i % d == 0)
            val++;
    }
}

//num of dimensions
//number of unique descriptors
//initial value (generally hand in zero)
float *neo4j_generate_desc_linear_increase(int d, int nb, float init) {
    float *xb = new float[d * nb];
    _generate_desc_linear_increase(d, nb, xb, init);
    return xb;
}

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

TEST(Neo4JHandlerTest, AddFind_DescriptorSet) {
    Json::FastWriter fastWriter;

    Json::Value add_set_q;
    Json::Value add_set_trans;

    Json::Value find_set_q;
    Json::Value find_set_trans;
    Json::Value results;
    Json::Value results_list;

    //Add Descriptor Set Query
    add_set_q["engine"] = "FaissFlat";
    add_set_q["metric"] = "L2";
    add_set_q["name"] = "test_set";
    add_set_q["dimensions"] = 128;
    add_set_trans["NeoAddDescriptorSet"] = add_set_q;

    //Find Descriptor Set Query
    results_list.append("set_name");
    results_list.append("engine");
    results_list.append("dimensions");
    results["list"] = results_list;
    find_set_q["results"] =  results;
    find_set_q["set"] = "test_set";
    find_set_trans["NeoFindDescriptorSet"] = find_set_q;

    //State initialization
    Json::Reader reader;
    Json::Value root;
    Json::Value parsed;
    Json::Value parsed_findset;


    VDMS::VDMSConfig::init("unit_tests/config-tests.json");
    VDMS::QueryHandlerNeo4j::init();
    VDMS::QueryHandlerNeo4j qh_base;
    VDMS::QueryHandlerNeo4jTester query_handler(qh_base);

    VDMS::protobufs::queryMessage proto_query_add_set;
    VDMS::protobufs::queryMessage proto_query_find_set;
    VDMS::protobufs::queryMessage response;
    VDMS::protobufs::queryMessage findset_response;

    // AddSet queries
    std::string add_set_str = fastWriter.write(add_set_trans);
    std::string final_add_set_str = "[" + add_set_str + "]";
    proto_query_add_set.set_json(final_add_set_str);

    //Issue AddSet Query
    Json::Value resp_obj_addset;
    Json::Value status_obj;
    query_handler.pq(proto_query_add_set, response);

    reader.parse(response.json().c_str(), parsed);
    resp_obj_addset = parsed[0];
    status_obj = resp_obj_addset["NeoAddDescriptorSet"];
    ASSERT_EQ(status_obj["status"], 0);

    Json::Value resp_obj_findset;
    Json::Value resp_obj_q;
    Json::Value resp_ent_list;
    Json::Value res_obj;
    Json::Value fin_results;
    std::string find_set_str = fastWriter.write(find_set_trans);
    std::string final_find_set_str = "[" + find_set_str + "]";
    proto_query_find_set.set_json(final_find_set_str);

    query_handler.pq(proto_query_find_set, findset_response);
    reader.parse(findset_response.json().c_str(), parsed_findset);
    resp_obj_findset = parsed_findset[0];
    resp_ent_list = resp_obj_findset["NeoFindDescriptorSet"];
    res_obj = resp_ent_list["entities"];


    fin_results = res_obj[0];

    ASSERT_EQ(fin_results["dimensions"], 128);
    ASSERT_EQ(fin_results["engine"], "FaissFlat");
    ASSERT_EQ(fin_results["set_name"], "test_set");

    VDMS::VDMSConfig::destroy();
}

TEST(Neo4JHandlerTest, AddFind_Descriptor) {

    Json::FastWriter fastWriter;


    Json::Value add_set_q;
    Json::Value add_set_trans;

    Json::Value find_desc_q;
    Json::Value find_desc_trans;
    Json::Value find_desc_constraints;
    Json::Value constraints_vals;

    Json::Value add_desc_q;
    Json::Value add_desc_trans;
    Json::Value add_desc_props;
    float *vec_val;
    int dims;
    int nr_vecs;

    Json::Value results;
    Json::Value results_list;

    //Add Descriptor Set Query
    dims = 128;
    add_set_q["engine"] = "FaissFlat";
    add_set_q["metric"] = "L2";
    add_set_q["name"] = "test_set_standalone";
    add_set_q["dimensions"] = dims;
    add_set_trans["NeoAddDescriptorSet"] = add_set_q;

    std::string add_set_str = fastWriter.write(add_set_trans);
    std::string final_add_set_str = "[" + add_set_str + "]";

    //Add Descriptor Query
    nr_vecs = 1;
    vec_val = neo4j_generate_desc_linear_increase(dims, nr_vecs,0);

    std::string vec_bytes_str;
    vec_bytes_str.resize(nr_vecs * dims *sizeof(float));
    std::memcpy((void *)vec_bytes_str.data(), vec_val,
        nr_vecs * sizeof(float) * dims);

    add_desc_props["prop_1"] = 10;
    add_desc_props["prop_2"] = "str_prop";

    add_desc_q["set"] = "test_set_standalone";
    add_desc_q["properties"] = add_desc_props;
    add_desc_trans["NeoAddDescriptor"] = add_desc_q;

    std::string add_desc_str = fastWriter.write(add_desc_trans);
    std::string final_add_desc_str = "[" + add_desc_str + "]";

    //Find Descriptor Query
    results_list["list"].append("prop_1");
    results_list["list"].append("prop_2");

    constraints_vals.append("==");
    constraints_vals.append(10);

    find_desc_constraints["prop_1"] = constraints_vals;

    find_desc_q["set"] = "test_set_standalone";
    find_desc_q["constraints"] = find_desc_constraints;
    find_desc_q["results"] = results_list;
    find_desc_trans["NeoFindDescriptor"] = find_desc_q;

    std::string find_desc_str = fastWriter.write(find_desc_trans);
    std::string final_find_desc_str = "[" + find_desc_str + "]";

    //State initialization
    Json::Reader reader;
    Json::Value root;
    Json::Value parsed;
    Json::Value parsed_addset;
    Json::Value parsed_finddesc;

    VDMS::VDMSConfig::init("unit_tests/config-tests.json");
    VDMS::QueryHandlerNeo4j::init();
    VDMS::QueryHandlerNeo4j qh_base;
    VDMS::QueryHandlerNeo4jTester query_handler(qh_base);

    VDMS::protobufs::queryMessage proto_query_add_set;
    VDMS::protobufs::queryMessage proto_query_add_desc;
    VDMS::protobufs::queryMessage proto_query_find_desc;
    VDMS::protobufs::queryMessage addset_response;
    VDMS::protobufs::queryMessage adddesc_response;
    VDMS::protobufs::queryMessage finddesc_response;

    //Adding and verifying set was created correctly
    //Issue AddSet Query
    Json::Value resp_obj_addset;
    Json::Value status_obj_addset;
    proto_query_add_set.set_json(final_add_set_str);
    query_handler.pq(proto_query_add_set, addset_response);

    reader.parse(addset_response.json().c_str(), parsed);
    resp_obj_addset = parsed[0];
    status_obj_addset = resp_obj_addset["NeoAddDescriptorSet"];
    ASSERT_EQ(status_obj_addset["status"], 0);

    // Adding a descriptor
    Json::Value resp_obj_adddesc;
    Json::Value status_obj_adddesc;

    proto_query_add_desc.add_blobs(vec_bytes_str);
    proto_query_add_desc.set_json(final_add_desc_str);

    query_handler.pq(proto_query_add_desc, adddesc_response);

    reader.parse(adddesc_response.json().c_str(), parsed);
    resp_obj_adddesc = parsed[0];

    //Finding a descriptor
    Json::Value resp_obj_finddesc;
    Json::Value status_obj_finddesc;

    proto_query_find_desc.set_json(final_find_desc_str);
    //proto_query_find_desc.add_blobs(vec_val);
    query_handler.pq(proto_query_find_desc, finddesc_response);

    reader.parse(finddesc_response.json().c_str(), parsed);
    resp_obj_adddesc = parsed[0];


    Json::Value find_desc_base = resp_obj_adddesc["NeoFindDescriptor"];
    Json::Value find_desc_entities = find_desc_base["entities"];
    Json::Value ind_entity = find_desc_entities[0];
    ASSERT_EQ(ind_entity["prop_1"], 10);

}

TEST(Neo4JHandlerTest, AddFind_DescriptorBatch_KNN) {

    Json::FastWriter fastWriter;

    Json::Value add_set_q;
    Json::Value add_set_trans;

    Json::Value find_desc_q;
    Json::Value find_desc_trans;
    Json::Value find_desc_constraints;
    Json::Value constraints_vals;

    Json::Value add_desc_q;
    Json::Value add_desc_trans;
    Json::Value add_desc_props_1;
    Json::Value add_desc_props_2;
    Json::Value add_desc_props_3;

    float *vec_val;
    int dims;
    int nr_vecs;

    Json::Value results;
    Json::Value results_list;

    //Add Descriptor Set Query
    dims = 128;
    add_set_q["engine"] = "FaissFlat";
    add_set_q["metric"] = "L2";
    add_set_q["name"] = "test_set_batch_knn";
    add_set_q["dimensions"] = dims;
    add_set_trans["NeoAddDescriptorSet"] = add_set_q;

    std::string add_set_str = fastWriter.write(add_set_trans);
    std::string final_add_set_str = "[" + add_set_str + "]";

    //Add Descriptor Batch Query
    Json::Value props_list;
    nr_vecs = 3;
    vec_val = neo4j_generate_desc_linear_increase(dims, nr_vecs,0);

    std::string vec_bytes_str;
    vec_bytes_str.resize(nr_vecs * dims *sizeof(float));
    std::memcpy((void *)vec_bytes_str.data(), vec_val,
            nr_vecs * sizeof(float) * dims);

    add_desc_props_1["prop_1"] = 10;
    add_desc_props_1["prop_2"] = "str_prop_1";

    add_desc_props_2["prop_1"] = 20;
    add_desc_props_2["prop_2"] = "str_prop_2";

    add_desc_props_3["prop_1"] = 30;
    add_desc_props_3["prop_2"] = "str_prop_3";

    props_list.append(add_desc_props_1);
    props_list.append(add_desc_props_2);
    props_list.append(add_desc_props_3);


    add_desc_q["set"] = "test_set_batch_knn";
    add_desc_q["batch_properties"] = props_list;
    add_desc_trans["NeoAddDescriptor"] = add_desc_q;

    std::string add_desc_str = fastWriter.write(add_desc_trans);
    std::string final_add_desc_str = "[" + add_desc_str + "]";

    //Find Descriptor Query + KNN
    results_list["list"].append("prop_1");
    results_list["list"].append("prop_2");

    constraints_vals.append("==");
    constraints_vals.append(10);

    find_desc_q["set"] = "test_set_batch_knn";
    find_desc_q["results"] = results_list;
    find_desc_q["k_neighbors"] = 2;
    find_desc_trans["NeoFindDescriptor"] = find_desc_q;

    std::string find_desc_str = fastWriter.write(find_desc_trans);
    std::string final_find_desc_str = "[" + find_desc_str + "]";

    //State initialization
    Json::Reader reader;
    Json::Value root;
    Json::Value parsed;
    Json::Value parsed_addset;
    Json::Value parsed_finddesc;

    VDMS::VDMSConfig::init("unit_tests/config-tests.json");
    VDMS::QueryHandlerNeo4j::init();
    VDMS::QueryHandlerNeo4j qh_base;
    VDMS::QueryHandlerNeo4jTester query_handler(qh_base);

    VDMS::protobufs::queryMessage proto_query_add_set;
    VDMS::protobufs::queryMessage proto_query_add_desc;
    VDMS::protobufs::queryMessage proto_query_find_desc;
    VDMS::protobufs::queryMessage addset_response;
    VDMS::protobufs::queryMessage adddesc_response;
    VDMS::protobufs::queryMessage finddesc_response;

    //Adding and verifying set was created correctly
    //Issue AddSet Query
    Json::Value resp_obj_addset;
    Json::Value status_obj_addset;
    proto_query_add_set.set_json(final_add_set_str);
    query_handler.pq(proto_query_add_set, addset_response);

    reader.parse(addset_response.json().c_str(), parsed);
    resp_obj_addset = parsed[0];
    status_obj_addset = resp_obj_addset["NeoAddDescriptorSet"];
    ASSERT_EQ(status_obj_addset["status"], 0);

    // Adding a descriptor
    Json::Value resp_obj_adddesc;
    Json::Value status_obj_adddesc;

    proto_query_add_desc.add_blobs(vec_bytes_str);
    proto_query_add_desc.set_json(final_add_desc_str);

    query_handler.pq(proto_query_add_desc, adddesc_response);

    reader.parse(adddesc_response.json().c_str(), parsed);
    resp_obj_adddesc = parsed[0];

    //Finding a descriptor
    Json::Value resp_obj_finddesc;
    Json::Value status_obj_finddesc;

    vec_val = neo4j_generate_desc_linear_increase(dims, 1,0);

    vec_bytes_str.resize(dims *sizeof(float));
    std::memcpy((void *)vec_bytes_str.data(), vec_val,
            128 * sizeof(float));

    proto_query_find_desc.set_json(final_find_desc_str);
    proto_query_find_desc.add_blobs(vec_bytes_str);
    query_handler.pq(proto_query_find_desc, finddesc_response);

    reader.parse(finddesc_response.json().c_str(), parsed);
    resp_obj_adddesc = parsed[0];

    Json::Value find_desc_base = resp_obj_adddesc["NeoFindDescriptor"];
    Json::Value find_desc_entities = find_desc_base["entities"];
    int nr_entities = find_desc_entities.size();
    Json::Value ind_entity = find_desc_entities[1];
    ASSERT_EQ(ind_entity["prop_1"], 20);
    ASSERT_EQ(nr_entities,2);


}