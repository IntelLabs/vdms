/**
 * @file   FlinngDescriptorSet.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2021 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <assert.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

#include <dirent.h>
#include <sys/types.h>

#include "FlinngDescriptorSet.h"
#include "vcl/Exception.h"

#define FLINNG_IDX_FILE_NAME "flinng.idx"
#define IDS_IDX_FILE_NAME "flinng_ids.arr"

using namespace VCL;

void FlinngDescriptorSet::getFlinngParams(VCL::DescriptorParams *par,
                                          flinng::FlinngBuilder *buidler) {
  buidler->num_rows = par->num_rows;
  buidler->cells_per_row = par->cells_per_row;
  buidler->num_hash_tables = par->num_hash_tables;
  buidler->hashes_per_table = par->hashes_per_table;
  buidler->sub_hash_bits =
      par->sub_hash_bits; // sub_hash_bits * hashes_per_table must be less than
                          // 32, otherwise segfault will happen
  buidler->cut_off = par->cut_off;
}

FlinngDescriptorSet::FlinngDescriptorSet(const std::string &set_path)
    : DescriptorSetData(set_path) {
  _flinng_file = _set_path + "/" + FLINNG_IDX_FILE_NAME;
  _index = flinng::BaseDenseFlinng32::from_index(_flinng_file.c_str());
  is_finalized = false;
  read_label_ids();
  read_labels_map();
}

FlinngDescriptorSet::FlinngDescriptorSet(const std::string &set_path,
                                         unsigned dim, DistanceMetric metric,
                                         VCL::DescriptorParams *par)
    : DescriptorSetData(set_path, dim) {
  _index = 0;
  _flinng_file = _set_path + "/" + FLINNG_IDX_FILE_NAME;
  is_finalized = false;
  flinng::FlinngBuilder *builder = new flinng::FlinngBuilder();
  getFlinngParams(par, builder);

  if (metric == L2) {
    _index = new flinng::L2DenseFlinng32(dim, builder);
    //_index = new L2DenseFlinng32(num_rows, cells_per_row, data_dimension,
    // num_hash_tables, hashes_per_table, sub_hash_bits, cutoff);

  } else if (metric == IP) {
    _index = new flinng::DenseFlinng32(dim, builder);
    //_index = new DenseFlinng32(num_rows, cells_per_row, data_dimension,
    // num_hash_tables, hashes_per_table);
  } else
    throw VCLException(UnsupportedIndex, "Metric Not implemented");
}

FlinngDescriptorSet::~FlinngDescriptorSet() {}

void FlinngDescriptorSet::finalize_index() {
  _index->finalize_construction();
  // placeholder for any operations post indexing
}

void FlinngDescriptorSet::write_label_ids() {
  std::ofstream out_ids(_set_path + "/" + IDS_IDX_FILE_NAME,
                        std::ofstream::binary);

  unsigned ids_size = _label_ids.size();
  out_ids.write((char *)&ids_size, sizeof(ids_size));
  out_ids.write((char *)_label_ids.data(), sizeof(long) * ids_size);
  out_ids.close();
}

void FlinngDescriptorSet::read_label_ids() {
  std::ifstream in_ids(_set_path + "/" + IDS_IDX_FILE_NAME,
                       std::ofstream::binary);

  if (!in_ids.good()) {
    throw VCLException(OpenFailed, "Cannot read labels file");
  }

  unsigned ids_size;
  in_ids.read((char *)&ids_size, sizeof(ids_size));
  _label_ids.resize(ids_size);
  in_ids.read((char *)_label_ids.data(), sizeof(long) * ids_size);
  in_ids.close();
}

long FlinngDescriptorSet::add(float *descriptors, unsigned n,
                              long *labels = NULL) {
  assert(n > 0);

  _lock.lock();

  is_finalized = false;

  long id_first = _n_total;

  if (labels != NULL) {
    _label_ids.resize(_n_total + n);
    long *dst = _label_ids.data() + _n_total;
    memcpy(dst, labels, n * sizeof(long));
  }

  _index->add(descriptors, n);
  _n_total += n;

  _lock.unlock();
  return id_first;
}

long FlinngDescriptorSet::add_and_store(float *descriptors, unsigned n,
                                        long *labels = NULL) {
  assert(n > 0);

  _lock.lock();

  is_finalized = false;

  long id_first = _n_total;

  if (labels != NULL) {
    _label_ids.resize(_n_total + n);
    long *dst = _label_ids.data() + _n_total;

    memcpy(dst, labels, n * sizeof(long));
  }

  _index->add_and_store(descriptors, n);
  _n_total += n;
  _lock.unlock();
  return id_first;
}

void FlinngDescriptorSet::train() {
  throw VCLException(NotImplemented,
                     "Train Operation not supported for used Index");
  // Not applicable for flinng
}

void FlinngDescriptorSet::train(float *descriptors, unsigned n) {
  throw VCLException(NotImplemented,
                     "Train Operation not supported for used Index");
  // Not applicable for flinng
}

bool FlinngDescriptorSet::is_trained() { return false; }

void FlinngDescriptorSet::search(float *query, unsigned n_queries, unsigned k,
                                 long *descriptors, float *distances) {
  if (!is_finalized) {
    _lock.lock();
    finalize_index();
    is_finalized = true;
    _lock.unlock();
  }
  _index->search_with_distance(query, n_queries, k, descriptors, distances);
}

void FlinngDescriptorSet::search(float *query, unsigned n_queries, unsigned k,
                                 long *descriptors) {
  if (!is_finalized) {
    _lock.lock();
    finalize_index();
    is_finalized = true;
    _lock.unlock();
  }
  _index->search(query, n_queries, k, descriptors);
}

void FlinngDescriptorSet::radius_search(float *query, float radius,
                                        long *descriptors, float *distances) {
  throw VCLException(NotImplemented,
                     "Radius Search Operation not supported for used Index");
  // TODO
}

void FlinngDescriptorSet::classify(float *descriptors, unsigned n, long *ids,
                                   unsigned quorum) {
  float *distances = new float[n * quorum];
  long *ids_aux = new long[n * quorum];

  search(descriptors, n, quorum, ids_aux, distances);

  _lock.lock();
  for (int j = 0; j < n; ++j) {
    std::map<long, int> map_voting;
    long winner = -1;
    unsigned max = 0;
    for (int i = 0; i < quorum; ++i) {
      long idx = ids_aux[quorum * j + i];
      if (idx < 0)
        continue; // Means not found

      long label_id = _label_ids.at(idx);
      map_voting[label_id] += 1;
      if (max < map_voting[label_id]) {
        max = map_voting[label_id];
        winner = label_id;
      }
    }
    ids[j] = winner;
  }
  _lock.unlock();
}

void FlinngDescriptorSet::get_labels(long *ids, unsigned n, long *labels) {
  _lock.lock();

  for (int i = 0; i < n; ++i) {
    long idx = ids[i];
    if (idx > _label_ids.size()) {
      throw VCLException(ObjectNotFound, "Label id does not exists");
    }
    labels[i] = _label_ids[idx];
  }

  _lock.unlock();
}

void FlinngDescriptorSet::get_descriptors(long *ids, unsigned n,
                                          float *descriptors) {
  int offset = 0;
  for (int i = 0; i < n; ++i) {
    _index->fetch_descriptors(ids[i], descriptors + i * _dimensions);
  }
}

void FlinngDescriptorSet::store() { store(_set_path); }

void FlinngDescriptorSet::store(std::string set_path) {
  _lock.lock();
  _set_path = set_path;
  _flinng_file = _set_path + "/" + FLINNG_IDX_FILE_NAME;

  int ret = create_dir(_set_path.c_str());
  if (ret == 0 || ret == EEXIST) { // Directory exists or created
    _index->write_index(_flinng_file.c_str());
    write_label_ids();
    _lock.unlock();

    write_labels_map();
  } else {
    throw VCLException(OpenFailed, _flinng_file +
                                       "cannot be created or written. " +
                                       "Error: " + std::to_string(ret));
  }
}
