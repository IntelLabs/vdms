/**
 * @file   DescriptorsCommand.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
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

#include <iostream>

#include "VDMSConfig.h"
#include "DescriptorsCommand.h"
#include "ExceptionsCommand.h"
#include "defines.h"

#include "vcl/utils.h"

using namespace VDMS;

DescriptorsCommand::DescriptorsCommand(const std::string& cmd_name) :
    RSCommand(cmd_name)
{
    _dm = DescriptorsManager::instance();
}

// This function only throws when there is a transaction error,
// but not when there is an input error (such as wrong set_name).
// In case of wrong input, we need to inform to the user what
// went wrong.
std::string DescriptorsCommand::get_set_path(PMGDQuery &query_tx,
                                             const std::string& set_name,
                                             int& dim)
{
    // Will issue a read-only transaction to check
    // if the Set exists
    PMGDQuery query(query_tx.get_pmgd_qh());

    Json::Value constraints, link;
    Json::Value name_arr;
    name_arr.append("==");
    name_arr.append(set_name);
    constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

    Json::Value results;
    Json::Value list_arr;
    list_arr.append(VDMS_DESC_SET_PATH_PROP);
    list_arr.append(VDMS_DESC_SET_DIM_PROP);
    results["list"] = list_arr;

    bool unique = true;

    // Query set node
    query.add_group();
    query.QueryNode(-1, VDMS_DESC_SET_TAG, link, constraints, results, unique);

    Json::Value& query_responses = query.run();

    if(query_responses.size() != 1 && query_responses[0].size() != 1) {
        throw ExceptionCommand(DescriptorSetError, "PMGD Transaction Error");
    }

    Json::Value& set_json = query_responses[0][0];

    if(!set_json.isMember("entities")) {
        throw ExceptionCommand(DescriptorSetError, "PMGD Transaction Error");
    }

    for (auto& ent : set_json["entities"]) {
        assert(ent.isMember(VDMS_DESC_SET_PATH_PROP));
        std::string set_path = ent[VDMS_DESC_SET_PATH_PROP].asString();
        dim = ent[VDMS_DESC_SET_DIM_PROP].asInt();
        return set_path;
    }

    return "";
}

bool DescriptorsCommand::check_blob_size(const std::string& blob, const int dimensions, const long n_desc)
{
    return (blob.size() / sizeof(float) / dimensions == n_desc);
}

// AddDescriptorSet Methods

AddDescriptorSet::AddDescriptorSet() :
    DescriptorsCommand("AddDescriptorSet")
{
    _storage_sets = VDMSConfig::instance()->get_path_descriptors();
}

int AddDescriptorSet::construct_protobuf(
        PMGDQuery& query,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    // Create PMGD cmd for AddNode
    int node_ref = get_value<int>(cmd, "_ref",
                                  query.get_available_reference());

    std::string set_name = cmd["name"].asString();
    std::string desc_set_path = _storage_sets + "/" + set_name;

    Json::Value props = get_value<Json::Value>(cmd, "properties");
    props[VDMS_DESC_SET_NAME_PROP] = cmd["name"].asString();
    props[VDMS_DESC_SET_DIM_PROP]  = cmd["dimensions"].asInt();
    props[VDMS_DESC_SET_PATH_PROP] = desc_set_path;

    Json::Value constraints;
    constraints[VDMS_DESC_SET_NAME_PROP].append("==");
    constraints[VDMS_DESC_SET_NAME_PROP].append(cmd["name"].asString());

    query.AddNode(node_ref, VDMS_DESC_SET_TAG, props, constraints);

    if (cmd.isMember("link")) {
        add_link(query, cmd["link"], node_ref, VDMS_DESC_SET_EDGE_TAG);
    }

    return 0;
}

Json::Value AddDescriptorSet::construct_responses(
    Json::Value &json_responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res,
    const std::string &blob)
{
    const Json::Value &cmd = json[_cmd_name];
    Json::Value resp = check_responses(json_responses);

    Json::Value ret;

    auto error = [&](Json::Value& res)
    {
        ret[_cmd_name] = res;
        return ret;
    };

    if (resp["status"] != RSCommand::Success) {
        return error(resp);
    }

    int dimensions = cmd["dimensions"].asInt();
    std::string set_name = cmd["name"].asString();
    std::string desc_set_path = _storage_sets + "/" + set_name;

    std::string metric_str = get_value<std::string>(cmd, "metric", "L2");
    VCL::DistanceMetric metric = metric_str == "L2"? VCL::L2 : VCL::IP;

    // For now, we use the default faiss index.
    std::string eng_str = get_value<std::string>(cmd, "engine", "FaissFlat");
    VCL::DescriptorSetEngine eng;

    if (eng_str == "FaissFlat")
        eng = VCL::FaissFlat;
    else if (eng_str == "FaissIVFFlat")
        eng = VCL::FaissIVFFlat;
    else if (eng_str == "TileDBDense")
        eng = VCL::TileDBDense;
    else if (eng_str == "TileDBSparse")
        eng = VCL::TileDBSparse;
    else
        throw ExceptionCommand(DescriptorSetError, "Engine not supported");

    // We can probably set up a mechanism
    // to fix a broken link when detected later, same with images.
    try {
        VCL::DescriptorSet desc_set(desc_set_path, dimensions, eng, metric);
        desc_set.store();
    }
    catch (VCL::Exception e) {
        print_exception(e);
        resp["status"]  = RSCommand::Error;
        resp["info"] = std::string("VCL Exception: ") + e.msg;
        return error(resp);
    }

    resp.clear();
    resp["status"] = RSCommand::Success;

    ret[_cmd_name] = resp;
    return ret;
}

// AddDescriptor Methods

AddDescriptor::AddDescriptor() :
    DescriptorsCommand("AddDescriptor")
{
}

long AddDescriptor::insert_descriptor(const std::string& blob,
                                     const std::string& set_path,
                                     int dim,
                                     const std::string& label,
                                     Json::Value& error)
{
    long id_first;

    try {

        VCL::DescriptorSet* desc_set = _dm->get_descriptors_handler(set_path);

        if (blob.length()/4 != dim) {
            std::cerr << "AddDescriptor::insert_descriptor: ";
            std::cerr << "Dimensions mismatch: ";
            std::cerr << blob.length()/4 << " " << dim << std::endl;
            error["info"] = "Blob Dimensions Mismatch";
            return -1;
        }

        if (!label.empty()) {
            long label_id = desc_set->get_label_id(label);
            long* label_ptr = &label_id;
            id_first = desc_set->add((float*)blob.data(), 1, label_ptr);
        }
        else {
            id_first = desc_set->add((float*)blob.data(), 1);
        }

    } catch (VCL::Exception e) {
        print_exception(e);
        error["info"] = "VCL Descriptors Exception";
        return -1;
    }

    return id_first;
}

int AddDescriptor::construct_protobuf(
        PMGDQuery& query,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    const std::string set_name = cmd["set"].asString();

    Json::Value props = get_value<Json::Value>(cmd, "properties");

    std::string label = get_value<std::string>(cmd, "label", "None");
    props[VDMS_DESC_LABEL_PROP] = label;

    int dimensions;
    std::string set_path = get_set_path(query, set_name, dimensions);

    if (set_path.empty()) {
        error["info"] = "Set " + set_name + " not found";
        error["status"] = RSCommand::Error;
        return -1;
    }

    long id = insert_descriptor(blob, set_path, dimensions, label, error);

    if (id < 0) {
        error["status"] = RSCommand::Error;
        return -1;
    }

    props[VDMS_DESC_ID_PROP] = Json::Int64(id);

    int node_ref = get_value<int>(cmd, "_ref",
                                  query.get_available_reference());

    query.AddNode(node_ref, VDMS_DESC_TAG, props, Json::nullValue);

    // It passed the checker, so it exists.
    int set_ref = query.get_available_reference();

    Json::Value link;
    Json::Value results;
    Json::Value list_arr;
    list_arr.append(VDMS_DESC_SET_PATH_PROP);
    list_arr.append(VDMS_DESC_SET_DIM_PROP);
    results["list"] = list_arr;

    Json::Value constraints;
    Json::Value name_arr;
    name_arr.append("==");
    name_arr.append(set_name);
    constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

    bool unique = true;

    // Query set node
    query.QueryNode(set_ref, VDMS_DESC_SET_TAG, link, constraints, results, unique);

    if (cmd.isMember("link")) {
        add_link(query, cmd["link"], node_ref, VDMS_DESC_EDGE_TAG);
    }

    Json::Value props_edge;
    query.AddEdge(-1, set_ref, node_ref, VDMS_DESC_SET_EDGE_TAG, props_edge);

    return 0;
}

Json::Value AddDescriptor::construct_responses(
    Json::Value &json_responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res,
    const std::string &blob)
{
    Json::Value resp = check_responses(json_responses);

    Json::Value ret;
    ret[_cmd_name] = resp;
    return ret;
}

// ClassifyDescriptors Methods

ClassifyDescriptor::ClassifyDescriptor() :
    DescriptorsCommand("ClassifyDescriptor")
{
}

int ClassifyDescriptor::construct_protobuf(
        PMGDQuery& query,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    const std::string set_name = cmd["set"].asString();

    int dimensions;
    const std::string set_path = get_set_path(query, set_name, dimensions);

    if (set_path.empty()) {
        error["status"] = RSCommand::Error;
        error["info"]   = "DescritorSet Not Found!";
        return -1;
    }

    Json::Value link;
    Json::Value results;
    Json::Value list_arr;
    list_arr.append(VDMS_DESC_SET_PATH_PROP);
    list_arr.append(VDMS_DESC_SET_DIM_PROP);
    results["list"] = list_arr;

    Json::Value constraints;
    Json::Value name_arr;
    name_arr.append("==");
    name_arr.append(set_name);
    constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

    bool unique = true;

    // Query set node
    query.QueryNode(
        get_value<int>(cmd, "_ref", -1),
        VDMS_DESC_SET_TAG,
        link, constraints, results, unique);

    return 0;
}

Json::Value ClassifyDescriptor::construct_responses(
    Json::Value &json_responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res,
    const std::string &blob)
{
    Json::Value classifyDesc;
    const Json::Value &cmd = json[_cmd_name];

    Json::Value ret;

    bool flag_error = false;

    if (json_responses.size() == 0) {
        classifyDesc["status"]  = RSCommand::Error;
        classifyDesc["info"]    = "Not Found!";
        flag_error = true;
        ret[_cmd_name] = classifyDesc;
        return ret;
    }

    for (auto res : json_responses) {

        if (res["status"] != 0) {
            flag_error = true;
            break;
        }

        if (!res.isMember("entities"))
            continue;

        classifyDesc = res;

        for (auto& ent : classifyDesc["entities"]) {

            assert(ent.isMember(VDMS_DESC_SET_PATH_PROP));
            std::string set_path = ent[VDMS_DESC_SET_PATH_PROP].asString();
            try {
                VCL::DescriptorSet* set = _dm->get_descriptors_handler(set_path);

                auto labels = set->classify((float*)blob.data(), 1);

                if (labels.size() == 0) {
                    classifyDesc["info"]   = "No labels, cannot classify";
                    classifyDesc["status"] = RSCommand::Error;
                }
                else {
                    classifyDesc["label"] = (set->label_id_to_string(labels)).at(0);
                }
            } catch (VCL::Exception e) {
                print_exception(e);
                classifyDesc["status"] = RSCommand::Error;
                classifyDesc["info"]   = "VCL Exception";
                flag_error = true;
                break;
            }
        }
    }

    if (!flag_error) {
        classifyDesc["status"] = RSCommand::Success;
    }

    classifyDesc.removeMember("entities");

    ret[_cmd_name] = classifyDesc;

    return ret;
}

// FindDescriptors Methods

FindDescriptor::FindDescriptor() :
    DescriptorsCommand("FindDescriptor")
{
}

bool FindDescriptor::need_blob(const Json::Value& cmd)
{
    return cmd[_cmd_name].isMember("k_neighbors");
}

int FindDescriptor::construct_protobuf(
        PMGDQuery& query,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& cp_result)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    const std::string set_name = cmd["set"].asString();

    int dimensions;
    const std::string set_path = get_set_path(query, set_name, dimensions);

    if (set_path.empty()) {
        cp_result["status"] = RSCommand::Error;
        cp_result["info"]   = "DescritorSet Not Found!";
        return -1;
    }

    Json::Value results_set;
    Json::Value list_arr_set;
    list_arr_set.append(VDMS_DESC_SET_PATH_PROP);
    list_arr_set.append(VDMS_DESC_SET_DIM_PROP);
    results_set["list"] = list_arr_set;

    Json::Value constraints_set;
    Json::Value name_arr;
    name_arr.append("==");
    name_arr.append(set_name);
    constraints_set[VDMS_DESC_SET_NAME_PROP] = name_arr;

    bool unique = true;

    Json::Value constraints = cmd["constraints"];
    if (constraints.isMember("_label")) {
        constraints[VDMS_DESC_LABEL_PROP] = constraints["_label"];
        constraints.removeMember("_label");
    }
    if (constraints.isMember("_id")) {
        constraints[VDMS_DESC_ID_PROP] = constraints["_id"];
        constraints.removeMember("_id");
    }

    Json::Value results = cmd["results"];

    // Add label/id as required.
    // Remove the variables with "_"
    if (results.isMember("list")) {
        int pos = -1;
        for (int i = 0; i <  results["list"].size(); ++i) {
            if (results["list"][i].asString() == "_label" ||
                results["list"][i].asString() == "_id" ||
                results["list"][i].asString() == "_distance" ) {
                pos = i;
                Json::Value aux;
                results["list"].removeIndex(i, &aux);
                --i;
            }
        }
    }

    results["list"].append(VDMS_DESC_LABEL_PROP);
    results["list"].append(VDMS_DESC_ID_PROP);

    // Case (1)
    if (cmd.isMember("link")) {

        // Query for the Descriptors related to user-defined link
        // that match the user-defined constraints
        // We will need to do the AND operation
        // on the construct_response.

        int desc_ref = get_value<int>(cmd, "_ref",
                                      query.get_available_reference());

        query.QueryNode(
            desc_ref,
            VDMS_DESC_TAG,
            cmd["link"], constraints, results, false);

        Json::Value link_to_desc;
        link_to_desc["ref"] = desc_ref;

        // Query for the set
        query.QueryNode(
            -1,
            VDMS_DESC_SET_TAG,
            link_to_desc, constraints_set, results_set, unique);
    }
    // Case (2)
    else if (!cmd.isMember("k_neighbors")) {

        // In this case, we either need properties of the descriptor
        // ("list") on the results block, or we need the descriptor nodes
        // because the user defined a reference.

        int ref_set = query.get_available_reference();

        Json::Value link_set; // null

        // Query for the set
        query.QueryNode(
            ref_set,
            VDMS_DESC_SET_TAG,
            link_set, constraints_set, results_set, unique);

        Json::Value link_to_set;
        link_to_set["ref"] = ref_set;

        // Query for the Descriptors related to that set
        // that match the user-defined constraints
        query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            VDMS_DESC_TAG,
            link_to_set, constraints, results, false);
    }
    // Case (3), Just want the descriptor by value, we only need the set
    else {
        Json::Value link_null; // null

        const int k_neighbors = get_value<int>(cmd, "k_neighbors", 0);

        int ref_set = query.get_available_reference();

        // Query for the set and detect if exist during transaction.
        query.QueryNode(
            ref_set,
            VDMS_DESC_SET_TAG,
            Json::nullValue, constraints_set, results_set, true);

        Json::Value link_to_set;
        link_to_set["ref"] = ref_set;

        if (!check_blob_size(blob, dimensions, 1)) {
            cp_result["status"] = RSCommand::Error;
            cp_result["info"] = "Blob (required) is null or size invalid";
            return -1;
        }

        try {
            VCL::DescriptorSet* set = _dm->get_descriptors_handler(set_path);

            // This is a way to pass state to the construct_response
            // We just pass the cache_object_id.
            auto cache_obj_id = VCL::get_uint64();
            cp_result["cache_obj_id"] = Json::Int64(cache_obj_id);

            _cache_map[cache_obj_id] = new IDDistancePair();

            IDDistancePair* pair = _cache_map[cache_obj_id];
            std::vector<long>&  ids       = pair->first;
            std::vector<float>& distances = pair->second;

            set->search((float*)blob.data(), 1, k_neighbors, ids, distances);

            long returned_counter = 0;
            std::string blob_return;

            Json::Value ids_array;

            for (int i = 0; i < ids.size(); ++i) {
                if (ids[i] >= 0) {
                    ids_array.append(Json::Int64(ids[i]));
                }
                else {
                    ids.erase(ids.begin() + i, ids.end());
                    distances.erase(distances.begin() + i, distances.end());
                    break;
                }
            }

            // This are needed to construct the response.
            if (!results.isMember("list")) {
                results["list"].append(VDMS_DESC_LABEL_PROP);
                results["list"].append(VDMS_DESC_ID_PROP);
            }

            Json::Value node_constraints = constraints;
            node_constraints[VDMS_DESC_ID_PROP].append("==");
            node_constraints[VDMS_DESC_ID_PROP].append(ids_array);

            query.QueryNode(
                get_value<int>(cmd, "_ref", -1),
                VDMS_DESC_TAG,
                link_to_set, node_constraints, results, false);

        } catch (VCL::Exception e) {
            print_exception(e);
            cp_result["status"] = RSCommand::Error;
            cp_result["info"]   = "VCL Exception";
            return -1;
        }
    }

    return 0;
}

void FindDescriptor::populate_blobs(const std::string& set_path,
                                    const Json::Value& results,
                                    Json::Value& entities,
                                    protobufs::queryMessage &query_res)
{
    if (get_value<bool>(results, "blob", false)) {

        VCL::DescriptorSet* set = _dm->get_descriptors_handler(set_path);
        int dim = set->get_dimensions();

        for (auto& ent : entities) {
            long id = ent[VDMS_DESC_ID_PROP].asInt64();

            ent["blob"] = true;

            std::string* desc_blob = query_res.add_blobs();
            desc_blob->resize(sizeof(float) * dim);

            set->get_descriptors(&id, 1,(float*)(*desc_blob).data());
        }
    }
}

void FindDescriptor::convert_properties(Json::Value& entities,
                                        Json::Value& list)
{
    bool flag_label = false;
    bool flag_id    = false;

    for (auto& prop : list) {
        if (prop.asString() == "_label") {
            flag_label = true;
        }
        if (prop.asString() == "_id") {
            flag_id = true;
        }
    }

    for (auto& element : entities) {

        if (element.isMember(VDMS_DESC_LABEL_PROP)) {
            if (flag_label)
                element["_label"] = element[VDMS_DESC_LABEL_PROP];
            element.removeMember(VDMS_DESC_LABEL_PROP);
        }
        if (element.isMember(VDMS_DESC_ID_PROP)) {
            if (flag_id)
                element["_id"] = element[VDMS_DESC_ID_PROP];
            element.removeMember(VDMS_DESC_ID_PROP);
        }
    }
}

Json::Value FindDescriptor::construct_responses(
    Json::Value &json_responses,
    const Json::Value &json,
    protobufs::queryMessage &query_res,
    const std::string &blob)
{
    Json::Value findDesc;
    const Json::Value &cmd   = json[_cmd_name];
    const Json::Value &cache = json["cp_result"];

    Json::Value ret;

    bool flag_error = false;

    auto error = [&](Json::Value& res)
    {
        ret[_cmd_name] = res;
        return ret;
    };

    if (json_responses.size() == 0) {
        Json::Value return_error;
        return_error["status"]  = RSCommand::Error;
        return_error["info"]    = "Not Found!";
        return error(return_error);
    }

    const Json::Value& results = cmd["results"];
    Json::Value list = get_value<Json::Value>(results, "list");

    // Case (1)
    if (cmd.isMember("link")) {

        assert(json_responses.size() == 2);

        findDesc = json_responses[0];

        if (findDesc["status"] != 0) {
            Json::Value return_error;
            return_error["status"]  = RSCommand::Error;
            return_error["info"]    = "Descriptors Not Found";
            return error(return_error);
        }

        const Json::Value& set_response = json_responses[1];
        const Json::Value& set = set_response["entities"][0];

        // These properties should always exist
        assert(set.isMember(VDMS_DESC_SET_PATH_PROP));
        assert(set.isMember(VDMS_DESC_SET_DIM_PROP));
        std::string set_path = set[VDMS_DESC_SET_PATH_PROP].asString();
        int dim = set[VDMS_DESC_SET_DIM_PROP].asInt();

        if (findDesc.isMember("entities")) {
            try {
                Json::Value& entities = findDesc["entities"];
                populate_blobs(set_path, results, entities, query_res);
                convert_properties(entities, list);
            } catch (VCL::Exception e) {
                print_exception(e);
                findDesc["status"] = RSCommand::Error;
                findDesc["info"]   = "VCL Exception";
                return error(findDesc);
            }
        }
    }
    // Case (2)
    else if (!cmd.isMember("k_neighbors")) {

        assert(json_responses.size() == 2);

        const Json::Value& set_response = json_responses[0];
        const Json::Value& set = set_response["entities"][0];

        // These properties should always exist
        assert(set.isMember(VDMS_DESC_SET_PATH_PROP));
        assert(set.isMember(VDMS_DESC_SET_DIM_PROP));
        std::string set_path = set[VDMS_DESC_SET_PATH_PROP].asString();
        int dim = set[VDMS_DESC_SET_DIM_PROP].asInt();

        findDesc = json_responses[1];

        if (findDesc.isMember("entities")) {
            try {
                Json::Value& entities = findDesc["entities"];
                populate_blobs(set_path, results, entities, query_res);
                convert_properties(entities, list);
            } catch (VCL::Exception e) {
                print_exception(e);
                findDesc["status"] = RSCommand::Error;
                findDesc["info"]   = "VCL Exception";
                return error(findDesc);
            }
        }

        if (findDesc["status"] != 0) {
            std::cerr << json_responses.toStyledString() << std::endl;
            Json::Value return_error;
            return_error["status"]  = RSCommand::Error;
            return_error["info"]    = "Descriptors Not Found";
            return error(return_error);
        }
    }
    // Case (3)
    else{

        assert(json_responses.size() == 2);

        // Get Set info.
        const Json::Value& set_response = json_responses[0];

        if (set_response["status"] != 0 ||
            !set_response.isMember("entities")) {

            Json::Value return_error;
            return_error["status"]  = RSCommand::Error;
            return_error["info"]    = "Set Not Found";
            return error(return_error);
        }

        assert(set_response["entities"].size() == 1);

        const Json::Value& set = set_response["entities"][0];

        // This properties should always exist
        assert(set.isMember(VDMS_DESC_SET_PATH_PROP));
        assert(set.isMember(VDMS_DESC_SET_DIM_PROP));
        std::string set_path = set[VDMS_DESC_SET_PATH_PROP].asString();
        int dim = set[VDMS_DESC_SET_DIM_PROP].asInt();

        if (!check_blob_size(blob, dim, 1)) {
            Json::Value return_error;
            return_error["status"] = RSCommand::Error;
            return_error["info"] = "Blob (required) is null or size invalid";
            return error(return_error);
        }

        std::vector<long>*  ids;
        std::vector<float>* distances;

        bool compute_distance = false;

        Json::Value list = get_value<Json::Value>(results, "list");

        for (auto& prop : list) {
            if (prop.asString() == "_distance") {
                compute_distance = true;
                break;
            }
        }

        // Test whether there is any cached result.
        assert(cache.isMember("cache_obj_id"));

        long cache_obj_id = cache["cache_obj_id"].asInt64();

        // Get from Cache
        IDDistancePair* pair = _cache_map[cache_obj_id];
        ids       = &(pair->first);
        distances = &(pair->second);

        findDesc = json_responses[1];

        if (findDesc["status"] != 0 || !findDesc.isMember("entities")) {

            Json::Value return_error;
            return_error["status"]  = RSCommand::Error;
            return_error["info"]    = "Descriptor Not Found in graph!";
            return error(return_error);
        }

        Json::Value aux_entities = findDesc["entities"];
        findDesc.removeMember("entities");

        for (int i = 0; i < (*ids).size(); ++i) {

            Json::Value desc_data;

            long d_id = (*ids)[i];
            bool pass_constraints = false;

            for (auto ent : aux_entities) {
                if (ent[VDMS_DESC_ID_PROP].asInt64() == d_id) {
                    desc_data = ent;
                    pass_constraints = true;
                    break;
                }
            }

            if (!pass_constraints)
                continue;

            if (compute_distance) {
                desc_data["_distance"] = (*distances)[i];

                // Should be already sorted,
                // but if not, we need to match the id with
                // whatever is on the cache
                // desc_data["cache_id"]  = Json::Int64((*ids)[i]);
            }

            findDesc["entities"].append(desc_data);
        }

        if (findDesc.isMember("entities")) {
            try {
                Json::Value& entities = findDesc["entities"];
                populate_blobs(set_path, results, entities, query_res);
                convert_properties(entities, list);
            } catch (VCL::Exception e) {
                print_exception(e);
                findDesc["status"] = RSCommand::Error;
                findDesc["info"]   = "VCL Exception";
                return error(findDesc);
            }
        }

        if (cache.isMember("cache_obj_id")) {
            // We remove the vectors associated with that entry to
            // free memory, without removing the entry from _cache_map
            // because tbb does not have a lock free way to do this.
            IDDistancePair* pair = _cache_map[cache["cache_obj_id"].asInt64()];
            delete pair;
        }
    }

    if (findDesc.isMember("entities")) {
        for (auto& ent : findDesc["entities"]) {
            if (ent.getMemberNames().size() == 0) {
                findDesc.removeMember("entities");
                break;
            }
        }
    }

    findDesc["status"] = RSCommand::Success;
    ret[_cmd_name] = findDesc;

    return ret;
}
