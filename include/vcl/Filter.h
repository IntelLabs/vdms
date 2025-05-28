/**
 * @file   Filter.h
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
 * This file declares the C++ API for Filter.
 
 *
 * The filter functionality is an implementation of a traditional
 * filter (for example Bloom Filter and cuckoo filter) structure that has
 * multiple usages in a variety of workloads and applications. The filter is
 * used to test if a key belongs to certain sets. Two types of such
 * filter structures are implemented: hash-table based (HT) and vector
 * bloom filter (vBF). For HT filter, two subtypes or modes are available,
 * cache and non-cache modes. 
 *
 * +==========+=====================+================+=========================+
 * |   type   |      vbf            |     CuckooFilter  |    CuckooCache       |
 * |          |                     |   (HT-non- Cache) |    (HT--cache)       |
 * +==========+=====================+==========================================+
 * |structure |  bloom-filter array |  hash-table like without storing key     |
 * +----------+---------------------+------------------------------------------+
 * |set id    | limited by bf count |           [1, 0x7fff]                    |
 * |          | up to 32.           |                                          |
 * +----------+---------------------+------------------------------------------+
 * |usages &  | small set range,    | can delete,    | cache most recent keys, |
 * |properties| user-specified      | big set range, | have both false-positive|
 * |          | false-positive rate,| small false    | and false-negative      |
 * |          | no deletion support.| positive depend| depend on table size,   |
 * |          |                     | on table size, | automatic overwritten.  |
 * |          |                     | new key does   |                         |
 * |          |                     | not overwrite  |                         |
 * |          |                     | existing key.  |                         |
 * +----------+---------------------+----------------+-------------------------+
 
 * 
 */

#pragma once

#include <VDMSConfigHelper.h>

#include <map>
#include <string>
#include <vector>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>

#include <memory>    //Required for unique_ptr
#include <unordered_map> 
#include <zlib.h> // Required for crc32
//placeholder until an AVX-enabled standalone library is implemented

#include "Exception.h"
#include "RemoteConnection.h"
#include "timers/TimerMap.h"
#include "utils.h"


namespace VCL {

typedef uint16_t filter_sig_t;			/* signature size is 16 bit */
typedef uint16_t filter_set_t;  /*set ID type stored internally in hash table based filter*/


/** Maximum number of pushes for cuckoo insert path in CuckooFilter HT mode. **/
#define FILTER_MAX_PUSHES 50
/** Invalid set ID used to mean no match found. */
#define FILTER_NO_MATCH 0
/** Maximum size of hash table that can be created. */
#define FILTER_ENTRIES_MAX (1 << 20) //1 Million 
/** Maximum number of keys that can be searched as a bulk */
#define FILTER_LOOKUP_BULK_MAX 64
/** Entry count per bucket in hash table based mode. */
#define FILTER_BUCKET_ENTRIES 8
/** Maximum number of characters in setsum name. */
#define FILTER_NAMESIZE 32
/** Max value of the random number */
#define FILTER_RAND_MAX ~0LLU
/** x86 typical cache line size in bytes */
#define CACHE_LINE_SIZE 64
/** Define cache aligned macro */
#define CACHE_ALIGNED __attribute__((aligned(CACHE_LINE_SIZE)))


enum FilterEngine {
  CuckooHT, //Cuckoo Hash Table, No deletes alowed
  CuckooCache, //Cuckoo Hash Table, Deletes are alowed with overwriting
  VBF //Vector Bloom Filter
};


/** @internal filter structure. */
struct CACHE_ALIGNED filter {
	enum FilterEngine engine; /* Type of the set summary. */
	uint32_t key_len;		/* Length of key. */
	uint32_t prim_hash_seed;	/* Primary hash function seed. */
	uint32_t sec_hash_seed;		/* Secondary hash function seed. */

