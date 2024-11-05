/**
 * @file   Neo4jHandlerCommands.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2024 Intel Corporation
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

#include "ExceptionsCommand.h"
#include "ImageLoop.h"
#include "Neo4JCommands.h"
#include "BackendNeo4j.h"
#include "VDMSConfig.h"
#include "defines.h"
#include "vcl/VCL.h"
#include <jsoncpp/json/writer.h>
#include "QueryHandlerNeo4j.h"

#include "OpsIOCoordinator.h"
#include <ctime>
#include <sys/time.h>

#include <chrono>

using namespace VDMS;

void append_results_to_cypher(std::string &tx, Json::Value &results){

    printf("Results Append to Cypher\n");
    std::cout << results <<std::endl;
    tx += "return ";
    for (Json::Value::ArrayIndex i = 0; i != results["list"].size(); i++){
        if(i == results["list"].size() -1) {
            tx += "DESCSET." + results["list"][i].asString() + ";";
        } else {
            tx += "DESCSET." + results["list"][i].asString() + ", ";
        }
    }
}

std::string NeoDescriptorsCommand::get_set_path(const std::string &set_name,
                                             int &dim) {

    // Check cache for descriptor set, if its found set dimensions and return
    // otherwise we go forward and query graph db to locate the descriptor set
    auto element = _desc_set_locator.find(set_name);
    std::string mapped_path;

    //if we have a cached location, use that.
    if (element != _desc_set_locator.end()) {
        mapped_path = element->second;
        dim = _desc_set_dims[set_name];
        return mapped_path;
    }

    //Neo4j Logic
    neo4j_transaction *tx;
    neo4j_connection_t *conn;
    neo4j_result_stream_t *res_stream;
    Json::Value neo4j_resp;
    Json::Value ind_metadata;
    std::string cypher_tx;
    std::string desc_path_str;

    //query for the descriptor set + path info
    cypher_tx = cypher_tx + "MATCH (DESCSET:VDMS_descset {set_name: '" + set_name +"'}) return DESCSET.set_path, DESCSET.dimensions";
    conn = QueryHandlerNeo4j::neoconn_pool->get_conn();

    // begin neo4j transaction
    tx = QueryHandlerNeo4j::neoconn_pool->open_tx(conn, 10000, "r");

    //issue cypher command and get result stream, convert response to JSON
    res_stream = QueryHandlerNeo4j::neoconn_pool->run_in_tx((char *)cypher_tx.c_str(), tx);
    neo4j_resp = QueryHandlerNeo4j::neoconn_pool->results_to_json(res_stream);
    QueryHandlerNeo4j::neoconn_pool->put_conn(conn);

    if (neo4j_resp.isMember("metadata_res")) {
        if(neo4j_resp["metadata_res"] == Json::Value::null){
            return "";
        } else{
            dim = neo4j_resp["metadata_res"][0]["DESCSET.dimensions"].asInt();
            desc_path_str = neo4j_resp["metadata_res"][0]["DESCSET.set_path"].asString();
            _desc_set_dims[set_name] = dim;
            _desc_set_locator[set_name] = desc_path_str;
            return desc_path_str;
        }
    } else {
        std::cout << "Find Set query Failed!" << std::endl;
    }

    return "";
}

NeoDescriptorsCommand::NeoDescriptorsCommand(const std::string &cmd_name)
        : Neo4jCommand(cmd_name) {
    _dm = DescriptorsManager::instance();
    output_vcl_timing =
            VDMSConfig::instance()->get_bool_value("print_vcl_timing", false);
}

//ADD DESCRIPTOR SET
Neo4jNeoAddDescSet::Neo4jNeoAddDescSet() : NeoDescriptorsCommand("NeoAddDescriptorSet") {

    _storage_sets = VDMSConfig::instance()->get_path_descriptors();
    _flinng_num_rows = 3; // set based on the default values of Flinng
    _flinng_cells_per_row = 1000;
    _flinng_num_hash_tables = 10;
    _flinng_hashes_per_table = 12;
    _flinng_sub_hash_bits = 2;
    _flinng_cut_off = 6;

}

int Neo4jNeoAddDescSet::data_processing(std::string &cypher_tx, const Json::Value &root,
                                        const std::string &blob, int grp_id, Json::Value &error){

    const Json::Value &cmd = root[_cmd_name];
    std::string set_name = cmd["name"].asString();
    std::string dimensions = cmd["dimensions"].asString();
    std::string engine = cmd["engine"].asString();
    std::string desc_set_path = _storage_sets + "/" + set_name;

    //TODO create constant for DESCSET label
    cypher_tx = "CREATE (VDMSNODE:VDMS_descset { set_name: '" + set_name +"', set_path: '" + desc_set_path + "'";
    cypher_tx += ", engine: '" + engine +"' ";
    cypher_tx += ", dimensions: " + dimensions;
    cypher_tx += "})";

    Json::Value props = get_value<Json::Value>(cmd, "properties");

    //loop over properties to create properties for new node
    int dim_prop = cmd["dimensions"].asInt();
    if (props[VDMS_DESC_SET_ENGIN_PROP] == "Flinng") {
        if (cmd.isMember("flinng_num_rows"))
            _flinng_num_rows = cmd["flinng_num_rows"].asInt();
        if (cmd.isMember("flinng_cells_per_row"))
            _flinng_cells_per_row = cmd["flinng_cells_per_row"].asInt();
        if (cmd.isMember("flinng_num_hash_tables"))
            _flinng_num_hash_tables = cmd["flinng_num_hash_tables"].asInt();
        if (cmd.isMember("flinng_hashes_per_table"))
            _flinng_hashes_per_table = cmd["flinng_hashes_per_table"].asInt();
        if (cmd.isMember("flinng_sub_hash_bits"))
            _flinng_sub_hash_bits = cmd["flinng_sub_hash_bits"].asInt();
        if (cmd.isMember("flinng_cut_off"))
            _flinng_cut_off = cmd["flinng_cut_off"].asInt();
    }
    // This is to throw an error if the desc-set already exists
    std::string pathcheck = get_set_path(set_name,dim_prop);
    if (pathcheck != ""){
        error["status"] = Neo4jCommand::Error;
        error["info"] = "Descriptor set already exists!";
        return -1;
    }

    //TODO Future dev to incorporate linking capability in some fashion
    //if (cmd.isMember("link")) {
    //    add_link(query, cmd["link"], node_ref, VDMS_DESC_SET_EDGE_TAG);
    //}

    return 0;
}

Json::Value Neo4jNeoAddDescSet::construct_responses(Json::Value &json_responses,
                                                    const Json::Value &json,
                                                    protobufs::queryMessage &response,
                                                    const std::string &blob) {

    const Json::Value &cmd = json[_cmd_name];
    Json::Value resp = check_responses(json_responses);
    Json::Value ret;

    auto error = [&](Json::Value &res) {
        ret[_cmd_name] = res;
        return ret;
    };//TODO CHECK THAT HANDLER IS SETTING ERROR

    if (resp["status"] ==  Neo4jCommand::Error) {
        printf("Status: Error");
        return error(resp);
    }

    int dimensions = cmd["dimensions"].asInt();
    std::string set_name = cmd["name"].asString();
    std::string desc_set_path = _storage_sets + "/" + set_name;

    std::string metric_str = get_value<std::string>(cmd, "metric", "L2");
    VCL::DistanceMetric metric = metric_str == "L2" ? VCL::L2 : VCL::IP;

    // For now, we use the default faiss index.
    std::string eng_str = get_value<std::string>(cmd, "engine", "FaissFlat");
    if (eng_str == "FaissFlat")
        _eng = VCL::FaissFlat;
    else if (eng_str == "FaissIVFFlat")
        _eng = VCL::FaissIVFFlat;
    else if (eng_str == "TileDBDense")
        _eng = VCL::TileDBDense;
    else if (eng_str == "TileDBSparse")
        _eng = VCL::TileDBSparse;
    else if (eng_str == "Flinng")
        _eng = VCL::Flinng;
    else if (eng_str == "FaissHNSWFlat")
        _eng = VCL::FaissHNSWFlat;
    else
        throw ExceptionCommand(DescriptorSetError, "Engine not supported");

    // We can probably set up a mechanism
    // to fix a broken link when detected later, same with images.
    VCL::DescriptorParams *param = nullptr;
    try {
        param = new VCL::DescriptorParams(_flinng_num_rows, _flinng_cells_per_row,
                                          _flinng_num_hash_tables,
                                          _flinng_hashes_per_table);
        VCL::DescriptorSet desc_set(desc_set_path, dimensions, _eng, metric, param);

        //TODO AWS storage not currently supported
        /*if (_use_aws_storage) {
            VCL::RemoteConnection *connection = new VCL::RemoteConnection();
            std::string bucket = VDMSConfig::instance()->get_bucket_name();
            connection->_bucket_name = bucket;
            desc_set.set_connection(connection);
        }*/

        desc_set.store();
        if (output_vcl_timing) {
            desc_set.timers.print_map_runtimes();
        }
        desc_set.timers.clear_all_timers();

        delete (param);
    } catch (VCL::Exception e) {
        print_exception(e);
        resp["status"] = Neo4jCommand::Error;
        resp["info"] = std::string("VCL Exception: ") + e.msg;
        delete (param);
        return error(resp);
    }
    resp.clear();
    resp["status"] = Neo4jCommand::Success;

    ret[_cmd_name] = resp;
    return ret;
}

