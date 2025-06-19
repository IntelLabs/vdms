/**
 * @file   CuckooCacheFilter.cc
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

#include "vcl/CuckooCacheFilter.h"
#include "vcl/CuckooCommon.h"
#include <iostream>
#include <stdlib.h> 
#include <cstring>  
#include <cstdlib>
#include <random> // For std::mt19937 and std::uniform_int_distribution
#include <chrono> // For seeding the random number generator



namespace VCL {

// random engine for CuckooCacheFilter's eviction
std::mt19937 cache_rand_engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());

// Helper function private to CuckooCacheFilter for eviction
static inline int evict_from_bucket_internal() {
    // For now, we randomly pick one entry to evict
    std::uniform_int_distribution<unsigned int> dist(0, FILTER_BUCKET_ENTRIES - 1);
    return dist(cache_rand_engine);
}


void CuckooCacheFilter::get_cache_bucket_info(const void *key,
                                               uint32_t *out_prim_bucket, uint32_t *out_sec_bucket, filter_sig_t *out_signature) const {
    uint32_t first_hash = crc32(prim_hash_seed_, static_cast<const Bytef*>(key), key_len_);
    uint32_t sec_hash = crc32(sec_hash_seed_, reinterpret_cast<const Bytef*>(&first_hash), sizeof(uint32_t));

    // *out_signature = static_cast<filter_sig_t>(first_hash & 0xFFFF); // 16-bit signature

    // increased signature to full 32-bit hash because of collisions in test cases
    // No truncation needed to 16 bit
    *out_signature = first_hash;

    // Cache mode: independent bucket derivations from sec_hash
    *out_prim_bucket = sec_hash & bucket_mask_;
    *out_sec_bucket = (sec_hash >> 16) & bucket_mask_;
    
}


CuckooCacheFilter::CuckooCacheFilter(const FilterParameters& params)
    : Filter(params), // Call base class constructor
      bucket_cnt_(0),
      bucket_mask_(0),
      table_(nullptr) {

    uint32_t size_bucket_t = sizeof(filter_ht_bucket);
    uint32_t num_entries = align32pow2(params.num_keys);

    if (!this->_valid) { 
        return;
    }

    if ((num_entries > FILTER_ENTRIES_MAX) ||
        !is_power_of_2(FILTER_BUCKET_ENTRIES) ||
        num_entries < FILTER_BUCKET_ENTRIES) {
        std::cerr << "ERROR: CuckooCacheFilter: Invalid parameters for HT creation (num_entries or bucket_entries)." << std::endl;
        this->_valid = false;
        return;
    }

    bucket_cnt_ = num_entries / FILTER_BUCKET_ENTRIES;
    bucket_mask_ = bucket_cnt_ - 1;

    //table_ = static_cast<filter_ht_bucket*>(std::malloc(bucket_cnt_ * size_bucket_t));
    table_ = static_cast<filter_ht_bucket*>(std::aligned_alloc(CACHE_LINE_SIZE, bucket_cnt_ * size_bucket_t));

    if (table_ == NULL) {
        std::cerr << "ERROR: CuckooCacheFilter: Memory allocation failed for HT buckets." << std::endl;
        this->_valid = false;
        return;
    }

    // Initialize all sigs and sets in the buckets
    for (uint32_t i = 0; i < bucket_cnt_; ++i) {
        for (uint32_t j = 0; j < FILTER_BUCKET_ENTRIES; ++j) {
            table_[i].sigs[j] = 0; // Initialize signatures to 0
            table_[i].sets[j] = FILTER_NO_MATCH; // Initialize set IDs to NO_MATCH
        }
    }
    this->_valid = true;
}

CuckooCacheFilter::~CuckooCacheFilter() {
    if (table_ != nullptr) {
        std::free(table_);
        table_ = nullptr;
    }
}



// --- Implementations of pure virtual functions ---

int CuckooCacheFilter::lookup(const void *key, filter_set_t *set_id) const {
    if (!key || !set_id) {
        return -EINVAL;
    }

    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;

    get_cache_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

    // Future optimization: Add check AVX support 
    // and AVX/SIMD comparison logic here
    // switch (support) { case AVX, AVX256, AVX512: ... default: ... }
    
    // Search in primary bucket
    if (search_bucket_single(prim_bucket, signature, table_, set_id)) {
        return 1; // Found
    }
    
    // Search in secondary bucket
    if (search_bucket_single(sec_bucket, signature, table_, set_id)) {
        return 1; // Found
    }

    *set_id = FILTER_NO_MATCH; // Ensure set_id is explicitly set if not found
    return 0; // Not found
}


int CuckooCacheFilter::lookup_bulk(const void **keys, uint32_t num_keys, filter_set_t *set_ids) const {
    if (!keys || !set_ids || num_keys == 0) {
        return -EINVAL;
    }
    if (num_keys > FILTER_LOOKUP_BULK_MAX) {
        std::cerr << "ERROR: CuckooCacheFilter::lookup_bulk: num_keys exceeds FILTER_LOOKUP_BULK_MAX." << std::endl;
        return -EINVAL; // Or handle larger bulk sizes dynamically
    }

    uint32_t num_matches = 0;
    filter_sig_t tmp_sig[FILTER_LOOKUP_BULK_MAX];
    uint32_t prim_buckets[FILTER_LOOKUP_BULK_MAX];
    uint32_t sec_buckets[FILTER_LOOKUP_BULK_MAX];

    for (uint32_t i = 0; i < num_keys; i++) {
        get_cache_bucket_info(keys[i], &prim_buckets[i], &sec_buckets[i], &tmp_sig[i]);
        // Future optimization: Add check Intel platform and Prefetch support
        // Future optimization: add prefetch to all cache levels LLC,L2, L1 prefetch0(&buckets[prim_buckets[i]]);
        // Future optimization: add prefetch to all cache levels LLC,L2, L1 prefetch0(&buckets[sec_buckets[i]]);
    }

    for (uint32_t i = 0; i < num_keys; i++) {
        filter_set_t current_set_id = FILTER_NO_MATCH;

        
        // Future optimization: Add check AVX support 
        // and AVX/SIMD comparison logic here
        // switch (support) { case AVX, AVX256, AVX512: ... default: ... }

        if (search_bucket_single(prim_buckets[i], tmp_sig[i], table_, &current_set_id) ||
            search_bucket_single(sec_buckets[i], tmp_sig[i], table_, &current_set_id)) {
            num_matches++;
            set_ids[i] = current_set_id;
        } else {
            set_ids[i] = FILTER_NO_MATCH;
        }
    }
    return num_matches;
}


// lookup_multi refers to a lookup operation designed to find all 
// (or a specified maximum number of) matching entries for a given key, 
// rather than just the first one found.
//However, for the cache mode, by design, it is expected at most 1 match 
// The max_match_per_key parameter is ignored as only one match is ever returned.

int CuckooCacheFilter::lookup_multi(const void *key, uint32_t max_match_per_key, filter_set_t *set_id) const {

    if (!key || !set_id || max_match_per_key == 0) {
        return -EINVAL;
    }

    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;
    
    get_cache_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

    // Future optimization: Add check AVX support 
    // and AVX/SIMD comparison logic here
    // switch (support) { case AVX, AVX256, AVX512: ... default: ... }

    // Search primary bucket. If a match is found, populate set_id and return 1.
    // We use search_bucket_single because CuckooCacheFilter expects at most 1 match.
    if (search_bucket_single(prim_bucket, signature, table_, set_id) == 1) {
        return 1; // Match found in primary bucket
    }
    
    // If no match found in primary, search secondary bucket.
    // If a match is found, populate set_id and return 1.
    if (search_bucket_single(sec_bucket, signature, table_, set_id) == 1) {
        return 1; // Match found in secondary bucket
    }
    
    // If no match found in either bucket, return 0.
    return 0;
    
}


int CuckooCacheFilter::lookup_multi_bulk(const void **keys, uint32_t num_keys, uint32_t max_match_per_key, uint32_t *match_count, filter_set_t *set_ids) const {
    // Note: For cache mode, expected at most 1 match per key as per Cuckoo Cache design.

    if (!keys || !match_count || !set_ids || num_keys == 0 || max_match_per_key == 0) {
        return -EINVAL;
    }
    if (num_keys > FILTER_LOOKUP_BULK_MAX) {
        std::cerr << "ERROR: CuckooCacheFilter::lookup_multi_bulk: num_keys exceeds FILTER_LOOKUP_BULK_MAX." << std::endl;
        return -EINVAL;
    }

    uint32_t total_num_keys_with_matches = 0;
    filter_sig_t tmp_sig[FILTER_LOOKUP_BULK_MAX];
    uint32_t prim_buckets[FILTER_LOOKUP_BULK_MAX];
    uint32_t sec_buckets[FILTER_LOOKUP_BULK_MAX];

    for (uint32_t i = 0; i < num_keys; i++) {
        get_cache_bucket_info(keys[i], &prim_buckets[i], &sec_buckets[i], &tmp_sig[i]);
        
        // Future optimization: Add check Intel platform and Prefetch support
        // Future optimization: add prefetch to all cache levels LLC,L2, L1 prefetch0(&buckets[prim_buckets[i]]);
        // Future optimization: add prefetch to all cache levels LLC,L2, L1 prefetch0(&buckets[sec_buckets[i]]);
    }

    for (uint32_t i = 0; i < num_keys; i++) {
        uint32_t current_key_match_count = 0;
        filter_set_t found_set_id = FILTER_NO_MATCH;

        // Future optimization: Add check AVX support 
        // and AVX/SIMD comparison logic here
        // switch (support) { case AVX, AVX256, AVX512: ... default: ... }

        // Search primary bucket first
        if (search_bucket_single(prim_buckets[i], tmp_sig[i], table_, &found_set_id) == 1) {
            current_key_match_count = 1;
        } else {
            // If not found in primary, search secondary bucket
            if (search_bucket_single(sec_buckets[i], tmp_sig[i], table_, &found_set_id) == 1) {
                current_key_match_count = 1;
            }
        }
        
        // Store the match count for the current key
        match_count[i] = current_key_match_count;

        // Store the found set_id (if any) in the first slot designated for this key.
        // Other slots within the max_match_per_key block will remain unused for CuckooCacheFilter.
        if (current_key_match_count == 1) {
            set_ids[i * max_match_per_key] = found_set_id;
            total_num_keys_with_matches++;
        } else {
            // Explicitly set to FILTER_NO_MATCH if nothing found
            set_ids[i * max_match_per_key] = FILTER_NO_MATCH;
        }
    }
    return total_num_keys_with_matches;    
}


int CuckooCacheFilter::add(const void *key, filter_set_t set_id) {
    
    filter_set_t flag_mask = 1U << (sizeof(filter_set_t) * 8 - 1); 

    if (set_id == FILTER_NO_MATCH || (set_id & flag_mask) != 0) {
        std::cerr << "ERROR: CuckooCacheFilter:add invalid set_id used or  MSB is set" << std::endl;
        return -EINVAL; // Invalid set_id
    }

    
    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;

    get_cache_bucket_info (key, &prim_bucket, &sec_bucket, &signature);

    /*
     * If it is cache based filter, we try overwriting (updating)
     * existing entry with the same signature first. In cache mode, we allow
     * false negatives and only cache the most recent keys.
     */
    if (update_entry_search(prim_bucket, signature, table_, set_id) ||
        update_entry_search(sec_bucket, signature, table_, set_id)) {
        return 0; // Updated an existing entry, return 0 for success (no eviction implied)
    }

    /* If not full then insert into one slot */
    int ret = try_insert(table_, prim_bucket, sec_bucket, signature, set_id);
    if (ret != -1) {
        return 0; // Inserted successfully into an empty slot
    }

    /* Randomly pick prim or sec for eviction (since no empty slot) */
    uint32_t select_bucket = (signature & 1U) ? prim_bucket : sec_bucket; // Use signature LSB for random choice
    int evicted_slot = evict_from_bucket_internal(); // Get a random slot within the chosen bucket

    // Overwrite the selected entry in the chosen bucket
    table_[select_bucket].sigs[evicted_slot] = signature;
    table_[select_bucket].sets[evicted_slot] = set_id;

    return 1; // Successfully added with an eviction (overwritten an existing entry)
}

