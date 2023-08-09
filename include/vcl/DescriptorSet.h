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

#include "DescriptorParams.h"
#include "Exception.h"
#include "RemoteConnection.h"
#include "utils.h"
#include <map>
#include <string>
#include <vector>

namespace VCL {

enum DescriptorSetEngine {
  FaissFlat,
  FaissIVFFlat,
  TileDBDense,
  TileDBSparse,
  Flinng
};

enum DistanceMetric { L2, IP };
// enum class Storage { LOCAL = 0, AWS = 1 };

class DescriptorSet {

public:
  typedef std::vector<long> DescIdVector;
  typedef std::vector<long> LabelIdVector;
  typedef std::vector<float> DistanceVector;
  typedef float *DescData;
  typedef float *DescDataArray;

  class DescriptorSetData;
  class DescriptorParams;

private:
  DescriptorSetData *_set;
  DescriptorSetEngine _eng;

  RemoteConnection *_remote;
  Storage _storage = Storage::LOCAL;

  void write_set_info();
  void read_set_info(const std::string &set_path);

public:
  /**
   *  Loads an existing collection located at set_path
   *
   *  @param set_path  Full Path to the collection folder
   */
  DescriptorSet(const std::string &set_path);

  /**
   *  Creates a new collection, if it does not exist
   *
   *  @param set_path  Full Path to the set folder
   *  @param dim  Dimension of the descriptor
   *  @param eng  DescriptorSet Engine (Default is FaissFlat)
   *  @param metric Metric for calculating distances (Default is L2)
   */
  DescriptorSet(const std::string &set_path, unsigned dim,
                DescriptorSetEngine eng = FaissFlat, DistanceMetric metric = L2,
                VCL::DescriptorParams *param = NULL);

  ~DescriptorSet();

  // For now, we don't allow copy of objects.
  // We will defined this behavoir later based on use-cases.
  // Out use-cases now do not require copies, as the
  // objects are besically used to access/operate the sets.
  DescriptorSet(const DescriptorSetData &) = delete;

  /**
   *  Writes the DescriptorSet Index to the system. This will overwrite
   *  the original
   */
  void store();

  /**
   *  Writes the DescriptorSet Index to the system into a defined path.
   *  This will overwrite any other index under the same set_path.
   */
  void store(std::string set_path);

  /*  *********************** */
  /*      CORE INTERFACE      */
  /*  *********************** */

  /**
   *  Returns the path to the root directory where all the
   *  files are for the Set are stored
   */
  std::string get_path();

  /**
   *  Returns the number of dimensions of each descriptor in the set
   */
  unsigned get_dimensions();

  void finalize_index();

  /**
   *  Returns the number of descriptors in the set
   */
  long get_n_descriptors();

  /**
   *  Inserts n descriptors and their labels into the set
   *  Both descriptors and labels must have the same number of elements,
   *  or labels can have no elements.
   *  If not labels are defined, -1 is assigned to signify "no label".

   *  Note: Given the in-memory nature of the Faiss library, adding
   *  elements on a set using Faiss as engine will not persist the data
   *  until the store() method is call. This is contrary to the TileDB
   *  engines, where every add will return after persisting the data.

   *  @param descriptors Buffer to descriptors (size n * dim)
   *  @param n Number of descriptors
   *  @param labels Array of labels, can be NULL.
   */
  long add(DescDataArray descriptors, unsigned n, long *labels = NULL);

  long add_and_store(DescDataArray descriptors, unsigned n,
                     long *labels = NULL);

  /**
   *  Search for the k closest neighborhs
   *
   *  @param query  Query descriptors buffer
   *  @param n Number of descriptors that will be queried
   *  @param k Number of maximun neighbors to be returned
   *  @return ids  id of each neighbor (size n * k) (padded with -1)
   *  @return distances  distances to each neighbor (size n * k).
                         (padded with -1)
   */
  void search(DescDataArray queries, unsigned n, unsigned k, long *ids,
              float *distances);

  void search(DescDataArray queries, unsigned n, unsigned k, long *ids);
  /**
   *  Search for neighborhs within a radius.
   *
   *  Note: We only allow the radius search of a single
   *  element to avoid having to deal with results that are
   *  of different (unknown) sized for each query.
   *  We will work on it once we have a more clear use case for
   *  this call
   *
   *  @param query  Query vector
   *  @param radius  Maximun distance allowed
   *  @param ids  Array of ID of the descriptors
   *  @param distances  Distances of each neighbor
   */
  void radius_search(DescData query, float radius, long *ids, float *distances);

  /**
   *  Find the label of the feature vector, based on the closest
   *  neighbors.
   *
   *  @param descriptors Buffer to descriptors (size n * dim)
   *  @param n  Number of descriptors in buffer
   *  @return labels  Label Ids
   *  @param quorum  Number of elements used for the classification vote.
   */
  void classify(DescDataArray descriptors, unsigned n, long *labels,
                unsigned quorum = 7);

  /**
   *  Get the descriptors by specifiying ids.
   *  This is an exact search by id.
   *
   *  @param ids  buffer with ids
   *  @param n  number of ids to query
   *  @return descriptors pointer to descriptors buffer
                          size: (n * dim * sizeof(float))
   */
  void get_descriptors(long *ids, unsigned n, DescDataArray descriptors);

