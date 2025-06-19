/**
 * @file   CuckooHTFilter.h
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
 * This file declares the CuckooHTFilter class, a concrete implementation
 * of the VCL::Filter abstract base class using a cuckoo hash table
 * in non-cache (no overwriting existing keys allowed) mode.
 *
 */

#ifndef CUCKOO_HT_FILTER_H
#define CUCKOO_HT_FILTER_H
#pragma once

#include "vcl/Filter.h" // Include the base Filter class header
#include "vcl/CuckooCommon.h"  // Include the common Cuckoo structs

namespace VCL {

/**
 * @class CuckooHTFilter
 * @brief Implements a cuckoo hash table filter without deletion support.
 *
 * This corresponds to the CuckooHT engine type. It does not allow
 * explicit deletions and keys are not overwritten.
 */
class CuckooHTFilter : public Filter {
private:
    uint32_t bucket_cnt_;   /* Number of buckets. */
    uint32_t bucket_mask_;  /* Bit mask to get bucket index. */
    filter_ht_bucket* table_; /* Pointer to the hash table data. */

    inline void get_ht_bucket_info(const void *key,
                                   uint32_t *out_prim_bucket, uint32_t *out_sec_bucket, filter_sig_t *out_signature) const;


public:
    /**
     * @brief Constructor for CuckooHTFilter.
     * @param params Filter creation parameters.
     */
    CuckooHTFilter(const FilterParameters& params);

    /**
     * @brief Destructor for CuckooHTFilter.
     * Frees the allocated hash table memory.
     */
    virtual ~CuckooHTFilter() override;

    // --- Implementations of pure virtual functions from VCL::Filter ---

    virtual int lookup(const void *key, filter_set_t *set_id) const override;
    virtual int lookup_bulk(const void **keys, uint32_t num_keys, filter_set_t *set_ids) const override;
    virtual int lookup_multi(const void *key, uint32_t max_match_per_key, filter_set_t *set_id) const override;
    virtual int lookup_multi_bulk(const void **keys, uint32_t num_keys, uint32_t max_match_per_key, uint32_t *match_count, filter_set_t *set_ids) const override;
    virtual int add(const void *key, filter_set_t set_id) override;
    virtual void reset() override;
    virtual int delete_key(const void *key, filter_set_t set_id) override; // Deletion not supported for CuckooHT, but must be implemented
};

} // namespace VCL

#endif // CUCKOO_HT_FILTER_H