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

using namespace VDMS;

DescriptorsCommand::DescriptorsCommand(const std::string& cmd_name) :
    RSCommand(cmd_name)
{
    _dm = DescriptorsManager::instance();
}

std::string DescriptorsCommand::check_set(const std::string& set_name, int& dim)
{
    // Will issue a read-only transaction to check
    // if the Set exists
    PMGDQuery query(*_pmgd_qh);

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
        // TODO, WE NEED TO HANDLE THIS ERROR BETTER
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

// AddDescriptorSet Methods

AddDescriptorSet::AddDescriptorSet() :
    DescriptorsCommand("AddDescriptorSet")
{
    _storage_sets = VDMSConfig::instance()
                ->get_string_value("descriptors", DEFAULT_DESCRIPTORS_PATH);
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
    int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

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
        error(resp);
    }

    int dimensions = cmd["dimensions"].asInt();
    std::string set_name = cmd["name"].asString();
    std::string desc_set_path = _storage_sets + "/" + set_name;

    // This can be a problem. If this fails, we will have a node in pmgd
    // linking to this broken directory. We can probably set up a mechanism
    // to fix a broken link when detected later.
    // For now, we use the defaul faiss index.
    try {
        VCL::DescriptorSet desc_set(desc_set_path, dimensions);
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

int AddDescriptor::insert_descriptor(const std::string& blob,
                                     const std::string& set_path,
                                     int dim,
                                     const std::string& label,
                                     Json::Value& error)
{
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
            long id = desc_set->get_label_id(label);
            long* label_ptr = &id;
            desc_set->add((float*)blob.data(), 1, label_ptr);
        }
        else {
            desc_set->add((float*)blob.data(), 1);
        }


    } catch (VCL::Exception e) {
        print_exception(e);
        error["info"] = "VCL Descriptors Exception";
        return -1;
    }

    return 0;
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

    int node_ref = get_value<int>(cmd, "_ref",
                                    query.get_available_reference());

    Json::Value props = get_value<Json::Value>(cmd, "properties");

    std::string label;
    if (cmd.isMember("label")) {
        label = cmd["label"].asString();
        props[VDMS_DESC_LABEL_PROP] = label;
    }

    int dimensions;
    std::string set_path = check_set(set_name, dimensions);

    if (set_path.empty()) {
        error["info"] = "Set " + set_name + " not found";
        error["status"] = RSCommand::Error;
        return -1;
    }

    int iret = insert_descriptor(blob, set_path, dimensions, label, error);

    if (iret != 0) {
        error["status"] = RSCommand::Error;
        return -1;
    }

    query.AddNode(node_ref, VDMS_DESC_TAG, props, Json::Value());

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
    // Add edge between collection and image
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

    // This control goes away.
    int dimensions;
    const std::string set_path = check_set(set_name, dimensions);
    if (set_path.empty()) {
        // todo throw
        // set the error json struct
        std::cerr << "Set non-existent: " << set_name << std::endl;
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

                for (auto l : labels) {
                    std::cerr << "Label: " << l << std::endl;
                }

                std::vector<long> ids;
                std::vector<float> distances;
                set->search((float*)blob.data(), 1, 1, ids, distances);
                // std::cerr << "Distance: " << distances.at(0) << std::endl;
                // std::cerr << (set->get_labels(labels)).at(0) << std::endl;

                if (labels.size() == 0) {
                    classifyDesc["info"]   = "No labels, cannot classify";
                    classifyDesc["status"] = RSCommand::Error;
                }
                else {
                    // std::cerr << "todo ok" << std::endl;
                    // std::cerr << labels.size() << std::endl;
                    // std::cerr << labels.at(0) << std::endl;
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
