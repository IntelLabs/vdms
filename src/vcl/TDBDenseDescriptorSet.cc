/**
 * @file   TDBDenseDescriptorSet.cc
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

#include <cmath>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdlib.h>
#include <string>

#include "TDBDescriptorSet.h"

// #include <tiledb/map.h>

#define ATTRIBUTE_DESC "descriptor"
#define ATTRIBUTE_LABEL "label"

using namespace VCL;

TDBDenseDescriptorSet::TDBDenseDescriptorSet(const std::string &filename)
    : TDBDescriptorSet(filename), _flag_buffer_updated(false) {
  TDBObject descriptorSetObject(_set_path);
  read_descriptor_metadata();
}

TDBDenseDescriptorSet::TDBDenseDescriptorSet(const std::string &filename,
                                             uint32_t dim,
                                             DistanceMetric metric)
    : TDBDescriptorSet(filename, dim), _flag_buffer_updated(true) {
  TDBObject descriptorSetObject;

  descriptorSetObject.set_full_dimensions(std::vector<std::string>{"d"},
                                          std::vector<uint64_t>{(MAX_DESC - 1)},
                                          std::vector<uint64_t>{0}, 10);
  std::string desc = ATTRIBUTE_DESC;
  std::string label = ATTRIBUTE_LABEL;
  descriptorSetObject.set_single_attribute(desc, VCL::CompressionType::LZ4,
                                           (float)_dimensions);
  descriptorSetObject.set_single_attribute(label, VCL::CompressionType::LZ4,
                                           (long)1);

  std::vector<long> num_values{_dimensions, 1};
  descriptorSetObject.set_schema_dense(_set_path, num_values);
  write_descriptor_metadata();
}

void TDBDenseDescriptorSet::load_buffer() {
  try {

    read_descriptor_metadata();

    tiledb::Array array(_ctx, _set_path, TILEDB_READ);
    {
      _buffer.resize(_dimensions * _n_total);
      _label_ids.resize(_n_total);

      tiledb::Query query(_ctx, array);
      query.set_layout(TILEDB_ROW_MAJOR);
      query.set_subarray<uint64_t>({0, _n_total - 1});
      query.set_data_buffer(ATTRIBUTE_DESC, _buffer);
      query.set_data_buffer(ATTRIBUTE_LABEL, _label_ids);
      query.submit();
    }

  } catch (tiledb::TileDBError &e) {
    throw VCLException(TileDBError, "Error: Reading Dense array");
  }

  _flag_buffer_updated = true;
}

void TDBDenseDescriptorSet::read_descriptor_metadata() {
  std::vector<uint64_t> subarray = {METADATA_OFFSET, (METADATA_OFFSET + 1)};
  std::vector<long> values(2);

  tiledb::Array array(_ctx, _set_path, TILEDB_READ);
  tiledb::Query md_read(_ctx, array, TILEDB_READ);

  md_read.set_subarray(subarray);
  md_read.set_layout(TILEDB_ROW_MAJOR);

  md_read.set_data_buffer(ATTRIBUTE_LABEL, values);
  md_read.submit();
  array.close();

  _dimensions = values[0];
  _n_total = values[1];
}

void TDBDenseDescriptorSet::write_descriptor_metadata() {
  std::vector<long> metadata;
  metadata.push_back(_dimensions);
  metadata.push_back(_n_total);

  // This is only here because tiledb requires all the
  // attributes when writing.
  std::vector<float> aux_dims(_dimensions * 2, .0f);

  // Write metadata
  tiledb::Array array(_ctx, _set_path, TILEDB_WRITE);
  tiledb::Query query(_ctx, array);
  query.set_layout(TILEDB_ROW_MAJOR);
  query.set_subarray<uint64_t>({METADATA_OFFSET, METADATA_OFFSET + 1});
  query.set_data_buffer(ATTRIBUTE_LABEL, metadata);
  query.set_data_buffer(ATTRIBUTE_DESC, aux_dims);
  query.submit();
  query.finalize();
}

long TDBDenseDescriptorSet::add(float *descriptors, unsigned n, long *labels) {
  try {
    std::vector<long> att_label;
    long *labels_buffer = labels;

    if (labels == NULL) {
      // By default, labels is -1
      att_label = std::vector<long>(n, -1);
      labels_buffer = att_label.data();
    }

    {
      tiledb::Array array(_ctx, _set_path, TILEDB_WRITE);
      tiledb::Query query(_ctx, array);
      query.set_layout(TILEDB_ROW_MAJOR);
      query.set_subarray<uint64_t>({_n_total, _n_total + n - 1});
      query.set_data_buffer(ATTRIBUTE_DESC, descriptors, n * _dimensions);
      query.set_data_buffer(ATTRIBUTE_LABEL, labels_buffer, n);

      query.submit();
      query.finalize();
    }
  } catch (tiledb::TileDBError &e) {
    _flag_buffer_updated = false;
    throw VCLException(UnsupportedOperation, e.what());
  }

  // Write _n_total into tiledb
  // This is good because we only write metadata
  // (_n_total) after the other two writes succedded.
  _n_total += n;
  write_descriptor_metadata();

  // - n becase we already increase _n_total for writing metadata on tdb
  long old_n_total = _n_total - n;

  _buffer.resize((_n_total)*_dimensions);
  std::memcpy(&_buffer[old_n_total * _dimensions], descriptors,
              n * _dimensions * sizeof(float));

  if (labels != NULL) {
    _label_ids.resize(_n_total);
    std::memcpy(&_label_ids[old_n_total], labels, n * sizeof(long));
  }

  return old_n_total;
}

void TDBDenseDescriptorSet::search(float *query, unsigned n_queries, unsigned k,
                                   long *ids, float *distances) {
  if (!_flag_buffer_updated) {
    load_buffer();
  }

  std::vector<float> d(_n_total);
  std::vector<long> idxs(_n_total);

  for (int i = 0; i < n_queries; ++i) {

    compute_distances(query + i * _dimensions, d, _buffer);
    std::iota(idxs.begin(), idxs.end(), 0);
    std::partial_sort(idxs.begin(), idxs.begin() + k, idxs.end(),
                      [&d](size_t i1, size_t i2) { return d[i1] < d[i2]; });

    for (int j = 0; j < k; ++j) {
      ids[i * k + j] = idxs[j];
      distances[i * k + j] = d[idxs[j]];
    }
  }
}

void TDBDenseDescriptorSet::get_descriptors(long *ids, unsigned n,
                                            float *descriptors) {
  if (!_flag_buffer_updated) {
    load_buffer();
  }

  for (int i = 0; i < n; ++i) {
    long idx = ids[i] * _dimensions;
    long offset = i * _dimensions;
    std::memcpy(descriptors + offset, &_buffer[idx],
                sizeof(float) * _dimensions);
  }
}