	/* Hash table based. */
	uint32_t bucket_cnt;		/* Number of buckets. */
	uint32_t bucket_mask;		/* Bit mask to get bucket index. */		
	void *table;	/* This is the handler of hash table. */
	char name[FILTER_NAMESIZE]; /* Name of this set summary. */
};


/* The bucket struct for Hash Table filters */
struct CACHE_ALIGNED filter_ht_bucket {
	filter_sig_t sigs[FILTER_BUCKET_ENTRIES];	/* 2-byte signature */
	filter_set_t sets[FILTER_BUCKET_ENTRIES];	/* 2-byte set */
};


/**
 * Parameter struct used to create filter
 */
struct filter_parameters {

	const char *name;			/**Name of the filter. */

	/**
	 * User to specify the type of the filter from one of
	 * FilterEngine types.
	 *
	 * CuckooHT and CuckooCache are implemented as a hash table. User should use
	 * this type when there are many sets.
	 * CuckooHT does not support deletes, it is a non-cache mode, 
	 * keys cannot be evicted out of the filter. So for
	 * this mode the filter can become full eventually. if number of Keys inserted is close 
	 * to the maximum to be inserted in a filter FILTER_ENTRIES_MAX 
	 * Keys with the same signature but map to the same bucket will occupy multiple
	 * entries. This mode does not give false-negative result. 
	 * But has only false positive probability
	 * false positive probability is in the order of:
	 * false_pos = (1/bucket_count)*(1/2^16), since we use 16-bit signature.
	 * This is because two keys needs to map to same bucket and same
	 * signature to have a collision (false positive). bucket_count is equal
	 * to number of entries (num_keys) divided by entry count per bucket
	 * (FILTER_BUCKET_ENTRIES). 
	 * 
	 * CuckooCache support deletes and entries will be overwritten
	 * It is a hash table with cache mode, keys can be evicted out of the HT filter.
	 * Keys with the same signature and map to the same bucket
	 * will overwrite each other in the setsummary table,
	 * if total number of inserts in a bucket exceeds FILTER_BUCKET_ENTRIES 
	 * This mode is useful for the case that the filter only
	 * needs to keep record of the recently inserted keys. Both
	 * false-negative and false-positive could happen.
	 *
	 *    
	 * vBF filter is a vector of bloom filters. It is used when number
	 * of sets is not big (less than 32)- To BE implemented later
	 */
	enum FilterEngine engine;

	/**
	 * For HT filter, num_keys equals to the number of entries of the
	 * table. When the number of keys inserted in the HT filter
	 * approaches this number, eviction could happen. For CuckooCache,
	 * keys could be evicted out of the table. For CuckooFilter, keys will
	 * be evicted to other secondry like a typical cuckoo hash table.
	 * The table will likely to become full before
	 * the number of inserted keys equal to the total
	 * number of entries, however for Cuckoo Hash Tables this happens
	 * typically at very high load (>95% of allocated hashtable memory) 
	 *
	 */
	uint32_t num_keys;

	/**
	 * The length of key is used for hash calculation. Since key is not
	 * stored in filter, large key does not require more memory space.
	 */
	uint32_t key_len;

	/**
	 * We use two seeds to calculate two independent hashes for each key.
	 *
	 * For HT type, one hash is used as signature, and the other is used
	 * for bucket location.
	 */
	uint32_t prim_hash_seed;

	/**
	 * The secondary seed should be a different value from the primary seed.
	 */
	uint32_t sec_hash_seed;

