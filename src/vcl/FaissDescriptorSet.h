/**
 * @file   DescriptorSet.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"),
 * to deal
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the C++ API for DescriptorSet.
 */

#pragma once

#include <map>
#include <mutex>
#include <stdlib.h>
#include <string>
#include <unordered_map>
#include <vector>

#include "DescriptorSetData.h"

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFFlat.h>

namespace VCL {

class FaissDescriptorSet : public DescriptorSet::DescriptorSetData {

protected:
  std::string _faiss_file;

  faiss::Index *_index;

  std::mutex _lock;
  std::vector<long> _label_ids;

  void write_label_ids();
  void read_label_ids();

  void train_core(float *descriptors, unsigned n);

public:
  FaissDescriptorSet(const std::string &set_path);
  FaissDescriptorSet(const std::string &set_path, unsigned dim);

  ~FaissDescriptorSet();

  virtual long add(float *descriptors, unsigned n_descriptors, long *classes);

  void train();

  void train(float *descriptors, unsigned n);

  bool is_trained();

  void search(float *query, unsigned n, unsigned k, long *ids,
              float *distances);

  void radius_search(float *query, float radius, long *ids, float *distances);

  void classify(float *descriptors, unsigned n, long *ids, unsigned quorum);

  void get_descriptors(long *ids, unsigned n, float *descriptors);

  void get_labels(long *ids, unsigned n, long *labels);

  void store();
  void store(std::string set_path);
};

class FaissFlatDescriptorSet : public FaissDescriptorSet {

public:
  FaissFlatDescriptorSet(const std::string &set_path);
  FaissFlatDescriptorSet(const std::string &set_path, unsigned dim,
                         DistanceMetric metric);
};

class FaissIVFFlatDescriptorSet : public FaissDescriptorSet {

public:
  FaissIVFFlatDescriptorSet(const std::string &set_path);
  FaissIVFFlatDescriptorSet(const std::string &set_path, unsigned dim,
                            DistanceMetric metric);

  long add(float *descriptors, unsigned n_descriptors, long *classes);
};
}; // namespace VCL
