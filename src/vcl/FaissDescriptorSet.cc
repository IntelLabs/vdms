/**
 * @file   FaissDescriptorSet.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
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

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <string>

#include <dirent.h>
#include <sys/types.h>

#include "FaissDescriptorSet.h"
#include "vcl/Exception.h"

#include "faiss/impl/AuxIndexStructures.h"
#include <faiss/impl/FaissException.h>
#include <faiss/index_io.h>

#define FAISS_IDX_FILE_NAME "faiss.idx"
#define IDS_IDX_FILE_NAME "ids.arr"

using namespace VCL;

FaissDescriptorSet::FaissDescriptorSet(const std::string &set_path)
    : DescriptorSetData(set_path) {
  _index = 0;
  _faiss_file = _set_path + "/" + FAISS_IDX_FILE_NAME;
  read_label_ids();
  read_labels_map();
}

FaissDescriptorSet::FaissDescriptorSet(const std::string &set_path,
                                       unsigned dim)
    : DescriptorSetData(set_path, dim) {
  _index = 0;
  _faiss_file = _set_path + "/" + FAISS_IDX_FILE_NAME;
}

FaissDescriptorSet::~FaissDescriptorSet() {}

void FaissDescriptorSet::write_label_ids() {
  std::ofstream out_ids(_set_path + "/" + IDS_IDX_FILE_NAME,
                        std::ofstream::binary);

  unsigned ids_size = _label_ids.size();
  out_ids.write((char *)&ids_size, sizeof(ids_size));
  out_ids.write((char *)_label_ids.data(), sizeof(long) * ids_size);
  out_ids.close();
}

void FaissDescriptorSet::read_label_ids() {
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

long FaissDescriptorSet::add(float *descriptors, unsigned n, long *labels) {
  assert(n > 0);

  _lock.lock();

  long id_first = _index->ntotal;

  if (labels != NULL) {
    _label_ids.resize(_index->ntotal + n);
    long *dst = _label_ids.data() + _index->ntotal;
    std::memcpy(dst, labels, n * sizeof(long));
  }

  _index->add(n, descriptors);
  _n_total = _index->ntotal;
  _lock.unlock();

  return id_first;
}

void FaissDescriptorSet::train() { train_core(NULL, 0); }

void FaissDescriptorSet::train(float *descriptors, unsigned n) {
  train_core(descriptors, n);
}

void FaissDescriptorSet::train_core(float *descriptors, unsigned n) {
  _lock.lock();
  long n_total = _index->ntotal;
  float *recons = new float[n_total * _dimensions];
  _index->reconstruct_n(0, n_total, recons);
  _index->reset();
  _index->train(n == 0 ? n_total : n, n == 0 ? recons : descriptors);
  _index->add(n_total, recons);
  _lock.unlock();

  delete[] recons;
}

bool FaissDescriptorSet::is_trained() { return _index->is_trained; }

// No need to use locks on this read-only operation as faiss handles internally
void FaissDescriptorSet::search(float *query, unsigned n_queries, unsigned k,
                                long *descriptors, float *distances) {
  _index->search(n_queries, query, k, distances, descriptors);
}

void FaissDescriptorSet::radius_search(float *query, float radius,
                                       long *descriptors, float *distances) {
  faiss::RangeSearchResult rs(1); // 1 is the Number of queries
  _index->range_search(1, query, radius, &rs);

  // rs.lims is of size 2, as nq is of size 1.
  // Check faiss::RangeSearchResult definition for more details.
  long found = rs.lims[1];
  std::memcpy(descriptors, rs.labels, sizeof(long) * found);
  std::memcpy(distances, rs.distances, sizeof(float) * found);
}

void FaissDescriptorSet::classify(float *descriptors, unsigned n, long *ids,
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

void FaissDescriptorSet::get_labels(long *ids, unsigned n, long *labels) {
  _lock.lock();

  for (int i = 0; i < n; ++i) {
    long idx = ids[i];
    if (idx > _label_ids.size()) {
      _lock.unlock(); // unlock before throwing exception
      throw VCLException(ObjectNotFound, "Label id does not exists");
    }
    labels[i] = _label_ids[idx];
  }

  _lock.unlock();
}

void FaissDescriptorSet::get_descriptors(long *ids, unsigned n,
                                         float *descriptors) {
  int offset = 0;

  try {
    for (int i = 0; i < n; ++i) {
      _index->reconstruct(ids[i], descriptors + i * _dimensions);
    }
  } catch (faiss::FaissException &e) {
    throw VCLException(UndefinedException, "faiss::reconstruct(3) failed");
  }
}

void FaissDescriptorSet::store() { store(_set_path); }

void FaissDescriptorSet::store(std::string set_path) {
  _lock.lock();
  _set_path = set_path;
  _faiss_file = _set_path + "/" + FAISS_IDX_FILE_NAME;

  int ret = create_dir(_set_path.c_str());
  if (ret == 0 || ret == EEXIST) { // Directory exists or created
    faiss::write_index((const faiss::IndexFlat *)(_index), _faiss_file.c_str());
    write_label_ids();
    _lock.unlock();

    write_labels_map();
  } else {
    _lock.unlock(); // unlock before throwing exception
    throw VCLException(OpenFailed, _faiss_file +
                                       "cannot be created or written. " +
                                       "Error: " + std::to_string(ret));
  }
}

// FaissFlatDescriptorSet

FaissFlatDescriptorSet::FaissFlatDescriptorSet(const std::string &set_path)
    : FaissDescriptorSet(set_path) {
  try {
    _index = faiss::read_index(_faiss_file.c_str());

  } catch (faiss::FaissException &e) {
    throw VCLException(OpenFailed, "Problem reading: " + _faiss_file);
  }

  // Faiss will sometimes throw, or sometimes set _index = NULL,
  // we check both just in case.
  if (!_index) {
    throw VCLException(OpenFailed, "Problem reading: " + _faiss_file);
  }

  _dimensions = _index->d;
  _n_total = _index->ntotal;
}

FaissFlatDescriptorSet::FaissFlatDescriptorSet(const std::string &set_path,
                                               unsigned dim,
                                               DistanceMetric metric)
    : FaissDescriptorSet(set_path, dim) {
  if (metric == L2)
    _index = new faiss::IndexFlatL2(_dimensions);
  else if (metric == IP)
    _index = new faiss::IndexFlatIP(_dimensions);
  else
    throw VCLException(UnsupportedIndex, "Metric Not implemented");
}

// FaissIVFFlatDescriptorSet

FaissIVFFlatDescriptorSet::FaissIVFFlatDescriptorSet(
    const std::string &set_path)
    : FaissDescriptorSet(set_path) {
  try {
    _index = (faiss::IndexIVFFlat *)faiss::read_index(_faiss_file.c_str());

  } catch (faiss::FaissException &e) {
    throw VCLException(OpenFailed, "Problem reading: " + _faiss_file);
  }

  // Faiss will sometimes throw, or sometimes set _index = NULL,
  // we check both just in case.
  if (!_index) {
    throw VCLException(OpenFailed, "Problem reading: " + _faiss_file);
  }

  _dimensions = _index->d;
  _n_total = _index->ntotal;
}

FaissIVFFlatDescriptorSet::FaissIVFFlatDescriptorSet(
    const std::string &set_path, unsigned dim, DistanceMetric metric)
    : FaissDescriptorSet(set_path, dim) {
  // TODO: Revise nlist param for future optimizations.
  // 4 is a suggested value by faiss for the IVFFlat index,
  // that's why we leave it for now.
  int nlist = 4;

  if (metric == L2) {
    faiss::IndexFlatL2 *quantizer = new faiss::IndexFlatL2(_dimensions);

    _index = new faiss::IndexIVFFlat(quantizer, _dimensions, nlist,
                                     faiss::METRIC_L2);
  } else if (metric == IP) {
    faiss::IndexFlatIP *quantizer = new faiss::IndexFlatIP(_dimensions);

    _index = new faiss::IndexIVFFlat(quantizer, _dimensions, nlist,
                                     faiss::METRIC_INNER_PRODUCT);
  } else
    throw VCLException(UnsupportedIndex, "Metric Not implemented");
}

long FaissIVFFlatDescriptorSet::add(float *descriptors, unsigned n,
                                    long *labels) {
  assert(n > 0);

  _lock.lock();

  // This index need training before inserting elements.
  // This will only happen the first time something is added,
  // and will attempt to use the inserted elements for training.
  if (!_index->is_trained) {

    long desc_4_training = _dimensions * 100;

    if (n < desc_4_training) {
      // Train with random data
      // The user can later call train() with better data.
      std::vector<float> aux_desc(desc_4_training * _dimensions);
      std::memcpy(aux_desc.data(), descriptors,
                  n * _dimensions * sizeof(float));
      _index->train(desc_4_training, aux_desc.data());
    } else {
      _index->train(n, descriptors);
    }

    // This is needed for doing reconstructions:
    // More info: https://github.com/facebookresearch/faiss/issues/374
    ((faiss::IndexIVFFlat *)_index)->make_direct_map();
  }
  _lock.unlock();

  long id_first = FaissDescriptorSet::add(descriptors, n, labels);

  return id_first;
}
