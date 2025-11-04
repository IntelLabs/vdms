/**
 * @file    CuckooCommon.h
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
 * This file declares common data structures and constants for Cuckoo-based
 * filters.
 *
 */

#ifndef CUCKOO_COMMON_H
#define CUCKOO_COMMON_H
#pragma once
#include <random>

#include "vcl/Filter.h"

namespace VCL {

/** Maximum number of pushes for cuckoo insert path in CuckooFilter HT/Cache
 * mode. **/
#define FILTER_MAX_PUSHES 50

extern std::mt19937 cuckoo_rand_engine;

// The bucket struct for Hash Table filters common to CuckooHTFilter and
// CuckooCacheFilter. common header to centralize definition.
struct CACHE_ALIGNED filter_ht_bucket {
  filter_sig_t sigs[FILTER_BUCKET_ENTRIES]; /* 2-byte signature */
  filter_set_t sets[FILTER_BUCKET_ENTRIES]; /* 2-byte set */
};

// --- DECLARATIONS FOR COMMON CUCKOO HELPER FUNCTIONS ---

/**
 * @brief Get the alternate bucket index for cuckoo hashing.
 * @param current_bucket_idx The primary or current bucket index.
 * @param signature The signature of the key.
 * @param bucket_mask The mask to ensure the index is within bounds (e.g.,
 * total_buckets - 1).
 * @return The alternate bucket index.
 */
uint32_t get_alt_bucket_idx(uint32_t current_bucket_idx, filter_sig_t signature,
                            uint32_t bucket_mask);

/**
 * @brief Search a specific bucket for an entry with a matching signature and
 * update its set ID.
 * @param bucket_id The index of the bucket to search.
 * @param signature The signature to look for.
 * @param buckets Pointer to the array of filter_ht_bucket structures (the hash
 * table).
 * @param new_set_id The new set ID to update the entry with if found.
 * @return 1 if an entry with the matching signature was found and updated, 0
 * otherwise.
 */
int update_entry_search(uint32_t bucket_id, filter_sig_t signature,
                        filter_ht_bucket *buckets, filter_set_t new_set_id);

/**
 * @brief Search a specific bucket for a single match for a given signature.
 * Returns the first match found.
 * @param bucket_id The index of the bucket to search.
 * @param signature The signature to look for.
 * @param buckets Pointer to the array of filter_ht_bucket structures (the hash
 * table).
 * @param set_id Output parameter to store the found set ID.
 * @return 1 if a match is found and its set ID is not FILTER_NO_MATCH, 0
 * otherwise.
 */
int search_bucket_single(uint32_t bucket_id, filter_sig_t signature,
                         filter_ht_bucket *buckets, filter_set_t *set_id);

/**
 * @brief Search a specific bucket for multiple matches for a given signature.
 * Populates a results array with found set IDs up to a maximum count.
 * @param bucket_id The index of the bucket to search.
 * @param signature The signature to look for.
 * @param buckets Pointer to the array of filter_ht_bucket structures (the hash
 * table).
 * @param counter Pointer to an integer that counts the number of matches found
 * so far (and is incremented).
 * @param max_match_per_key The maximum number of matches to record for this
 * key.
 * @param set_ids Array to store the found set IDs.
 */
void search_bucket_multi(uint32_t bucket_id, filter_sig_t signature,
                         filter_ht_bucket *buckets, uint32_t *counter,
                         uint32_t max_match_per_key, filter_set_t *set_ids);

/**
 * @brief Attempts to insert a signature and set_id into an empty slot in either
 * the primary or secondary bucket.
 * @param buckets Pointer to the array of filter_ht_bucket structures (the hash
 * table).
 * @param prim Primary bucket index.
 * @param sec Secondary bucket index.
 * @param sig The signature to insert.
 * @param set_id The set ID to associate with the signature.
 * @return 0 if inserted successfully, -1 if no empty slot found in either
 * bucket.
 */
int try_insert(filter_ht_bucket *buckets, uint32_t prim, uint32_t sec,
               filter_sig_t sig, filter_set_t set_id);

/**
 * @brief make_space function for Cuckoo hash using primary and secondary
 * locations
 * @param buckets Pointer to the hash table.
 * @param bucket_mask The mask for bucket indices.
 * @param bkt_idx The current bucket index where space is needed.
 * @param nr_pushes Pointer to an unsigned int counting the number of recursive
 * pushes.
 * @param rand_engine A reference to a random number generator engine.
 * @return The index within the `bkt_idx` bucket where the new entry can be
 * placed (0-FILTER_BUCKET_ENTRIES-1), or a negative error code if space cannot
 * be made (-ENOSPC).
 */
int make_space_bucket(filter_ht_bucket *buckets, uint32_t bucket_mask,
                      uint32_t bkt_idx, unsigned int *nr_pushes,
                      std::mt19937 &rand_engine);

}  // namespace VCL

#endif  // CUCKOO_COMMON_H