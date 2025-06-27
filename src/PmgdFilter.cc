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
    enum FilterEngine eng_val;
    struct FilterParameters fparams = FilterParameters();

    //check for optional hash seeds
    if (cmd.isMember("prim_hash_seed")){
        prim_hash = get_value<int>(cmd, "prim_hash_seed",0);
    }

    if (cmd.isMember("sec_hash_seed")){
        sec_hash = get_value<int>(cmd, "sec_hash_seed",0);
    }

    //convert engine choice to enum val
    if(engine == "CuckooHT"){
        printf("Cuckhoo Hash Table\n");
        eng_val = CuckooHT;
    } else if(engine == "CuckooCache"){
        printf("Cuckhoo Cache\n");
        eng_val = CuckooCache;
    } else if(engine == "VBF"){
        printf("VBF\n");
        eng_val = VBF;
    } else {
        error["Status"] = RSCommand::Error;
        error["Info"] = engine + " is not a recognized or supported filter type";
        return -1;
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

    Filter *fp = filter_create(&fparams);
    if(fp == NULL){
        error["Status"] = RSCommand::Error;
        error["Info"] = "Filter creation failed. Check for duplicate filter name and valid parameters.\n";
        return -1;
    }

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
    const Json::Value &cmd = json[_cmd_name];
    std::string filtername = get_value<std::string>(cmd, "name","");

    //attempt to retrieve filter
    Filter *filt_ptr;

    filt_ptr = filter_find_existing(filtername.c_str());
    if(filt_ptr == NULL){

        ret["status"] = RSCommand::Success;
        ret["filter_info"] = "Filter not found";

        return ret;
    }


    //if filter is found, return available stats and what not in return JSON
    std::string name = filt_ptr->get_name();
    FilterEngine eng = filt_ptr->get_engine_type();
    uint32_t key_len = filt_ptr->get_key_len();
    uint32_t nr_keys = filt_ptr->get_num_keys();
    uint32_t flg = filt_ptr->get_ef();
    std::string eng_name;

    //convert engine to string
    if(eng == CuckooHT){
        eng_name = "CuckooHT";
    } else if(eng == CuckooCache){
        eng_name = "CuckooCache";
    } else if(eng == VBF){
        eng_name = "VBF";
    }

    Json::Value filter_info;

    filter_info["name"] = name;
    filter_info["engine"] = eng_name;
    filter_info["key_len"] = key_len;
    filter_info["nr_keys"] = nr_keys;
    filter_info["flg"] = flg;

    ret["status"] = RSCommand::Success;
    ret["filter_info"] = filter_info;

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
    std::vector<std::string> filter_list;
    filter_list = filter_list_all_names();
    std::string cur_name;
    Json::Value filters;


    for(long unsigned int i = 0; i < filter_list.size(); i++){
        cur_name = filter_list[i];
        filters.append(cur_name.c_str());
    }

    //return in JSON val
    ret["status"] = RSCommand::Success;
    ret["filter_list"] = filters;

    return ret;
}