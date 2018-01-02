#include <string>
#include <iostream>

#include "QueryHandler.h"

using namespace athena;

RSCommand::RSCommand(const std::string& cmd_name):
    _cmd_name(cmd_name)
{
}

bool RSCommand::check_params(const Json::Value& cmd)
{
    std::map<std::string, int> valid = _valid_params_map;
    std::map<std::string, int> params_map;

    for (auto& param : cmd.getMemberNames()){
        params_map[param] += 1;
    }

    for (auto& param : params_map){
        auto it = valid.find(param.first);
        if ( it == valid.end() ) {
            return false;
        }
        valid[param.first] = 0;
    }

    for (auto& param : valid)
    {
        if (param.second > 1) {
            return false;
        }
    }

    return true;
}

void RSCommand::run_operations(VCL::Image& vclimg, const Json::Value& op)
{
    for (auto& operation : op) {
        std::string type = operation["type"].asString();
        if (type == "threshold") {
            vclimg.threshold(operation["value"].asInt());
        }
        else if (type == "resize") {
            vclimg.resize(operation["height"].asInt(),
                    operation["width" ].asInt());
        }
        else if (type == "crop") {
            vclimg.crop(VCL::Rectangle (
                        operation["x"].asInt(),
                        operation["y"].asInt(),
                        operation["height"].asInt(),
                        operation["width" ].asInt()));
        }
        else {
            // GENERIC_LOGGER << "Operation not recognised: "
            //                << type << std::endl;
        }
    }
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
