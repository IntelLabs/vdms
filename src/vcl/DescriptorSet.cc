/**
 * @file   DescriptorSet.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
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
#include <iostream>
#include <stdlib.h>
#include <string>

// clang-format off
#include "vcl/DescriptorSet.h"
#include "DescriptorSetData.h"
#include "DescriptorParams.h"
#include "FaissDescriptorSet.h"
#include "TDBDescriptorSet.h"
#include "FlinngDescriptorSet.h"
// clang-format on

#define INFO_FILE_NAME "eng_info.txt"

namespace fs = std::filesystem;

namespace VCL {

DescriptorSet::DescriptorSet(const std::string &set_path) {
  read_set_info(set_path);
  _remote = nullptr;

  if (_eng == DescriptorSetEngine(FaissFlat))
    _set = new FaissFlatDescriptorSet(set_path);
  else if (_eng == DescriptorSetEngine(FaissIVFFlat))
    _set = new FaissIVFFlatDescriptorSet(set_path);
  else if (_eng == DescriptorSetEngine(TileDBDense))
    _set = new TDBDenseDescriptorSet(set_path);
  else if (_eng == DescriptorSetEngine(TileDBSparse))
    _set = new TDBSparseDescriptorSet(set_path);
  else if (_eng == DescriptorSetEngine(Flinng))
    _set = new FlinngDescriptorSet(set_path);
  else {
    std::cerr << "Index Not supported" << std::endl;
    throw VCLException(UnsupportedIndex, "Index not supported");
  }
}

DescriptorSet::DescriptorSet(const std::string &set_path, unsigned dim,
                             DescriptorSetEngine eng, DistanceMetric metric,
                             VCL::DescriptorParams *param)
    : _eng(eng) {
  _remote = nullptr;

  if (eng == DescriptorSetEngine(FaissFlat))
    _set = new FaissFlatDescriptorSet(set_path, dim, metric);
  else if (eng == DescriptorSetEngine(FaissIVFFlat))
    _set = new FaissIVFFlatDescriptorSet(set_path, dim, metric);
  else if (eng == DescriptorSetEngine(TileDBDense))
    _set = new TDBDenseDescriptorSet(set_path, dim, metric);
  else if (eng == DescriptorSetEngine(TileDBSparse))
    _set = new TDBSparseDescriptorSet(set_path, dim, metric);
  else if (eng == DescriptorSetEngine(Flinng))
    _set = new FlinngDescriptorSet(set_path, dim, metric, param);
  else {
    std::cerr << "Index Not supported" << std::endl;
    throw VCLException(UnsupportedIndex, "Index not supported");
  }
}

DescriptorSet::~DescriptorSet() { delete _set; }

void DescriptorSet::write_set_info() {
  timers.add_timestamp("write_set_info");
  std::string path = _set->get_path() + "/" + INFO_FILE_NAME;
  std::ofstream info_file(path);
  info_file << _eng << std::endl;
  info_file.close();
  timers.add_timestamp("write_set_info");
}

void DescriptorSet::read_set_info(const std::string &set_path) {
  timers.add_timestamp("read_set_info");
  std::string path = set_path + "/" + INFO_FILE_NAME;
  std::ifstream info_file(path);

  if (!info_file.good()) {
    std::cout << "cannot open: " << path << std::endl;
    throw VCLException(OpenFailed, "Cannot open: " + path);
  }

  int num;
  std::string str;
  std::getline(info_file, str);
  std::stringstream sstr(str);
  sstr >> num;
  _eng = (DescriptorSetEngine)num;
  info_file.close();
  timers.add_timestamp("read_set_info");
}

/*  *********************** */
/*      CORE INTERFACE      */
/*  *********************** */

std::string DescriptorSet::get_path() { return _set->get_path(); }

unsigned DescriptorSet::get_dimensions() { return _set->get_dimensions(); }

long DescriptorSet::get_n_descriptors() { return _set->get_n_total(); }

