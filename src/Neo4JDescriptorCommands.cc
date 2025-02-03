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

void append_results_to_cypher(std::string &tx, std::string varnam, Json::Value &results){

    tx += " return ";
    for (Json::Value::ArrayIndex i = 0; i != results["list"].size(); i++){
        if(i == results["list"].size() -1) {
            tx += varnam + "." + results["list"][i].asString() + ";";
        } else {
            tx += varnam + "." + results["list"][i].asString() + ", ";
        }
    }
}

void append_and_constraints(std::string &tx, std::string varnam, Json::Value &constraints, bool leading_and){

    //TODO this is working, we should have confidence that each value associated with a particular
    // value will be in a consistent format
    //outerloop extracts the property names we are creating constraints against
    std::string cons_string = "";
    int ctr = 0;
    for (auto it = constraints.begin(); it != constraints.end(); it++){
        std::string prop_key = it.key().asString();
        auto cur_list = constraints[prop_key];


        // inner loop(s) we look at the size of the contraints list
        // to determine how we're assembling the cypher query (upper and lower bound vs upper XOR lower bound XOR equality)
        int list_sz = cur_list.size();

        if(leading_and || ctr != 0) cons_string += " AND ";
        ctr += 1;

        if(list_sz == 2) {
            auto eq_1 = cur_list[0];
            auto val_1 = cur_list[1];

            std::string eq_1_str;

            if(eq_1.asString() == "=="){
                eq_1_str = "=";
            } else {
                eq_1_str = eq_1.asString();
            }

            cons_string += varnam + "." + prop_key + " " + eq_1_str + " " + val_1.asString();

        } else if (list_sz == 4){
            auto eq_1 = cur_list[0];
            auto val_1 = cur_list[1];
            auto eq_2 = cur_list[2];
            auto val_2 = cur_list[3];

            std::string eq_1_str;
            std::string eq_2_str;

            if(eq_1.asString() == "=="){
                eq_1_str = "=";
            } else {
                eq_1_str = eq_1.asString();
            }

            if(eq_2.asString() == "=="){
                eq_2_str = "=";
            } else {
                eq_2_str = eq_2.asString();
            }

            cons_string += varnam + "." + prop_key + " " + eq_1_str + " " + val_1.asString() + " AND ";
            cons_string += varnam + "." + prop_key + " " + eq_2_str + " " + val_2.asString() + " ";
        }

    }

    tx += cons_string;

}

void Neo4jNeoFindDesc::populate_blobs(const std::string &set_path,
                                    std::string set_name,
                                    const Json::Value &results,
                                    Json::Value &entities,
                                    protobufs::queryMessage &query_res) {

    std::string desc_id_prop_name =
            VDMS_DESC_ID_PROP + std::string("_") + set_name;
    if (get_value<bool>(results, "blob", false)) {
        VCL::DescriptorSet *set = _dm->get_descriptors_handler(set_path);
        int dim = set->get_dimensions();

        for (auto &ent : entities) {
            long id = ent[desc_id_prop_name].asInt64();

            ent["blob"] = true;

            std::string *desc_blob = query_res.add_blobs();
            desc_blob->resize(sizeof(float) * dim);

            set->get_descriptors(&id, 1, (float *)(*desc_blob).data());
            if (output_vcl_timing) {
                set->timers.print_map_runtimes();
            }
            set->timers.clear_all_timers();
        }
    }
}

