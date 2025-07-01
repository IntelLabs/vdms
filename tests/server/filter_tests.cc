/**
 * @file   filter_tests.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2025 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files
 * (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <stdlib.h> /* system, NULL, EXIT_FAILURE */
#include <string>
#include <vector>

#include "gtest/gtest.h"
#include <jsoncpp/json/writer.h>

#include "QueryHandlerExample.h"
#include "QueryHandlerPMGD.h"
#include "QueryHandlerTester.h"
#include "VDMSConfig.h"
#include "pmgd.h"

using namespace VDMS;
using namespace PMGD;
using namespace std;

TEST(PMGDFilter, addAndListFilters) {

    Json::Reader reader;
    Json::FastWriter fastWriter;

    //Adding filters
    Json::Value add_filter_q_cch;
    Json::Value add_filter_q_ccc;
    Json::Value add_filter_q_vbf;

    Json::Value base_q_cch;
    Json::Value base_q_ccc;
    Json::Value base_q_vbf;

    add_filter_q_cch["name"] = "test_filt_cch";
    add_filter_q_cch["nr_keys"] = 100000;
    add_filter_q_cch["key_len"] = 8;
    add_filter_q_cch["engine"] = "CuckooHT";

    add_filter_q_ccc["name"] = "test_filt_ccc";
    add_filter_q_ccc["nr_keys"] = 100000;
    add_filter_q_ccc["key_len"] = 8;
    add_filter_q_ccc["engine"] = "CuckooCache";

    add_filter_q_vbf["name"] = "test_filt_vbf";
    add_filter_q_vbf["nr_keys"] = 100000;
    add_filter_q_vbf["key_len"] = 8;
    add_filter_q_vbf["engine"] = "VBF";

    VDMS::VDMSConfig::init("server/config-tests.json"); //Setup JSOn for this set of tests
    base_q_cch["AddFilter"] =  add_filter_q_cch;
    base_q_ccc["AddFilter"] =  add_filter_q_ccc;
    base_q_vbf["AddFilter"] =  add_filter_q_vbf;

    VDMS::protobufs::queryMessage proto_query_add_cch;
    VDMS::protobufs::queryMessage proto_query_add_ccc;
    VDMS::protobufs::queryMessage proto_query_add_vbf;
    VDMS::protobufs::queryMessage cch_response;
    VDMS::protobufs::queryMessage ccc_response;
    VDMS::protobufs::queryMessage vbf_response;

    Json::Value parsed_add_cch;
    Json::Value parsed_add_ccc;
    Json::Value parsed_add_vbf;
    Json::Value ret_obj;

    PMGDQueryHandler::init();
    QueryHandlerPMGD::init();

    QueryHandlerPMGD qh_base;
    qh_base.reset_autodelete_init_flag(); // set flag to show autodelete initialized
    QueryHandlerPMGDTester query_handler(qh_base);

    std::string add_cch = fastWriter.write(base_q_cch);
    std::string add_ccc = fastWriter.write(base_q_ccc);
    std::string add_vbf = fastWriter.write(base_q_vbf);

    std::string add_cch_final = "[" + add_cch + "]";
    std::string add_ccc_final = "[" + add_ccc + "]";
    std::string add_vbf_final = "[" + add_vbf + "]";

    //test add cuckoo hash table
    proto_query_add_cch.set_json(add_cch_final);
    query_handler.pq(proto_query_add_cch, cch_response);
    reader.parse(cch_response.json().c_str(), parsed_add_cch);

    ret_obj = parsed_add_cch[0];
    ASSERT_EQ(ret_obj["Info"], "New Filter Added");

    //test add cuckoo cache
    proto_query_add_ccc.set_json(add_ccc_final);
    query_handler.pq(proto_query_add_ccc, ccc_response);
    reader.parse(ccc_response.json().c_str(), parsed_add_ccc);

    ret_obj = parsed_add_ccc[0];
    ASSERT_EQ(ret_obj["Info"], "New Filter Added");

    //test add VBF
    proto_query_add_vbf.set_json(add_vbf_final);
    query_handler.pq(proto_query_add_vbf, vbf_response);
    reader.parse(vbf_response.json().c_str(), parsed_add_vbf);

    ret_obj =  parsed_add_vbf[0];
    ASSERT_EQ(ret_obj["Info"], "New Filter Added");

    //list added filters
    Json::Value list_filters_q;
    Json::Value base_list_q;

    base_list_q["ListFilter"] = list_filters_q;
    std::string list_filters_str = fastWriter.write(base_list_q);
    std::string list_filters_final = "[" + list_filters_str + "]";

    Json::Value parsed_list;
    VDMS::protobufs::queryMessage proto_query_list;
    VDMS::protobufs::queryMessage list_response;
    Json::Value json_filt_list;

    proto_query_list.set_json(list_filters_final);
    query_handler.pq(proto_query_list, list_response);
    reader.parse(list_response.json().c_str(), parsed_list);

    ret_obj = parsed_list[0];
    json_filt_list = ret_obj["filter_list"];

    ASSERT_EQ(json_filt_list.size(),3);

    //get filter details
    Json::Value filter_details;
    Json::Value base_filter_details;

    filter_details["name"] = "test_filt_cch";
    base_filter_details["FindFilter"] = filter_details;

    Json::Value find_filt_json;

    std::string find_filt = fastWriter.write(base_filter_details);
    std::string find_filt_final = "[" + find_filt + "]";

    VDMS::protobufs::queryMessage proto_query_find_filt;
    VDMS::protobufs::queryMessage find_filt_response;

    proto_query_find_filt.set_json(find_filt_final);
    query_handler.pq(proto_query_find_filt, find_filt_response);
    reader.parse(find_filt_response.json().c_str(), parsed_list);

    ret_obj = parsed_list[0];

    Json::Value filter_info;
    filter_info = ret_obj["filter_info"];

    ASSERT_EQ(filter_info["name"],"test_filt_cch");
    ASSERT_EQ(filter_info["flg"], 0);
    ASSERT_EQ(filter_info["key_len"], 8);
    ASSERT_EQ(filter_info["nr_keys"], 100000);

}