void DescriptorSet::search(DescDataArray queries, unsigned n_queries,
                           unsigned k, long *descriptors_ids,
                           float *distances) {
  timers.add_timestamp("desc_set_search");
  _set->search(queries, n_queries, k, descriptors_ids, distances);
  timers.add_timestamp("desc_set_search");
}

void DescriptorSet::search(DescDataArray queries, unsigned n_queries,
                           unsigned k, long *descriptors_ids) {
  timers.add_timestamp("desc_set_search");
  _set->search(queries, n_queries, k, descriptors_ids);
  timers.add_timestamp("desc_set_search");
}

void DescriptorSet::radius_search(DescData queries, float radius,
                                  long *descriptors_ids, float *distances) {
  timers.add_timestamp("desc_set_radius_search");
  _set->radius_search(queries, radius, descriptors_ids, distances);
  timers.add_timestamp("desc_set_radius_search");
}

long DescriptorSet::add(DescDataArray descriptors, unsigned n, long *labels) {
  long rc;
  timers.add_timestamp("desc_set_add");
  rc = _set->add(descriptors, n, labels);
  timers.add_timestamp("desc_set_add");
  return rc;
}

long DescriptorSet::add_and_store(DescDataArray descriptors, unsigned n,
                                  long *labels) {
  long rc;
  timers.add_timestamp("desc_set_add_and_store");
  rc = _set->add_and_store(descriptors, n, labels);
  timers.add_timestamp("desc_set_add_and_store");
  return rc;
}

void DescriptorSet::train() {
  timers.add_timestamp("desc_set_add_and_store");
  _set->train();
  timers.add_timestamp("desc_set_add_and_store");
}

void DescriptorSet::finalize_index() {
  timers.add_timestamp("desc_set_finalize_idx");
  _set->finalize_index();
  timers.add_timestamp("desc_set_finalize_idx");
}

void DescriptorSet::train(DescDataArray descriptors, unsigned n) {
  timers.add_timestamp("desc_set_train");
  _set->train(descriptors, n);
  timers.add_timestamp("desc_set_train");
}

bool DescriptorSet::is_trained() { return _set->is_trained(); }

void DescriptorSet::classify(DescDataArray descriptors, unsigned n,
                             long *labels, unsigned quorum) {
  timers.add_timestamp("desc_set_classify");
  _set->classify(descriptors, n, labels, quorum);
  timers.add_timestamp("desc_set_classify");
}

void DescriptorSet::get_descriptors(long *ids, unsigned n,
                                    DescDataArray descriptors) {
  timers.add_timestamp("desc_set_get_descs");
  _set->get_descriptors(ids, n, descriptors);
  timers.add_timestamp("desc_set_get_descs");
}

void DescriptorSet::store() {
  timers.add_timestamp("desc_set_store");
  _set->store();
  write_set_info();

  // grab the descriptor files from local storage, upload them, delete the local
  // copies not deleting the local copies currently to resolve concurrency
  // issues
  if (_storage == VDMS::StorageType::AWS) {
    std::string dir_path = _set->get_path();
    std::vector<std::string> filenames;

    for (const auto &file : fs::directory_iterator(dir_path)) {
      filenames.push_back(file.path());
    }

    for (int i = 0; i < filenames.size(); i++) {
      bool result = _remote->Write(filenames[i]);
      if (!result) {
        throw VCLException(ObjectNotFound,
                           "Descriptor: File was not added: " + filenames[i]);
      }
      // std::remove(filename.c_str());
    }
  }
  timers.add_timestamp("desc_set_store");
}

void DescriptorSet::store(std::string set_path) {
  timers.add_timestamp("desc_set_store");
  _set->store(set_path);
  write_set_info();
  timers.add_timestamp("desc_set_store");
}

/*  *********************** */
/*   VECTOR-BASED INTERFACE */
/*  *********************** */

