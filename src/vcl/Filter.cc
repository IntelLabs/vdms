/**
 * @file   Filter.cc
 *
 * @section LICENSE
 *
 * The MIT License
 *
 * @copyright Copyright (c) 2017 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files
 * (the "Software"), to deal
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

#include <filesystem>
#include <iostream>
#include <stdlib.h>
#include <stdexcept> 
#include <string>
#include <cstring>  

// clang-format off
#include "vcl/Filter.h"
// clang-format on





namespace {
// The single, global instance of the FilterCollectionManager.
// manages collection of all filters
VCL::FilterCollectionManager s_global_filter_manager;

// Forward declarations for internal helper functions
int filter_create_ht(struct VCL::filter *filter, const struct VCL::filter_parameters *params);
int filter_create_VBF(struct VCL::filter *filter, const struct VCL::filter_parameters *params);

int filter_lookup_ht(const struct VCL::filter *filter, const void *key, VCL::filter_set_t *set_id);
uint32_t filter_lookup_bulk_ht(const struct VCL::filter *filter, const void **keys, uint32_t num_keys, VCL::filter_set_t *set_ids);
uint32_t filter_lookup_multi_ht(const struct VCL::filter *filter, const void *key, uint32_t match_per_key, VCL::filter_set_t *set_id);
uint32_t filter_lookup_multi_bulk_ht(const struct VCL::filter *filter, const void **keys, uint32_t num_keys, uint32_t match_per_key, uint32_t *match_count, VCL::filter_set_t *set_ids);

int filter_add_ht(const struct VCL::filter *filter, const void *key, VCL::filter_set_t set_id);

int filter_delete_ht(const struct VCL::filter *filter, const void *key, VCL::filter_set_t set_id);
void filter_reset_ht(const struct VCL::filter *filter);
} // unnamed namespace





/*
**********************************
Collection of Filters interface 
***********************************
*/

namespace VCL {

// function to get reference to the FilterCollectionManager instance.
FilterCollectionManager& get_global_filter_manager() {
    return s_global_filter_manager;
}

// add a filter pointer to a collection 
bool FilterCollectionManager::collection_add_filter(UniqueFilterPtr filter_ptr) {
    if (!filter_ptr) {
        std::cerr << "ERROR: Attempted to add a null filter pointer." << std::endl;
        return false;
    }
    
    std::string name_str = filter_ptr->name;

    // Check if a filter with this name already exists in the collection.
    // This prevents adding duplicates by name.
    if (filters_.count(name_str) > 0) {
        std::cerr << "WARNING: Filter with name '" << name_str << "' already exists. Not adding." << std::endl;
        return false;
    }

    // Insert the new filter into the map.
    // std::move is crucial here: it transfers ownership of the 'struct filter'
    // from the 'filter_ptr' parameter (which is a temporary unique_ptr)
    // to the unique_ptr held within the 'filters_' map.
    filters_.emplace(name_str, std::move(filter_ptr));
    //std::cout << "DEBUG: Added filter '" << name_str << "' to collection." << std::endl;
    return true;
}


// Get a filter pointer from a collection
// using the name of the filter 
struct filter* FilterCollectionManager::collection_get_filter(const std::string& name) {
    auto it = filters_.find(name);
    if (it != filters_.end()) {
        // If found, 'it->second' is the UniqueFilterPtr.
        // '.get()' returns the raw pointer to the underlying 'struct filter'.        
        return it->second.get();
    }
    return nullptr; // Filter not found
}


// remove a filter pointer from a collection
// using the name of the filter 
bool FilterCollectionManager::collection_remove_filter(const std::string& name) {    
    // erase() returns the number of elements removed (0 or 1 for a map).
    auto count = filters_.erase(name);    
    return count > 0; 
}

}






/*
*****************************
FILTER interface 
******************************
*/

