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
 *
 *
 * This file declares the C++ API for the base Filter class and its management.
 * It defines the abstract interface for different filter implementations
 * (e.g., CuckooFilter, CuckooCacheFilter, VBF_Filter).
 *
 */
#ifndef FILTER_H
#define FILTER_H

#pragma once

#include <VDMSConfigHelper.h>
#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <zlib.h>  // Required for crc32

#include <cmath>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
// place holder for CRC implementation for now

// Assuming these are internal dependencies
#include "Exception.h"
#include "RemoteConnection.h"
#include "timers/TimerMap.h"
#include "utils.h"

namespace VCL {

using std::nullopt;

// typedef uint16_t filter_sig_t;   /* signature size is 16 bit */

typedef uint32_t filter_sig_t; /* signature increased to 32 bit */
// test cases revealed collision with CRC32 for 16 bit that's why it is
// increased may revisit for other hash functions

typedef uint16_t
    filter_set_t; /* set ID type stored internally in hash table based filter */

/** Invalid set ID used to mean no match found. */
#define FILTER_NO_MATCH 0
/** Maximum size of hash table that can be created. */
#define FILTER_ENTRIES_MAX (1 << 20)  // 1 Million
/** Maximum number of keys that can be looked up as a bulk */
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
  CuckooHT,     // Cuckoo Hash Table, No deletes allowed (corresponds to
                // CuckooHTFilter class)
  CuckooCache,  // Cuckoo Hash Table, Deletes are allowed with overwriting
                // (corresponds to CuckooCacheFilter class)
  VBF           // Vector Bloom Filter (corresponds to VBF_Filter class)
};

/**
 * Parameter struct used to create filter instances.
 */
struct FilterParameters {
  const char *name;         /** Name of the filter. */
  enum FilterEngine engine; /** Type of the filter. */
  uint32_t num_keys;        /** Expected number of keys (e.g., for sizing). */
  uint32_t key_len;         /** Length of key for hash calculation. */
  std::optional<uint32_t> prim_hash_seed; /** Primary hash function seed. */
  std::optional<uint32_t> sec_hash_seed;  /** Secondary hash function seed. */
  uint32_t extra_flag;                    /** Extra flags. */

  // Constructor for convenience
  FilterParameters(const char *n = nullptr, FilterEngine e = CuckooHT,
                   uint32_t nk = static_cast<uint32_t>(std::pow(2, 17)),
                   uint32_t kl = 16, std::optional<uint32_t> phs = nullopt,
                   std::optional<uint32_t> shs = nullopt, uint32_t ef = 0)
      : name(n),
        engine(e),
        num_keys(nk),
        key_len(kl),
        prim_hash_seed(phs),
        sec_hash_seed(shs),
        extra_flag(ef) {}
};

// Forward Declaration of Filter class, needed for UniqueFilterPtr
class Filter;

// Helper struct for std::unique_ptr to manage VCL::Filter objects.
// This deleter will call the C++ `delete` operator, which correctly invokes
// the virtual destructor of the `Filter` class (and subsequently, the derived
// class).
struct FilterDeleter {
  void operator()(VCL::Filter *f) const;
};

// Type alias for our smart pointer that correctly calls the C++ delete
using UniqueFilterPtr = std::unique_ptr<VCL::Filter, FilterDeleter>;

/**
 * @class Filter
 * @brief Abstract base class for all filter implementations.
 *
 * Defines the common interface (pure virtual functions) for filter operations
 * (lookup, add, delete, reset). Specific filter types (e.g., Cuckoo, VBF)
 * will derive from this class and provide concrete implementations.
 */
class Filter {
 protected:
  char name_[FILTER_NAMESIZE];  // Name of this filter instance
  enum FilterEngine engine_;    // Type of the filter
  uint32_t num_keys_;           // expected number of keys
  uint32_t key_len_;            // Length of key used for hash calculation
  uint32_t prim_hash_seed_;     // Primary hash function seed
  uint32_t sec_hash_seed_;      // Secondary hash function seed
  uint32_t extra_flag_;         // Extra flags if needed
  bool _valid = false;          // internal filter valid state

