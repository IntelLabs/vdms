/**
 * @file   CuckooCacheFilter.h
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
 * This file declares the CuckooCacheFilter class, a concrete implementation
 * of the VCL::Filter abstract base class using a cuckoo hash table
 * in cache (deletion allowed with overwriting) mode.
 *
 */

#pragma once

#include "vcl/Filter.h" // Include the base Filter class header
#include "vcl/CuckooCommon.h"  // Include the common Cuckoo structs

namespace VCL {

/**
 * @class CuckooCacheFilter
 * @brief Implements a cuckoo hash table filter with deletion and overwriting support.
 *
 * This corresponds to the CuckooCache engine type. It's designed for caching
 * recent keys, allowing new keys to overwrite older ones.
 */
class CuckooCacheFilter : public Filter {
private:
    uint32_t bucket_cnt_;   /* Number of buckets. */
    uint32_t bucket_mask_;  /* Bit mask to get bucket index. */
    filter_ht_bucket* table_; /* Pointer to the hash table data. */

public:
    /**
     * @brief Constructor for CuckooCacheFilter.
     * @param params Filter creation parameters.
     */
    CuckooCacheFilter(const FilterParameters& params);

    /**
     * @brief Destructor for CuckooCacheFilter.
     * Frees the allocated hash table memory.
     */
    virtual ~CuckooCacheFilter() override;

    // --- Implementations of pure virtual functions from VCL::Filter ---
    // Note: Some lookup/multi-lookup behavior might differ from CuckooHT due to cache nature.

    virtual int lookup(const void *key, filter_set_t *set_id) const override;
    virtual int lookup_bulk(const void **keys, uint32_t num_keys, filter_set_t *set_ids) const override;
    virtual int lookup_multi(const void *key, uint32_t max_match_per_key, filter_set_t *set_id) const override; // Expected to return at most 1 match
    virtual int lookup_multi_bulk(const void **keys, uint32_t num_keys, uint32_t max_match_per_key, uint32_t *match_count, filter_set_t *set_ids) const override; // Expected to return at most 1 match per key
    virtual int add(const void *key, filter_set_t set_id) override;
    virtual void reset() override;
    virtual int delete_key(const void *key, filter_set_t set_id) override;
};

} // namespace VCL