long DescriptorSet::add(DescDataArray descriptors, unsigned n,
                        LabelIdVector &labels) {
  long rc;
  timers.add_timestamp("desc_set_add_vec");
  if (n != labels.size() && labels.size() != 0)
    throw VCLException(SizeMismatch, "Labels Vector of Wrong Size");
  rc = add(descriptors, n, labels.size() > 0 ? (long *)labels.data() : NULL);
  timers.add_timestamp("desc_set_add_vec");

  return rc;
}

long DescriptorSet::add_and_store(DescDataArray descriptors, unsigned n,
                                  LabelIdVector &labels) {
  timers.add_timestamp("desc_set_add_store_vec");
  long rc;
  if (n != labels.size() && labels.size() != 0)
    throw VCLException(SizeMismatch, "Labels Vector of Wrong Size");

  rc = add_and_store(descriptors, n,
                     labels.size() > 0 ? (long *)labels.data() : NULL);

  timers.add_timestamp("desc_set_add_store_vec");
  return rc;
}

void DescriptorSet::search(DescDataArray queries, unsigned n, unsigned k,
                           DescIdVector &ids, DistanceVector &distances) {
  timers.add_timestamp("search");
  ids.resize(n * k);
  distances.resize(n * k);
  search(queries, n, k, ids.data(), distances.data());
  timers.add_timestamp("search");
}

void DescriptorSet::search(DescDataArray queries, unsigned n, unsigned k,
                           DescIdVector &ids) {
  timers.add_timestamp("desc_set_search");
  ids.resize(n * k);
  search(queries, n, k, ids.data());
  timers.add_timestamp("desc_set_search");
}

std::vector<long> DescriptorSet::classify(DescDataArray descriptors, unsigned n,
                                          unsigned quorum) {
  timers.add_timestamp("desc_set_vec_classify");
  LabelIdVector labels;
  labels.resize(n);
  classify(descriptors, n, labels.data(), quorum);
  timers.add_timestamp("desc_set_vec_classify");
  return labels;
}

void DescriptorSet::get_descriptors(std::vector<long> &ids,
                                    float *descriptors) {
  timers.add_timestamp("desc_set_vec_get_desc");
  get_descriptors(ids.data(), ids.size(), descriptors);
  timers.add_timestamp("desc_set_vec_get_desc");
}

/*  *********************** */
/*   STRING-LABELS SUPPORT  */
/*  *********************** */

void DescriptorSet::set_labels_map(std::map<long, std::string> &labels) {
  return _set->set_labels_map(labels);
}

std::map<long, std::string> DescriptorSet::get_labels_map() {
  return _set->get_labels_map();
}

void DescriptorSet::set_labels_map(LabelIdVector &ids,
                                   std::vector<std::string> &labels) {
  assert(ids.size() == labels.size());
  std::map<long, std::string> labels_map;
  for (int i = 0; i < ids.size(); ++i) {
    labels_map[ids[i]] = labels[i];
  }

  set_labels_map(labels_map);
}

std::vector<std::string>
DescriptorSet::label_id_to_string(LabelIdVector &l_id) {
  std::vector<std::string> ret_labels(l_id.size());
  std::map<long, std::string> labels_map = _set->get_labels_map();

  for (int i = 0; i < l_id.size(); ++i) {
    ret_labels[i] = labels_map[l_id[i]];
  }
  return ret_labels;
}

long DescriptorSet::get_label_id(const std::string &label) {
  auto map = _set->get_labels_map();

  for (auto it = map.begin(); it != map.end(); ++it) {
    if (it->second == label) {
      return it->first;
    }
  }

  long id = map.size();
  map[id] = label;
  _set->set_labels_map(map);

  return id;
}

std::vector<std::string> DescriptorSet::get_str_labels(DescIdVector &ids) {
  return _set->get_str_labels(ids.data(), ids.size());
}

void DescriptorSet::set_connection(RemoteConnection *remote) {
  if (!remote->connected())
    remote->start();

  if (!remote->connected()) {
    throw VCLException(SystemNotFound, "No remote connection started");
  }

  _remote = remote;
  _storage = VDMS::StorageType::AWS;
}

} // namespace VCL