  /**
   * @brief Calculates a hash for the given key, its length, and a seed.
   * @param key Pointer to the key data.
   * @param key_len The length of the key data in bytes.
   * @param seed The seed for the CRC32 computation.
   * @return The computed 32-bit hash value.
   */
  inline uint32_t calculate_hash(const void *key, uint32_t key_len,
                                 uint32_t seed) const {
    return crc32(seed, static_cast<const Bytef *>(key), key_len);
  }

  /**
   * @brief Generates a 16-bit signature from a key and its length.
   * @param key Pointer to the key data.
   * @param key_len The length of the key data in bytes.
   * @return The generated 16-bit signature (filter_sig_t).
   */
  inline filter_sig_t generate_signature(const void *key,
                                         uint32_t key_len) const {
    // Use the primary hash seed to generate a hash, then truncate for signature
    uint32_t hash_val = calculate_hash(key, key_len, prim_hash_seed_);
    return static_cast<filter_sig_t>(hash_val & 0xFFFF);  // Use lower 16 bits
  }

 public:
  // constructor
  Filter(const FilterParameters &params);

  // Virtual destructor to ensure proper cleanup
  virtual ~Filter();

  // return valid bit for filter
  inline bool is_valid() const { return _valid; };

  // --- virtual functions define the common filter interface ---
  // Derived classes MUST implement these.

  /**
   * @brief Lookup key in filter. Single key lookup.
   * @param key Pointer to the key to be looked up.
   * @param set_id Output: set id matches the key.
   * @return 1 for found a match, 0 for not found.
   */
  virtual int lookup(const void *key, filter_set_t *set_id) const = 0;

  /**
   * @brief Lookup bulk of keys in filter.
   * @param keys Pointer to array of keys.
   * @param num_keys Number of keys.
   * @param set_ids Output: array to store set IDs for all keys.
   * @return The number of keys that found a match.
   */
  virtual int lookup_bulk(const void **keys, uint32_t num_keys,
                          filter_set_t *set_ids) const = 0;

  /**
   * @brief Lookup a key for multiple matches.
   * @param key Pointer to the key.
   * @param max_match_per_key User specified maximum number of matches.
   * @param set_id Output: array to store set IDs for all matches of the key.
   * @return The number of matches found for the key.
   */
  virtual int lookup_multi(const void *key, uint32_t max_match_per_key,
                           filter_set_t *set_id) const = 0;

  /**
   * @brief Lookup a bulk of keys for multiple matches each key.
   * @param keys Pointer to array of keys.
   * @param num_keys Number of keys.
   * @param max_match_per_key The possible maximum number of matches for each
   * key.
   * @param match_count Output: array storing number of matches for each key.
   * @param set_ids Output: 2D array to store set IDs
   * (set_ids[key_idx][match_idx]).
   * @return The number of keys that found one or more matches.
   */
  virtual int lookup_multi_bulk(const void **keys, uint32_t num_keys,
                                uint32_t max_match_per_key,
                                uint32_t *match_count,
                                filter_set_t *set_ids) const = 0;

  /**
   * @brief Insert key into filter.
   * @param key Pointer to the key to be added.
   * @param set_id The set ID associated with the key. (0 is reserved for
   * NO_MATCH).
   * @return Status code (0 for success, negative for error, 1 for
   * eviction/cuckoo move).
   */
  virtual int add(const void *key, filter_set_t set_id) = 0;

  /**
   * @brief Reset the filter tables (e.g., clear all entries).
   */
  virtual void reset() = 0;

  /**
   * @brief Delete items from the filter.
   * @param key Pointer to the key to be deleted.
   * @param set_id The set ID corresponding to the key to be deleted.
   * @return 0 for success, negative for error (e.g., -ENOENT if not found).
   */
  virtual int delete_key(const void *key, filter_set_t set_id) = 0;

  // --- Accessor methods ---
  const char *get_name() const { return name_; }
  FilterEngine get_engine_type() const { return engine_; }
  uint32_t get_key_len() const { return key_len_; }
  uint32_t get_num_keys() const { return num_keys_; }
  uint32_t get_ef() const { return extra_flag_; }

  /**
   * @brief Lists the names of all currently managed filters.
   * @return A vector of strings, where each string is the name of an active
   * filter.
   * * Returns an empty vector if no filters are currently defined.
   * */
  std::vector<std::string> filter_list_all_names();

  // --- create instances of derived classes based on FilterParameters::engine
  // ---
  static UniqueFilterPtr create_filter_instance(const FilterParameters *params);
};