TEST(PMGDFilter, addBadFilter) {

    Json::Reader reader;
    Json::FastWriter fastWriter;

    //Adding filters
    Json::Value add_filter_bad;

    Json::Value base_q_bad;

    add_filter_bad["name"] = "test_filt_bad";
    add_filter_bad["nr_keys"] = 100000;
    add_filter_bad["key_len"] = 8;
    add_filter_bad["engine"] = "BadFilter";

    base_q_bad["AddFilter"] = add_filter_bad;

    Json::Value parsed_filt_bad;
    Json::Value ret_obj;

    PMGDQueryHandler::init();
    QueryHandlerPMGD::init();

    QueryHandlerPMGD qh_base;
    qh_base.reset_autodelete_init_flag(); // set flag to show autodelete initialized
    QueryHandlerPMGDTester query_handler(qh_base);

    std::string add_bad = fastWriter.write(base_q_bad);
    std::string add_bad_final = "[" + add_bad + "]";

    VDMS::protobufs::queryMessage proto_query_add_bad;
    VDMS::protobufs::queryMessage bad_response;

    //test add cuckoo hash table
    proto_query_add_bad.set_json(add_bad_final);
    query_handler.pq(proto_query_add_bad, bad_response);
    reader.parse(bad_response.json().c_str(), parsed_filt_bad);
    ret_obj = parsed_filt_bad[0];
    ASSERT_EQ(ret_obj["status"], -1);
}

TEST(PMGDFilter, missingFilterFind){

    Json::Reader reader;
    Json::FastWriter fastWriter;

    //Adding filters
    Json::Value add_filter;

    Json::Value base_q;

    add_filter["name"] = "test_filt_not_this_one";
    add_filter["nr_keys"] = 100000;
    add_filter["key_len"] = 8;
    add_filter["engine"] = "CuckooCache";

    base_q["AddFilter"] = add_filter;

    Json::Value parsed_filt;
    Json::Value ret_obj;

    PMGDQueryHandler::init();
    QueryHandlerPMGD::init();

    QueryHandlerPMGD qh_base;
    qh_base.reset_autodelete_init_flag(); // set flag to show autodelete initialized
    QueryHandlerPMGDTester query_handler(qh_base);

    std::string add = fastWriter.write(base_q);
    std::string add_final = "[" + add + "]";

    VDMS::protobufs::queryMessage proto_query_add_filt;
    VDMS::protobufs::queryMessage response;

    //test add cuckoo hash table
    proto_query_add_filt.set_json(add_final);
    query_handler.pq(proto_query_add_filt, response);
    reader.parse(response.json().c_str(), parsed_filt);
    //ret_obj = parsed_filt[0];

    //get filter details
    Json::Value filter_details;
    Json::Value base_filter_details;

    filter_details["name"] = "missing_filter";
    base_filter_details["FindFilter"] = filter_details;

    Json::Value find_filt_json;

    std::string find_filt = fastWriter.write(base_filter_details);
    std::string find_filt_final = "[" + find_filt + "]";

    VDMS::protobufs::queryMessage proto_query_find_filt;
    VDMS::protobufs::queryMessage find_filt_response;

    proto_query_find_filt.set_json(find_filt_final);
    query_handler.pq(proto_query_find_filt, find_filt_response);
    reader.parse(find_filt_response.json().c_str(), parsed_filt);

    ret_obj = parsed_filt[0];

    Json::Value filter_info;
    filter_info = ret_obj["filter_info"];

    ASSERT_EQ(filter_info,"Filter not found");




}