	/**
	 * Extra flags that may passed in by user
	 */
	uint32_t extra_flag;
};

//internal memory deallocation function.

void internal_filter_destroy(struct VCL::filter *f);


/**
 * De-allocate memory used by filter.
 *
 * @param filter
 *   Pointer to the filter data structure.
 *   If filter is NULL, no operation is performed.
 */
void
filter_free(struct filter *filter);


/***
* Some Bit Manipulation Helper Functions
***/

static inline uint32_t align32pow2(uint32_t x) {
    if (x == 0) {
        return 1;
		}
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

static inline bool is_power_of_2(uint32_t n) { return n > 0 && (n & (n - 1)) == 0; }


struct FilterDeleter {
    void operator()(struct filter* f) const {
        if (f) {
            //std::cout << "DEBUG: Calling VCL::filter_free for filter: " << f->name << std::endl;
            VCL::internal_filter_destroy(f); // Call the helper free function
        }
    }
};

// Type alias for our smart pointer that correctly calls filter_free
// This makes the unique_ptr usage cleaner and explicitly ties it to the custom deleter.
using UniqueFilterPtr = std::unique_ptr<struct filter, FilterDeleter>;


// =============================================================
// Internal Filter Collection Management Class
// This class is responsible for owning and managing the lifecycle
// of the 'struct filter' objects. 
// =============================================================
class FilterCollectionManager {
public:
    // Adds a filter to the collection. Takes ownership of the unique_ptr.
    // The 'std::move' is critical when calling this function.
    bool collection_add_filter(UniqueFilterPtr filter_ptr);

    // Retrieves a raw pointer to an existing filter. Does not transfer ownership.
    // The caller of this function should NOT delete the returned pointer.
    struct filter* collection_get_filter(const std::string& name);

    // Removes a filter from the collection and triggers its deallocation.
    // When an entry is erased from the map, the UniqueFilterPtr's destructor
    // is called, which in turn invokes our FilterDeleter.
    bool collection_remove_filter(const std::string& name);

    // Default constructor and destructor are sufficient as unique_ptr
    // handles its own memory management.
    FilterCollectionManager() = default;
    ~FilterCollectionManager() = default;

    // Delete copy constructor and assignment operator to prevent accidental copies
    // which would violate unique_ptr's exclusive ownership.
    FilterCollectionManager(const FilterCollectionManager&) = delete;
    FilterCollectionManager& operator=(const FilterCollectionManager&) = delete;

private:
    // The core data structure for the collection.
    // Maps a filter's name (std::string) to a UniqueFilterPtr that owns the filter.
    std::unordered_map<std::string, UniqueFilterPtr> filters_;
};

// Global accessor for the single instance of FilterCollectionManager.
// The actual instance is defined in the .cc file (often in an unnamed namespace)
// to ensure it's a true singleton and its lifetime is correctly managed
// across the entire program.
FilterCollectionManager& get_global_filter_manager();

//Filter Functions

/**
 * Find an existing filter and return a pointer to it.
 *
 * @param name
 *   Name of the filter
 * @return
 *   Pointer to the filter or NULL if object not found
 */
struct filter *
filter_find_existing(const char *name);

/**
 * Create fileter of certain type
 *
 * @param params
 *   Parameters to initialize the filter
 * @return
 *   Return the pointer to the filter.
 *   Return value is NULL if the creation failed.
 */
struct filter *
filter_create(const struct filter_parameters *params);

//Lookup Functions

/**
 * Lookup key in filter.
 * Single key lookup and return as soon as the first match found
 *
 * @param filter
 *   Pointer of a filter.
 * @param key
 *   Pointer of the key to be looked up.
 * @param set_id
 *   Output the set id matches the key.
 * @return
 *   Return 1 for found a match and 0 for not found a match.
 */
int
filter_lookup(const struct filter *filter, const void *key,
			filter_set_t *set_id);

/**
 * Lookup bulk of keys in filter 
 * Each key lookup returns as soon as the first match found
 *
 * @param filter
 *   Pointer of a filter.
 * @param keys
 *   Pointer of the bulk of keys to be looked up.
 * @param num_keys
 *   Number of keys that will be lookup.
 * @param set_ids
 *   Output set ids for all the keys to this array.
 *   User should preallocate array that can contain all results, which size is
 *   the num_keys.
 * @return
 *   The number of keys that found a match.
 */
int
filter_lookup_bulk(const struct filter *filter,
			const void **keys, uint32_t num_keys,
			filter_set_t *set_ids);

/**
 * Lookup a key in filter for multiple matches.
 * The key lookup will find all matched entries (multiple match).
 * Note that for CuckooCache, each key can have at most one match. This is
 * because keys with same signature that maps to same bucket will overwrite
 * each other. So multi-match lookup should be used for CuckooFilter
 *
 * @param filter
 *   Pointer of a filter.
 * @param key
 *   Pointer of the key that to be looked up.
 * @param max_match_per_key
 *   User specified maximum number of matches for each key. The function returns
 *   as soon as this number of matches found for the key.
 * @param set_id
 *   Output set ids for all the matches of the key. User needs to preallocate
 *   the array that can contain max_match_per_key number of results.
 * @return
 *   The number of matches that found for the key.
 *   For CuckooCache filter, the number should be at most 1.
 */
int
filter_lookup_multi(const struct filter *filter,
		const void *key, uint32_t max_match_per_key,
		filter_set_t *set_id);

/**
 * Lookup a bulk of keys in a filter for multiple matches each key.
 * Each key lookup will find all matched entries (multiple match).
 * Note that for CuckooCache mode, each key can have at most one match. So
 * multi-match function is mainly used for CuckooFilter.
 *
 * @param filter
 *   Pointer of a filter.
 * @param keys
 *   Pointer of the keys to be looked up.
 * @param num_keys
 *   The number of keys that will be lookup.
 * @param max_match_per_key
 *   The possible maximum number of matches for each key.
 * @param match_count
 *   Output the number of matches for each key in an array.
 * @param set_ids
 *   Return set ids for all the matches of all keys. Users pass in a
 *   preallocated 2D array with first dimension as key index and second
 *   dimension as match index. For example set_ids[bulk_size][max_match_per_key]
 * @return
 *   The number of keys that found one or more matches in the filter.
 */
int
filter_lookup_multi_bulk(const struct filter *filter,
		const void **keys, uint32_t num_keys,
		uint32_t max_match_per_key,
		uint32_t *match_count,
		filter_set_t *set_ids);



//Insert Functions

/**
 * Insert key into filter.
 *
 * @param filter
 *   Pointer of a filter
 * @param key
 *   Pointer of the key to be added.
 * @param set_id
 *   The set id associated with the key that needs to be added.
 *   0 cannot be used as set_id since 
 *   FILTER_NO_MATCH by default is set as 0. 
 *   For HT filters, the set_id has range as [1, 0x7FFF], MSB is reserved.
 * @return
 *   CuckooCache insert should never fail unless the set_id is not in the
 *   valid range. In such case -EINVAL is returned.
 *   For CuckooFilter (non-cache mode) it could fail with -ENOSPC error code when 
 *   hash table is full.
 *   For success it returns different values for different modes to provide
 *   extra information for users.
 *   Return 0 for CuckooCache if the add does not cause
 *   eviction, return 1 otherwise. Return 0 for CuckooFilter mode if success,
 *   -ENOSPC for full, and 1 if the insert caused cuckoo eviction happens.
 */

int
filter_add(const struct filter *filter, const void *key,
			filter_set_t set_id);



/**
 * Reset the filter tables. 
 * e.g. reset set_id in each entry to be FILTER_NO_MATCH
 *
 * @param filter
 *   Pointer to the filter.
 */
void
filter_reset(const struct filter *filter);

/**
 * Delete items from the filter. 
 *
 * @param filter
 *   Pointer to the filter
 * @param key
 *   Pointer of the key to be deleted.
 * @param set_id
 *   For HT mode, we need both key and its corresponding set_id to
 *   properly delete the key. Without set_id, we may delete other keys with the
 *   same signature.
 * @return
 *   If no entry found to delete, an error code of -ENOENT could be returned.
 */
int
filter_delete(const struct filter *filter, const void *key,
			filter_set_t set_id);


}; //namespace VCL