void Neo4jNeoFindDesc::convert_properties(Json::Value &entities,
                                        Json::Value &list,
                                        std::string set_name) {
    bool flag_label = false;
    bool flag_id = false;

    std::string desc_id_prop_name =
            VDMS_DESC_ID_PROP + std::string("_") + set_name;
    for (auto &prop : list) {
        if (prop.asString() == "_label") {
            flag_label = true;
        }
        if (prop.asString() == "_id") {
            flag_id = true;
        }
    }

    for (auto &element : entities) {

        if (element.isMember(VDMS_DESC_LABEL_PROP)) {
            if (flag_label)
                element["_label"] = element[VDMS_DESC_LABEL_PROP];
            element.removeMember(VDMS_DESC_LABEL_PROP);
        }
        if (element.isMember(desc_id_prop_name)) {
            if (flag_id)
                element["_id"] = element[desc_id_prop_name];
            element.removeMember(desc_id_prop_name);
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
        std::cerr << "Find Set query Failed!" << std::endl;
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

    append_results_to_cypher(tx,"DESCSET", results);


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
Neo4jNeoAddDesc::Neo4jNeoAddDesc() : NeoDescriptorsCommand("NeoAddDescriptor") {}

// update to handle multiple descriptors at a go
long Neo4jNeoAddDesc::insert_descriptor(const std::string &blob,
                                      const std::string &set_path, int nr_desc,
                                      const std::string &label,
                                      Json::Value &error) {
    long id_first;

    try {

        VCL::DescriptorSet *desc_set = _dm->get_descriptors_handler(set_path);

        if (!label.empty()) {
            long label_id = desc_set->get_label_id(label);
            long *label_ptr = &label_id;
            id_first = desc_set->add((float *)blob.data(), nr_desc, label_ptr);

        } else {
            id_first = desc_set->add((float *)blob.data(), nr_desc);
        }

        //TODO reintegrate timers
        /*if (output_vcl_timing) {
            desc_set->timers.print_map_runtimes();
        }
        desc_set->timers.clear_all_timers();*/
    } catch (VCL::Exception e) {
        print_exception(e);
        error["info"] = "VCL Descriptors Exception";
        return -1;
    }

    return id_first;
}

int Neo4jNeoAddDesc::add_single_descriptor(std::string &tx,
                                         const Json::Value &jsoncmd,
                                         const std::string &blob, int grp_id,
                                         Json::Value &error) {
    const Json::Value &cmd = jsoncmd[_cmd_name];
    const std::string set_name = cmd["set"].asString();

    Json::Value props = get_value<Json::Value>(cmd, "properties");

    //TODO named constants for system generated property labels
    std::string label = get_value<std::string>(cmd, "label", "null");
    props["VD_label"] = label;

    int dim;
    const std::string set_path = get_set_path(set_name, dim);

    if (set_path.empty()) {
        error["info"] = "Set " + set_name + " not found";
        error["status"] = Neo4jCommand::Error;
        return -1;
    }

    if (blob.length() / 4 != dim) {
        std::cerr << "AddDescriptor::insert_descriptor: ";
        std::cerr << "Dimensions mismatch: ";
        std::cerr << blob.length() / 4 << " " << dim << std::endl;
        error["status"] = Neo4jCommand::Error;
        error["info"] = "Blob Dimensions Mismatch";
        return -1;
    }

    long id = insert_descriptor(blob, set_path, 1, label, error);

    if (id < 0) {
        error["status"] = Neo4jCommand::Error;
        return -1;
    }

    std::string desc_id_prop_name =
            "VD_descId_" + set_name;
    props[desc_id_prop_name] = Json::Int64(id);

    //query.AddNode(node_ref, VDMS_DESC_TAG, props, Json::nullValue);
    //Add properties to query
    tx += "MATCH (descset:VDMS_descset) WHERE descset.set_name = \"" + set_name + "\" ";
    tx += "CREATE (VDMSNODE:VDMS_desc { ";

    // append properties
    int ctr = 0;
    for (Json::Value::iterator it=props.begin(); it != props.end(); it++){
        Json::Value key = it.key();
        Json::Value value = (*it);

        if(ctr == 0) {
            tx += key.asString() + ": " + value.asString();
        } else {
            tx += ", " + key.asString() + ": " + value.asString();
        }
        ctr++;
    }

    tx += "})-[:part_of]->(descset)";
    std::cout << tx << std::endl;
    return 0;
}

int Neo4jNeoAddDesc::add_descriptor_batch(std::string &tx,
                                        const Json::Value &jsoncmd,
                                        const std::string &blob, int grp_id,
                                        Json::Value &error) {

    const int FOUR_BYTE_INT = 4;
    int expected_blb_size;
    int nr_expected_descs;
    int dimensions;

    // Extract set name
    const Json::Value &cmd = jsoncmd[_cmd_name];
    const std::string set_name = cmd["set"].asString();

    Json::Value props = get_value<Json::Value>(cmd, "properties");

    // extract properties list and get filepath/object location of set
    Json::Value prop_list = get_value<Json::Value>(cmd, "batch_properties");
    const std::string set_path = get_set_path(set_name, dimensions);

    if (set_path.empty()) {
        error["info"] = "Set " + set_name + " not found";
        error["status"] = Neo4jCommand::Error;
        return -1;
    }

    std::string label = get_value<std::string>(cmd, "label", "null");

    // Note dimensionse are based on a 32 bit integer, hence the /4 math on size
    // as the string blob is sized in 8 bit ints.
    nr_expected_descs = prop_list.size();
    expected_blb_size = nr_expected_descs * dimensions * FOUR_BYTE_INT;

    // Verify length of input is matching expectations
    if (blob.length() != expected_blb_size) {
        std::cerr << "AddDescriptor::insert_descriptor: ";
        std::cerr << "Expected Blob Length Does Not Match Input ";
        std::cerr << "Input Length: " << blob.length() << " != "
                  << "Expected Length: " << expected_blb_size << std::endl;
        error["info"] = "FV Input Length Mismatch";
        return -1;
    }

    long id = insert_descriptor(blob, set_path, nr_expected_descs, label, error);

    //TODO convert to cypher
    std::string desc_id_prop_name =
            "VD_descId_" + set_name;
    tx += "MATCH (descset:VDMS_descset) WHERE descset.set_name =\"" + set_name +"\" ";
    ;
    for (int i = 0; i < nr_expected_descs; i++) {
        Json::Value cur_props;
        cur_props = prop_list[i];
        tx += "CREATE (:VDMS_desc { ";

        cur_props[desc_id_prop_name.c_str()] = Json::Int64(id + i);
        cur_props["VD_label"] = label;

        // append properties
        int ctr = 0;
        for (Json::Value::iterator it=cur_props.begin(); it != cur_props.end(); it++){
            Json::Value key = it.key();
            Json::Value value = (*it);

            if(ctr == 0) {
                tx += key.asString() + ": " + value.asString();
            } else {
                tx += ", " + key.asString() + ": " + value.asString();
            }
            ctr++;
        }
        tx += "})-[:part_of]->(descset) ";
    }

    std::cout << tx << std::endl;
    return 0;
}

int Neo4jNeoAddDesc::data_processing(std::string &tx, const Json::Value &root,
                                     const std::string &blob, int grp_id,
                                     Json::Value &result) {

    bool batch_mode;
    int rc;
    const Json::Value &cmd = root[_cmd_name];
    const std::string set_name = cmd["set"].asString();

    Json::Value prop_list = get_value<Json::Value>(cmd, "batch_properties");
    if (prop_list.size() == 0) {
        rc = add_single_descriptor(tx, root, blob, grp_id, result);
    } else {
        rc = add_descriptor_batch(tx, root, blob, grp_id, result);
    }

    if (rc < 0) {
        result["status"] = Neo4jCommand::Error;
    } else {
        result["status"] = Neo4jCommand::Success;
    }
    return rc;
}

Json::Value Neo4jNeoAddDesc::construct_responses(Json::Value &json_responses,
                                                 const Json::Value &json,
                                                 protobufs::queryMessage &response,
                                                 const std::string &blob) {

    Json::Value ret;

    return ret;
}

//FIND DESCRIPTOR
Neo4jNeoFindDesc::Neo4jNeoFindDesc() : NeoDescriptorsCommand("NeoFindDescriptor") {}

bool Neo4jNeoFindDesc::need_blob(const Json::Value &cmd) {
    return cmd[_cmd_name].isMember("k_neighbors");
}

bool NeoDescriptorsCommand::check_blob_size(const std::string &blob,
                                         const int dimensions,
                                         const long n_desc) {
    return (blob.size() / sizeof(float) / dimensions == n_desc);
}

int Neo4jNeoFindDesc::data_processing(std::string &tx, const Json::Value &root,
                                      const std::string &blob, int grp_id,
                                      Json::Value &error) {

    const Json::Value &cmd = root[_cmd_name];

    const std::string set_name = cmd["set"].asString();

    int dimensions;
    const std::string set_path = get_set_path(set_name, dimensions);

    std::string desc_id_prop_name =
            "VD_descId_" + set_name;

    if (set_path.empty()) {
        error["info"] = "Set " + set_name + " not found";
        error["status"] = Neo4jCommand::Error;
        return -1;
    }

    Json::Value constraints = cmd["constraints"];
    if (constraints.isMember("_label")) {
        constraints[VDMS_DESC_LABEL_PROP] = constraints["_label"];
        constraints.removeMember("_label");
    }
    if (constraints.isMember("_id")) {
        constraints[desc_id_prop_name.c_str()] = constraints["_id"];
        constraints.removeMember("_id");
    }

    Json::Value results = cmd["results"];
    std::vector<long> ids;
    std::vector<float> distances;

    // Add label/id as required.
    // Remove the variables with "_"
    if (results.isMember("list")) {
        int pos = -1;
        for (int i = 0; i < results["list"].size(); ++i) {
            if (results["list"][i].asString() == "_label" ||
                results["list"][i].asString() == "_id" ||
                results["list"][i].asString() == "_distance") {
                pos = i;
                Json::Value aux;
                results["list"].removeIndex(i, &aux);
                --i;
            }
        }
    }

    //TODO DESCRIPTOR LABEL PROPERTY
    //results["list"].append(VDMS_DESC_LABEL_PROP);
    results["list"].append(desc_id_prop_name.c_str());

    // Case (1)
    if (cmd.isMember("link")) {
        //TODO link is currently not supported

        // Query for the Descriptors related to user-defined link
        // that match the user-defined constraints
        // We will need to do the AND operation
        // on the construct_response.
        /*
        int desc_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

        query.QueryNode(desc_ref, VDMS_DESC_TAG, cmd["link"], constraints, results,
                        false);

        Json::Value link_to_desc;
        link_to_desc["ref"] = desc_ref;*/
    }
        // Case (2)
    else if (!cmd.isMember("k_neighbors")) {
        // In this case, we either need properties of the descriptor
        // ("list") on the results block, or we need the descriptor nodes
        // because the user defined a reference.
        //TODO need to verify match against set

        tx = "MATCH (n:VDMS_desc) WHERE ";
        //Add in additional constraints (all AND-ed together), this currently matches PMGD model
        append_and_constraints(tx, "n", constraints, false);
        tx += " AND n." + desc_id_prop_name + " IS NOT NULL ";
        //append returns
        append_results_to_cypher(tx,"n", results);


        std::cout << tx << std::endl;
    } else {

        Json::Value link_null; // null
        const int k_neighbors = get_value<int>(cmd, "k_neighbors", 0);


        if (!check_blob_size(blob, dimensions, 1)) {
            error["status"] = Neo4jCommand::Error;
            error["info"] = "Blob (required) is null or size invalid";
            return -1;
        }

        try {
            VCL::DescriptorSet *set = _dm->get_descriptors_handler(set_path);

            set->search((float *)blob.data(), 1, k_neighbors, ids, distances);

            auto cache_obj_id = VCL::get_uint64();
            //note "error" is just a piece of JSON state that persists beyond this
            //call so we can use it to transfer info to construct_response
            error["cache_obj_id"] = Json::Int64(cache_obj_id);

            _cache_map[cache_obj_id] = new IDDistancePair();

            IDDistancePair *pair = _cache_map[cache_obj_id];
            std::vector<long> &ids = pair->first;
            std::vector<float> &distances = pair->second;

            set->search((float *)blob.data(), 1, k_neighbors, ids, distances);

            long returned_counter = 0;
            std::string blob_return;

            Json::Value ids_array;

            for (int i = 0; i < ids.size(); ++i) {
                if (ids[i] >= 0) {
                    ids_array.append(Json::Int64(ids[i]));
                } else {
                    ids.erase(ids.begin() + i, ids.end());
                    distances.erase(distances.begin() + i, distances.end());
                    break;
                }
            }

            error["ids_array"] = ids_array;
            // This are needed to construct the response.
            if (!results.isMember("list")) {
                results["list"].append(VDMS_DESC_LABEL_PROP);
                results["list"].append(desc_id_prop_name);
            }

            Json::Value node_constraints = constraints;
            tx = "MATCH (n:VDMS_desc) WHERE n." + desc_id_prop_name + " IN [";
            //creates an OR list for the query

            for (int i = 0; i < ids.size() - 1; ++i) {
                tx += std::to_string(ids[i]) + ",";
            }
            tx += std::to_string(ids[ids.size()-1]) + "] ";

            //Add in additional constraints (all AND-ed together), this currently matches PMGD model
            append_and_constraints(tx, "n", constraints, true);

            //append returns
            append_results_to_cypher(tx,"n", results);
            std::cout << tx << std::endl;

        } catch (VCL::Exception e) {
            print_exception(e);
            error["status"] = Neo4jCommand::Error;
            error["info"] = "VCL Exception";
            return -1;
        }
    }
    return 0;
}

Json::Value Neo4jNeoFindDesc::construct_responses(Json::Value &neo4j_responses,
                                                  const Json::Value &orig_query,
                                                  protobufs::queryMessage &query_res,
                                                  const std::string &blob) {



    Json::Value findDesc;
    const Json::Value &cmd = orig_query[_cmd_name];
    const Json::Value &cache = orig_query["cp_result"];

    Json::Value ret;

    bool flag_error = false;

    const std::string set_name = cmd["set"].asString();
    std::string desc_id_prop_name =
            "VD_descId_" + set_name;

    auto error = [&](Json::Value &res) {
        ret[_cmd_name] = res;
        return ret;
    };

    if (neo4j_responses.size() == 0) {
        Json::Value return_error;
        return_error["status"] = Neo4jCommand::Error;
        return_error["info"] = "Not Found!";
        return error(return_error);
    }

    const Json::Value &results = cmd["results"];
    Json::Value res_list = get_value<Json::Value>(results, "list");

    int dim;
    const std::string set_path = get_set_path(set_name, dim);

    // Case (1)
    if (cmd.isMember("link")) {

        //TODO link is currently not supported
    }
        // Case (2)
    else if (!cmd.isMember("k_neighbors")) {

        //iterate over metadata returns
        Json::Value md_list = neo4j_responses["metadata_res"];
        Json::Value resp_list;
        for(Json::Value::ArrayIndex i = 0; i != md_list.size(); i++){
            Json::Value cur_obj;
            cur_obj = md_list[i];
            Json::Value resp_obj;
            long desc_id = cur_obj["n." + desc_id_prop_name].asInt();

            //iterate over desired results and extract
            for(Json::Value::ArrayIndex k = 0; k != res_list.size(); k++){
                std::string res_str = res_list[k].asString();
                resp_obj[res_str] = cur_obj["n." + res_str];
            }
            resp_list.append(resp_obj);
        }

        findDesc["status"] = Neo4jCommand::Success;
        findDesc["entities"] = resp_list;
        ret[_cmd_name] = findDesc;
        printf("CASE 2 RETURN\n");
        std::cout << ret <<std::endl;
        std::cout << md_list << std::endl;

    } else { // Case (3)

        // Get Set info.
        //const Json::Value &set_response = neo4j_responses[0];
        const Json::Value &metadata_desc_res = neo4j_responses["metadata_res"];

        //TODO error check and verify we got some metadata back
        assert(metadata_desc_res.size() >= 1);


        /*if (!check_blob_size(blob, dim, 1)) {
            Json::Value return_error;
            return_error["status"] = Neo4jCommand::Error;
            return_error["info"] = "Blob (required) is null or size invalid";
            return error(return_error);
        }*/

        std::vector<long> *ids;
        std::vector<float> *distances;

        //TODO Keep this, as it bolts distance onto the results
        bool compute_distance = false;
        for (auto &prop: res_list) {
            if (prop.asString() == "_distance") {
                compute_distance = true;
                break;
            }
        }


        // Test whether there is any cached result.
        assert(cache.isMember("cache_obj_id"));
        long cache_obj_id = cache["cache_obj_id"].asInt64();

        assert(cache.isMember("ids_array"));
        Json::Value ids_array = cache["ids_array"];

        // Get from Cache
        IDDistancePair *pair = _cache_map[cache_obj_id];
        ids = &(pair->first);
        distances = &(pair->second);

        uint64_t new_cnt = 0;

        //Loop over IDs, match distances to metadata returns
        std::map<long, float> id_dist_map;
        for (int i = 0; i < (*ids).size(); ++i) {
            long d_id = (*ids)[i];
            float cur_dist = (*distances)[i];
            id_dist_map[d_id] = cur_dist;
        }

        //clean up cache used to hand off distances
        if (cache.isMember("cache_obj_id")) {
            // We remove the vectors associated with that entry to
            // free memory, without removing the entry from _cache_map
            // because tbb does not have a lock free way to do this.
            IDDistancePair *pair = _cache_map[cache["cache_obj_id"].asInt64()];
            delete pair;
        }

        //iterate over metadata returns
        Json::Value md_list = neo4j_responses["metadata_res"];
        Json::Value resp_list;
        for(Json::Value::ArrayIndex i = 0; i != md_list.size(); i++){
            Json::Value cur_obj;
            cur_obj = md_list[i];
            Json::Value resp_obj;
            long desc_id = cur_obj["n." + desc_id_prop_name].asInt();
            float dist = id_dist_map[desc_id];


            //iterate over desired results and extract
            for(Json::Value::ArrayIndex k = 0; k != res_list.size(); k++){
                std::string res_str = res_list[k].asString();

                if(res_str == "_id"){
                    resp_obj["_id"] = desc_id;
                    continue;
                }
                if(res_str == "_distance"){
                    resp_obj["_distance"] = dist;
                    continue;
                }

                resp_obj[res_str] = cur_obj["n." + res_str];
            }
            resp_list.append(resp_obj);
        }


        findDesc["status"] = Neo4jCommand::Success;
        findDesc["entities"] = resp_list;
        ret[_cmd_name] = findDesc;

    }

    //Now populate the blobs as needed
    //TODO metadata existence check from Neo4J returns
    try {
        Json::Value &entities = findDesc["entities"];
        populate_blobs(set_path, set_name, results, entities, query_res);
        //convert_properties(entities, list, set_name);
    } catch (VCL::Exception e) {
        print_exception(e);
        findDesc["status"] = Neo4jCommand::Error;
        findDesc["info"] = "VCL Exception";
        return error(findDesc);
    }

    return ret;
}