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
#include <chrono> 

namespace VCL {

// Static random engine for make_space_bucket's internal use (passed as ref)
extern std::mt19937 cuckoo_rand_engine; // Declared in CuckooCommon.cc


void CuckooHTFilter::get_ht_bucket_info(const void *key,
                                         uint32_t *out_prim_bucket, uint32_t *out_sec_bucket, filter_sig_t *out_signature) const {
    
    uint32_t first_hash = crc32(prim_hash_seed_, static_cast<const Bytef*>(key), key_len_);
    uint32_t sec_hash = crc32(sec_hash_seed_, reinterpret_cast<const Bytef*>(&first_hash), sizeof(uint32_t));

    //*out_signature = static_cast<filter_sig_t>(first_hash & 0xFFFF); // 16-bit signature

    // increased the signature to 32-bit hash because of collisions 
    // No truncation needed to 16 bit
    *out_signature = first_hash;

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

    if (!this->_valid) { 
        return;
    }

    if ((num_entries > FILTER_ENTRIES_MAX) ||
        !is_power_of_2(FILTER_BUCKET_ENTRIES) ||
        num_entries < FILTER_BUCKET_ENTRIES) {
        std::cerr << "ERROR: CuckooHTFilter: Invalid parameters for HT creation (num_entries or bucket_entries)." << std::endl;
        this->_valid = false;
        return; 
    }

    bucket_cnt_ = num_entries / FILTER_BUCKET_ENTRIES;
    bucket_mask_ = bucket_cnt_ - 1;

    //table_ = static_cast<filter_ht_bucket*>(std::malloc(bucket_cnt_ * size_bucket_t));
    table_ = static_cast<filter_ht_bucket*>(std::aligned_alloc(CACHE_LINE_SIZE, bucket_cnt_ * size_bucket_t));

    if (table_ == NULL) {
        std::cerr << "ERROR: CuckooHTFilter: Memory allocation failed for HT buckets." << std::endl;
        this->_valid = false;
        return;
    }

    // Initialize all sigs and sets in the buckets
    for (uint32_t i = 0; i < bucket_cnt_; ++i) {
        for (uint32_t j = 0; j < FILTER_BUCKET_ENTRIES; ++j) {
            table_[i].sigs[j] = 0; // Initialize signatures to 0 (or some 'empty' value)
            table_[i].sets[j] = FILTER_NO_MATCH; // Initialize set IDs to NO_MATCH
        }
    }
    this->_valid = true;
}

CuckooHTFilter::~CuckooHTFilter() {
    if (table_ != nullptr) {
        std::free(table_);
        table_ = nullptr;
    }
}

// --- Implementations of pure virtual functions ---

int CuckooHTFilter::lookup(const void *key, filter_set_t *set_id) const {
    if (!key || !set_id) {
        return -EINVAL;
    }

    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;

    get_ht_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

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


int CuckooHTFilter::lookup_bulk(const void **keys, uint32_t num_keys, filter_set_t *set_ids) const {
    if (!keys || !set_ids || num_keys == 0) {
        return -EINVAL;
    }
    if (num_keys > FILTER_LOOKUP_BULK_MAX) {
        std::cerr << "ERROR: CuckooHTFilter::lookup_bulk: num_keys exceeds FILTER_LOOKUP_BULK_MAX." << std::endl;
        return -EINVAL; // Or handle larger bulk sizes dynamically
    }

    uint32_t num_matches = 0;
    filter_sig_t tmp_sig[FILTER_LOOKUP_BULK_MAX];
    uint32_t prim_buckets[FILTER_LOOKUP_BULK_MAX];
    uint32_t sec_buckets[FILTER_LOOKUP_BULK_MAX];

    
    for (uint32_t i = 0; i < num_keys; i++) {
        
        get_ht_bucket_info(keys[i], &prim_buckets[i], &sec_buckets[i], &tmp_sig[i]);
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

int CuckooHTFilter::lookup_multi(const void *key, uint32_t max_match_per_key, filter_set_t *set_id) const {
    if (!key || !set_id || max_match_per_key == 0) {
        return -EINVAL;
    }

    uint32_t num_matches = 0;
    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;
    
    get_ht_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

    // Future optimization: Add check AVX support 
    // and AVX/SIMD comparison logic here
    // switch (support) { case AVX, AVX256, AVX512: ... default: ... }


    // Search primary bucket and populate matches
    search_bucket_multi(prim_bucket, signature, table_, &num_matches, max_match_per_key, set_id);
    
    // If not all desired matches found, search secondary bucket
    if (num_matches < max_match_per_key) {
        search_bucket_multi(sec_bucket, signature, table_, &num_matches, max_match_per_key, set_id);
    }
    
    return num_matches;
}


int CuckooHTFilter::lookup_multi_bulk(const void **keys, uint32_t num_keys, uint32_t max_match_per_key, uint32_t *match_count, filter_set_t *set_ids) const {
    if (!keys || !match_count || !set_ids || num_keys == 0 || max_match_per_key == 0) {
        return -EINVAL;
    }
    if (num_keys > FILTER_LOOKUP_BULK_MAX) {
        std::cerr << "ERROR: CuckooHTFilter::lookup_multi_bulk: num_keys exceeds FILTER_LOOKUP_BULK_MAX." << std::endl;
        return -EINVAL;
    }

    uint32_t total_num_keys_with_matches = 0;
    filter_sig_t tmp_sig[FILTER_LOOKUP_BULK_MAX];
    uint32_t prim_buckets[FILTER_LOOKUP_BULK_MAX];
    uint32_t sec_buckets[FILTER_LOOKUP_BULK_MAX];

    for (uint32_t i = 0; i < num_keys; i++) {
        get_ht_bucket_info(keys[i], &prim_buckets[i], &sec_buckets[i], &tmp_sig[i]);
        // Future optimization: Add check Intel platform and Prefetch support
        // Future optimization: add prefetch to all cache levels LLC,L2, L1 prefetch0(&buckets[prim_buckets[i]]);
        // Future optimization: add prefetch to all cache levels LLC,L2, L1 prefetch0(&buckets[sec_buckets[i]]);
    }

    for (uint32_t i = 0; i < num_keys; i++) {
        uint32_t current_key_match_count = 0;

        
        // Future optimization: Add check AVX support 
        // and AVX/SIMD comparison logic here
        // switch (support) { case AVX, AVX256, AVX512: ... default: ... }

        search_bucket_multi(prim_buckets[i], tmp_sig[i], table_,
                            &current_key_match_count, max_match_per_key,
                            &set_ids[i * max_match_per_key]);
        
        if (current_key_match_count < max_match_per_key) {
            search_bucket_multi(sec_buckets[i], tmp_sig[i], table_,
                                &current_key_match_count, max_match_per_key,
                                &set_ids[i * max_match_per_key]);
        }
        
        match_count[i] = current_key_match_count;
        if (current_key_match_count != 0) {
            total_num_keys_with_matches++;
        }
    }
    return total_num_keys_with_matches;

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
    uint32_t prim_bucket, sec_bucket;
    filter_sig_t signature;

    // Get bucket indices and signature for the given key
    get_ht_bucket_info(key, &prim_bucket, &sec_bucket, &signature);

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
    return -ENOENT;
}

} // namespace VCL
