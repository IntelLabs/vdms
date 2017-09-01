#include <map>
#include <sstream>
#include <fstream>
#include <iostream>

#include <jsoncpp/json/json.h>

#include "AthenaConfig.h"

using namespace athena;

AthenaConfig* AthenaConfig::cfg;

bool AthenaConfig::init(std::string config_file)
{
    if(cfg)
        return false;

    cfg = new AthenaConfig(config_file);
    return true;
}

AthenaConfig* AthenaConfig::instance()
{
    if(cfg)
        return cfg;

    std::cout << "ERROR: Config not init" << std::endl;
    return NULL;
}

AthenaConfig::AthenaConfig(std::string config_file)
{
    Json::Reader reader;
    std::ifstream file(config_file);

    bool parsingSuccessful = reader.parse(file, json_config);

    if (!parsingSuccessful){
        std::cout << "Error parsing config file" << std::endl;
    }
}

int AthenaConfig::get_int_value(std::string val, int def)
{
    return json_config.get(val, def).asInt();
}

std::string AthenaConfig::get_string_value(std::string val, std::string def)
{
    return json_config.get(val, def).asString();
}
