/**
 * @file   RSCommand.cc
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

#include <string>
#include <iostream>

#include "QueryHandler.h"
#include "ExceptionsCommand.h"

using namespace athena;

RSCommand::RSCommand(const std::string& cmd_name):
    _cmd_name(cmd_name)
{
}

Json::Value RSCommand::construct_responses(
    Json::Value& response,
    const Json::Value& json,
    protobufs::queryMessage &query_res)
{
    assert(response.size() == 1);

    Json::Value ret;
    ret[_cmd_name] = response[0];
    return ret;
}

Json::Value RSCommand::check_responses(Json::Value& responses)
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

namespace athena {
template<>
int RSCommand::get_value(const Json::Value& json, const std::string& key,
                         const int& def)
{
    if (json.isMember(key))
        return json[key].asInt();

    return def;
}

template<>
bool RSCommand::get_value(const Json::Value& json, const std::string& key,
                          const bool& def)
{
    if (json.isMember(key))
        return json[key].asBool();

    return def;
}

template<>
std::string RSCommand::get_value(const Json::Value& json,
                                 const std::string& key,
                                 const std::string& def)
{
    if (json.isMember(key))
        return json[key].asString();

    return def;
}

template<>
Json::Value RSCommand::get_value(const Json::Value& json,
                                 const std::string& key,
                                 const Json::Value& def)
{
    return json[key];
}
}

//========= AddEntity definitions =========

AddEntity::AddEntity() : RSCommand("AddEntity")
{
}

int AddEntity::construct_protobuf(PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    query.AddNode(
            get_value<int>(cmd, "_ref", -1),
            get_value<std::string>(cmd, "class"),
            get_value<Json::Value>(cmd, "properties"),
            get_value<Json::Value>(cmd, "constraints"),
            get_value<bool>(cmd, "unique", false)
            );

    return 0;
}

//========= Connect definitions =========

Connect::Connect() : RSCommand("Connect")
{
}

int Connect::construct_protobuf(
        PMGDQuery& query,
        const Json::Value& jsoncmd,
        const std::string& blob,
        int grp_id,
        Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    query.AddEdge(
            get_value<int>(cmd, "_ref", -1),
            get_value<int>(cmd, "ref1", -1), // src
            get_value<int>(cmd, "ref2", -1), // dst
            get_value<std::string>(cmd, "class"), // tag
            get_value<Json::Value>(cmd, "properties")
            );

    return 0;
}

//========= FindEntity definitions =========

FindEntity::FindEntity() : RSCommand("FindEntity")
{
}

int FindEntity::construct_protobuf(
    PMGDQuery& query,
    const Json::Value& jsoncmd,
    const std::string& blob,
    int grp_id,
    Json::Value& error)
{
    const Json::Value& cmd = jsoncmd[_cmd_name];

    query.QueryNode(
            get_value<int>(cmd, "_ref", -1),
            get_value<std::string>(cmd, "class"),
            get_value<Json::Value>(cmd, "link"),
            get_value<Json::Value>(cmd, "constraints"),
            get_value<Json::Value>(cmd, "results"),
            get_value<bool>(cmd, "unique", false)
            );

    return 0;
}