  /**
   *  Trains the index with the data present in the collection
   *  using the specified metric
   */
  void train();

  /**
   *  Trains the index using specified descriptors
   *
   *  @param descriptors Reference Descriptors
   *  @param n Number of descriptors
   */
  void train(DescDataArray descriptors, unsigned n);

  /**
   *  Returns true if the index is trained (train() method called),
   *  false otherwhise.
   */
  bool is_trained();

  /*  *********************** */
  /*   VECTOR-BASED INTERFACE */
  /*  *********************** */

  // This are all wrapper around the core-interface
  // That are usually useful.

  /**
   *  Inserts several Descriptors and their labels into the collection
   *  Both Descriptors and labels must have the same length.
   *
   *  @param descriptors  Pointer to buffer containing the DescriptorSet
   *  @param n  Number of elements per descriptor
   *  @param labels  Vector of labels
   *  @return id of the first (sequential ids)
   */
  long add(DescDataArray descriptors, unsigned n, LabelIdVector &labels);

  long add_and_store(DescDataArray descriptors, unsigned n,
                     LabelIdVector &labels);
  /**
   *  Search for the k closest neighborhs
   *      // Add comment on why we use k and n_queries.
   *      // We can also get rid of the
   *
   *  @param query  Query descriptors buffer
   *  @param n_queries Number of descriptors that will be queried
   *  @param k  Number of maximun neighbors to be returned
   *  @return distances  distances of each neighbor (size n * k).
   *  @return descriptors_ids  distances of each neighbor (size n * k).
   */
  void search(DescDataArray query, unsigned n, unsigned k, DescIdVector &ids,
              DistanceVector &distances);

  void search(DescDataArray query, unsigned n, unsigned k, DescIdVector &ids);
  /**
   *  Find the label of the feature vector, based on the closest
   *  neighbors.
   *
   *  @param query  Query descriptors buffer
   *  @param n_queries Number of descriptors that will be classified
   *  @param quorum  Number of elements used for the classification vote.
   *  @return Vector with LabelIds.
   */
  LabelIdVector classify(DescDataArray descriptors, unsigned n,
                         unsigned quorum = 7);

  /**
   *  Get the label of the descriptors for the spcified ids.
   *  NOTE: This is a vector becase this is what we return.
   *  We can, make wrapper functions that recieve arrays as well.
   *
   *  @param ids  vector of ids of size n
   *  @param descriptors return pointer for the float values (n * d)
   */
  void get_descriptors(DescIdVector &ids, DescDataArray descriptors);

  /*  *********************** */
  /*   STRING-LABELS SUPPORT  */
  /*  *********************** */

  /**
   *  Set the matching between label id and the string corresponding
   *  to the label
   *
   *  @param ids  ids of the labels
   *  @param labels  string for each label
   */
  void set_labels_map(std::map<long, std::string> &labels);

  /**
   *  Get the label of the descriptors for the spcified ids.
   *  NOTE: This is a vector becase this is what we return.
   *  We can, make wrapper functions that recieve arrays as well.
   *
   *  @param ids  vector of ids
   *  @return vector with the string labels
   */
  std::map<long, std::string> get_labels_map();

  /**
   *  Set the matching between label id and the string corresponding
   *  to the label
   *
   *  @param ids  vector of ids of the labels
   *  @param labels  vector of string for each label
   */
  void set_labels_map(LabelIdVector &ids, std::vector<std::string> &labels);

  /**
   *  Get the label of the descriptors for the spcified ids.
   *  NOTE: This is a vector becase this is what we return.
   *  We can, make wrapper functions that recieve arrays as well.
   *
   *  @param ids  vector of descriptor's id
   *  @return vector with the string labels
   */
  std::vector<std::string> get_str_labels(DescIdVector &ids);

  /**
   *  Get the label of the descriptors for the spcified ids.
   *  NOTE: This is a vector becase this is what we return.
   *  We can, make wrapper functions that recieve arrays as well.
   *
   *  @param ids  vector of ids
   *  @return vector with the string labels
   */
  std::vector<std::string> label_id_to_string(LabelIdVector &l_id);

  /**
   *  Get the label of the descriptors for the spcified ids.
   *  NOTE: This is a vector becase this is what we return.
   *  We can, make wrapper functions that recieve arrays as well.
   *
   *  @param ids  vector of ids
   *  @return vector with the string labels
   */
  std::vector<std::string> get_str_labels(long *ids, unsigned n);

  /**
   *  Get the label of the descriptors for the spcified ids.
   *  NOTE: This is a vector becase this is what we return.
   *  We can, make wrapper functions that recieve arrays as well.
   *
   *  @param ids  vector of ids
   *  @return vector with the string labels
   */
  long get_label_id(const std::string &label);

  /**
   * Set the remote connection used to write to AWS
   *
   *  @param remote pointer to RemoteConnection object
   *  @return void
   */
  void set_connection(RemoteConnection *remote);
};
}; // namespace VCL
