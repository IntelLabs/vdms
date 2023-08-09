/**
 * @file   DescriptorSet.h
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2021 Intel Corporation
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
//#include "../../../FLINNG/include/lib_flinng.h" //todo update make files for
// flinng lib include directory
#include "DescriptorParams.h"
#include "lib_flinng.h"

namespace VCL {

class FlinngDescriptorSet : public DescriptorSet::DescriptorSetData {

protected:
  std::string _flinng_file;
  bool is_finalized;

  flinng::BaseDenseFlinng32 *_index; // FLinng have a base class by this name
  // depending on metric to be used will point to the right index
  flinng::FlinngBuilder *_builder;

  std::mutex _lock;
  std::vector<long> _label_ids;

  void write_label_ids();
  void read_label_ids();

  void train_core(float *descriptors, unsigned n);
  void getFlinngParams(VCL::DescriptorParams *par,
                       flinng::FlinngBuilder *builder);

public:
  FlinngDescriptorSet(const std::string &set_path);
  FlinngDescriptorSet(const std::string &set_path, unsigned dim,
                      DistanceMetric metric, VCL::DescriptorParams *par = NULL);

  ~FlinngDescriptorSet();

  virtual long add(float *descriptors, unsigned n_descriptors, long *classes);
  virtual long add_and_store(float *descriptors, unsigned n_descriptors,
                             long *classes);

  void train();

  void train(float *descriptors, unsigned n);

  bool is_trained();

  void finalize_index();

  void search(float *query, unsigned n, unsigned k, long *ids);

  void search(float *query, unsigned n, unsigned k, long *ids,
              float *distances);

  void radius_search(float *query, float radius, long *ids, float *distances);

  void classify(float *descriptors, unsigned n, long *ids, unsigned quorum);

  void get_descriptors(long *ids, unsigned n, float *descriptors);

  void get_labels(long *ids, unsigned n, long *labels);

  void store();
  void store(std::string set_path);
};
}; // namespace VCL
