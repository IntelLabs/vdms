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
 * This file declares the C++ Interface for the abstract DescriptorSetData
 * object.
 */

#pragma once

#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <vector>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "vcl/DescriptorSet.h"

namespace VCL {

class DescriptorSet::DescriptorSetData {

protected:
  std::string _set_path;
  unsigned _dimensions;
  uint64_t _n_total;

  DistanceMetric _metric;

  // String labels handling
  std::mutex _labels_map_lock;
  std::map<long, std::string> _labels_map;

  inline bool file_exist(const std::string &name) {
    std::ifstream f(name.c_str());
    if (f.good()) {
      f.close();
      return true;
    }
    return false;
  }

  inline bool dir_exist(const std::string &dir_name) {
    DIR *dir = opendir(dir_name.c_str());
    if (dir) {
      closedir(dir);
      return true;
    }

    return false;
  }

  inline int create_dir(const char *path) {
    struct stat sb;
    while (1)
      if (stat(path, &sb) == 0)
        if (sb.st_mode & S_IFDIR)
          return 0;
        else
          return EEXIST;
      else if (errno != ENOENT)
        return errno;
      else if (mkdir(path, 0777) == 0)
        return 0;
      else if (errno != EEXIST)
        return errno;
  }

  void write_labels_map();
  void read_labels_map();

public:
  /**
   *  Loads an existing collection located at collection_path
   *  or created a new collection if it does not exist
   *  Returns error if the set does not exist
   *
   *  @param filename  Full Path to the collection folder
   */
  DescriptorSetData(const std::string &filename);

  /**
   *  Creates collection located at filename
   *  or created a new collection if it does not exist
   *
   *  @param filename  Full Path to the collection folder
   *  @param dim  Dimension of the descriptor
   */
  DescriptorSetData(const std::string &filename, unsigned dim);

  virtual ~DescriptorSetData();

  DescriptorSetData(const DescriptorSetData &) = delete;

  std::string get_path() { return _set_path; }

  unsigned get_dimensions() { return _dimensions; }

  /**
   *  Returns the number of descriptors in the set
   */
  long get_n_total() { return _n_total; }

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
  virtual long add(float *descriptors, unsigned n_descriptors,
                   long *labels = NULL) = 0;

  virtual long add_and_store(float *descriptors, unsigned n_descriptors,
                             long *labels = NULL) {
    return 0;
  }

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
  virtual void search(float *query, unsigned n, unsigned k, long *descriptors,
                      float *distances) = 0;

  virtual void search(float *query, unsigned n, unsigned k, long *descriptors) {
  }

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
  virtual void radius_search(float *query, float radius, long *descriptors,
                             float *distances);

  /**
   *  Find the label of the feature vector, based on the closest
   *  neighbors.
   *
   *  @param descriptors Buffer to descriptors (size n * dim)
   *  @param n  Number of descriptors in buffer
   *  @return labels  Label Ids
   *  @param quorum  Number of elements used for the classification vote.
   */
  virtual void classify(float *descriptors, unsigned n, long *ids,
                        unsigned quorum) = 0;

  /**
   *  Get the descriptors by specifiying ids.
   *  This is an exact search by id.
   *
   *  @param ids  buffer with ids
   *  @param n  number of ids to query
   *  @return descriptors pointer to descriptors buffer
                          size: (n * dim * sizeof(float))
   */
  virtual void get_descriptors(long *ids, unsigned n, float *descriptors) = 0;

  virtual void get_labels(long *ids, unsigned n, long *labels) = 0;

  /**
   *  Trains the index with the data present in the collection
   *  using the specified metric
   */
  virtual void train() {}

  /**
   *  Trains the index using specified descriptors
   *
   *  @param descriptors Reference Descriptors
   *  @param n Number of descriptors
   */
  virtual void train(float *descriptors, unsigned n) { train(); }

  virtual bool is_trained() { return false; }

  virtual void finalize_index() {}

  /**
   *  Writes the DescriptorSet Index to the system. This will overwrite
   *  the original
   */
  virtual void store() = 0;

  /**
   *  Writes the DescriptorSet Index to the system into a defined path.
   *  This will overwrite any other index under the same set_path.
   */
  virtual void store(std::string collection_path) = 0;

  // String labels handling

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
  std::map<long, std::string> get_labels_map() { return _labels_map; }

  /**
   *  Set the matching between label id and the string corresponding
   *  to the label
   *
   *  @param ids  ids of the labels
   *  @param labels  string for each label
   */
  void set_labels_map(std::map<long, std::string> &labels);
};

}; // namespace VCL