// =============================================================
// Internal Filter Collection Management Class
// This class is responsible for owning and managing the lifecycle
// of Filter objects. It acts as a registry for all active filters.
// =============================================================
class FilterCollectionManager {
 public:
  /**
   * @brief Adds a filter to the collection. Takes ownership of the unique_ptr.
   * @param filter_ptr A std::unique_ptr to the filter object. Ownership is
   * transferred.
   * @return true if added successfully, false if a filter with the same name
   * already exists or pointer is null.
   */
  bool collection_add_filter(UniqueFilterPtr filter_ptr);

  /**
   * @brief Retrieves a raw pointer to an existing filter by name. Does not
   * transfer ownership.
   * @warning The caller of this function should NOT delete the returned
   * pointer.
   * @param name The name of the filter to retrieve.
   * @return Pointer to the filter, or nullptr if not found.
   */
  Filter *collection_get_filter(const std::string &name);

  /**
   * @brief Removes a filter from the collection and triggers its deallocation.
   * When an entry is erased from the map, the UniqueFilterPtr's destructor
   * is called, which in turn invokes our FilterDeleter.
   * @param name The name of the filter to remove.
   * @return true if the filter was found and removed, false otherwise.
   */
  bool collection_remove_filter(const std::string &name);

  /**
   * @brief Gets a list of names of all filters currently in the collection.
   * @return A std::vector of strings, where each string is the name of a
   * filter. Returns an empty vector if no filters are present.
   */
  std::vector<std::string> collection_get_all_filter_names() const;

  // Default constructor and destructor are sufficient as unique_ptr
  // handles its own memory management.
  FilterCollectionManager() = default;
  ~FilterCollectionManager() = default;

  // Delete copy constructor and assignment operator to prevent accidental
  // copies which would violate unique_ptr's exclusive ownership.
  FilterCollectionManager(const FilterCollectionManager &) = delete;
  FilterCollectionManager &operator=(const FilterCollectionManager &) = delete;

 private:
  // The core data structure for the collection.
  // Maps a filter's name (std::string) to a UniqueFilterPtr that owns the
  // filter.
  std::unordered_map<std::string, UniqueFilterPtr> filters_;
};

// Global accessor for the single instance of FilterCollectionManager.
FilterCollectionManager &get_global_filter_manager();

// --- Public API Filter external interface ---

/**
 * @brief Find an existing filter and return a pointer to it.
 * @param name Name of the filter.
 * @return Pointer to the filter or NULL if object not found.
 */
VCL::Filter *filter_find_existing(const char *name);

/**
 * @brief Create filter of certain type.
 * @param params Parameters to initialize the filter.
 * @return Return the pointer to the filter. Return value is NULL if the
 * creation failed.
 */
VCL::Filter *filter_create(const VCL::FilterParameters *params);

/**
 * @brief De-allocate memory used by filter.
 * @param filter Pointer to the filter data structure. If filter is NULL, no
 * operation is performed.
 */
void filter_free(VCL::Filter *filter);

// Lookup Functions (external API wrappers)
int filter_lookup(const VCL::Filter *filter, const void *key,
                  filter_set_t *set_id);
int filter_lookup_bulk(const VCL::Filter *filter, const void **keys,
                       uint32_t num_keys, filter_set_t *set_ids);
int filter_lookup_multi(const VCL::Filter *filter, const void *key,
                        uint32_t max_match_per_key, filter_set_t *set_id);
int filter_lookup_multi_bulk(const VCL::Filter *filter, const void **keys,
                             uint32_t num_keys, uint32_t max_match_per_key,
                             uint32_t *match_count, filter_set_t *set_ids);

// Insert Function (external API wrapper)
int filter_add(VCL::Filter *filter, const void *key, filter_set_t set_id);

// Reset Function (external API wrapper)
void filter_reset(VCL::Filter *filter);

// Delete Function (external API wrapper)
int filter_delete_key(VCL::Filter *filter, const void *key,
                      filter_set_t set_id);

// List all filters from filter manager (external API wrapper0
std::vector<std::string> filter_list_all_names();

// Some Bit Manipulation Helper Functions
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

static inline bool is_power_of_2(uint32_t n) {
  return n > 0 && (n & (n - 1)) == 0;
}

}  // namespace VCL

#endif  // FILTER_H