//FIND DESCRIPTOR SET
Neo4jNeoFindDescSet::Neo4jNeoFindDescSet() : NeoDescriptorsCommand("NeoFindDescriptorSet") {}

int Neo4jNeoFindDescSet::data_processing(std::string &tx, const Json::Value &jsoncmd,
                                         const std::string &blob, int grp_id,
                                         Json::Value &error) {

    const Json::Value &cmd = jsoncmd[_cmd_name];
    Json::Value results = get_value<Json::Value>(cmd, "results");

    const std::string set_name = cmd["set"].asString();
    const std::string set_path = _storage_sets + "/" + set_name;

    Json::Value constraints, link;
    Json::Value name_arr;
    name_arr.append("==");
    name_arr.append(set_name);
    constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

    Json::Value list_arr;
    list_arr.append("set_name");
    list_arr.append("set_path");
    list_arr.append("engine");
    list_arr.append("dimensions");

    tx = "MATCH (DESCSET:VDMS_descset {set_name: '" + set_name +"'})";

    results["list"] = list_arr;

    append_results_to_cypher(tx, results);
    std::cout << "Test Run" << std::endl;
    std::cout << tx << std::endl;


    return 0;

    return 0;
}

Json::Value Neo4jNeoFindDescSet::construct_responses(Json::Value &json_responses,
                                                     const Json::Value &json,
                                                     protobufs::queryMessage &response,
                                                     const std::string &blob) {
    Json::Value ret;
    return ret;
}

