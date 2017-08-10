#pragma once

#include <string>
#include <jsoncpp/json/value.h>

namespace athena{

    class AthenaConfig
    {

    public:
        static bool init(std::string config_file);
        static AthenaConfig* instance();

    private:
        static AthenaConfig* cfg;
        Json::Value json_config;

        AthenaConfig(std::string config_file);

    public:
        int get_int_value(std::string val, int def);
        std::string get_string_value(std::string val, std::string def);
    };

}; // athena namespace
