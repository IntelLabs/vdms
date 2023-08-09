/**
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
 */

#include <assert.h>
#include <sstream>

#include "DescriptorSetData.h"
#include "vcl/Exception.h"

using namespace VCL;

DescriptorSet::DescriptorSetData::DescriptorSetData(const std::string &set_path)
    : _set_path(set_path) {
  _dimensions = 0;
  _n_total = 0;
  _metric = VCL::DistanceMetric::L2; // by default

  if (!dir_exist(set_path)) {
    throw VCLException(OpenFailed, "File does not exists");
  }
}

DescriptorSet::DescriptorSetData::DescriptorSetData(const std::string &set_path,
                                                    uint32_t dim)
    : _set_path(set_path), _dimensions(dim), _n_total(0) {
  _metric = VCL::DistanceMetric::L2; // by default

  if (dir_exist(set_path)) {
    throw VCLException(OpenFailed, "File already exists");
  }
}

DescriptorSet::DescriptorSetData::~DescriptorSetData() {}

void DescriptorSet::DescriptorSetData::radius_search(float *query, float radius,
                                                     long *descriptors,
                                                     float *distances) {
  throw VCLException(UnsupportedOperation, "Not Implemented");
}

// String labels handling

void DescriptorSet::DescriptorSetData::set_labels_map(
    std::map<long, std::string> &labels) {
  _labels_map_lock.lock();
  _labels_map = labels;
  _labels_map_lock.unlock();
}

std::vector<std::string>
DescriptorSet::DescriptorSetData::get_str_labels(long *ids, unsigned n) {
  std::vector<std::string> str_labels(n);
  std::vector<long> labels(n);

  get_labels(ids, n, labels.data());

  _labels_map_lock.lock();
  for (int i = 0; i < n; ++i) {
    assert(labels[i] < _labels_map.size());
    str_labels[i] = _labels_map[labels[i]];
  }
  _labels_map_lock.unlock();

  return str_labels;
}

void DescriptorSet::DescriptorSetData::write_labels_map() {
  std::ofstream out_labels(_set_path + "/labels.txt");

  _labels_map_lock.lock();
  for (auto &label : _labels_map) {
    out_labels << label.first << " " << label.second << std::endl;
  }
  _labels_map_lock.unlock();

  out_labels.close();
}

void DescriptorSet::DescriptorSetData::read_labels_map() {
  std::ifstream in_labels(_set_path + "/labels.txt");
  std::string str;
  _labels_map_lock.lock();
  _labels_map.clear();
  while (std::getline(in_labels, str)) {
    std::stringstream sstr(str);
    long id;
    sstr >> id;
    sstr >> str;
    _labels_map[id] = str;
  }
  _labels_map_lock.unlock();
  in_labels.close();
}
