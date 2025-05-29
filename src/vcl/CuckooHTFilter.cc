/**
 * @file   CuckooHTFilter.cc
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
 */

#include "vcl/CuckooHTFilter.h"
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <cstdlib>


namespace VCL {

CuckooHTFilter::CuckooHTFilter(const FilterParameters& params)
    : Filter(params), // Call base class constructor
      bucket_cnt_(0),
      bucket_mask_(0),
      table_(nullptr) {

    uint32_t size_bucket_t = sizeof(filter_ht_bucket);
    uint32_t num_entries = align32pow2(params.num_keys);

    if ((num_entries > FILTER_ENTRIES_MAX) ||
        !is_power_of_2(FILTER_BUCKET_ENTRIES) ||
        num_entries < FILTER_BUCKET_ENTRIES) {
        std::cerr << "ERROR: CuckooHTFilter: Invalid parameters for HT creation (num_entries or bucket_entries)." << std::endl;
        // In a real scenario, you might throw an exception or set an internal error flag
        return; // Constructor cannot return an error directly, consider throwing or a factory method
    }

    bucket_cnt_ = num_entries / FILTER_BUCKET_ENTRIES;
    bucket_mask_ = bucket_cnt_ - 1;

    //table_ = static_cast<filter_ht_bucket*>(std::malloc(bucket_cnt_ * size_bucket_t));
    table_ = static_cast<filter_ht_bucket*>(std::aligned_alloc(CACHE_LINE_SIZE, bucket_cnt_ * size_bucket_t));

    if (table_ == NULL) {
        std::cerr << "ERROR: CuckooHTFilter: Memory allocation failed for HT buckets." << std::endl;
        // Handle memory allocation failure, perhaps throw std::bad_alloc or set internal state
        return;
    }

    // Initialize all sigs and sets in the buckets
    for (uint32_t i = 0; i < bucket_cnt_; ++i) {
        for (uint32_t j = 0; j < FILTER_BUCKET_ENTRIES; ++j) {
            table_[i].sigs[j] = 0; // Initialize signatures to 0 (or some 'empty' value)
            table_[i].sets[j] = FILTER_NO_MATCH; // Initialize set IDs to NO_MATCH
        }
    }
    //std::cout << "DEBUG: CuckooHTFilter '" << name_ << "' constructed and table allocated." << std::endl;
}

CuckooHTFilter::~CuckooHTFilter() {
    if (table_ != nullptr) {
        std::free(table_);
        table_ = nullptr;
    }
    //std::cout << "DEBUG: CuckooHTFilter '" << name_ << "' destructor called and table freed." << std::endl;
}

// --- Implementations of pure virtual functions ---
// These are currently stubs and need actual CuckooHT logic.

int CuckooHTFilter::lookup(const void *key, filter_set_t *set_id) const {
    std::cout << "CuckooHTFilter::lookup(key) - STUB" << std::endl;
    if (set_id) *set_id = FILTER_NO_MATCH;
    return 0; // Not found
}

int CuckooHTFilter::lookup_bulk(const void **keys, uint32_t num_keys, filter_set_t *set_ids) const {
    std::cout << "CuckooHTFilter::lookup_bulk - STUB" << std::endl;
    // Fill set_ids with FILTER_NO_MATCH for all keys as a placeholder
    for (uint32_t i = 0; i < num_keys; ++i) {
        if (set_ids) set_ids[i] = FILTER_NO_MATCH;
    }
    return 0; // No keys found
}

int CuckooHTFilter::lookup_multi(const void *key, uint32_t max_match_per_key, filter_set_t *set_id) const {
    std::cout << "CuckooHTFilter::lookup_multi - STUB" << std::endl;
    // As CuckooHT can have multiple matches, a proper implementation would iterate
    // through bucket entries. For now, it returns 0 matches.
    if (set_id) set_id[0] = FILTER_NO_MATCH;
    return 0; // No matches found
}

int CuckooHTFilter::lookup_multi_bulk(const void **keys, uint32_t num_keys, uint32_t max_match_per_key, uint32_t *match_count, filter_set_t *set_ids) const {
    std::cout << "CuckooHTFilter::lookup_multi_bulk - STUB" << std::endl;
    // Initialize match_count and set_ids as not found
    for (uint32_t i = 0; i < num_keys; ++i) {
        if (match_count) match_count[i] = 0;
        for (uint32_t j = 0; j < max_match_per_key; ++j) {
            if (set_ids) set_ids[i * max_match_per_key + j] = FILTER_NO_MATCH;
        }
    }
    return 0; // No keys found with matches
}

int CuckooHTFilter::add(const void *key, filter_set_t set_id) {
    std::cout << "CuckooHTFilter::add - STUB (key: " << reinterpret_cast<const char*>(key) << ", set_id: " << set_id << ")" << std::endl;
    if (set_id == FILTER_NO_MATCH) {
        return -EINVAL; // Invalid set_id
    }
    // Placeholder logic for adding to HT. Would involve hash calculations and insertion attempts.
    return 0; // Success, no cuckoo eviction
}

void CuckooHTFilter::reset() {
    std::cout << "CuckooHTFilter::reset() - STUB" << std::endl;
    // Reset all entries in the hash table to empty/no match state
    if (table_) {
        for (uint32_t i = 0; i < bucket_cnt_; ++i) {
            for (uint32_t j = 0; j < FILTER_BUCKET_ENTRIES; ++j) {
                table_[i].sigs[j] = 0;
                table_[i].sets[j] = FILTER_NO_MATCH;
            }
        }
    }
}

int CuckooHTFilter::delete_key(const void *key, filter_set_t set_id) {
    std::cout << "CuckooHTFilter::delete_key - STUB (deletion not supported for CuckooHTFilter)" << std::endl;
    return -ENOSYS; // Operation not supported for CuckooHTFilter (non-cache mode)
}

} // namespace VCL
