/**
 * @file   DescriptorsCommand.cc
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

#include <filesystem>
#include <iostream>

#include "DescriptorsCommand.h"
#include "ExceptionsCommand.h"
#include "VDMSConfig.h"
#include "defines.h"

#include "vcl/utils.h"

using namespace VDMS;
namespace fs = std::filesystem;

DescriptorsCommand::DescriptorsCommand(const std::string &cmd_name)
    : RSCommand(cmd_name) {
  _dm = DescriptorsManager::instance();
  output_vcl_timing =
      VDMSConfig::instance()->get_bool_value("print_vcl_timing", false);
}

// This function only throws when there is a transaction error,
// but not when there is an input error (such as wrong set_name).
// In case of wrong input, we need to inform to the user what
// went wrong.
std::string DescriptorsCommand::get_set_path(PMGDQuery &query_tx,
                                             const std::string &set_name,
                                             int &dim) {

  // Check cache for descriptor set, if its found set dimensions and return
  // path,
  // otherwise we go forward and query PMGD to locate the descriptor set
  auto element = _desc_set_locator.find(set_name);
  std::string mapped_path;
  if (element != _desc_set_locator.end()) {
    mapped_path = element->second;
    dim = _desc_set_dims[set_name];
    return mapped_path;
  }

  // Will issue a read-only transaction to check
  // if the Set exists
  PMGDQuery query(query_tx.get_pmgd_qh());

  Json::Value constraints, link;
  Json::Value name_arr;
  name_arr.append("==");
  name_arr.append(set_name);
  constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

  Json::Value results;
  Json::Value list_arr;
  list_arr.append(VDMS_DESC_SET_PATH_PROP);
  list_arr.append(VDMS_DESC_SET_DIM_PROP);
  list_arr.append(VDMS_DESC_SET_ENGIN_PROP);

  results["list"] = list_arr;

  bool unique = true;

  // Query set node
  query.add_group();
  query.QueryNode(-1, VDMS_DESC_SET_TAG, link, constraints, results, unique,
                  true);

  Json::Value &query_responses = query.run();
  if (query_responses.size() != 1 && query_responses[0].size() != 1) {
    throw ExceptionCommand(DescriptorSetError, "PMGD Transaction Error");
  }

  Json::Value &set_json = query_responses[0][0];

  if (!set_json.isMember("entities")) {
    throw ExceptionCommand(DescriptorSetError, "PMGD Transaction Error");
  }

  for (auto &ent : set_json["entities"]) {
    assert(ent.isMember(VDMS_DESC_SET_PATH_PROP));
    std::string set_path = ent[VDMS_DESC_SET_PATH_PROP].asString();
    dim = ent[VDMS_DESC_SET_DIM_PROP].asInt();
    _desc_set_dims[set_name] = dim;
    _desc_set_locator[set_name] = set_path;
    return set_path;
  }
  return "";
}

bool DescriptorsCommand::check_blob_size(const std::string &blob,
                                         const int dimensions,
                                         const long n_desc) {
  return (blob.size() / sizeof(float) / dimensions == n_desc);
}
// FindDescriptorSet Method
FindDescriptorSet::FindDescriptorSet()
    : DescriptorsCommand("FindDescriptorSet") {
  _storage_sets = VDMSConfig::instance()->get_path_descriptors();
  _dm->flush(); // store the descriptor set
}
int FindDescriptorSet::construct_protobuf(PMGDQuery &query,
                                          const Json::Value &jsoncmd,
                                          const std::string &blob, int grp_id,
                                          Json::Value &error) {

  const Json::Value &cmd = jsoncmd[_cmd_name];
  Json::Value results = get_value<Json::Value>(cmd, "results");

  const std::string set_name = cmd["set"].asString();
  const std::string set_path = _storage_sets + "/" + set_name;

  Json::Value constraints, link;
  Json::Value name_arr;
  name_arr.append("==");
  name_arr.append(set_name);
  constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

  Json::Value list_arr;
  list_arr.append(VDMS_DESC_SET_NAME_PROP);
  list_arr.append(VDMS_DESC_SET_PATH_PROP);
  list_arr.append(VDMS_DESC_SET_DIM_PROP);
  list_arr.append(VDMS_DESC_SET_ENGIN_PROP);
  results["list"] = list_arr;
  bool unique = true;

  query.QueryNode(-1, VDMS_DESC_SET_TAG, Json::nullValue, constraints, results,
                  unique, true);

  return 0;
}

Json::Value FindDescriptorSet::construct_responses(
    Json::Value &json_responses, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {

  const Json::Value &cmd = json[_cmd_name];
  Json::Value resp = check_responses(json_responses);
  Json::Value ret;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  if (resp["status"] != RSCommand::Success) {
    return error(resp);
  }

  /* Get Set information using set name */
  const std::string set_name = cmd["set"].asString();
  const std::string set_path = _storage_sets + "/" + set_name;
  try {
    VCL::DescriptorSet *desc_set = _dm->get_descriptors_handler(set_path);
    resp["status"] = RSCommand::Success;
    ret[_cmd_name] = resp;

    if (cmd.isMember("storeIndex") && cmd["storeIndex"].asBool()) {
      desc_set->store();
    }
  } catch (VCL::Exception e) {
    print_exception(e);
    resp["status"] = RSCommand::Error;
    resp["info"] = "DescriptorSet details not available";
    return -1;
  }

  return ret;
}