namespace VCL {

//helper function for memory deallocation
void internal_filter_destroy(struct VCL::filter *filter) {
    if (!filter) {
        return;
    }
    // Free the internal table if it was allocated
    if (filter->table != nullptr) {
        std::free(filter->table);
        filter->table = nullptr; // Clear pointer after freeing
    }

    // Finally, free the main filter struct itself
    std::free(filter);
    // std::cout << "DEBUG: Raw memory for filter '" << filter->name << "' destroyed." << std::endl;
}

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
filter_find_existing(const char *name){
	if (!name) {
        return nullptr; 
    }
    return get_global_filter_manager().collection_get_filter(name);
}

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
filter_create(const struct filter_parameters *params){
	if (!params || !params->name) {
        std::cerr << "ERROR: filter_create: Invalid parameters (null params or name)." << std::endl;
        return nullptr;
    }

    if (params->key_len == 0 ||
        params->prim_hash_seed == params->sec_hash_seed) {
        std::cerr << "ERROR: filter_create: Invalid parameters (key length or hash seeds cannot be zero/equal)." << std::endl;
        return nullptr;
    }

    // Check if a filter with this name already exists in our collection before creating.
    if (get_global_filter_manager().collection_get_filter(params->name)) {
        std::cerr << "ERROR: filter_create: Filter with name '" << params->name << "' already exists. Aborting creation." << std::endl;
        return nullptr;
    }

    // Allocate memory for the core 'struct filter'
    struct filter* new_filter = static_cast<struct filter*>(std::malloc(sizeof(struct filter)));
    if (!new_filter) {
        std::cerr << "ERROR: filter_create: Memory allocation failed for struct filter." << std::endl;
        return nullptr;
    }

    // Initialize the new filter's common members
    std::strncpy(new_filter->name, params->name, FILTER_NAMESIZE - 1);
    new_filter->name[FILTER_NAMESIZE - 1] = '\0'; // Ensure null termination
    new_filter->engine = params->engine;
    new_filter->key_len = params->key_len;
    new_filter->prim_hash_seed = params->prim_hash_seed;
    new_filter->sec_hash_seed = params->sec_hash_seed;
    new_filter->table = nullptr; // Initialize to null, will be set by type-specific create functions

    int ret = 0; 
    switch (new_filter->engine) {
        case CuckooHT:
        case CuckooCache: // Both CuckooHT and CuckooCache use the same underlying HT structure
            ret = filter_create_ht(new_filter, params);            
            break;
        case VBF:
            ret = filter_create_VBF(new_filter, params);
            break;
        default:
            std::cerr << "ERROR: filter_create: Filter type not supported." << std::endl;
            // Clean up the initially allocated new_filter before returning nullptr
            std::free(new_filter);
            return nullptr;
    }

    // If type-specific creation failed, clean up and return
    if (ret != 0) {
        std::cerr << "ERROR: filter_create: Failed to create internal filter structure (error: " << ret << ")." << std::endl;
        // Clean up the initially allocated new_filter (and potentially its table if allocated by helper)
        // This is handled by FilterDeleter if we put it into UniqueFilterPtr
        std::free(new_filter); // Free the initial allocation
        return nullptr;
    }

    UniqueFilterPtr managed_filter_ptr(new_filter);

    // Add the managed filter to our global collection.
    // to the `UniqueFilterPtr` inside the `filters_` map.
    if (get_global_filter_manager().collection_add_filter(std::move(managed_filter_ptr))) {
        // If successfully added, return the raw pointer to the filter
        // now owned by the manager. This is safe.
        //std::cout << "DEBUG: Successfully created and added filter '" << params->name << "'." << std::endl;
        return get_global_filter_manager().collection_get_filter(params->name);
    } else {
        // If `collection_add_filter` failed, the `managed_filter_ptr` local variable
        // will go out of scope here. Its destructor will be called, which will
        // automatically invoke `VCL::filter_free` on `new_filter` (and its internal table),
        // preventing a memory leak for the filter that couldn't be added.
        std::cerr << "ERROR: filter_create: Failed to add filter to collection (after successful internal creation)." << std::endl;
        return nullptr;
    }

}









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
			filter_set_t *set_id){
				return 0; //placeholder
			}

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
			filter_set_t *set_ids){
				return 0; //placeholder
			}

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
		filter_set_t *set_id){
			return 0; //placeholder
		}

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
		filter_set_t *set_ids){
			return 0; //placeholder
		}



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
			filter_set_t set_id){
				return 0; //placeholder
			}


/**
 * De-allocate memory used by filter.
 *
 * @param filter
 *   Pointer to the filter data structure.
 *   If filter is NULL, no operation is performed.
 */
void
filter_free(struct filter *filter){
	if (!filter) {
        std::cerr << "filter_free: Null filter pointer passed, no operation performed." << std::endl;
        return;
    }

    // The public filter_free API's role is to remove the filter from the collection.
    // Removing the unique_ptr from the map will trigger its destructor,
    // which in turn calls our FilterDeleter, which then calls `internal_filter_destroy`.
    if (!get_global_filter_manager().collection_remove_filter(filter->name)) {
        // This warning indicates that the filter pointer passed was either
        // already freed, or was never successfully added to the collection.
        // In either case, we do not attempt to free the raw pointer here,
        // as its ownership is not clear or it's already deallocated.
        std::cerr << "WARNING: filter_free: Filter '" << filter->name << "' not found in collection. Likely already freed or not managed." << std::endl;
    }
}


