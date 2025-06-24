/**
 * @file   PmgdFilter.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2025 Intel Corporation
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

#include "PmgdFilter.h"

using namespace VDMS;
using namespace VCL;
FilterCommand::FilterCommand(const std::string &cmd_name) : RSCommand(cmd_name) {
}

//========= AddFilter definitions =========
AddFilter::AddFilter() : FilterCommand("AddFilter") {
    _fmgr = &get_global_filter_manager();
}

int AddFilter::construct_protobuf(PMGDQuery &tx, const Json::Value &jsoncmd,
                                  const std::string &blob, int grp_id,
                                  Json::Value &error) {

    error["skip_pmgd"] = true;

    const Json::Value &cmd = jsoncmd[_cmd_name];

    //Check if filter already exists

    //create filter if it does not
    //note filter names must be mapped to known property names
    //note also checks and retrievals of argument values

    /*"name":   { "type": "string" },
    "engine": {"$ref": "#/definitions/filterEngineFormatString" },
    "nr_keys": {"$ref": "#/definitions/refInt"},
    "key_len": { "$ref": "#/definitions/refInt"},
    "prim_hash_seed": { "$ref": "#/definitions/refInt"},
    "sec_hash_seed": { "$ref": "#/definitions/refInt"}*/

    //these should be gauranteed to exist based on API definitions
    std::string filtername = get_value<std::string>(cmd, "name","");
    std::string engine = get_value<std::string>(cmd, "engine","");
    uint32_t nr_keys = get_value<int>(cmd,"nr_keys",0);
    uint32_t key_len = get_value<int>(cmd, "key_len",0);
    uint32_t prim_hash = 0;
    uint32_t sec_hash = 0;
    FilterEngine eng_val;
    struct FilterParameters fparams = FilterParameters();

    //check for optional hash seeds
    if (cmd.isMember("prim_hash_seed")){
        prim_hash = get_value<int>(cmd, "prim_hash_seed",0);
    }

    if (cmd.isMember("sec_hash_seed")){
        sec_hash = get_value<int>(cmd, "sec_hash_seed",0);
    }

    //convert engine choice to enum val
    if(filtername == "CuckooHT"){
        eng_val = CuckooHT;
    } else if(filtername == "CuckooCache"){
        eng_val = CuckooCache;
    } else if(filtername == "VBF"){
        eng_val = VBF;
    }

    //load up filter paramter structure
    fparams.name = filtername.c_str();
    fparams.engine = eng_val;
    fparams.num_keys = nr_keys;
    fparams.key_len = key_len;

    //if hash seeds are specified, use them, otherwise leave as defaults from
    //constructor
    if(prim_hash != 0) {
        fparams.prim_hash_seed = prim_hash;
    }

    if(sec_hash != 0 ){
        fparams.sec_hash_seed = sec_hash;
    }

    UniqueFilterPtr fp = Filter::create_filter_instance(&fparams);




    return 0;

}

Json::Value AddFilter::construct_responses(Json::Value &json_responses,
                                          const Json::Value &json,
                                          protobufs::queryMessage &response,
                                          const std::string &blob){

    //at this point should just be a filter add success message
    Json::Value ret;

    ret["status"] = RSCommand::Success;
    ret["Info"] = "New Filter Added";

    return ret;
}




//======== FindFilter definitions ========
FindFilter::FindFilter() : FilterCommand("FindFilter") {

}

int FindFilter::construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                                  const std::string &blob, int grp_id,
                                  Json::Value &error) {

    error["skip_pmgd"] = true;

       return 0;


}

Json::Value FindFilter::construct_responses(Json::Value &json_responses,
                                           const Json::Value &json,
                                           protobufs::queryMessage &response,
                                           const std::string &blob){

    Json::Value ret;

    //attempt to retrieve filter

    //if filter is found, return available stats and what not in return JSON

    ret["status"] = "FindFilter E2E Works";

    return ret;

}

//======== ListFilter definitions ========
ListFilter::ListFilter() : FilterCommand("ListFilter") {

}

int ListFilter::construct_protobuf(PMGDQuery &tx, const Json::Value &root,
                                   const std::string &blob, int grp_id,
                                   Json::Value &error) {

    error["skip_pmgd"] = true;

    return 0;


}

Json::Value ListFilter::construct_responses(Json::Value &json_responses,
                                            const Json::Value &json,
                                            protobufs::queryMessage &response,
                                            const std::string &blob){

    Json::Value ret;

    //retrieve list of all filters by name

    //return in JSON val

    ret["stub_val"] = "ListFilter E2E Works";

    return ret;

}