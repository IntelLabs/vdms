#include <string>
#include <iostream>

#include "QueryHandler.h"
#include "ExceptionsCommand.h"

using namespace athena;

RSCommand::RSCommand(const std::string& cmd_name):
    _cmd_name(cmd_name)
{
}

bool RSCommand::check_params(const Json::Value& cmd, Json::Value& error)
{
    std::map<std::string, int> valid = _valid_params_map;
    std::map<std::string, int> params_map;

    for (auto& param : cmd.getMemberNames()) {
        params_map[param] += 1;
    }

    for (auto& param : params_map) {
        auto it = valid.find(param.first);
        if ( it == valid.end() ) {
            error["info"] = _cmd_name + " does allow param: " + param.first;
            return false;
        }
        valid[param.first] = 0;
    }

    for (auto& param : valid) {
        if (param.second > 1) {
            return false;
        }
    }

    return true;
}

Json::Value RSCommand::check_responses(Json::Value &responses)
{
    bool flag_error = false;
    Json::Value ret;

    if (responses.size() == 0) {
        ret["status"] = PMGDCmdResponse::Error;
        ret["info"]   = "No responses!";
        return ret;
    }

    for (auto res : responses) {
        if (res["status"] != PMGDCmdResponse::Success
            &&
            res["status"] != PMGDCmdResponse::Exists)
        {
            flag_error = true;
            break;
        }
    }

    if (!flag_error) {
        ret["status"] = PMGDCmdResponse::Success;
    }

    return ret;
}

//========= AddEntity definitions =========

AddEntity::AddEntity() : RSCommand("AddEntity")
{
    _valid_params_map["class"]       = PARAM_MANDATORY;
    _valid_params_map["_ref"]        = PARAM_OPTIONAL;
    _valid_params_map["properties"]  = PARAM_OPTIONAL;
    _valid_params_map["constraints"] = PARAM_OPTIONAL;
}

int AddEntity::construct_protobuf(PMGDQuery& tx,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    std::string tag = cmd["class"].asString();

    int node_ref = -1;
    if (cmd.isMember("_ref")) {
        node_ref = cmd["_ref"].asInt();
    }

    Json::Value props;
    if (cmd.isMember("properties")) {
        props = cmd["properties"];
    }

    // Check if conditional add
    bool unique = false;
    if (cmd.isMember("unique")) {
        unique = cmd["unique"].asBool();
    }

    Json::Value constraints;
    if (cmd.isMember("constraints")) {
        constraints = cmd["constraints"];
    }

    tx.AddNode(node_ref, tag, props, constraints, unique);

    return 0;
}

Json::Value AddEntity::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value addEntity;
    addEntity[_cmd_name] = response[0];
    return addEntity;
}

//========= Connect definitions =========

Connect::Connect() : RSCommand("Connect")
{
    _valid_params_map["ref1"]       = PARAM_MANDATORY;
    _valid_params_map["ref2"]       = PARAM_MANDATORY;
    _valid_params_map["class"]      = PARAM_OPTIONAL;
    _valid_params_map["properties"] = PARAM_OPTIONAL;
}

int Connect::construct_protobuf(
        PMGDQuery& tx,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    int edge_ref = -1;
    if (cmd.isMember("_ref")) {
        edge_ref = cmd["_ref"].asInt();
    }

    Json::Value props;
    if (cmd.isMember("properties")) {
        props = cmd["properties"];
    }

    int src = cmd["ref1"].asInt();
    int dst = cmd["ref2"].asInt();
    const std::string &tag = cmd["class"].asString();

    tx.AddEdge(edge_ref, src, dst, tag, props);
    return 0;
}

Json::Value Connect::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = response[0];
    return ret;
}

//========= FindEntity definitions =========

FindEntity::FindEntity() : RSCommand("FindEntity")
{
    _valid_params_map["class"]       = PARAM_OPTIONAL;
    _valid_params_map["_ref"]        = PARAM_OPTIONAL;
    _valid_params_map["constraints"] = PARAM_OPTIONAL;
    _valid_params_map["results"]     = PARAM_OPTIONAL;
    _valid_params_map["unique"]      = PARAM_OPTIONAL;
    _valid_params_map["link"]        = PARAM_OPTIONAL;
}

int FindEntity::construct_protobuf(
    PMGDQuery& tx,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value &cmd = jsoncmd[_cmd_name];

    int node_ref = -1;
    if (cmd.isMember("_ref")) {
        node_ref = cmd["_ref"].asInt();
    }

    const std::string& tag = cmd["class"].asString();

    bool unique = false;
    if (cmd.isMember("unique")) {
        unique = cmd["unique"].asBool();
    }

    Json::Value link;
    if (cmd.isMember("link")) {
        link = cmd["link"];
    }

    Json::Value constraints;
    if (cmd.isMember("constraints")) {
        constraints = cmd["constraints"];
    }

    Json::Value results;
    if (cmd.isMember("results")) {
        results = cmd["results"];
    }

    tx.QueryNode(node_ref, tag, link, constraints, results, unique);

    return 0;
}

Json::Value FindEntity::construct_responses(
    Json::Value& response,
    const Json::Value &json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = response[0];
    return ret;
}
