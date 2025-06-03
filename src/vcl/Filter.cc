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

#include "vcl/Filter.h"
#include "vcl/CuckooHTFilter.h"
#include "vcl/CuckooCacheFilter.h"
#include "vcl/VBFFilter.h"

#include <iostream>
#include <string>
#include <cstring>

namespace VCL {

// The single, global instance of the FilterCollectionManager.
// Manages the collection of all filters.
FilterCollectionManager s_global_filter_manager;

// Filter class implementations
Filter::Filter(const FilterParameters& params)
    : engine_(params.engine),
      key_len_(params.key_len),
      num_keys_(params.num_keys),
      prim_hash_seed_(params.prim_hash_seed),
      sec_hash_seed_(params.sec_hash_seed),
      extra_flag_(params.extra_flag) {
    std::strncpy(name_, params.name, FILTER_NAMESIZE - 1);
    name_[FILTER_NAMESIZE - 1] = '\0'; // Ensure null termination
}

Filter::~Filter() {
    // Base class destructor. Derived classes will handle their specific cleanup.
    // std::cout << "DEBUG: Base Filter destructor called for: " << name_ << std::endl;
}


//method to create specific filter types
UniqueFilterPtr Filter::create_filter_instance(const FilterParameters* params) {
    if (!params) {
        std::cerr << "ERROR: create_filter_instance: Invalid parameters (null params)." << std::endl;
        return nullptr;
    }

    UniqueFilterPtr new_filter_instance = nullptr;

    switch (params->engine) {
        case CuckooHT:
            new_filter_instance = UniqueFilterPtr(new CuckooHTFilter(*params), FilterDeleter());
            break;
        case CuckooCache:
            new_filter_instance = UniqueFilterPtr(new CuckooCacheFilter(*params), FilterDeleter());
            break;
        case VBF:
            new_filter_instance = UniqueFilterPtr(new VBF_Filter(*params), FilterDeleter());
            break;
        default:
            std::cerr << "ERROR: create_filter_instance: Unknown FilterEngine type." << std::endl;
            return nullptr;
    }

    return new_filter_instance;
}


// FilterCollectionManager implementations
FilterCollectionManager& get_global_filter_manager() {
    return s_global_filter_manager;
}

bool FilterCollectionManager::collection_add_filter(UniqueFilterPtr filter_ptr) {
    if (!filter_ptr) {
        std::cerr << "ERROR: Attempted to add a null filter pointer to collection." << std::endl;
        return false;
    }

    std::string name_str = filter_ptr->get_name();

    if (filters_.count(name_str) > 0) {
        std::cerr << "WARNING: Filter with name '" << name_str << "' already exists in collection. Not adding." << std::endl;
        return false;
    }

    filters_.emplace(name_str, std::move(filter_ptr));
    //std::cout << "DEBUG: Added filter '" << name_str << "' to collection." << std::endl;
    return true;
}

Filter* FilterCollectionManager::collection_get_filter(const std::string& name) {
    auto it = filters_.find(name);
    if (it != filters_.end()) {
        return it->second.get();
    }
    return nullptr;
}

bool FilterCollectionManager::collection_remove_filter(const std::string& name) {
    auto count = filters_.erase(name);
    return count > 0;
}


// Public C-style API wrappers implementations
Filter* filter_find_existing(const char *name) {
    if (!name) {
        return nullptr;
    }
    return get_global_filter_manager().collection_get_filter(name);
}

Filter* filter_create(const FilterParameters *params) {
    if (!params || !params->name) {
        std::cerr << "ERROR: filter_create: Invalid parameters (null params or name)." << std::endl;
        return nullptr;
    }

    // Check if a filter with this name already exists in our collection.
    if (get_global_filter_manager().collection_get_filter(params->name)) {
        std::cerr << "ERROR: filter_create: Filter with name '" << params->name << "' already exists. Aborting creation." << std::endl;
        return nullptr;
    }

    // Use the static factory method of the Filter base class
    UniqueFilterPtr new_filter_ptr = Filter::create_filter_instance(params);

    if (!new_filter_ptr) {
        std::cerr << "ERROR: filter_create: Failed to create specific filter instance." << std::endl;
        return nullptr;
    }

    // Add the newly created (and managed by unique_ptr) filter to the global collection.
    if (get_global_filter_manager().collection_add_filter(std::move(new_filter_ptr))) {
        // If successfully added, return the raw pointer to the filter
        // now owned by the manager. This is safe.
        //std::cout << "DEBUG: Successfully created and added filter '" << params->name << "'." << std::endl;
        return get_global_filter_manager().collection_get_filter(params->name);
    } else {
        // If collection_add_filter failed, the new_filter_ptr (which was moved)
        // is now null, and the original filter was owned by the manager and then failed to be added.
        // The unique_ptr passed to collection_add_filter would have gone out of scope and
        // been properly deallocated if the add failed within that function.
        std::cerr << "ERROR: filter_create: Failed to add filter to collection (after successful internal creation)." << std::endl;
        return nullptr;
    }
}

void filter_free(Filter *filter) {
    if (!filter) {
        std::cerr << "filter_free: Null filter pointer passed, no operation performed." << std::endl;
        return;
    }

    // The public filter_free API's role is to remove the filter from the collection.
    // Removing the unique_ptr from the map will trigger its destructor,
    // which in turn calls our FilterDeleter, which then calls `delete filter`.
    if (!get_global_filter_manager().collection_remove_filter(filter->get_name())) {
        std::cerr << "WARNING: filter_free: Filter '" << filter->get_name() << "' not found in collection. Likely already freed or not managed." << std::endl;
    }
}

// Delegating external API calls to the virtual methods of the Filter object
int filter_lookup(const Filter *filter, const void *key, filter_set_t *set_id) {
    if (!filter) {
        return -EINVAL;
    }
    return filter->lookup(key, set_id);
}

int filter_lookup_bulk(const Filter *filter, const void **keys, uint32_t num_keys, filter_set_t *set_ids) {
    if (!filter) {
        return -EINVAL;
    }
    return filter->lookup_bulk(keys, num_keys, set_ids);
}

int filter_lookup_multi(const Filter *filter, const void *key, uint32_t max_match_per_key, filter_set_t *set_id) {
    if (!filter) {
        return -EINVAL;
    }
    return filter->lookup_multi(key, max_match_per_key, set_id);
}

int filter_lookup_multi_bulk(const Filter *filter, const void **keys, uint32_t num_keys, uint32_t max_match_per_key, uint32_t *match_count, filter_set_t *set_ids) {
    if (!filter) {
        return -EINVAL;
    }
    return filter->lookup_multi_bulk(keys, num_keys, max_match_per_key, match_count, set_ids);
}

int filter_add(Filter *filter, const void *key, filter_set_t set_id) {
    if (!filter) {
        return -EINVAL;
    }
    return filter->add(key, set_id);
}

void filter_reset(Filter *filter) {
    if (!filter) {
        return;
    }
    // reset is non-const
    filter->reset();
}

int filter_delete_key(Filter *filter, const void *key, filter_set_t set_id) {
    if (!filter) {
        return -EINVAL;
    }
    return filter->delete_key(key, set_id);
}

} // namespace VCL