//ADD DESCRIPTOR
Neo4jNeoAddDesc::Neo4jNeoAddDesc() : NeoDescriptorsCommand("NeoAddDesc") {}

int Neo4jNeoAddDesc::data_processing(std::string &tx, const Json::Value &root,
                                     const std::string &blob, int grp_id,
                                     Json::Value &error) {

    return 0;
}

Json::Value Neo4jNeoAddDesc::construct_responses(Json::Value &json_responses,
                                                 const Json::Value &json,
                                                 protobufs::queryMessage &response,
                                                 const std::string &blob) {

    Json::Value ret;
    return ret;
}

//FIND DESCRIPTOR
Neo4jNeoFindDesc::Neo4jNeoFindDesc() : NeoDescriptorsCommand("NeoFindDesc") {}

bool Neo4jNeoFindDesc::need_blob(const Json::Value &cmd) {
    return cmd[_cmd_name].isMember("k_neighbors");
}

int Neo4jNeoFindDesc::data_processing(std::string &tx, const Json::Value &root,
                                      const std::string &blob, int grp_id,
                                      Json::Value &error) {

    return 0;
}

Json::Value Neo4jNeoFindDesc::construct_responses(Json::Value &json_responses,
                                                  const Json::Value &json,
                                                  protobufs::queryMessage &response,
                                                  const std::string &blob) {

    Json::Value ret;
    return ret;
}