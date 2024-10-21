
#include "Neo4JCommands.h"
#include "VDMSConfig.h"

using namespace VDMS;

Neo4jCommand::Neo4jCommand(const std::string &cmd_name) : _cmd_name(cmd_name) {
  _use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

template <>
int Neo4jCommand::get_value(const Json::Value &json, const std::string &key,
                            int def) {
  if (json.isMember(key))
    return json[key].asInt();

  return def;
}

template <>
double Neo4jCommand::get_value(const Json::Value &json, const std::string &key,
                               double def) {
  if (json.isMember(key))
    return json[key].asDouble();

  return def;
}

template <>
bool Neo4jCommand::get_value(const Json::Value &json, const std::string &key,
                             bool def) {
  if (json.isMember(key))
    return json[key].asBool();

  return def;
}

template <>
std::string Neo4jCommand::get_value(const Json::Value &json,
                                    const std::string &key, std::string def) {
  if (json.isMember(key))
    return json[key].asString();

  return def;
}

template <>
Json::Value Neo4jCommand::get_value(const Json::Value &json,
                                    const std::string &key, Json::Value def) {
  return json[key];
}

Json::Value Neo4jCommand::construct_responses(
    Json::Value &response, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {

  Json::Value ret;
  ret[_cmd_name] = check_responses(response);
  return ret;
}

Json::Value Neo4jCommand::check_responses(Json::Value &responses) {
  bool flag_error = false;
  Json::Value ret;
  if (responses.size() == 0) {
    printf("NO responses found! Setting Errror!\n");
    ret["status"] = Neo4jCommand::Error;
    ret["info"] = "No responses!";
    printf("Error Response!\n");
    return ret;
  }

  ret = responses[0];

  if (!flag_error) {
    ret["status"] = Neo4jCommand::Success;
  }

  return ret;
}