void CuckooCacheFilter::reset() {
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

int CuckooCacheFilter::delete_key(const void *key, filter_set_t set_id) {
    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;

    // Get bucket indices and signature for the given key
    get_cache_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

    // Search in the primary bucket
    for (int i = 0; i < FILTER_BUCKET_ENTRIES; i++) {
        // If both signature and set_id match, mark the entry as unused (FILTER_NO_MATCH)
        if (signature == table_[prim_bucket].sigs[i] &&
            set_id == table_[prim_bucket].sets[i]) {
            table_[prim_bucket].sets[i] = FILTER_NO_MATCH;
            return 0; // Successfully deleted from primary bucket
        }
    }

    // Search in the secondary bucket if not found in primary
    for (int i = 0; i < FILTER_BUCKET_ENTRIES; i++) {
        // If both signature and set_id match, mark the entry as unused (FILTER_NO_MATCH)
        if (signature == table_[sec_bucket].sigs[i] &&
            set_id == table_[sec_bucket].sets[i]) {
            table_[sec_bucket].sets[i] = FILTER_NO_MATCH;
            return 0; // Successfully deleted from secondary bucket
        }
        
    }

    // If the key with the given set_id was not found in either bucket
    return -ENOENT; // Key not found with the given set_id
}

} // namespace VCL