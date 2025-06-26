/**
 * @file   VBF_Filter.h
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * @section DESCRIPTION
 *
 * This file declares the VBF_Filter class, a concrete implementation
 * of the VCL::Filter abstract base class for a Vector Bloom Filter.
 *
 */

#pragma once

#include "vcl/Filter.h"  // Include the base Filter class header

namespace VCL {

// Add any specific structures or parameters for VBF here if needed.
// For a bloom filter, this might involve array of bits, hash functions, etc.
// For now, a placeholder.
// struct VBF_specific_data {
//    // e.g., std::vector<uint64_t> bit_array;
//    // e.g., uint32_t num_hashes;
//    // e.g., double false_positive_rate;
// };

/**
 * @class VBF_Filter
 * @brief Implements a Vector Bloom Filter.
 *
 * This corresponds to the VBF engine type. It's designed for small set ranges
 * and does not support deletions.
 */
class VBF_Filter : public Filter {
 private:
  // VBF-specific data members would go here.
  // void* vbf_data_; // Placeholder for the actual VBF structure

 public:
  /**
   * @brief Constructor for VBF_Filter.
   * @param params Filter creation parameters.
   */
  VBF_Filter(const FilterParameters &params);

  /**
   * @brief Destructor for VBF_Filter.
   * Frees any VBF-specific allocated memory.
   */
  virtual ~VBF_Filter() override;

  // --- Implementations of pure virtual functions from VCL::Filter ---
  // These will initially be stubs, similar to the original filter.cc for VBF.

  virtual int lookup(const void *key, filter_set_t *set_id) const override;
  virtual int lookup_bulk(const void **keys, uint32_t num_keys,
                          filter_set_t *set_ids) const override;
  virtual int lookup_multi(const void *key, uint32_t max_match_per_key,
                           filter_set_t *set_id) const override;
  virtual int lookup_multi_bulk(const void **keys, uint32_t num_keys,
                                uint32_t max_match_per_key,
                                uint32_t *match_count,
                                filter_set_t *set_ids) const override;
  virtual int add(const void *key, filter_set_t set_id) override;
  virtual void reset() override;
  virtual int delete_key(const void *key, filter_set_t set_id)
      override;  // Deletion not supported for VBF, but must be implemented
};

}  // namespace VCL