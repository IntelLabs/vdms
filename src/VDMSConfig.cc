/**
 * @file   VDMSConfig.cc
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

#include <map>
#include <sstream>
#include <fstream>
#include <iostream>

#include <jsoncpp/json/json.h>

#include "VDMSConfig.h"

using namespace vdms;

VDMSConfig* VDMSConfig::cfg;

bool VDMSConfig::init(std::string config_file)
{
    if(cfg)
        return false;

    cfg = new VDMSConfig(config_file);
    return true;
}

VDMSConfig* VDMSConfig::instance()
{
    if(cfg)
        return cfg;

    std::cout << "ERROR: Config not init" << std::endl;
    return NULL;
}

VDMSConfig::VDMSConfig(std::string config_file)
{
    Json::Reader reader;
    std::ifstream file(config_file);

    bool parsingSuccessful = reader.parse(file, json_config);

    if (!parsingSuccessful){
        std::cout << "Error parsing config file" << std::endl;
    }
}

int VDMSConfig::get_int_value(std::string val, int def)
{
    return json_config.get(val, def).asInt();
}

std::string VDMSConfig::get_string_value(std::string val, std::string def)
{
    return json_config.get(val, def).asString();
}