/**
 * Reset the filter tables. 
 * e.g. reset set_id in each entry to be FILTER_NO_MATCH
 *
 * @param filter
 *   Pointer to the filter.
 */
void
filter_reset(const struct filter *filter){}

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
			filter_set_t set_id){
				return 0; //placeholder
			}


} //namespace VCL


/*
*******************************
End of Filter Interface
********************************
********************************
*/


namespace
{   
// Helper function to create and initialize Cuckoo Hash Table filters
int filter_create_ht(struct VCL::filter *filter,
                     const struct VCL::filter_parameters *params) {
	uint32_t i, j;
	uint32_t size_bucket_t = sizeof(struct VCL::filter_ht_bucket);	
    uint32_t num_entries = VCL::align32pow2(params->num_keys);

    if ((num_entries > FILTER_ENTRIES_MAX) ||
        !VCL::is_power_of_2(FILTER_BUCKET_ENTRIES) ||         
		num_entries < FILTER_BUCKET_ENTRIES) {
        std::cerr << "ERROR: Filter HT create with invalid parameters (num_entries or bucket_entries)." << std::endl;
        return -EINVAL; // Invalid argument
    }

    uint32_t num_buckets = num_entries / FILTER_BUCKET_ENTRIES;

    // Corrected malloc size calculation: num_buckets * size_bucket_t
    struct VCL::filter_ht_bucket *buckets = static_cast<VCL::filter_ht_bucket*>(
        std::malloc(num_buckets * size_bucket_t));

    if (buckets == NULL) {
        std::cerr << "ERROR: Memory allocation failed for HT buckets." << std::endl;
        return -ENOMEM; // Out of memory
    }

    filter->table = buckets;
    filter->bucket_cnt = num_buckets;
    filter->bucket_mask = num_buckets - 1;

    // Initialize all sigs and sets in the buckets
    for (i = 0; i < num_buckets; i++) {
        for (j = 0; j < FILTER_BUCKET_ENTRIES; j++) {
            buckets[i].sigs[j] = 0; // Initialize signatures to 0 (or some 'empty' value)
            buckets[i].sets[j] = FILTER_NO_MATCH; // Initialize set IDs to NO_MATCH
        }
    }
    //std::cout << "DEBUG: CuckooHT/CuckooCache filter table allocated and initialized." << std::endl;
    return 0; // Success
}

// Helper function to create and initialize Vector Bloom Filter filters
int filter_create_VBF(struct VCL::filter *filter,
                      const struct VCL::filter_parameters *params) {
    // Implement actual VBF allocation and initialization logic here
    // For now, it's a stub that reports "not yet implemented".
    // Remember to allocate filter->table if needed and return 0 on success, or a negative error code on failure.
    std::cerr << "ERROR: VBF filter type not yet implemented." << std::endl;
    return -ENOSYS; // Operation not supported
}

	

    int
    filter_lookup_ht(const struct filter *filter,
		const void *key, VCL::filter_set_t *set_id){
			return 0; //placeholder
		}

    uint32_t
    filter_lookup_bulk_ht(const struct filter *filter,
		const void **keys, uint32_t num_keys,
		VCL::filter_set_t *set_ids){
			return 0; //placeholder
		}

    uint32_t
    filter_lookup_multi_ht(const struct filter *filter,
		const void *key, uint32_t match_per_key,
		VCL::filter_set_t *set_id){
			return 0; //placeholder
		}

    uint32_t
    filter_lookup_multi_bulk_ht(const struct filter *filter,
		const void **keys, uint32_t num_keys, uint32_t match_per_key,
		uint32_t *match_count,
		VCL::filter_set_t *set_ids){
			return 0; //placeholder
		}

    int
    filter_add_ht(const struct filter *filter,
		const void *key, VCL::filter_set_t set_id){
			return 0; //placeholder
		}    

    int
    filter_delete_ht(const struct filter *filter, const void *key,
		VCL::filter_set_t set_id){
			return 0; //placeholder
		}

    void
    filter_reset_ht(const struct filter *filter){}

     
} //namespace private unnamed
