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

#include <fstream>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>

#include <tiledb/tiledb>

#include "DescriptorSetData.h"
#include "TDBObject.h"
#include "vcl/Exception.h"

namespace VCL {

typedef std::vector<float> DescBuffer;
typedef std::vector<float> DistanceData;

class TDBDescriptorSet : public DescriptorSet::DescriptorSetData,
                         public TDBObject {

protected:
  const unsigned long MAX_DESC = 100000;
  const unsigned long METADATA_OFFSET = MAX_DESC - 2;

  // this is caching data
  std::vector<long> _label_ids; // we need to move this

  void compute_distances(float *q, DistanceData &d, DescBuffer &data);

  virtual void read_descriptor_metadata() = 0;
  virtual void write_descriptor_metadata() = 0;

public:
  /**
   *  Loads an existing collection located at collection_path
   *  or created a new collection if it does not exist
   *
   *  @param collection_path  Full Path to the collection folder
   */
  TDBDescriptorSet(const std::string &collection_path);

  TDBDescriptorSet(const std::string &collection_path, unsigned dim);

  ~TDBDescriptorSet();

  virtual long add(float *descriptors, unsigned n_descriptors,
                   long *classes) = 0;

  virtual void train();

  virtual void train(float *descriptors, unsigned n) { train(); }

  bool is_trained() { return true; }

  virtual void search(float *query, unsigned n_queries, unsigned k,
                      long *descriptors, float *distances) = 0;

  virtual void classify(float *descriptors, unsigned n, long *labels,
                        unsigned quorum);

  virtual void get_descriptors(long *ids, unsigned n, float *descriptors);

  virtual void get_labels(long *ids, unsigned n, long *labels);

  void store();
  void store(std::string set_path);
};

class TDBDenseDescriptorSet : public TDBDescriptorSet {

private:
  // This is for caching, accelerates searches fairly well.
  bool _flag_buffer_updated;
  std::vector<float> _buffer;

  void load_buffer();
  void read_descriptor_metadata();
  void write_descriptor_metadata();

public:
  TDBDenseDescriptorSet(const std::string &collection_path);

  TDBDenseDescriptorSet(const std::string &collection_path, unsigned dim,
                        DistanceMetric metric);

  ~TDBDenseDescriptorSet(){};

  long add(float *descriptors, unsigned n_descriptors, long *classes);

  void search(float *query, unsigned n_queries, unsigned k, long *descriptors,
              float *distances);

  void get_descriptors(long *ids, unsigned n, float *descriptors);
};

class TDBSparseDescriptorSet : public TDBDescriptorSet {

private:
  void read_descriptor_metadata();
  void write_descriptor_metadata();

  void load_neighbors(float *query, unsigned k, std::vector<float> &descriptors,
                      std::vector<long> &desc_ids,
                      std::vector<long> &desc_labels);

  void search(float *query, unsigned n_queries, unsigned k, long *descriptors,
              float *distances, long *labels);

public:
  TDBSparseDescriptorSet(const std::string &collection_path);

  TDBSparseDescriptorSet(const std::string &collection_path, unsigned dim,
                         DistanceMetric metric);

  ~TDBSparseDescriptorSet(){};

  long add(float *descriptors, unsigned n_descriptors, long *classes);

  void search(float *query, unsigned n_queries, unsigned k, long *descriptors,
              float *distances);

  void classify(float *descriptors, unsigned n, long *labels, unsigned quorum);

  void get_descriptors(long *ids, unsigned n, float *descriptors);

  void get_labels(long *ids, unsigned n, long *labels);
};
}; // namespace VCL