//---------------------------------------------------------------

// AddDescriptorSet Methods

AddDescriptorSet::AddDescriptorSet() : DescriptorsCommand("AddDescriptorSet") {
  _storage_sets = VDMSConfig::instance()->get_path_descriptors();
  _flinng_num_rows = 3; // set based on the default values of Flinng
  _flinng_cells_per_row = 1000;
  _flinng_num_hash_tables = 10;
  _flinng_hashes_per_table = 12;
  _flinng_sub_hash_bits = 2;
  _flinng_cut_off = 6;

  //_use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

int AddDescriptorSet::construct_protobuf(PMGDQuery &query,
                                         const Json::Value &jsoncmd,
                                         const std::string &blob, int grp_id,
                                         Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  // Create PMGD cmd for AddNode
  int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

  std::string set_name = cmd["name"].asString();
  std::string desc_set_path = _storage_sets + "/" + set_name;

  Json::Value props = get_value<Json::Value>(cmd, "properties");
  props[VDMS_DESC_SET_NAME_PROP] = cmd["name"].asString();
  props[VDMS_DESC_SET_DIM_PROP] = cmd["dimensions"].asInt();
  props[VDMS_DESC_SET_PATH_PROP] = desc_set_path;
  props[VDMS_DESC_SET_ENGIN_PROP] = cmd["engine"].asString();
  if (props[VDMS_DESC_SET_ENGIN_PROP] == "Flinng") {
    if (cmd.isMember("flinng_num_rows"))
      _flinng_num_rows = cmd["flinng_num_rows"].asInt();
    if (cmd.isMember("flinng_cells_per_row"))
      _flinng_cells_per_row = cmd["flinng_cells_per_row"].asInt();
    if (cmd.isMember("flinng_num_hash_tables"))
      _flinng_num_hash_tables = cmd["flinng_num_hash_tables"].asInt();
    if (cmd.isMember("flinng_hashes_per_table"))
      _flinng_hashes_per_table = cmd["flinng_hashes_per_table"].asInt();
    if (cmd.isMember("flinng_sub_hash_bits"))
      _flinng_sub_hash_bits = cmd["flinng_sub_hash_bits"].asInt();
    if (cmd.isMember("flinng_cut_off"))
      _flinng_cut_off = cmd["flinng_cut_off"].asInt();
  }

  Json::Value constraints;
  constraints[VDMS_DESC_SET_NAME_PROP].append("==");
  constraints[VDMS_DESC_SET_NAME_PROP].append(cmd["name"].asString());

  query.AddNode(node_ref, VDMS_DESC_SET_TAG, props, constraints);

  if (cmd.isMember("link")) {
    add_link(query, cmd["link"], node_ref, VDMS_DESC_SET_EDGE_TAG);
  }

  // create a new index based on the descriptor set name
  try {
    std::string idx_prop = VDMS_DESC_ID_PROP + std::string("_") + set_name;
    query.AddIntNodeIndexImmediate(VDMS_DESC_TAG, (char *)idx_prop.c_str());
  } catch (...) {
    printf("Descriptor Set Index Creation Failed for %s\n", set_name.c_str());
    error["info"] = "Set index creation failed for" + set_name;
    error["status"] = RSCommand::Error;
    return -1;
  }

  return 0;
}

Json::Value AddDescriptorSet::construct_responses(
    Json::Value &json_responses, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {
  const Json::Value &cmd = json[_cmd_name];
  Json::Value resp = check_responses(json_responses);

  Json::Value ret;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  if (resp["status"] != RSCommand::Success) {
    return error(resp);
  }

  int dimensions = cmd["dimensions"].asInt();
  std::string set_name = cmd["name"].asString();
  std::string desc_set_path = _storage_sets + "/" + set_name;

  std::string metric_str = get_value<std::string>(cmd, "metric", "L2");
  VCL::DistanceMetric metric = metric_str == "L2" ? VCL::L2 : VCL::IP;

  // For now, we use the default faiss index.
  std::string eng_str = get_value<std::string>(cmd, "engine", "FaissFlat");

  if (eng_str == "FaissFlat")
    _eng = VCL::FaissFlat;
  else if (eng_str == "FaissIVFFlat")
    _eng = VCL::FaissIVFFlat;
  else if (eng_str == "TileDBDense")
    _eng = VCL::TileDBDense;
  else if (eng_str == "TileDBSparse")
    _eng = VCL::TileDBSparse;
  else if (eng_str == "Flinng")
    _eng = VCL::Flinng;
  else if (eng_str == "FaissHNSWFlat")
    _eng = VCL::FaissHNSWFlat;
  else
    throw ExceptionCommand(DescriptorSetError, "Engine not supported");

  // We can probably set up a mechanism
  // to fix a broken link when detected later, same with images.
  VCL::DescriptorParams *param = nullptr;
  try {
    param = new VCL::DescriptorParams(_flinng_num_rows, _flinng_cells_per_row,
                                      _flinng_num_hash_tables,
                                      _flinng_hashes_per_table);
    VCL::DescriptorSet desc_set(desc_set_path, dimensions, _eng, metric, param);

    if (_use_aws_storage) {
      VCL::RemoteConnection *connection = new VCL::RemoteConnection();
      std::string bucket = VDMSConfig::instance()->get_bucket_name();
      connection->_bucket_name = bucket;
      desc_set.set_connection(connection);
    }

    desc_set.store();
    if (output_vcl_timing) {
      desc_set.timers.print_map_runtimes();
    }
    desc_set.timers.clear_all_timers();

    delete (param);
  } catch (VCL::Exception e) {
    print_exception(e);
    resp["status"] = RSCommand::Error;
    resp["info"] = std::string("VCL Exception: ") + e.msg;
    delete (param);
    return error(resp);
  }

  resp.clear();
  resp["status"] = RSCommand::Success;

  ret[_cmd_name] = resp;
  return ret;
}

// AddDescriptor Methods

AddDescriptor::AddDescriptor() : DescriptorsCommand("AddDescriptor") {
  //_use_aws_storage = VDMSConfig::instance()->get_aws_flag();
}

// update to handle multiple descriptors at a go
long AddDescriptor::insert_descriptor(const std::string &blob,
                                      const std::string &set_path, int nr_desc,
                                      const std::string &label,
                                      Json::Value &error) {
  long id_first;

  try {

    VCL::DescriptorSet *desc_set = _dm->get_descriptors_handler(set_path);

    if (!label.empty()) {
      long label_id = desc_set->get_label_id(label);
      long *label_ptr = &label_id;
      id_first = desc_set->add((float *)blob.data(), nr_desc, label_ptr);

    } else {
      id_first = desc_set->add((float *)blob.data(), nr_desc);
    }

    if (output_vcl_timing) {
      desc_set->timers.print_map_runtimes();
    }
    desc_set->timers.clear_all_timers();
  } catch (VCL::Exception e) {
    print_exception(e);
    error["info"] = "VCL Descriptors Exception";
    return -1;
  }

  return id_first;
}

void AddDescriptor::retrieve_aws_descriptorSet(const std::string &set_path) {
  // check if folder already exists at path, if so, don't even try to hit AWS
  if (fs::exists(set_path)) {
    return;
  }

  VCL::RemoteConnection *connection = new VCL::RemoteConnection();
  std::string bucket = VDMSConfig::instance()->get_bucket_name();
  connection->_bucket_name = bucket;

  if (!connection->connected()) {
    connection->start();
  }
  if (!connection->connected()) {
    throw VCLException(SystemNotFound, "No remote connection started");
  }

  std::vector<std::string> files = connection->ListFilesInFolder(set_path);
  for (auto file : files) {
    // if file isn't already on disk, retrieve it from AWS
    if (!fs::exists(file)) {
      if (!connection->RetrieveFile(file)) {
        throw VCLException(ObjectNotFound, "File was not found");
      }
    }
  }
}

int AddDescriptor::add_single_descriptor(PMGDQuery &query,
                                         const Json::Value &jsoncmd,
                                         const std::string &blob, int grp_id,
                                         Json::Value &error) {

  const Json::Value &cmd = jsoncmd[_cmd_name];
  const std::string set_name = cmd["set"].asString();

  Json::Value props = get_value<Json::Value>(cmd, "properties");

  std::string label = get_value<std::string>(cmd, "label", "None");
  props[VDMS_DESC_LABEL_PROP] = label;

  int dim;
  const std::string set_path = get_set_path(query, set_name, dim);

  if (set_path.empty()) {
    error["info"] = "Set " + set_name + " not found";
    error["status"] = RSCommand::Error;
    return -1;
  }

  if (blob.length() / 4 != dim) {
    std::cerr << "AddDescriptor::insert_descriptor: ";
    std::cerr << "Dimensions mismatch: ";
    std::cerr << blob.length() / 4 << " " << dim << std::endl;
    error["info"] = "Blob Dimensions Mismatch";
    return -1;
  }

  // retrieve the descriptor set from AWS here
  // operations are currently done in memory with no subsequent write to disk
  // so there's no need to re-upload to AWS
  if (_use_aws_storage) {
    retrieve_aws_descriptorSet(set_path);
  }

  long id = insert_descriptor(blob, set_path, 1, label, error);

  if (id < 0) {
    error["status"] = RSCommand::Error;

    if (_use_aws_storage) {
      // delete files in set_path
      std::uintmax_t n = fs::remove_all(set_path);
      std::cout << "Deleted " << n << " files or directories\n";
    }

    return -1;
  }

  std::string desc_id_prop_name =
      VDMS_DESC_ID_PROP + std::string("_") + set_name;
  props[desc_id_prop_name] = Json::Int64(id);

  int node_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

  query.AddNode(node_ref, VDMS_DESC_TAG, props, Json::nullValue);

  // It passed the checker, so it exists.
  int set_ref = query.get_available_reference();

  Json::Value link;
  Json::Value results;
  Json::Value list_arr;
  list_arr.append(VDMS_DESC_SET_PATH_PROP);
  list_arr.append(VDMS_DESC_SET_DIM_PROP);
  results["list"] = list_arr;

  Json::Value constraints;
  Json::Value name_arr;
  name_arr.append("==");
  name_arr.append(set_name);
  constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

  bool unique = true;

  // Query set node
  query.QueryNode(set_ref, VDMS_DESC_SET_TAG, link, constraints, results,
                  unique);

  if (cmd.isMember("link")) {
    add_link(query, cmd["link"], node_ref, VDMS_DESC_EDGE_TAG);
  }

  Json::Value props_edge;
  query.AddEdge(-1, set_ref, node_ref, VDMS_DESC_SET_EDGE_TAG, props_edge);

  // TODO: deleting files here causes problems with concurrency (TestRetail.py)
  // keeping local copies as a temporary solution
  // if(_use_aws_storage)
  // {
  //     //delete files in set_path
  //     std::uintmax_t n = fs::remove_all(set_path);
  //     std::cout << "Deleted " << n << " files or directories\n";
  // }

  return 0;
}

int AddDescriptor::add_descriptor_batch(PMGDQuery &query,
                                        const Json::Value &jsoncmd,
                                        const std::string &blob, int grp_id,
                                        Json::Value &error) {

  const int FOUR_BYTE_INT = 4;
  int expected_blb_size;
  int nr_expected_descs;
  int dimensions;

  // Extract set name
  const Json::Value &cmd = jsoncmd[_cmd_name];
  const std::string set_name = cmd["set"].asString();

  // Json::Value props = get_value<Json::Value>(cmd, "properties");

  // extract properties list and get filepath/object location of set
  Json::Value prop_list = get_value<Json::Value>(cmd, "batch_properties");
  const std::string set_path = get_set_path(query, set_name, dimensions);

  if (set_path.empty()) {
    error["info"] = "Set " + set_name + " not found";
    error["status"] = RSCommand::Error;
    return -1;
  }

  std::string label = get_value<std::string>(cmd, "label", "None");
  // props[VDMS_DESC_LABEL_PROP] = label;

  // retrieve the descriptor set from AWS here
  // operations are currently done in memory with no subsequent write to disk
  // so there's no need to re-upload to AWS
  if (_use_aws_storage) {
    retrieve_aws_descriptorSet(set_path);
  }

  // Note dimensionse are based on a 32 bit integer, hence the /4 math on size
  // as the string blob is sized in 8 bit ints.
  nr_expected_descs = prop_list.size();
  expected_blb_size = nr_expected_descs * dimensions * FOUR_BYTE_INT;

  // Verify length of input is matching expectations
  if (blob.length() != expected_blb_size) {
    std::cerr << "AddDescriptor::insert_descriptor: ";
    std::cerr << "Expected Blob Length Does Not Match Input ";
    std::cerr << "Input Length: " << blob.length() << " != "
              << "Expected Length: " << expected_blb_size << std::endl;
    error["info"] = "FV Input Length Mismatch";
    return -1;
  }

  long id = insert_descriptor(blob, set_path, nr_expected_descs, label, error);

  if (id < 0) {
    error["status"] = RSCommand::Error;

    if (_use_aws_storage) {
      // delete files in set_path
      std::uintmax_t n = fs::remove_all(set_path);
      std::cout << "Deleted " << n << " files or directories\n";
    }
    error["info"] = "FV Index Insert Failed";
    return -1;
  }

  // It passed the checker, so it exists.
  int set_ref = query.get_available_reference();

  Json::Value link;
  Json::Value results;
  Json::Value list_arr;
  list_arr.append(VDMS_DESC_SET_PATH_PROP);
  list_arr.append(VDMS_DESC_SET_DIM_PROP);
  results["list"] = list_arr;

  // constraints for getting set node to link to.
  Json::Value constraints;
  Json::Value name_arr;
  name_arr.append("==");
  name_arr.append(set_name);
  constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;
  bool unique = true;

  // Query set node-We only need to do this once, outside of the loop
  query.QueryNode(set_ref, VDMS_DESC_SET_TAG, link, constraints, results,
                  unique);

  for (int i = 0; i < nr_expected_descs; i++) {
    int node_ref = query.get_available_reference();
    Json::Value cur_props;
    cur_props = prop_list[i];

    std::string desc_id_prop_name =
        VDMS_DESC_ID_PROP + std::string("_") + set_name;
    cur_props[desc_id_prop_name.c_str()] = Json::Int64(id + i);

    cur_props[VDMS_DESC_LABEL_PROP] = label;

    query.AddNode(node_ref, VDMS_DESC_TAG, cur_props, Json::nullValue);

    // note this implicitly means that every node of a batch uses the same link
    if (cmd.isMember("link")) {
      add_link(query, cmd["link"], node_ref, VDMS_DESC_EDGE_TAG);
    }

    Json::Value props_edge;
    query.AddEdge(-1, set_ref, node_ref, VDMS_DESC_SET_EDGE_TAG, props_edge);
  }

  return 0;
}

int AddDescriptor::construct_protobuf(PMGDQuery &query,
                                      const Json::Value &jsoncmd,
                                      const std::string &blob, int grp_id,
                                      Json::Value &error) {

  bool batch_mode;
  int rc;
  const Json::Value &cmd = jsoncmd[_cmd_name];
  const std::string set_name = cmd["set"].asString();

  Json::Value prop_list = get_value<Json::Value>(cmd, "batch_properties");
  if (prop_list.size() == 0) {
    rc = add_single_descriptor(query, jsoncmd, blob, grp_id, error);
  } else {
    rc = add_descriptor_batch(query, jsoncmd, blob, grp_id, error);
  }

  if (rc < 0)
    error["status"] = RSCommand::Error;

  return rc;
}

Json::Value AddDescriptor::construct_responses(
    Json::Value &json_responses, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {
  Json::Value resp = check_responses(json_responses);

  Json::Value ret;
  ret[_cmd_name] = resp;
  return ret;
}

// ClassifyDescriptors Methods

ClassifyDescriptor::ClassifyDescriptor()
    : DescriptorsCommand("ClassifyDescriptor") {}

int ClassifyDescriptor::construct_protobuf(PMGDQuery &query,
                                           const Json::Value &jsoncmd,
                                           const std::string &blob, int grp_id,
                                           Json::Value &error) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  const std::string set_name = cmd["set"].asString();

  int dimensions;
  const std::string set_path = get_set_path(query, set_name, dimensions);

  if (set_path.empty()) {
    error["status"] = RSCommand::Error;
    error["info"] = "DescriptorSet Not Found!";
    return -1;
  }

  Json::Value link;
  Json::Value results;
  Json::Value list_arr;
  list_arr.append(VDMS_DESC_SET_PATH_PROP);
  list_arr.append(VDMS_DESC_SET_DIM_PROP);
  results["list"] = list_arr;

  Json::Value constraints;
  Json::Value name_arr;
  name_arr.append("==");
  name_arr.append(set_name);
  constraints[VDMS_DESC_SET_NAME_PROP] = name_arr;

  bool unique = true;

  // Query set node
  query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_DESC_SET_TAG, link,
                  constraints, results, unique);
  _dm->flush();

  return 0;
}

Json::Value ClassifyDescriptor::construct_responses(
    Json::Value &json_responses, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {
  Json::Value classifyDesc;
  const Json::Value &cmd = json[_cmd_name];

  Json::Value ret;

  bool flag_error = false;

  if (json_responses.size() == 0) {
    classifyDesc["status"] = RSCommand::Error;
    classifyDesc["info"] = "Not Found!";
    flag_error = true;
    ret[_cmd_name] = classifyDesc;
    return ret;
  }

  for (auto res : json_responses) {

    if (res["status"] != 0) {
      flag_error = true;
      break;
    }

    if (!res.isMember("entities"))
      continue;

    classifyDesc = res;

    for (auto &ent : classifyDesc["entities"]) {

      assert(ent.isMember(VDMS_DESC_SET_PATH_PROP));
      std::string set_path = ent[VDMS_DESC_SET_PATH_PROP].asString();
      try {
        VCL::DescriptorSet *set = _dm->get_descriptors_handler(set_path);

        auto labels = set->classify((float *)blob.data(), 1);

        if (labels.size() == 0) {
          classifyDesc["info"] = "No labels, cannot classify";
          classifyDesc["status"] = RSCommand::Error;
        } else {
          classifyDesc["label"] = (set->label_id_to_string(labels)).at(0);
        }
      } catch (VCL::Exception e) {
        print_exception(e);
        classifyDesc["status"] = RSCommand::Error;
        classifyDesc["info"] = "VCL Exception";
        flag_error = true;
        break;
      }
    }
  }

  if (!flag_error) {
    classifyDesc["status"] = RSCommand::Success;
  }

  classifyDesc.removeMember("entities");

  ret[_cmd_name] = classifyDesc;

  return ret;
}

// FindDescriptors Methods

FindDescriptor::FindDescriptor() : DescriptorsCommand("FindDescriptor") {}

bool FindDescriptor::need_blob(const Json::Value &cmd) {
  return cmd[_cmd_name].isMember("k_neighbors");
}

int FindDescriptor::construct_protobuf(PMGDQuery &query,
                                       const Json::Value &jsoncmd,
                                       const std::string &blob, int grp_id,
                                       Json::Value &cp_result) {
  const Json::Value &cmd = jsoncmd[_cmd_name];

  const std::string set_name = cmd["set"].asString();

  int dimensions;
  const std::string set_path = get_set_path(query, set_name, dimensions);
  std::string desc_id_prop_name =
      VDMS_DESC_ID_PROP + std::string("_") + set_name;

  if (set_path.empty()) {
    cp_result["status"] = RSCommand::Error;
    cp_result["info"] = "DescriptorSet Not Found!";
    return -1;
  }

  Json::Value results_set;
  Json::Value list_arr_set;
  list_arr_set.append(VDMS_DESC_SET_PATH_PROP);
  list_arr_set.append(VDMS_DESC_SET_DIM_PROP);
  results_set["list"] = list_arr_set;

  Json::Value constraints_set;
  Json::Value name_arr;
  name_arr.append("==");
  name_arr.append(set_name);
  constraints_set[VDMS_DESC_SET_NAME_PROP] = name_arr;

  bool unique = true;

  Json::Value constraints = cmd["constraints"];
  if (constraints.isMember("_label")) {
    constraints[VDMS_DESC_LABEL_PROP] = constraints["_label"];
    constraints.removeMember("_label");
  }
  if (constraints.isMember("_id")) {
    constraints[desc_id_prop_name.c_str()] = constraints["_id"];
    constraints.removeMember("_id");
  }

  Json::Value results = cmd["results"];

  // Add label/id as required.
  // Remove the variables with "_"
  if (results.isMember("list")) {
    int pos = -1;
    for (int i = 0; i < results["list"].size(); ++i) {
      if (results["list"][i].asString() == "_label" ||
          results["list"][i].asString() == "_id" ||
          results["list"][i].asString() == "_distance") {
        pos = i;
        Json::Value aux;
        results["list"].removeIndex(i, &aux);
        --i;
      }
    }
  }

  results["list"].append(VDMS_DESC_LABEL_PROP);
  results["list"].append(desc_id_prop_name.c_str());

  // Case (1)
  if (cmd.isMember("link")) {
    // Query for the Descriptors related to user-defined link
    // that match the user-defined constraints
    // We will need to do the AND operation
    // on the construct_response.

    int desc_ref = get_value<int>(cmd, "_ref", query.get_available_reference());

    query.QueryNode(desc_ref, VDMS_DESC_TAG, cmd["link"], constraints, results,
                    false);

    Json::Value link_to_desc;
    link_to_desc["ref"] = desc_ref;

    // Query for the set
    query.QueryNode(-1, VDMS_DESC_SET_TAG, link_to_desc, constraints_set,
                    results_set, false);
  }
  // Case (2)
  else if (!cmd.isMember("k_neighbors")) {
    // In this case, we either need properties of the descriptor
    // ("list") on the results block, or we need the descriptor nodes
    // because the user defined a reference.

    int ref_set = query.get_available_reference();

    Json::Value link_set; // null

    // Query for the set
    query.QueryNode(ref_set, VDMS_DESC_SET_TAG, link_set, constraints_set,
                    results_set, unique, true);

    Json::Value link_to_set;
    link_to_set["ref"] = ref_set;

    // Query for the Descriptors related to that set
    // that match the user-defined constraints
    query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_DESC_TAG, link_to_set,
                    constraints, results, false, false);
  }
  // Case (3), Just want the descriptor by value, we only need the set
  else {

    Json::Value link_null; // null
    if (cmd.isMember("_ref")) {
      cp_result["status"] = RSCommand::Error;
      cp_result["info"] = "_ref is not supported for KNN search";
      return -1;
    }
    const int k_neighbors = get_value<int>(cmd, "k_neighbors", 0);

    // This set query is a little weird and may be optimized away in the future
    //  as we no longer explicitly need the set links, however subsequent logic
    // uses this to look up the path (again) in construct response
    int ref_set = query.get_available_reference();

    // Query for the set and detect if exist during transaction.
    query.QueryNode(ref_set, VDMS_DESC_SET_TAG, Json::nullValue,
                    constraints_set, results_set, true);

    Json::Value link_to_set;
    link_to_set["ref"] = ref_set;

    if (!check_blob_size(blob, dimensions, 1)) {
      cp_result["status"] = RSCommand::Error;
      cp_result["info"] = "Blob (required) is null or size invalid";
      return -1;
    }

    try {
      VCL::DescriptorSet *set = _dm->get_descriptors_handler(set_path);

      // This is a way to pass state to the construct_response
      // We just pass the cache_object_id.
      auto cache_obj_id = VCL::get_uint64();
      cp_result["cache_obj_id"] = Json::Int64(cache_obj_id);

      _cache_map[cache_obj_id] = new IDDistancePair();

      IDDistancePair *pair = _cache_map[cache_obj_id];
      std::vector<long> &ids = pair->first;
      std::vector<float> &distances = pair->second;

      set->search((float *)blob.data(), 1, k_neighbors, ids, distances);

      long returned_counter = 0;
      std::string blob_return;

      Json::Value ids_array;

      for (int i = 0; i < ids.size(); ++i) {
        if (ids[i] >= 0) {
          ids_array.append(Json::Int64(ids[i]));
        } else {
          ids.erase(ids.begin() + i, ids.end());
          distances.erase(distances.begin() + i, distances.end());
          break;
        }
      }

      // This are needed to construct the response.
      if (!results.isMember("list")) {
        results["list"].append(VDMS_DESC_LABEL_PROP);
        results["list"].append(desc_id_prop_name);
      }

      Json::Value node_constraints = constraints;
      cp_result["ids_array"] = ids_array;
      for (int i = 0; i < ids.size(); ++i) {
        Json::Value k_node_constraints;

        // Theoretically this makes a deep copy
        k_node_constraints = constraints;

        // Create a vector with a string and an integer
        std::vector<Json::Value> values;
        values.push_back("==");
        values.push_back(ids[i]);

        // Add the vector to the JSON object with a key
        Json::Value jsonArray(Json::arrayValue);
        for (const auto &value : values) {
          jsonArray.append(value);
        }
        k_node_constraints[desc_id_prop_name] = jsonArray;

        results["limit"] = 1;

        query.QueryNode(get_value<int>(cmd, "_ref", -1), VDMS_DESC_TAG,
                        Json::nullValue, k_node_constraints, results, false);
      }

    } catch (VCL::Exception e) {
      print_exception(e);
      cp_result["status"] = RSCommand::Error;
      cp_result["info"] = "VCL Exception";
      return -1;
    }
  }

  return 0;
}

void FindDescriptor::populate_blobs(const std::string &set_path,
                                    std::string set_name,
                                    const Json::Value &results,
                                    Json::Value &entities,
                                    protobufs::queryMessage &query_res) {

  std::string desc_id_prop_name =
      VDMS_DESC_ID_PROP + std::string("_") + set_name;
  if (get_value<bool>(results, "blob", false)) {
    VCL::DescriptorSet *set = _dm->get_descriptors_handler(set_path);
    int dim = set->get_dimensions();

    for (auto &ent : entities) {
      long id = ent[desc_id_prop_name].asInt64();

      ent["blob"] = true;

      std::string *desc_blob = query_res.add_blobs();
      desc_blob->resize(sizeof(float) * dim);

      set->get_descriptors(&id, 1, (float *)(*desc_blob).data());
      if (output_vcl_timing) {
        set->timers.print_map_runtimes();
      }
      set->timers.clear_all_timers();
    }
  }
}

void FindDescriptor::convert_properties(Json::Value &entities,
                                        Json::Value &list,
                                        std::string set_name) {
  bool flag_label = false;
  bool flag_id = false;

  std::string desc_id_prop_name =
      VDMS_DESC_ID_PROP + std::string("_") + set_name;
  for (auto &prop : list) {
    if (prop.asString() == "_label") {
      flag_label = true;
    }
    if (prop.asString() == "_id") {
      flag_id = true;
    }
  }

  for (auto &element : entities) {

    if (element.isMember(VDMS_DESC_LABEL_PROP)) {
      if (flag_label)
        element["_label"] = element[VDMS_DESC_LABEL_PROP];
      element.removeMember(VDMS_DESC_LABEL_PROP);
    }
    if (element.isMember(desc_id_prop_name)) {
      if (flag_id)
        element["_id"] = element[desc_id_prop_name];
      element.removeMember(desc_id_prop_name);
    }
  }
}

Json::Value FindDescriptor::construct_responses(
    Json::Value &json_responses, const Json::Value &json,
    protobufs::queryMessage &query_res, const std::string &blob) {
  Json::Value findDesc;
  const Json::Value &cmd = json[_cmd_name];
  const Json::Value &cache = json["cp_result"];

  Json::Value ret;

  bool flag_error = false;

  const std::string set_name = cmd["set"].asString();
  std::string desc_id_prop_name =
      VDMS_DESC_ID_PROP + std::string("_") + set_name;

  auto error = [&](Json::Value &res) {
    ret[_cmd_name] = res;
    return ret;
  };

  if (json_responses.size() == 0) {
    Json::Value return_error;
    return_error["status"] = RSCommand::Error;
    return_error["info"] = "Not Found!";
    return error(return_error);
  }

  const Json::Value &results = cmd["results"];
  Json::Value list = get_value<Json::Value>(results, "list");

  // Case (1)
  if (cmd.isMember("link")) {

    assert(json_responses.size() == 2);

    findDesc = json_responses[0];

    if (findDesc["status"] != 0) {
      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "Descriptors Not Found";
      return error(return_error);
    }

    const Json::Value &set_response = json_responses[1];
    const Json::Value &set = set_response["entities"][0];

    // These properties should always exist
    assert(set.isMember(VDMS_DESC_SET_PATH_PROP));
    assert(set.isMember(VDMS_DESC_SET_DIM_PROP));
    std::string set_path = set[VDMS_DESC_SET_PATH_PROP].asString();
    int dim = set[VDMS_DESC_SET_DIM_PROP].asInt();

    if (findDesc.isMember("entities")) {
      try {
        Json::Value &entities = findDesc["entities"];
        populate_blobs(set_path, set_name, results, entities, query_res);
        convert_properties(entities, list, set_name);
      } catch (VCL::Exception e) {
        print_exception(e);
        findDesc["status"] = RSCommand::Error;
        findDesc["info"] = "VCL Exception";
        return error(findDesc);
      }
    }
  }
  // Case (2)
  else if (!cmd.isMember("k_neighbors")) {

    assert(json_responses.size() == 2);

    const Json::Value &set_response = json_responses[0];
    const Json::Value &set = set_response["entities"][0];

    // These properties should always exist
    assert(set.isMember(VDMS_DESC_SET_PATH_PROP));
    assert(set.isMember(VDMS_DESC_SET_DIM_PROP));
    std::string set_path = set[VDMS_DESC_SET_PATH_PROP].asString();
    int dim = set[VDMS_DESC_SET_DIM_PROP].asInt();

    findDesc = json_responses[1];

    if (findDesc.isMember("entities")) {
      try {
        Json::Value &entities = findDesc["entities"];
        populate_blobs(set_path, set_name, results, entities, query_res);
        convert_properties(entities, list, set_name);
      } catch (VCL::Exception e) {
        print_exception(e);
        findDesc["status"] = RSCommand::Error;
        findDesc["info"] = "VCL Exception";
        return error(findDesc);
      }
    }

    if (findDesc["status"] != 0) {
      std::cerr << json_responses.toStyledString() << std::endl;
      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "Descriptors Not Found";
      return error(return_error);
    }
  }
  // Case (3)
  else {
    // Get Set info.
    const Json::Value &set_response = json_responses[0];

    if (set_response["status"] != 0 || !set_response.isMember("entities")) {

      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "Set Not Found";
      return error(return_error);
    }

    assert(set_response["entities"].size() == 1);

    const Json::Value &set = set_response["entities"][0];

    // This properties should always exist
    assert(set.isMember(VDMS_DESC_SET_PATH_PROP));
    assert(set.isMember(VDMS_DESC_SET_DIM_PROP));
    std::string set_path = set[VDMS_DESC_SET_PATH_PROP].asString();
    int dim = set[VDMS_DESC_SET_DIM_PROP].asInt();

    if (!check_blob_size(blob, dim, 1)) {
      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "Blob (required) is null or size invalid";
      return error(return_error);
    }

    std::vector<long> *ids;
    std::vector<float> *distances;

    bool compute_distance = false;

    Json::Value list = get_value<Json::Value>(results, "list");

    for (auto &prop : list) {
      if (prop.asString() == "_distance") {
        compute_distance = true;
        break;
      }
    }

    // Test whether there is any cached result.
    assert(cache.isMember("cache_obj_id"));

    long cache_obj_id = cache["cache_obj_id"].asInt64();

    assert(cache.isMember("ids_array"));
    Json::Value ids_array = cache["ids_array"];

    // Get from Cache
    IDDistancePair *pair = _cache_map[cache_obj_id];
    ids = &(pair->first);
    distances = &(pair->second);

    Json::Value combined_tx_constraints;
    Json::Value set_values;
    Json::Value ent_values;
    for (int i = 0; i < json_responses.size(); ++i) {

      // Create a vector with a string and an integer
      if (i == 0)
        continue;
      else {
        Json::Value desc_data;
        for (auto ent : json_responses[i]["entities"])
          desc_data = ent;
        ent_values.append(desc_data);
      }
      // Add the vector to the JSON object with a key
    }
    set_values["status"] = 0;
    set_values["returned"] = json_responses.size() - 1;
    set_values["entities"] = ent_values;

    findDesc = set_values;

    if (findDesc["status"] != 0 || !findDesc.isMember("entities")) {

      Json::Value return_error;
      return_error["status"] = RSCommand::Error;
      return_error["info"] = "Descriptor Not Found in graph!";
      return error(return_error);
    }

    Json::Value aux_entities = findDesc["entities"];
    findDesc.removeMember("entities");

    uint64_t new_cnt = 0;
    for (int i = 0; i < (*ids).size(); ++i) {

      Json::Value desc_data;

      long d_id = (*ids)[i];
      bool pass_constraints = false;

      for (auto ent : aux_entities) {
        if (ent[desc_id_prop_name].asInt64() == d_id) {
          for (int idx = 0; idx < ids_array.size(); ++idx) {
            if (ent[desc_id_prop_name].asInt64() == ids_array[idx].asInt64()) {
              desc_data = ent;
              pass_constraints = true;
              break;
            }
          }
        }
        if (pass_constraints) {
          break;
        }
      }

      if (!pass_constraints)
        continue;

      if (compute_distance) {
        desc_data["_distance"] = (*distances)[i];

        // Should be already sorted,
        // but if not, we need to match the id with
        // whatever is on the cache
        // desc_data["cache_id"]  = Json::Int64((*ids)[i]);
      }

      findDesc["entities"].append(desc_data);
      new_cnt++;
    }

    if (findDesc.isMember("returned"))
      findDesc["returned"] = Json::Int64(new_cnt);

    if (findDesc.isMember("entities")) {
      try {
        Json::Value &entities = findDesc["entities"];
        populate_blobs(set_path, set_name, results, entities, query_res);
        convert_properties(entities, list, set_name);
      } catch (VCL::Exception e) {
        print_exception(e);
        findDesc["status"] = RSCommand::Error;
        findDesc["info"] = "VCL Exception";
        return error(findDesc);
      }
    }

    if (cache.isMember("cache_obj_id")) {
      // We remove the vectors associated with that entry to
      // free memory, without removing the entry from _cache_map
      // because tbb does not have a lock free way to do this.
      IDDistancePair *pair = _cache_map[cache["cache_obj_id"].asInt64()];
      delete pair;
    }
  }

  if (findDesc.isMember("entities")) {
    for (auto &ent : findDesc["entities"]) {
      if (ent.getMemberNames().size() == 0) {
        findDesc.removeMember("entities");
        break;
      }
    }
  }

  findDesc["status"] = RSCommand::Success;
  ret[_cmd_name] = findDesc;

  return ret;
}
