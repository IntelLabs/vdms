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
#include <random> // For std::mt19937 and std::uniform_int_distribution
#include <chrono> // For seeding the random number generator

namespace VCL {

// Static random engine for make_space_bucket's internal use (passed as ref)
extern std::mt19937 cuckoo_rand_engine; // Declared in CuckooCommon.cc


void CuckooHTFilter::get_ht_bucket_info(const void *key,
                                         uint32_t *out_prim_bucket, uint32_t *out_sec_bucket, filter_sig_t *out_signature) const {

    uint32_t first_hash = crc32(prim_hash_seed_, static_cast<const Bytef*>(key), key_len_);
    uint32_t sec_hash = crc32(sec_hash_seed_, reinterpret_cast<const Bytef*>(&first_hash), sizeof(uint32_t));

    *out_signature = static_cast<filter_sig_t>(first_hash & 0xFFFF); // 16-bit signature

    // HT Non-cache mode: XOR derivation
    *out_prim_bucket = sec_hash & bucket_mask_;
    // get_alt_bucket_idx is (signature ^ current_bucket_idx) & bucket_mask
    *out_sec_bucket = get_alt_bucket_idx(*out_prim_bucket, *out_signature, bucket_mask_);
}


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
        return; // Constructor cannot return an error directly, consider throwing an error
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

    //Most Significant Bit of Set_ID is reserved, it is used internally as a flag
    //to indicate that this entry has been pushed before or not
    filter_set_t flag_mask = 1U << (sizeof(filter_set_t) * 8 - 1);

    if (set_id == FILTER_NO_MATCH || (set_id & flag_mask) != 0) {
        std::cerr << "ERROR: CuckooHTFilter:add invalid set_id used or  MSB is set" << std::endl;
        return -EINVAL; // Invalid set_id
    }

    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;

    get_ht_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

    /*
     * For HT non-cache mode, we do not update existing entry with the same
     * signature. This is because if two keys with same signature update
     * each other, false negative may happen, which is not the expected
     * behavior for non-cache setsummary.
     */
    // No try_update for HTFilter

    /* If not full then insert into one slot */
    int ret = try_insert(table_, prim_bucket, sec_bucket, signature, set_id);
    if (ret != -1) {
        return 0; // Inserted successfully into an empty slot
    }

    /* Random pick prim or sec for recursive displacement */
    uint32_t select_bucket = (signature & 1U) ? prim_bucket : sec_bucket; // Use signature LSB for random choice

    unsigned int nr_pushes = 0;
    ret = make_space_bucket(table_, bucket_mask_, select_bucket, &nr_pushes, cuckoo_rand_engine); // Pass the global random engine
    if (ret >= 0) { // ret is the index in select_bucket that became free
        table_[select_bucket].sigs[ret] = signature;
        table_[select_bucket].sets[ret] = set_id;
        return 1; // Successfully added after push(es)
    }

    return ret; // Return -ENOSPC or other error from make_space_bucket
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
