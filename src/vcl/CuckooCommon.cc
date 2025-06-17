/**
 * @file    CuckooCommon.cc
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

#include "vcl/CuckooCommon.h" // Include its own header for declarations
#include <iostream>           // For potential debugging/error output if needed
#include <string.h>           // For memcpy or other byte-level operations
#include <random>
#include <chrono>
#include <functional>

namespace VCL {

// --- IMPLEMENTATIONS OF COMMON CUCKOO HELPER FUNCTIONS ---

// random engine for make_space_bucket and evict_from_bucket
std::mt19937 cuckoo_rand_engine(std::chrono::high_resolution_clock::now().time_since_epoch().count());


// This function is specific to Cuckoo hashing's alternate path calculation.
uint32_t get_alt_bucket_idx(uint32_t current_bucket_idx, filter_sig_t signature, uint32_t bucket_mask) {
    return (static_cast<uint32_t>(signature) ^ current_bucket_idx) & bucket_mask;
}



// --- Bucket Search and Update Functions ---
int update_entry_search(uint32_t bucket_id, filter_sig_t signature,
                        filter_ht_bucket *buckets, filter_set_t new_set_id) {
    for (uint32_t i = 0; i < FILTER_BUCKET_ENTRIES; ++i) {
        if (buckets[bucket_id].sigs[i] == signature) {
            buckets[bucket_id].sets[i] = new_set_id;
            return 1; // Found and updated
        }
    }
    return 0; // Not found
}

int try_insert(filter_ht_bucket *buckets, uint32_t prim, uint32_t sec,
               filter_sig_t sig, filter_set_t set_id) {
    // Try primary bucket first
    for (int i = 0; i < FILTER_BUCKET_ENTRIES; i++) {
        if (buckets[prim].sets[i] == FILTER_NO_MATCH) {
            buckets[prim].sigs[i] = sig;
            buckets[prim].sets[i] = set_id;
            return 0; // Inserted into primary bucket
        }
    }
    // If primary failed, try secondary bucket
    for (int i = 0; i < FILTER_BUCKET_ENTRIES; i++) {
        if (buckets[sec].sets[i] == FILTER_NO_MATCH) {
            buckets[sec].sigs[i] = sig;
            buckets[sec].sets[i] = set_id;
            return 0; // Inserted into secondary bucket
        }
    }
    // No empty slot found in both primary and secondary
    //add function will have to go next through the cuckoo path
    return -1;
}


int make_space_bucket(filter_ht_bucket *buckets, uint32_t bucket_mask,
                      uint32_t bkt_idx, unsigned int *nr_pushes,
                      std::mt19937& rand_engine) {
    unsigned int i, j;
    int ret_val;
    filter_ht_bucket *bkt = &buckets[bkt_idx];
    filter_set_t flag_mask = 1U << (sizeof(filter_set_t) * 8 - 1); // MSB for pushed flag

    /*
     * Push existing item (search for bucket with space in
     * alternative locations) to its alternative location
     */
    for (i = 0; i < FILTER_BUCKET_ENTRIES; i++) {
        // Calculate the alternate bucket for the entry currently at bkt->sigs[i]
        uint32_t next_bucket_idx = get_alt_bucket_idx(bkt_idx, bkt->sigs[i], bucket_mask);
        filter_ht_bucket *next_bkt = &buckets[next_bucket_idx];

        // Search for space in this alternative bucket
        for (j = 0; j < FILTER_BUCKET_ENTRIES; j++) {
            if (next_bkt->sets[j] == FILTER_NO_MATCH)
                break;
        }

        if (j != FILTER_BUCKET_ENTRIES)
            break; // Found a slot for this entry in its alt bucket
    }

    /* Alternative location has spare room (end of recursive function) */
    if (i != FILTER_BUCKET_ENTRIES) {
        // Move the entry from current bucket (bkt_idx, slot i) to its alternate bucket (next_bucket_idx, slot j)
        uint32_t next_bucket_idx_to_move = get_alt_bucket_idx(bkt_idx, bkt->sigs[i], bucket_mask);
        filter_ht_bucket *next_bkt_to_move = &buckets[next_bucket_idx_to_move];

        next_bkt_to_move->sigs[j] = bkt->sigs[i];
        next_bkt_to_move->sets[j] = bkt->sets[i];
        return i; // Return the index in the original bucket that is now free
    }

    /* Pick entry that has not been pushed yet (to avoid infinite loops) */
    // Use std::uniform_int_distribution for random selection within the bucket
    std::uniform_int_distribution<unsigned int> dist(0, FILTER_BUCKET_ENTRIES - 1);
    unsigned int start_idx = dist(rand_engine); // Random starting point for iteration

    i = FILTER_BUCKET_ENTRIES; // Initialize to indicate no suitable entry found yet
    for (unsigned int k = 0; k < FILTER_BUCKET_ENTRIES; ++k) {
        unsigned int current_entry_idx = (start_idx + k) % FILTER_BUCKET_ENTRIES;
        if ((bkt->sets[current_entry_idx] & flag_mask) == 0) {
            i = current_entry_idx;
            break;
        }
    }

    /* All entries have been pushed, so entry cannot be added or max pushes exceeded */
    if (i == FILTER_BUCKET_ENTRIES || ++(*nr_pushes) > FILTER_MAX_PUSHES)
        return -ENOSPC;


    // Store the signature and set_id of the entry to be pushed *before* modifying sets[i]
    filter_sig_t pushed_sig = bkt->sigs[i];
    filter_set_t pushed_set = bkt->sets[i];

    uint32_t next_bucket_idx_for_push = get_alt_bucket_idx(bkt_idx, pushed_sig, bucket_mask);
    /* Set flag to indicate that this entry is going to be pushed */
    bkt->sets[i] |= flag_mask;

    /* Need room in alternative bucket to insert the pushed entry */
    ret_val = make_space_bucket(buckets, bucket_mask, next_bucket_idx_for_push, nr_pushes, rand_engine);
    /*
     * After recursive function.
     * Clear flags and insert the pushed entry
     * in its alternative location if successful,
     * or return error
     */
    bkt->sets[i] &= ~flag_mask; // Clear the pushed flag
    if (ret_val >= 0) {
        // The element at bkt->sigs[i] has successfully been moved to its alt location
        // The slot 'i' in the current bucket 'bkt_idx' is now free.
        buckets[next_bucket_idx_for_push].sigs[ret_val] = pushed_sig;
        buckets[next_bucket_idx_for_push].sets[ret_val] = pushed_set;
        return i;
    } else {
        // Recursion failed to make space
        return ret_val;
    }
}




int search_bucket_single(uint32_t bucket_id, filter_sig_t signature,
                         filter_ht_bucket *buckets, filter_set_t *set_id) {
    for (uint32_t iter = 0; iter < FILTER_BUCKET_ENTRIES; ++iter) {
        if (signature == buckets[bucket_id].sigs[iter] &&
            buckets[bucket_id].sets[iter] != FILTER_NO_MATCH) {
            *set_id = buckets[bucket_id].sets[iter];
            return 1; // Found a match
        }
    }
    return 0; // No match found
}

void search_bucket_multi(uint32_t bucket_id, filter_sig_t signature,
                         filter_ht_bucket *buckets, uint32_t *counter,
                         uint32_t max_match_per_key, filter_set_t *set_ids) {
    for (uint32_t iter = 0; iter < FILTER_BUCKET_ENTRIES; ++iter) {
        if (signature == buckets[bucket_id].sigs[iter] &&
            buckets[bucket_id].sets[iter] != FILTER_NO_MATCH) {
            set_ids[*counter] = buckets[bucket_id].sets[iter];
            (*counter)++;
            if (*counter >= max_match_per_key) {
                return; // Reached maximum matches for this key
            }
        }
    }
}


} // namespace VCL