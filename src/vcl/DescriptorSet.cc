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
  std::string path = _set->get_path() + "/" + INFO_FILE_NAME;
  std::ofstream info_file(path);
  info_file << _eng << std::endl;
  info_file.close();
}

void DescriptorSet::read_set_info(const std::string &set_path) {
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
  _set->search(queries, n_queries, k, descriptors_ids, distances);
}

void DescriptorSet::search(DescDataArray queries, unsigned n_queries,
                           unsigned k, long *descriptors_ids) {
  _set->search(queries, n_queries, k, descriptors_ids);
}

void DescriptorSet::radius_search(DescData queries, float radius,
                                  long *descriptors_ids, float *distances) {
  _set->radius_search(queries, radius, descriptors_ids, distances);
}

long DescriptorSet::add(DescDataArray descriptors, unsigned n, long *labels) {
  return _set->add(descriptors, n, labels);
}

long DescriptorSet::add_and_store(DescDataArray descriptors, unsigned n,
                                  long *labels) {
  return _set->add_and_store(descriptors, n, labels);
}

void DescriptorSet::train() { _set->train(); }

void DescriptorSet::finalize_index() { _set->finalize_index(); }

void DescriptorSet::train(DescDataArray descriptors, unsigned n) {
  _set->train(descriptors, n);
}

bool DescriptorSet::is_trained() { return _set->is_trained(); }

void DescriptorSet::classify(DescDataArray descriptors, unsigned n,
                             long *labels, unsigned quorum) {
  _set->classify(descriptors, n, labels, quorum);
}

void DescriptorSet::get_descriptors(long *ids, unsigned n,
                                    DescDataArray descriptors) {
  _set->get_descriptors(ids, n, descriptors);
}

void DescriptorSet::store() {
  _set->store();
  write_set_info();

  // grab the descriptor files from local storage, upload them, delete the local
  // copies not deleting the local copies currently to resolve concurrency
  // issues
  if (_storage == Storage::AWS) {
    std::string dir_path = _set->get_path();
    std::vector<std::string> filenames;

    for (const auto &file : fs::directory_iterator(dir_path)) {
      filenames.push_back(file.path());
    }

    for (int i = 0; i < filenames.size(); i++) {
      _remote->Write(filenames[i]);
      // std::remove(filename.c_str());
    }
  }
}

void DescriptorSet::store(std::string set_path) {
  _set->store(set_path);
  write_set_info();
}

/*  *********************** */
/*   VECTOR-BASED INTERFACE */
/*  *********************** */

long DescriptorSet::add(DescDataArray descriptors, unsigned n,
                        LabelIdVector &labels) {
  if (n != labels.size() && labels.size() != 0)
    throw VCLException(SizeMismatch, "Labels Vector of Wrong Size");

  return add(descriptors, n, labels.size() > 0 ? (long *)labels.data() : NULL);
}

long DescriptorSet::add_and_store(DescDataArray descriptors, unsigned n,
                                  LabelIdVector &labels) {
  if (n != labels.size() && labels.size() != 0)
    throw VCLException(SizeMismatch, "Labels Vector of Wrong Size");

  return add_and_store(descriptors, n,
                       labels.size() > 0 ? (long *)labels.data() : NULL);
}

void DescriptorSet::search(DescDataArray queries, unsigned n, unsigned k,
                           DescIdVector &ids, DistanceVector &distances) {
  ids.resize(n * k);
  distances.resize(n * k);
  search(queries, n, k, ids.data(), distances.data());
}

void DescriptorSet::search(DescDataArray queries, unsigned n, unsigned k,
                           DescIdVector &ids) {
  ids.resize(n * k);
  search(queries, n, k, ids.data());
}

std::vector<long> DescriptorSet::classify(DescDataArray descriptors, unsigned n,
                                          unsigned quorum) {
  LabelIdVector labels;
  labels.resize(n);
  classify(descriptors, n, labels.data(), quorum);
  return labels;
}

void DescriptorSet::get_descriptors(std::vector<long> &ids,
                                    float *descriptors) {
  get_descriptors(ids.data(), ids.size(), descriptors);
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
  _storage = Storage::AWS;
}
} // namespace VCL
