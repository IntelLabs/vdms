/**
 * @file   TDBDescriptorSet.cc
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

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
#include <string>

// By default, we use OMP.
// #define USE_COMPUTE_MKL

#ifndef USE_COMPUTE_MKL
#define USE_COMPUTE_OMP
#endif

#ifdef USE_COMPUTE_MKL
#include "mkl.h" // INTEL MKL
#endif

#include "TDBDescriptorSet.h"

#define ATTRIBUTE_NAME "val"
#define METADATA_PATH "/metadata"

using namespace VCL;

TDBDescriptorSet::TDBDescriptorSet(const std::string &filename)
    : DescriptorSetData(filename) {
  read_labels_map();
}

TDBDescriptorSet::TDBDescriptorSet(const std::string &filename, uint32_t dim)
    : DescriptorSetData(filename, dim) {}

TDBDescriptorSet::~TDBDescriptorSet() {}

void TDBDescriptorSet::train() {
  // For now, we just consolidate arrays which
  // should make the reads faster (according to TileDB docs).
  // There are more fancy tricks that can be implemented
  // in the future, specially for the sparse arrays.
  // Consolidation is needed since many of the insertions done
  // through TileDB fragments.

  // Consolidate array
  // tiledb::Array::consolidate(_tiledb_ctx, _set_path);
}

void TDBDescriptorSet::compute_distances(float *q, std::vector<float> &d,
                                         std::vector<float> &data) {
  size_t n = data.size() / _dimensions;

  float *sub = new float[_dimensions * n];

#ifdef USE_COMPUTE_MKL

  // Intel MKL
  // #pragma omp parallel for
  for (int i = 0; i < n; ++i) {
    size_t idx = i * _dimensions;
    vsSub(_dimensions, q, data.data() + idx, sub + idx);
    d[i] = std::pow(cblas_snrm2(_dimensions, sub + idx, 1), 2);
  }
#endif

#ifdef USE_COMPUTE_OMP
// Using RAW OpenMP / This can be optimized
#pragma omp parallel for
  for (int i = 0; i < n; ++i) {
    size_t idx = i * _dimensions;

    float sum = 0;
    // #pragma omp parallel for // has to be a reduction
    for (int j = 0; j < _dimensions; ++j) {
      sum += std::pow(data[idx + j] - q[j], 2);
    }

    d[i] = sum; // std::sqrt(sum);
  }
#endif
  delete[] sub;
}

void TDBDescriptorSet::classify(float *descriptors, unsigned n, long *labels,
                                unsigned quorum) {
  float *distances = new float[n * quorum];
  long *ids_aux = new long[n * quorum];

  search(descriptors, n, quorum, ids_aux, distances);

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
    labels[j] = winner;
  }
  delete[] distances;
  delete[] ids_aux;
}

void TDBDescriptorSet::get_labels(long *ids, unsigned n, long *labels) {
  for (int i = 0; i < n; ++i) {
    labels[i] = _label_ids[ids[i]];
  }
}

void TDBDescriptorSet::get_descriptors(long *ids, unsigned n,
                                       float *descriptors) {
  throw VCLException(UnsupportedOperation, "get_descriptors Not implemented");
}

void TDBDescriptorSet::store() { write_labels_map(); }

void TDBDescriptorSet::store(std::string filename) {
  // TODO: Allow user to store in a different file,
  // which is basically make a copy of the TileDB folder.
  throw VCLException(UnsupportedOperation, "Unsupported operation");
}
