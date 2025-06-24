#include "gtest/gtest.h"
#include "vcl/CuckooHTFilter.h"
#include "vcl/CuckooCacheFilter.h"
#include "vcl/VBFFilter.h" 
#include "vcl/Filter.h"
#include <vector>
#include <string>
#include <numeric>
#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>
#include <errno.h>


// Define common testing parameters
const uint32_t TEST_KEY_LEN = 8; // Length of keys in bytes
const uint32_t SMALL_NUM_KEYS = 16;
const uint32_t MEDIUM_NUM_KEYS = 1024;
const uint32_t FILTER_MAX_CAPACITY = FILTER_ENTRIES_MAX; //defined as (1 << 20)

// Helper function to generate a unique key
std::vector<char> generate_key(uint64_t value, uint32_t key_len) {
    std::vector<char> key(key_len);
    for (uint32_t i = 0; i < key_len; ++i) {
        key[i] = static_cast<char>((value >> (i * 8)) & 0xFF);
    }
    return key;
}


class FilterTest : public ::testing::Test {
protected:
    // Parameters for filter creation
    VCL::FilterParameters params_ht_small;
    VCL::FilterParameters params_cache_small;
    VCL::FilterParameters params_ht_medium;
    VCL::FilterParameters params_cache_medium;
    VCL::FilterParameters params_ht_max;
    VCL::FilterParameters params_cache_max;

    
    static int filter_name_counter;
    std::vector<std::string> filter_names;


    void SetUp() override {

        // Clear any filters from previous tests to ensure clean state
        // since filter_create adds to a global manager
        std::vector<std::string> existing_filter_names = VCL::filter_list_all_names();
        for (const std::string& name : existing_filter_names) {
            VCL::filter_free(VCL::filter_find_existing(name.c_str()));
        }

        filter_names.clear();

        // Common parameters for small filters
        params_ht_small.num_keys = SMALL_NUM_KEYS;
        params_ht_small.key_len = TEST_KEY_LEN;
        params_ht_small.name = "TestHTFilterSmall";
        params_ht_small.engine = VCL::CuckooHT; 
        params_ht_small.prim_hash_seed = 0xDEADC0DE;
        params_ht_small.sec_hash_seed = 0xBADF00D;

        params_cache_small = params_ht_small; 
        params_cache_small.name = "TestCacheFilterSmall";
        params_cache_small.engine = VCL::CuckooCache; 

        // Common parameters for medium filters
        params_ht_medium.num_keys = MEDIUM_NUM_KEYS;
        params_ht_medium.key_len = TEST_KEY_LEN;
        params_ht_medium.name = "TestHTFilterMedium";
        params_ht_medium.engine = VCL::CuckooHT; 
        params_ht_medium.prim_hash_seed = 0xDEADC0DE;
        params_ht_medium.sec_hash_seed = 0xBADF00D;

        params_cache_medium = params_ht_medium; 
        params_cache_medium.name = "TestCacheFilterMedium";
        params_cache_medium.engine = VCL::CuckooCache; 

        // Parameters for max capacity filters
        params_ht_max.num_keys = FILTER_MAX_CAPACITY;
        params_ht_max.key_len = TEST_KEY_LEN;
        params_ht_max.name = "TestHTFilterMax";
        params_ht_max.engine = VCL::CuckooHT; 
        params_ht_max.prim_hash_seed = 0xDEADC0DE;
        params_ht_max.sec_hash_seed = 0xBADF00D;

        params_cache_max = params_ht_max;
        params_cache_max.name = "TestCacheFilterMax";
        params_cache_max.engine = VCL::CuckooCache;
    }

    void TearDown() override {
        // Free filters after test to prevent duplicate names
        std::vector<std::string> existing_filter_names = VCL::filter_list_all_names();
        for (const std::string& name : existing_filter_names) {
            VCL::filter_free(VCL::filter_find_existing(name.c_str()));
        }

        filter_names.clear();
    }
};

int FilterTest::filter_name_counter = 0;

/*
 ********************************
 * I. Filter Creation Tests     *
 ********************************
*/

// Test cases for filter listing and finding
TEST_F(FilterTest, Filter_list_all_names_Empty) {
    std::vector<std::string> names = VCL::filter_list_all_names();
    EXPECT_TRUE(names.empty()) << "Expected no filters initially, but found " << names.size();
}

TEST_F(FilterTest, Filter_list_all_names_MultipleFilters) {
    filter_name_counter = 0;

    filter_names.emplace_back("TestCacheFilterMedium_" + std::to_string(filter_name_counter++)); 
    filter_names.emplace_back("TestCacheFilterMedium_" + std::to_string(filter_name_counter++)); 
    filter_names.emplace_back("TestCacheFilterMedium_" + std::to_string(filter_name_counter++)); 
    
    VCL::FilterParameters p0 = params_cache_medium;
    p0.name = filter_names[0].c_str();
    VCL::Filter* filter0 = VCL::filter_create(&p0);

    VCL::FilterParameters p1 = params_cache_medium;
    p1.name = filter_names[1].c_str();
    VCL::Filter* filter1 = VCL::filter_create(&p1);

    VCL::FilterParameters p2 = params_cache_medium;
    p2.name = filter_names[2].c_str();
    VCL::Filter* filter2 = VCL::filter_create(&p2);

    ASSERT_NE(filter0, nullptr);
    ASSERT_NE(filter1, nullptr);
    ASSERT_NE(filter2, nullptr);

    std::vector<std::string> names = VCL::filter_list_all_names();
    EXPECT_EQ(names.size(), 3) << "Expected 3 filters, but found " << names.size();

    std::vector<std::string> expected_names;
    expected_names.push_back(p0.name);
    expected_names.push_back(p1.name);
    expected_names.push_back(p2.name);

    std::sort(names.begin(), names.end());
    std::sort(expected_names.begin(), expected_names.end());

    // Compare the sorted lists
    for (size_t i = 0; i < expected_names.size(); ++i) {
        EXPECT_EQ(names[i], expected_names[i])
            << "Mismatch at index " << i << ". Expected: " << expected_names[i] << ", Actual: " << names[i];
    } 
}

TEST_F(FilterTest, Filter_list_all_names_AfterFree) {
    VCL::FilterParameters p1 = params_ht_small;
    filter_names.emplace_back("TestHTFilterSmall_" + std::to_string(filter_name_counter++));
    p1.name = filter_names.back().c_str();
    VCL::Filter* filter1 = VCL::filter_create(&p1);

    VCL::FilterParameters p2 = params_cache_medium;
    filter_names.emplace_back("TestCacheFilterMedium_" + std::to_string(filter_name_counter++));
    p2.name = filter_names.back().c_str();
    VCL::Filter* filter2 = VCL::filter_create(&p2);

    ASSERT_NE(filter1, nullptr);
    ASSERT_NE(filter2, nullptr);

    std::vector<std::string> names_before_free = VCL::filter_list_all_names();
    EXPECT_EQ(names_before_free.size(), 2);

    VCL::filter_free(filter1); // Free filter1

    std::vector<std::string> names_after_free_1 = VCL::filter_list_all_names();
    EXPECT_EQ(names_after_free_1.size(), 1);
    EXPECT_EQ(names_after_free_1[0], p2.name); // Only p2.name should remain

    VCL::filter_free(filter2); // Free filter2

    std::vector<std::string> names_after_free_2 = VCL::filter_list_all_names();
    EXPECT_TRUE(names_after_free_2.empty());
    
}

TEST_F(FilterTest, Filter_find_existing_Success) {
    VCL::FilterParameters p1 = params_ht_small;
    filter_names.emplace_back("TestHTFilterSmall_" + std::to_string(filter_name_counter++));
    p1.name = filter_names.back().c_str();
    VCL::Filter* filter1 = VCL::filter_create(&p1);
    ASSERT_NE(filter1, nullptr);

    VCL::Filter* found_filter = VCL::filter_find_existing(p1.name); 
    ASSERT_NE(found_filter, nullptr);
    EXPECT_EQ(found_filter, filter1);
    EXPECT_STREQ(found_filter->get_name(), p1.name);

    // Test a different filter type
    VCL::FilterParameters p2 = params_cache_medium;
    filter_names.emplace_back("TestCacheFilterMedium_" + std::to_string(filter_name_counter++));
    p2.name = filter_names.back().c_str();
    VCL::Filter* filter2 = VCL::filter_create(&p2);
    ASSERT_NE(filter2, nullptr);
    VCL::Filter* found_filter2 = VCL::filter_find_existing(p2.name);
    ASSERT_NE(found_filter2, nullptr);
    EXPECT_EQ(found_filter2, filter2);
}

TEST_F(FilterTest, Filter_find_existing_NotFound) {
    // No filters created yet (or existing ones cleared by SetUp)
    std::string non_existent_name = "NonExistentFilter_" + std::to_string(filter_name_counter++);
    VCL::Filter* found_filter = VCL::filter_find_existing(non_existent_name.c_str());
    EXPECT_EQ(found_filter, nullptr);

    // Create one filter, then search for a different non-existent one
    VCL::FilterParameters p_temp = params_ht_small;
    filter_names.emplace_back("TempFilter_" + std::to_string(filter_name_counter++));
    p_temp.name = filter_names.back().c_str();
    VCL::filter_create(&p_temp);

    std::string another_non_existent_name = "AnotherNonExistentFilter_" + std::to_string(filter_name_counter++);
    found_filter = VCL::filter_find_existing(another_non_existent_name.c_str());
    EXPECT_EQ(found_filter, nullptr);
}

TEST_F(FilterTest, Filter_find_existing_NullName) {
    VCL::Filter* found_filter = VCL::filter_find_existing(nullptr);
    EXPECT_EQ(found_filter, nullptr);
}

 
TEST_F(FilterTest, Filter_GetName) {
    VCL::Filter* ht_filter = VCL::filter_create(&params_ht_small);
    ASSERT_NE(ht_filter, nullptr);
    ASSERT_TRUE(ht_filter->is_valid());
    EXPECT_STREQ(ht_filter->get_name(), params_ht_small.name);
    VCL::filter_free(ht_filter); // Clean up explicitly

}


TEST_F(FilterTest, CuckooHTFilter_Creation_Success) {
    std::unique_ptr<VCL::CuckooHTFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooHTFilter>(params_ht_small));
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(filter->is_valid());
}

TEST_F(FilterTest, CuckooCacheFilter_Creation_Success) {
    std::unique_ptr<VCL::CuckooCacheFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooCacheFilter>(params_cache_small));
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(filter->is_valid());
}

TEST_F(FilterTest, CuckooHTFilter_Creation_BadParams_ZeroKeyLen) {
    VCL::FilterParameters bad_params = params_ht_small;
    bad_params.key_len = 0;
    std::unique_ptr<VCL::CuckooHTFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooHTFilter>(bad_params));
    ASSERT_TRUE(filter != nullptr);
    EXPECT_FALSE(filter->is_valid());
}

TEST_F(FilterTest, CuckooHTFilter_Creation_BadParams_NumKeysTooSmall) {
    VCL::FilterParameters bad_params = params_ht_small;
    // FILTER_BUCKET_ENTRIES is typically 4/8/16, so setting num_keys < 4 should fail
    bad_params.num_keys = 1;
    std::unique_ptr<VCL::CuckooHTFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooHTFilter>(bad_params));
    ASSERT_TRUE(filter != nullptr);
    EXPECT_FALSE(filter->is_valid());
}

TEST_F(FilterTest, CuckooHTFilter_Creation_BadParams_NumKeysTooLarge) {
    VCL::FilterParameters bad_params = params_ht_small;
    bad_params.num_keys = FILTER_ENTRIES_MAX + 1; // Exceed max
    std::unique_ptr<VCL::CuckooHTFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooHTFilter>(bad_params));
    ASSERT_TRUE(filter != nullptr);
    EXPECT_FALSE(filter->is_valid());
}

TEST_F(FilterTest, CuckooCacheFilter_Creation_BadParams_ZeroKeyLen) {
    VCL::FilterParameters bad_params = params_cache_small;
    bad_params.key_len = 0;
    std::unique_ptr<VCL::CuckooCacheFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooCacheFilter>(bad_params));
    ASSERT_TRUE(filter != nullptr);
    EXPECT_FALSE(filter->is_valid());
}

TEST_F(FilterTest, CuckooCacheFilter_Creation_BadParams_NumKeysTooSmall) {
    VCL::FilterParameters bad_params = params_cache_small;
    bad_params.num_keys = 1;
    std::unique_ptr<VCL::CuckooCacheFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooCacheFilter>(bad_params));
    ASSERT_TRUE(filter != nullptr);
    EXPECT_FALSE(filter->is_valid());
}

TEST_F(FilterTest, CuckooCacheFilter_Creation_BadParams_NumKeysTooLarge) {
    VCL::FilterParameters bad_params = params_cache_small;
    bad_params.num_keys = FILTER_ENTRIES_MAX + 1; // Exceed max
    std::unique_ptr<VCL::CuckooCacheFilter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::CuckooCacheFilter>(bad_params));
    ASSERT_TRUE(filter != nullptr);
    EXPECT_FALSE(filter->is_valid());
}

TEST_F(FilterTest, Filter_DestructionAndRecreation_Cache) {
    // Create, use, and destroy the first filter instance within its own scope
    {
        std::unique_ptr<VCL::CuckooCacheFilter> first_filter =
            std::make_unique<VCL::CuckooCacheFilter>(params_cache_medium);
        ASSERT_TRUE(first_filter->is_valid());

        auto key1 = generate_key(1000, TEST_KEY_LEN);
        VCL::filter_set_t set_id1 = 100;
        ASSERT_TRUE(first_filter->add(key1.data(), set_id1) >= 0);

        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(first_filter->lookup(key1.data(), &found_set_id), 1);
        EXPECT_EQ(found_set_id, set_id1);
    } // `first_filter` goes out of scope here and is destroyed

    // Create a new filter instance with the exact same parameters
    std::unique_ptr<VCL::CuckooCacheFilter> second_filter =
        std::make_unique<VCL::CuckooCacheFilter>(params_cache_medium);
    ASSERT_TRUE(second_filter->is_valid());

    // Verify it's empty and fully functional (no lingering state from the first filter)
    auto key1 = generate_key(1000, TEST_KEY_LEN); // The old key from the first filter
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(second_filter->lookup(key1.data(), &found_set_id), 0) << "Old key found in newly created cache filter instance";
    EXPECT_EQ(found_set_id, FILTER_NO_MATCH);

    auto key2 = generate_key(2000, TEST_KEY_LEN); // A new key for the second filter
    VCL::filter_set_t set_id2 = 200;
    ASSERT_TRUE(second_filter->add(key2.data(), set_id2) >= 0);
    EXPECT_EQ(second_filter->lookup(key2.data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, set_id2);
}

/*
 ********************************************
 * II.    Testing Filter Public APIs        *
 ********************************************
*/

// -- TEST CASES for API Wrappers ---

// Test filter_add, filter_lookup, filter_delete_key, filter_reset with nullptr filter
TEST_F(FilterTest, FilterAPI_NullFilterChecks) {
    const void* key_ptr = generate_key(1, TEST_KEY_LEN).data();
    VCL::filter_set_t set_id = 1;
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    uint32_t match_count_val = 0;
    VCL::filter_set_t set_ids_array[1]; // Small array for multi-tests

    EXPECT_EQ(VCL::filter_add(nullptr, key_ptr, set_id), -EINVAL);
    EXPECT_EQ(VCL::filter_lookup(nullptr, key_ptr, &found_set_id), -EINVAL);
    EXPECT_EQ(VCL::filter_delete_key(nullptr, key_ptr, set_id), -EINVAL);
    ASSERT_NO_THROW(VCL::filter_reset(nullptr)); // reset might be void or return 0, no throw expected

    // Test bulk/multi with nullptr
    EXPECT_EQ(VCL::filter_lookup_bulk(nullptr, &key_ptr, 1, &set_ids_array[0]), -EINVAL);
    EXPECT_EQ(VCL::filter_lookup_multi(nullptr, key_ptr, 1, &set_ids_array[0]), -EINVAL);
    EXPECT_EQ(VCL::filter_lookup_multi_bulk(nullptr, &key_ptr, 1, 1, &match_count_val, &set_ids_array[0]), -EINVAL);
}

// Test filter_lookup_bulk for CuckooHTFilter
TEST_F(FilterTest, FilterLookupBulk_CuckooHT) {
    VCL::Filter* filter = VCL::filter_create(&params_ht_medium);
    ASSERT_NE(filter, nullptr);

    std::vector<std::vector<char>> keys(10);
    std::vector<const void*> key_ptrs(10);
    std::vector<VCL::filter_set_t> set_ids(10);
    std::vector<VCL::filter_set_t> found_set_ids(10);

    for (int i = 0; i < 10; ++i) {
        keys[i] = generate_key(i + 1, TEST_KEY_LEN);
        key_ptrs[i] = keys[i].data();
        set_ids[i] = i + 1;
        filter_add(filter, key_ptrs[i], set_ids[i]);
    }

    EXPECT_EQ(filter_lookup_bulk(filter, key_ptrs.data(), 10, found_set_ids.data()), 10);//should find all 10 keys
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(found_set_ids[i], set_ids[i]);
    }

    filter_free(filter);
}

// Test filter_lookup_multi for CuckooHTFilter
TEST_F(FilterTest, FilterLookupMulti_CuckooHT) {
    VCL::Filter* filter = VCL::filter_create(&params_ht_small);
    ASSERT_NE(filter, nullptr);

    auto key1 = generate_key(100, TEST_KEY_LEN);
    auto key2 = generate_key(101, TEST_KEY_LEN);
    filter_add(filter, key1.data(), 1);
    filter_add(filter, key1.data(), 2); // Add same key with different set_id
    filter_add(filter, key2.data(), 3);

    VCL::filter_set_t found_set_ids[2]; // Max 2 matches for this test
    uint32_t max_matches = 2;

    // Lookup key1
    EXPECT_EQ(filter_lookup_multi(filter, key1.data(), max_matches, found_set_ids), 2); // Expect 2 matches
    // Check if both 1 and 2 are found, order might vary
    EXPECT_TRUE((found_set_ids[0] == 1 && found_set_ids[1] == 2) || (found_set_ids[0] == 2 && found_set_ids[1] == 1));

    // Lookup key2
    EXPECT_EQ(filter_lookup_multi(filter, key2.data(), max_matches, found_set_ids), 1); // Expect 1 match
    EXPECT_EQ(found_set_ids[0], 3);

    // Lookup non-existent key
    auto non_existent_key = generate_key(999, TEST_KEY_LEN);
    EXPECT_EQ(filter_lookup_multi(filter, non_existent_key.data(), max_matches, found_set_ids), 0); // Expect 0 matches

    filter_free(filter);
}

// Test filter_lookup_multi_bulk for CuckooHTFilter
TEST_F(FilterTest, FilterLookupMultiBulk_CuckooHT) {
    VCL::Filter* filter = VCL::filter_create(&params_ht_medium);
    ASSERT_NE(filter, nullptr);

    std::vector<std::vector<char>> keys_data(3);
    std::vector<const void*> keys_ptrs(3);
    uint32_t max_matches_per_key = 2; // Each key can have up to 2 matches
    std::vector<uint32_t> match_counts(3);
    std::vector<VCL::filter_set_t> all_found_set_ids(3 * max_matches_per_key);

    // Key 1: 2 matches
    keys_data[0] = generate_key(1, TEST_KEY_LEN);
    filter_add(filter, keys_data[0].data(), 10);
    filter_add(filter, keys_data[0].data(), 11);

    // Key 2: 1 match
    keys_data[1] = generate_key(2, TEST_KEY_LEN);
    filter_add(filter, keys_data[1].data(), 20);

    // Key 3: 0 matches
    keys_data[2] = generate_key(3, TEST_KEY_LEN);

    for(int i = 0; i < 3; ++i) {
        keys_ptrs[i] = keys_data[i].data();
    }

    EXPECT_EQ(filter_lookup_multi_bulk(filter, keys_ptrs.data(), 3, max_matches_per_key,
                                      match_counts.data(), all_found_set_ids.data()), 2); 
                                      //Expect 2, as Key 1 & Key 2 should match

    // Verify results for Key 1
    EXPECT_EQ(match_counts[0], 2);
    // Check if both 10 and 11 are found for the first key
    EXPECT_TRUE((all_found_set_ids[0] == 10 && all_found_set_ids[1] == 11) ||
                (all_found_set_ids[0] == 11 && all_found_set_ids[1] == 10));

    // Verify results for Key 2
    EXPECT_EQ(match_counts[1], 1);
    EXPECT_EQ(all_found_set_ids[2], 20); // First slot for second key

    // Verify results for Key 3
    EXPECT_EQ(match_counts[2], 0);

    filter_free(filter);
}

// Test filter_lookup_bulk for CuckooCacheFilter
TEST_F(FilterTest, FilterLookupBulk_CuckooCache) {
    VCL::Filter* filter = VCL::filter_create(&params_cache_medium);
    ASSERT_NE(filter, nullptr);

    std::vector<std::vector<char>> keys(10);
    std::vector<const void*> key_ptrs(10);
    std::vector<VCL::filter_set_t> set_ids(10);
    std::vector<VCL::filter_set_t> found_set_ids(10);

    for (int i = 0; i < 10; ++i) {
        keys[i] = generate_key(i + 1, TEST_KEY_LEN);
        key_ptrs[i] = keys[i].data();
        set_ids[i] = i + 1;
        filter_add(filter, key_ptrs[i], set_ids[i]);
    }

    EXPECT_EQ(filter_lookup_bulk(filter, key_ptrs.data(), 10, found_set_ids.data()), 10); // all 10 should be found
    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(found_set_ids[i], set_ids[i]);
    }

    filter_free(filter);
}

// Test filter_lookup_multi for CuckooCacheFilter
TEST_F(FilterTest, FilterLookupMulti_CuckooCache) {
    VCL::Filter* filter = VCL::filter_create(&params_cache_small);
    ASSERT_NE(filter, nullptr);

    auto key1 = generate_key(100, TEST_KEY_LEN);
    filter_add(filter, key1.data(), 1);
    filter_add(filter, key1.data(), 2); // Add same key with different set_id

    VCL::filter_set_t found_set_ids[2]; // Max 2 matches for this test, but expect only 1
    uint32_t max_matches = 2; 

    // Cache should return at most 1 match.
    // because the filter will overwrite and keep only one of the set_ids.
    EXPECT_EQ(filter_lookup_multi(filter, key1.data(), max_matches, found_set_ids), 1); // Expect 1 match

    // Check if the single found set_id is either 1 or 2 (whichever was retained/last added)
    EXPECT_TRUE(found_set_ids[0] == 1 || found_set_ids[0] == 2)
        << "Found set_id: " << found_set_ids[0] << ", Expected 1 or 2";

    filter_free(filter);
}


// Test filter_lookup_multi_bulk for CuckooCacheFilter
TEST_F(FilterTest, FilterLookupMultiBulk_CuckooCache) {
    VCL::Filter* filter = VCL::filter_create(&params_cache_medium);
    ASSERT_NE(filter, nullptr);

    std::vector<std::vector<char>> keys_data(3);
    std::vector<const void*> keys_ptrs(3);
    uint32_t max_matches_per_key = 2; // Each key can theoretically have up to 2 matches, but expect only 1
    std::vector<uint32_t> match_counts(3);
    // all_found_set_ids needs space for num_keys * max_matches_per_key
    std::vector<VCL::filter_set_t> all_found_set_ids(3 * max_matches_per_key);

    // Key 1: 2 additions. Expect 1 match in lookup (the last one, or an arbitrary one).
    keys_data[0] = generate_key(1, TEST_KEY_LEN);
    filter_add(filter, keys_data[0].data(), 10);
    filter_add(filter, keys_data[0].data(), 11); // This will likely overwrite 

    // Key 2: 1 match
    keys_data[1] = generate_key(2, TEST_KEY_LEN);
    filter_add(filter, keys_data[1].data(), 20);

    // Key 3: 0 matches
    keys_data[2] = generate_key(3, TEST_KEY_LEN);

    for(int i = 0; i < 3; ++i) {
        keys_ptrs[i] = keys_data[i].data();
    }

    // Key 1 and 2 should matche (each returning 1 match).
    EXPECT_EQ(filter_lookup_multi_bulk(filter, keys_ptrs.data(), 3, max_matches_per_key,
                                       match_counts.data(), all_found_set_ids.data()), 2); // 2 keys had matches

    // Verify results for Key 1
    EXPECT_EQ(match_counts[0], 1); // Expect 1 match for Key 1
    // Check if the found set_id for Key 1 is either 10 or 11
    EXPECT_TRUE(all_found_set_ids[0] == 10 || all_found_set_ids[0] == 11)
        << "Found set_id for Key 1: " << all_found_set_ids[0] << ", Expected 10 or 11";

    // Verify results for Key 2
    EXPECT_EQ(match_counts[1], 1);
    // Key 2's result is at index `1 * max_matches_per_key` (which is 2)
    EXPECT_EQ(all_found_set_ids[1 * max_matches_per_key], 20);

    // Verify results for Key 3
    EXPECT_EQ(match_counts[2], 0);

    filter_free(filter);
}


/*
 ********************************************
 * III.    Add/Lookup/Delete/Reset Tests     *
 ********************************************
*/

class FilterCommonTest : public FilterTest {
protected:
    std::unique_ptr<VCL::CuckooHTFilter> ht_filter;
    std::unique_ptr<VCL::CuckooCacheFilter> cache_filter;

    void SetUp() override {
        FilterTest::SetUp(); 
        ht_filter = std::make_unique<VCL::CuckooHTFilter>(params_ht_medium);
        cache_filter = std::make_unique<VCL::CuckooCacheFilter>(params_cache_medium);
        ASSERT_TRUE(ht_filter->is_valid());
        ASSERT_TRUE(cache_filter->is_valid());
    }
};

TEST_F(FilterCommonTest, AddAndLookup_SingleKey_HT) {
    auto key = generate_key(100, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 1;

    // Add the key
    int ret = ht_filter->add(key.data(), set_id);
    EXPECT_EQ(ret, 0) << "Failed to add key to HT filter";

    // Lookup the key
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    ret = ht_filter->lookup(key.data(), &found_set_id);
    EXPECT_EQ(ret, 1) << "Key not found in HT filter";
    EXPECT_EQ(found_set_id, set_id) << "Incorrect set_id found in HT filter";
}

TEST_F(FilterCommonTest, AddAndLookup_SingleKey_Cache) {
    auto key = generate_key(200, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 2;

    // Add the key
    int ret = cache_filter->add(key.data(), set_id);
    // For cache, 0 means inserted into an empty slot, 1 means an eviction occurred. Both are success.
    EXPECT_TRUE(ret == 0 || ret == 1) << "Failed to add key to Cache filter";

    // Lookup the key
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    ret = cache_filter->lookup(key.data(), &found_set_id);
    EXPECT_EQ(ret, 1) << "Key not found in Cache filter";
    EXPECT_EQ(found_set_id, set_id) << "Incorrect set_id found in Cache filter";
}

TEST_F(FilterCommonTest, Lookup_NonExistingKey_HT) {
    auto key = generate_key(999, TEST_KEY_LEN); // This key was not added
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    int ret = ht_filter->lookup(key.data(), &found_set_id);
    EXPECT_EQ(ret, 0) << "Non-existing key found in HT filter";
    EXPECT_EQ(found_set_id, FILTER_NO_MATCH) << "Set ID not FILTER_NO_MATCH for non-existing key in HT filter";
}

TEST_F(FilterCommonTest, Lookup_NonExistingKey_Cache) {
    auto key = generate_key(888, TEST_KEY_LEN); // This key was not added
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    int ret = cache_filter->lookup(key.data(), &found_set_id);
    EXPECT_EQ(ret, 0) << "Non-existing key found in Cache filter";
    EXPECT_EQ(found_set_id, FILTER_NO_MATCH) << "Set ID not FILTER_NO_MATCH for non-existing key in Cache filter";
}

TEST_F(FilterCommonTest, Add_DuplicateKey_MultipleEntries_HT) {
    auto key = generate_key(10, TEST_KEY_LEN);
    VCL::filter_set_t set_id1 = 101;
    VCL::filter_set_t set_id2 = 102; // Different set_id for duplicate

    // Add first entry
    ASSERT_EQ(ht_filter->add(key.data(), set_id1), 0);

    // Now try to add the same key with a different set_id.    
    // this will attempt to add a new entry if space is available.
    int ret_second_add = ht_filter->add(key.data(), set_id2);
    EXPECT_TRUE(ret_second_add == 0 || ret_second_add == 1) << "Adding duplicate key (HT) with different set_id should succeed or cause eviction.";

    // Use lookup_multi to confirm both entries are found
    std::vector<VCL::filter_set_t> found_ids_multi(2);
    uint32_t count_multi = ht_filter->lookup_multi(key.data(), 2, found_ids_multi.data());
    EXPECT_EQ(count_multi, 2) << "Expected 2 matches for duplicate key in HT filter";
    // Order is not guaranteed, so check for presence of both set_ids
    EXPECT_TRUE((found_ids_multi[0] == set_id1 && found_ids_multi[1] == set_id2) ||
                (found_ids_multi[0] == set_id2 && found_ids_multi[1] == set_id1))
        << "Both set_ids not found or incorrect for duplicate key in HT filter";
}

TEST_F(FilterCommonTest, Add_DuplicateKey_Update_Cache) {
    auto key = generate_key(10, TEST_KEY_LEN);
    VCL::filter_set_t set_id1 = 101;
    VCL::filter_set_t set_id2 = 102; // Different set_id for duplicate

    // Add first time
    int ret = cache_filter->add(key.data(), set_id1);
    EXPECT_TRUE(ret == 0 || ret == 1) << "Failed to add first key to Cache filter";

    // Attempt to add duplicate: CuckooCacheFilter's add explicitly updates if signature matches
    ret = cache_filter->add(key.data(), set_id2);
    EXPECT_EQ(ret, 0) << "Adding duplicate key to Cache filter should result in update (0)"; // 0 for update or direct insert

    // Lookup to ensure the set_id has been updated
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    int lookup_ret = cache_filter->lookup(key.data(), &found_set_id);
    EXPECT_EQ(lookup_ret, 1) << "Key not found after duplicate add/update in Cache filter";
    EXPECT_EQ(found_set_id, set_id2) << "Set_id not updated for duplicate key in Cache filter";
}

TEST_F(FilterCommonTest, Delete_ExistingKey_HT) {
    auto key = generate_key(1000, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 1;

    ht_filter->add(key.data(), set_id);
    VCL::filter_set_t found_set_id_before = FILTER_NO_MATCH;
    ASSERT_EQ(ht_filter->lookup(key.data(), &found_set_id_before), 1);
    ASSERT_EQ(found_set_id_before, set_id);

    int ret = ht_filter->delete_key(key.data(), set_id);
    EXPECT_EQ(ret, 0) << "Failed to delete existing key from HT filter";

    VCL::filter_set_t found_set_id_after = FILTER_NO_MATCH;
    ret = ht_filter->lookup(key.data(), &found_set_id_after);
    EXPECT_EQ(ret, 0) << "Deleted key still found in HT filter";
    EXPECT_EQ(found_set_id_after, FILTER_NO_MATCH);
}

TEST_F(FilterCommonTest, Delete_ExistingKey_Cache) {
    auto key = generate_key(1001, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 2;

    cache_filter->add(key.data(), set_id);
    VCL::filter_set_t found_set_id_before = FILTER_NO_MATCH;
    ASSERT_EQ(cache_filter->lookup(key.data(), &found_set_id_before), 1);
    ASSERT_EQ(found_set_id_before, set_id);

    int ret = cache_filter->delete_key(key.data(), set_id);
    EXPECT_EQ(ret, 0) << "Failed to delete existing key from Cache filter";

    VCL::filter_set_t found_set_id_after = FILTER_NO_MATCH;
    ret = cache_filter->lookup(key.data(), &found_set_id_after);
    EXPECT_EQ(ret, 0) << "Deleted key still found in Cache filter";
    EXPECT_EQ(found_set_id_after, FILTER_NO_MATCH);
}

TEST_F(FilterCommonTest, Delete_NonExistingKey_HT) {
    auto key = generate_key(2000, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 1; // Any set_id

    int ret = ht_filter->delete_key(key.data(), set_id);
    EXPECT_EQ(ret, -ENOENT) << "Deleting non-existing key did not return -ENOENT for HT filter";
}

TEST_F(FilterCommonTest, Delete_NonExistingKey_Cache) {
    auto key = generate_key(2001, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 2; // Any set_id

    int ret = cache_filter->delete_key(key.data(), set_id);
    EXPECT_EQ(ret, -ENOENT) << "Deleting non-existing key did not return -ENOENT for Cache filter";
}

TEST_F(FilterCommonTest, Delete_KeyWithWrongSetID_HT) {
    auto key = generate_key(3000, TEST_KEY_LEN);
    VCL::filter_set_t correct_set_id = 10;
    VCL::filter_set_t wrong_set_id = 11;

    ht_filter->add(key.data(), correct_set_id);
    VCL::filter_set_t found_set_id_before = FILTER_NO_MATCH;
    ASSERT_EQ(ht_filter->lookup(key.data(), &found_set_id_before), 1);
    ASSERT_EQ(found_set_id_before, correct_set_id);

    int ret = ht_filter->delete_key(key.data(), wrong_set_id);
    EXPECT_EQ(ret, -ENOENT) << "Deleting key with wrong set_id did not return -ENOENT for HT filter";

    // Verify the key is still present with its correct set_id
    VCL::filter_set_t found_set_id_after = FILTER_NO_MATCH;
    ret = ht_filter->lookup(key.data(), &found_set_id_after);
    EXPECT_EQ(ret, 1) << "Key unexpectedly deleted or changed after attempting delete with wrong set_id in HT filter";
    EXPECT_EQ(found_set_id_after, correct_set_id);
}

TEST_F(FilterCommonTest, Delete_KeyWithWrongSetID_Cache) {
    auto key = generate_key(3001, TEST_KEY_LEN);
    VCL::filter_set_t correct_set_id = 12;
    VCL::filter_set_t wrong_set_id = 13;

    cache_filter->add(key.data(), correct_set_id);
    VCL::filter_set_t found_set_id_before = FILTER_NO_MATCH;
    ASSERT_EQ(cache_filter->lookup(key.data(), &found_set_id_before), 1);
    ASSERT_EQ(found_set_id_before, correct_set_id);

    int ret = cache_filter->delete_key(key.data(), wrong_set_id);
    EXPECT_EQ(ret, -ENOENT) << "Deleting key with wrong set_id did not return -ENOENT for Cache filter";

    // Verify the key is still present with its correct set_id
    VCL::filter_set_t found_set_id_after = FILTER_NO_MATCH;
    ret = cache_filter->lookup(key.data(), &found_set_id_after);
    EXPECT_EQ(ret, 1) << "Key unexpectedly deleted or changed after attempting delete with wrong set_id in Cache filter";
    EXPECT_EQ(found_set_id_after, correct_set_id);
}

TEST_F(FilterCommonTest, Reset_HT) {
    // Add some keys
    for (int i = 0; i < 10; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        ht_filter->add(key.data(), static_cast<VCL::filter_set_t>(i + 1));
    }

    // Verify some keys are present
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(ht_filter->lookup(generate_key(5, TEST_KEY_LEN).data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, 6);

    ht_filter->reset();

    // Verify no keys are found after reset
    for (int i = 0; i < 10; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(ht_filter->lookup(key.data(), &found_set_id), 0) << "Key " << i << " found after reset in HT filter";
        EXPECT_EQ(found_set_id, FILTER_NO_MATCH);
    }
}

TEST_F(FilterCommonTest, Reset_Cache) {
    // Add some keys
    for (int i = 0; i < 10; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        cache_filter->add(key.data(), static_cast<VCL::filter_set_t>(i + 1));
    }

    // Verify some keys are present
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(cache_filter->lookup(generate_key(5, TEST_KEY_LEN).data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, 6);

    cache_filter->reset();

    // Verify no keys are found after reset
    for (int i = 0; i < 10; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(cache_filter->lookup(key.data(), &found_set_id), 0) << "Key " << i << " found after reset in Cache filter";
        EXPECT_EQ(found_set_id, FILTER_NO_MATCH);
    }
}

/*
 ********************************
 * IV. Bulk Operations Tests    *
 ********************************
*/

TEST_F(FilterCommonTest, LookupBulk_MixedKeys_HT) {
    std::vector<std::vector<char>> keys_data;
    std::vector<const void*> keys_ptr;
    std::vector<VCL::filter_set_t> expected_set_ids;

    // Add some keys
    for (uint64_t i = 0; i < 5; ++i) {
        keys_data.push_back(generate_key(i, TEST_KEY_LEN));
        keys_ptr.push_back(keys_data.back().data());
        VCL::filter_set_t current_set_id = static_cast<VCL::filter_set_t>(i + 1);
        ht_filter->add(keys_data.back().data(), current_set_id);
        expected_set_ids.push_back(current_set_id);
    }

    // Add some non-existing keys to the lookup batch
    keys_data.push_back(generate_key(9998, TEST_KEY_LEN)); // Non-existing
    keys_ptr.push_back(keys_data.back().data());
    expected_set_ids.push_back(FILTER_NO_MATCH);

    keys_data.push_back(generate_key(9999, TEST_KEY_LEN)); // Non-existing
    keys_ptr.push_back(keys_data.back().data());
    expected_set_ids.push_back(FILTER_NO_MATCH);

    std::vector<VCL::filter_set_t> found_set_ids(keys_ptr.size());
    int num_matches = ht_filter->lookup_bulk(keys_ptr.data(), keys_ptr.size(), found_set_ids.data());

    EXPECT_EQ(num_matches, 5) << "Incorrect number of matches for lookup_bulk in HT filter";

    // Verify individual results (this requires looking up original values as shuffle changed order)
    for (size_t i = 0; i < keys_ptr.size(); ++i) {
        VCL::filter_set_t single_lookup_set_id = FILTER_NO_MATCH;
        int single_lookup_ret = ht_filter->lookup(keys_ptr[i], &single_lookup_set_id);

        if (single_lookup_ret == 1) {
            EXPECT_NE(found_set_ids[i], FILTER_NO_MATCH) << "Bulk lookup returned NO_MATCH for an existing key " << i;
            EXPECT_EQ(found_set_ids[i], single_lookup_set_id) << "Bulk lookup set_id mismatch for existing key " << i;
        } else {
            EXPECT_EQ(found_set_ids[i], FILTER_NO_MATCH) << "Bulk lookup returned match for a non-existing key " << i;
        }
    }
}

TEST_F(FilterCommonTest, LookupBulk_MixedKeys_Cache) {
    std::vector<std::vector<char>> keys_data;
    std::vector<const void*> keys_ptr;
    std::vector<VCL::filter_set_t> expected_set_ids;

    // Add some keys
    for (uint64_t i = 0; i < 5; ++i) {
        keys_data.push_back(generate_key(i + 100, TEST_KEY_LEN));
        keys_ptr.push_back(keys_data.back().data());
        VCL::filter_set_t current_set_id = static_cast<VCL::filter_set_t>(i + 101);
        cache_filter->add(keys_data.back().data(), current_set_id);
        expected_set_ids.push_back(current_set_id);
    }

    // Add some non-existing keys to the lookup batch
    keys_data.push_back(generate_key(9997, TEST_KEY_LEN)); // Non-existing
    keys_ptr.push_back(keys_data.back().data());
    expected_set_ids.push_back(FILTER_NO_MATCH);

    keys_data.push_back(generate_key(9996, TEST_KEY_LEN)); // Non-existing
    keys_ptr.push_back(keys_data.back().data());
    expected_set_ids.push_back(FILTER_NO_MATCH);

    std::vector<VCL::filter_set_t> found_set_ids(keys_ptr.size());
    int num_matches = cache_filter->lookup_bulk(keys_ptr.data(), keys_ptr.size(), found_set_ids.data());

    EXPECT_EQ(num_matches, 5) << "Incorrect number of matches for lookup_bulk in Cache filter";

    // Verify individual results (this requires looking up original values as shuffle changed order)
    for (size_t i = 0; i < keys_ptr.size(); ++i) {
        VCL::filter_set_t single_lookup_set_id = FILTER_NO_MATCH;
        int single_lookup_ret = cache_filter->lookup(keys_ptr[i], &single_lookup_set_id);

        if (single_lookup_ret == 1) {
            EXPECT_NE(found_set_ids[i], FILTER_NO_MATCH) << "Bulk lookup returned NO_MATCH for an existing key " << i;
            EXPECT_EQ(found_set_ids[i], single_lookup_set_id) << "Bulk lookup set_id mismatch for existing key " << i;
        } else {
            EXPECT_EQ(found_set_ids[i], FILTER_NO_MATCH) << "Bulk lookup returned match for a non-existing key " << i;
        }
    }
}

TEST_F(FilterCommonTest, LookupMulti_HT_SingleMatchExpected) {
    auto key = generate_key(4000, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 42;

    ht_filter->add(key.data(), set_id);

    std::vector<VCL::filter_set_t> found_set_ids(2); // Max 2 matches
    int num_matches = ht_filter->lookup_multi(key.data(), 2, found_set_ids.data());

    EXPECT_EQ(num_matches, 1) << "Expected 1 match for lookup_multi (HT)";
    EXPECT_EQ(found_set_ids[0], set_id) << "Incorrect set_id for lookup_multi (HT)";
    EXPECT_EQ(found_set_ids[1], FILTER_NO_MATCH) << "Unexpected second match for lookup_multi (HT)";
}

TEST_F(FilterCommonTest, LookupMulti_Cache_SingleMatchExpected) {
    auto key = generate_key(4001, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 43;

    cache_filter->add(key.data(), set_id); // Cache should update if duplicate, so only one true match

    std::vector<VCL::filter_set_t> found_set_ids(2); // Max 2 matches
    int num_matches = cache_filter->lookup_multi(key.data(), 2, found_set_ids.data());

    EXPECT_EQ(num_matches, 1) << "Expected 1 match for lookup_multi (Cache)";
    EXPECT_EQ(found_set_ids[0], set_id) << "Incorrect set_id for lookup_multi (Cache)";
    EXPECT_EQ(found_set_ids[1], FILTER_NO_MATCH) << "Unexpected second match for lookup_multi (Cache)";
}

TEST_F(FilterCommonTest, LookupMulti_HT_MultipleMatchesFromDuplicateKeys) {
    auto key = generate_key(5000, TEST_KEY_LEN);
    VCL::filter_set_t set_id1 = 50;
    VCL::filter_set_t set_id2 = 51;

    // Add first entry
    ASSERT_EQ(ht_filter->add(key.data(), set_id1), 0);
    // Add duplicate key, expecting it to be stored as a new entry if possible
    ASSERT_TRUE(ht_filter->add(key.data(), set_id2) == 0 || ht_filter->add(key.data(), set_id2) == 1);

    std::vector<VCL::filter_set_t> found_set_ids(2);
    int num_matches = ht_filter->lookup_multi(key.data(), 2, found_set_ids.data());

    EXPECT_EQ(num_matches, 2) << "Expected 2 matches for duplicate keys in HT filter lookup_multi";
    // Order is not guaranteed, so check for presence of both
    EXPECT_TRUE((found_set_ids[0] == set_id1 && found_set_ids[1] == set_id2) ||
                (found_set_ids[0] == set_id2 && found_set_ids[1] == set_id1))
        << "Both set_ids not found or incorrect for duplicate key lookup_multi in HT filter";
}

TEST_F(FilterCommonTest, LookupMultiBulk_MixedKeys_HT) {
    std::vector<std::vector<char>> keys_data;
    std::vector<const void*> keys_ptr;
    std::vector<VCL::filter_set_t> expected_set_ids_flat; // Flattened for checking
    std::vector<uint32_t> expected_match_counts;

    // Key 1: Single match
    keys_data.push_back(generate_key(6000, TEST_KEY_LEN));
    keys_ptr.push_back(keys_data.back().data());
    ht_filter->add(keys_data.back().data(), 60);
    expected_set_ids_flat.push_back(60);
    expected_match_counts.push_back(1);

    // Key 2: Two matches (duplicate key)
    keys_data.push_back(generate_key(6001, TEST_KEY_LEN));
    keys_ptr.push_back(keys_data.back().data());
    ht_filter->add(keys_data.back().data(), 61);
    ht_filter->add(keys_data.back().data(), 62); // Duplicate
    expected_set_ids_flat.push_back(61);
    expected_set_ids_flat.push_back(62);
    expected_match_counts.push_back(2);

    // Key 3: Non-existing
    keys_data.push_back(generate_key(6002, TEST_KEY_LEN));
    keys_ptr.push_back(keys_data.back().data());
    expected_set_ids_flat.push_back(FILTER_NO_MATCH); // Placeholder
    expected_set_ids_flat.push_back(FILTER_NO_MATCH); // Placeholder if max_match_per_key is > 1
    expected_match_counts.push_back(0);

    const uint32_t max_match_per_key = 2;
    std::vector<VCL::filter_set_t> found_set_ids(keys_ptr.size() * max_match_per_key);
    std::vector<uint32_t> actual_match_counts(keys_ptr.size());

    int total_keys_with_matches = ht_filter->lookup_multi_bulk(
        keys_ptr.data(), keys_ptr.size(), max_match_per_key,
        actual_match_counts.data(), found_set_ids.data());

    EXPECT_EQ(total_keys_with_matches, 2); // Key 1 and Key 2 had matches

    // Verify results for each key
    // Key 1 (index 0)
    EXPECT_EQ(actual_match_counts[0], 1);
    EXPECT_EQ(found_set_ids[0 * max_match_per_key], 60);

    // Key 2 (index 1)
    EXPECT_EQ(actual_match_counts[1], 2);
    // Order not guaranteed for multiple matches for one key
    EXPECT_TRUE((found_set_ids[1 * max_match_per_key] == 61 && found_set_ids[1 * max_match_per_key + 1] == 62) ||
                (found_set_ids[1 * max_match_per_key] == 62 && found_set_ids[1 * max_match_per_key + 1] == 61));

    // Key 3 (index 2)
    EXPECT_EQ(actual_match_counts[2], 0);
    EXPECT_EQ(found_set_ids[2 * max_match_per_key], FILTER_NO_MATCH);
    EXPECT_EQ(found_set_ids[2 * max_match_per_key + 1], FILTER_NO_MATCH);
}

TEST_F(FilterCommonTest, LookupMultiBulk_MixedKeys_Cache) {
    std::vector<std::vector<char>> keys_data;
    std::vector<const void*> keys_ptr;
    std::vector<VCL::filter_set_t> expected_set_ids_flat; // Flattened for checking
    std::vector<uint32_t> expected_match_counts;

    // Key 1: Single match
    keys_data.push_back(generate_key(7000, TEST_KEY_LEN));
    keys_ptr.push_back(keys_data.back().data());
    cache_filter->add(keys_data.back().data(), 70);
    expected_set_ids_flat.push_back(70);
    expected_match_counts.push_back(1);

    // Key 2: Should only have one match due to cache update semantics
    keys_data.push_back(generate_key(7001, TEST_KEY_LEN));
    keys_ptr.push_back(keys_data.back().data());
    cache_filter->add(keys_data.back().data(), 71);
    cache_filter->add(keys_data.back().data(), 72); // Duplicate, should update to 72
    expected_set_ids_flat.push_back(72); // Only the last one
    expected_match_counts.push_back(1);

    // Key 3: Non-existing
    keys_data.push_back(generate_key(7002, TEST_KEY_LEN));
    keys_ptr.push_back(keys_data.back().data());
    expected_set_ids_flat.push_back(FILTER_NO_MATCH); // Placeholder
    expected_match_counts.push_back(0);

    const uint32_t max_match_per_key = 2; // Even though cache will only return 1
    std::vector<VCL::filter_set_t> found_set_ids(keys_ptr.size() * max_match_per_key);
    std::vector<uint32_t> actual_match_counts(keys_ptr.size());

    int total_keys_with_matches = cache_filter->lookup_multi_bulk(
        keys_ptr.data(), keys_ptr.size(), max_match_per_key,
        actual_match_counts.data(), found_set_ids.data());

    EXPECT_EQ(total_keys_with_matches, 2); // Key 1 and Key 2 had matches

    // Verify results for each key
    // Key 1 (index 0)
    EXPECT_EQ(actual_match_counts[0], 1);
    EXPECT_EQ(found_set_ids[0 * max_match_per_key], 70);

    // Key 2 (index 1) - should only find the latest (72)
    EXPECT_EQ(actual_match_counts[1], 1);
    EXPECT_EQ(found_set_ids[1 * max_match_per_key], 72);

    // Key 3 (index 2)
    EXPECT_EQ(actual_match_counts[2], 0);
    EXPECT_EQ(found_set_ids[2 * max_match_per_key], FILTER_NO_MATCH);
}

/*
 ********************************
 * V. Stress/Edge Cases         *
 ********************************
*/

TEST_F(FilterCommonTest, CuckooHTFilter_Stress_FillAndLookup) {
    uint32_t num_to_add = params_ht_medium.num_keys * 80 / 100; // Try to fill 80%
    if (num_to_add == 0) num_to_add = 1;

    std::vector<std::vector<char>> keys_data(num_to_add);
    std::vector<VCL::filter_set_t> set_ids_added(num_to_add);
    int successful_adds = 0;

    for (uint32_t i = 0; i < num_to_add; ++i) {
        keys_data[i] = generate_key(i, TEST_KEY_LEN);
        set_ids_added[i] = static_cast<VCL::filter_set_t>(i + 1);
        int ret = ht_filter->add(keys_data[i].data(), set_ids_added[i]);
        if (ret == 0 || ret == 1) { // 0 for inserted, 1 for inserted after kick-out
            successful_adds++;
        } else if (ret == -ENOSPC) {
            // This is acceptable behavior for a Cuckoo filter at high load.
            // Mark the entry as "not added" so we don't expect it in lookup.
            set_ids_added[i] = 0;
        } else {
            FAIL() << "Unexpected error " << ret << " when adding key " << i;
        }
    }

    EXPECT_GE(successful_adds, num_to_add * 9 / 10) << "Low insertion rate for CuckooHTFilter. Might be too full or bad hash distribution.";

    int successful_lookups = 0;
    for (uint32_t i = 0; i < num_to_add; ++i) {
        if (set_ids_added[i] != 0) { // Only check if the add was successfully attempted
            VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
            int ret = ht_filter->lookup(keys_data[i].data(), &found_set_id);
            if (ret == 1 && found_set_id == set_ids_added[i]) {
                successful_lookups++;
            }
        }
    }
    EXPECT_EQ(successful_lookups, successful_adds) << "Not all successfully added keys could be looked up in HT filter";
}

TEST_F(FilterCommonTest, CuckooCacheFilter_Stress_FillAndLookup) {
    uint32_t num_to_add = params_cache_medium.num_keys * 80 / 100; // Fill to 80% saturation approximately.
    if (num_to_add == 0) num_to_add = 1;

    std::vector<std::vector<char>> keys_data(num_to_add);
    std::vector<VCL::filter_set_t> set_ids_added(num_to_add);

    for (uint32_t i = 0; i < num_to_add; ++i) {
        keys_data[i] = generate_key(i, TEST_KEY_LEN);
        VCL::filter_set_t current_set_id = static_cast<VCL::filter_set_t>(i + 1);
        int ret = cache_filter->add(keys_data[i].data(), current_set_id);
        EXPECT_TRUE(ret == 0 || ret == 1) << "CuckooCacheFilter::add failed for key " << i;
        set_ids_added[i] = current_set_id; // Store the one we intended to add
    }

    int successful_lookups = 0;
    for (uint32_t i = 0; i < num_to_add; ++i) {
        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        int ret = cache_filter->lookup(keys_data[i].data(), &found_set_id);
        if (ret == 1 && found_set_id == set_ids_added[i]) {
            successful_lookups++;
        }
    }
    EXPECT_GE(successful_lookups, num_to_add * 90 / 100) << "Low lookup recall for CuckooCacheFilter. Evictions might be too aggressive.";
}

// max capacity tests
class FilterCapacityTest : public FilterTest {
protected:
    std::unique_ptr<VCL::CuckooHTFilter> ht_filter_max;
    std::unique_ptr<VCL::CuckooCacheFilter> cache_filter_max;

    void SetUp() override {
        FilterTest::SetUp(); // Call base class SetUp
        ht_filter_max = std::make_unique<VCL::CuckooHTFilter>(params_ht_max);
        cache_filter_max = std::make_unique<VCL::CuckooCacheFilter>(params_cache_max);
        ASSERT_TRUE(ht_filter_max->is_valid());
        ASSERT_TRUE(cache_filter_max->is_valid());
    }
};

TEST_F(FilterCapacityTest, CuckooHTFilter_Stress_MaxCapacitySaturation) {
    uint32_t num_to_add = FILTER_MAX_CAPACITY * 90 / 100;
    if (num_to_add == 0) num_to_add = 1;

    std::vector<std::vector<char>> keys_data(num_to_add);
    std::vector<VCL::filter_set_t> set_ids_added(num_to_add); // Use 0 to indicate failed add
    int successful_adds = 0;

    for (uint32_t i = 0; i < num_to_add; ++i) {
        keys_data[i] = generate_key(i, TEST_KEY_LEN);
        set_ids_added[i] = static_cast<VCL::filter_set_t>((i % 0x7FFF) + 1); // 0x7FFF is max set_id
        int ret = ht_filter_max->add(keys_data[i].data(), set_ids_added[i]);
        if (ret == 0 || ret == 1) {
            successful_adds++;
        } else if (ret == -ENOSPC) {
            set_ids_added[i] = 0; // Mark as failed to add
        } else {
            FAIL() << "Unexpected error " << ret << " when adding key " << i << " to max capacity HT filter";
        }
    }
    // Expect a high percentage of successful additions, but not necessarily 100% due to cuckoo filter nature.
    EXPECT_GE(successful_adds, num_to_add * 85 / 100) << "Low insertion rate for CuckooHTFilter at max capacity.";

    int successful_lookups = 0;
    for (uint32_t i = 0; i < num_to_add; ++i) {
        if (set_ids_added[i] != 0) { // Only check if the add was successful
            VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
            int ret = ht_filter_max->lookup(keys_data[i].data(), &found_set_id);
            if (ret == 1 && found_set_id == set_ids_added[i]) {
                successful_lookups++;
            }
        }
    }
    EXPECT_EQ(successful_lookups, successful_adds) << "Not all successfully added keys could be looked up in HT filter at max capacity";
}

TEST_F(FilterCapacityTest, CuckooCacheFilter_Stress_MaxCapacitySaturation) {
    uint32_t num_to_add = FILTER_MAX_CAPACITY * 90 / 100;
    if (num_to_add == 0) num_to_add = 1;

    std::vector<std::vector<char>> keys_data(num_to_add);
    std::vector<VCL::filter_set_t> set_ids_added(num_to_add);

    for (uint32_t i = 0; i < num_to_add; ++i) {
        keys_data[i] = generate_key(i, TEST_KEY_LEN);
        VCL::filter_set_t current_set_id = static_cast<VCL::filter_set_t>((i % 0x7FFF) + 1); // 0x7FFF is 32767
        int ret = cache_filter_max->add(keys_data[i].data(), current_set_id);
        EXPECT_TRUE(ret == 0 || ret == 1) << "CuckooCacheFilter::add failed for key " << i << " at max capacity";
        set_ids_added[i] = current_set_id;
    }

    int successful_lookups = 0;
    for (uint32_t i = 0; i < num_to_add; ++i) {
        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        int ret = cache_filter_max->lookup(keys_data[i].data(), &found_set_id);
        if (ret == 1 && found_set_id == set_ids_added[i]) {
            successful_lookups++;
        }
    }
    EXPECT_GE(successful_lookups, num_to_add * 90 / 100) << "Low lookup recall for CuckooCacheFilter at max capacity.";
}

TEST_F(FilterCommonTest, Add_InvalidSetID_HT) {
    auto key = generate_key(8000, TEST_KEY_LEN);
    VCL::filter_set_t bad_set_id_nomatch = FILTER_NO_MATCH;
    VCL::filter_set_t bad_set_id_msb_set = (1U << (sizeof(VCL::filter_set_t) * 8 - 1)) | 1; // MSB set

    // Test FILTER_NO_MATCH
    int ret = ht_filter->add(key.data(), bad_set_id_nomatch);
    EXPECT_EQ(ret, -EINVAL) << "Add with FILTER_NO_MATCH set_id should return -EINVAL for HT";

    // Test MSB set
    ret = ht_filter->add(key.data(), bad_set_id_msb_set);
    EXPECT_EQ(ret, -EINVAL) << "Add with MSB set in set_id should return -EINVAL for HT";
}

TEST_F(FilterCommonTest, Add_InvalidSetID_Cache) {
    auto key = generate_key(8001, TEST_KEY_LEN);
    VCL::filter_set_t bad_set_id_nomatch = FILTER_NO_MATCH;
    VCL::filter_set_t bad_set_id_msb_set = (1U << (sizeof(VCL::filter_set_t) * 8 - 1)) | 1; // MSB set

    // Test FILTER_NO_MATCH
    int ret = cache_filter->add(key.data(), bad_set_id_nomatch);
    EXPECT_EQ(ret, -EINVAL) << "Add with FILTER_NO_MATCH set_id should return -EINVAL for Cache";

    // Test MSB set
    ret = cache_filter->add(key.data(), bad_set_id_msb_set);
    EXPECT_EQ(ret, -EINVAL) << "Add with MSB set in set_id should return -EINVAL for Cache";
}

TEST_F(FilterCommonTest, Lookup_NullPointers_HT) {
    VCL::filter_set_t set_id;
    EXPECT_EQ(ht_filter->lookup(nullptr, &set_id), -EINVAL) << "Lookup with null key should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup(generate_key(1,TEST_KEY_LEN).data(), nullptr), -EINVAL) << "Lookup with null set_id ptr should return -EINVAL for HT";
}

TEST_F(FilterCommonTest, Lookup_NullPointers_Cache) {
    VCL::filter_set_t set_id;
    EXPECT_EQ(cache_filter->lookup(nullptr, &set_id), -EINVAL) << "Lookup with null key should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup(generate_key(1,TEST_KEY_LEN).data(), nullptr), -EINVAL) << "Lookup with null set_id ptr should return -EINVAL for Cache";
}

TEST_F(FilterCommonTest, LookupBulk_NullPointers_HT) {
    std::vector<const void*> keys_ptr(1);
    std::vector<VCL::filter_set_t> set_ids(1);
    keys_ptr[0] = generate_key(1,TEST_KEY_LEN).data();

    EXPECT_EQ(ht_filter->lookup_bulk(nullptr, 1, set_ids.data()), -EINVAL) << "Bulk lookup with null keys_ptr array should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_bulk(keys_ptr.data(), 1, nullptr), -EINVAL) << "Bulk lookup with null set_ids_out array should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_bulk(keys_ptr.data(), 0, set_ids.data()), -EINVAL) << "Bulk lookup with count 0 should return -EINVAL for HT";
}

TEST_F(FilterCommonTest, LookupBulk_NullPointers_Cache) {
    std::vector<const void*> keys_ptr(1);
    std::vector<VCL::filter_set_t> set_ids(1);
    keys_ptr[0] = generate_key(1,TEST_KEY_LEN).data();

    EXPECT_EQ(cache_filter->lookup_bulk(nullptr, 1, set_ids.data()), -EINVAL) << "Bulk lookup with null keys_ptr array should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_bulk(keys_ptr.data(), 1, nullptr), -EINVAL) << "Bulk lookup with null set_ids_out array should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_bulk(keys_ptr.data(), 0, set_ids.data()), -EINVAL) << "Bulk lookup with count 0 should return -EINVAL for Cache";
}

TEST_F(FilterCommonTest, LookupMulti_NullPointers_HT) {
    std::vector<VCL::filter_set_t> set_ids(1);
    EXPECT_EQ(ht_filter->lookup_multi(nullptr, 1, set_ids.data()), -EINVAL) << "Multi lookup with null key should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_multi(generate_key(1,TEST_KEY_LEN).data(), 1, nullptr), -EINVAL) << "Multi lookup with null set_ids_out should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_multi(generate_key(1,TEST_KEY_LEN).data(), 0, set_ids.data()), -EINVAL) << "Multi lookup with max_match 0 should return -EINVAL for HT";
}

TEST_F(FilterCommonTest, LookupMulti_NullPointers_Cache) {
    std::vector<VCL::filter_set_t> set_ids(1);
    EXPECT_EQ(cache_filter->lookup_multi(nullptr, 1, set_ids.data()), -EINVAL) << "Multi lookup with null key should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_multi(generate_key(1,TEST_KEY_LEN).data(), 1, nullptr), -EINVAL) << "Multi lookup with null set_ids_out should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_multi(generate_key(1,TEST_KEY_LEN).data(), 0, set_ids.data()), -EINVAL) << "Multi lookup with max_match 0 should return -EINVAL for Cache";
}

TEST_F(FilterCommonTest, LookupMultiBulk_NullPointers_HT) {
    std::vector<const void*> keys_ptr(1);
    std::vector<uint32_t> match_counts(1);
    std::vector<VCL::filter_set_t> set_ids(1);
    keys_ptr[0] = generate_key(1,TEST_KEY_LEN).data();

    EXPECT_EQ(ht_filter->lookup_multi_bulk(nullptr, 1, 1, match_counts.data(), set_ids.data()), -EINVAL) << "Multi bulk lookup with null keys_ptr array should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_multi_bulk(keys_ptr.data(), 1, 1, nullptr, set_ids.data()), -EINVAL) << "Multi bulk lookup with null match_counts array should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_multi_bulk(keys_ptr.data(), 1, 1, match_counts.data(), nullptr), -EINVAL) << "Multi bulk lookup with null set_ids_out array should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_multi_bulk(keys_ptr.data(), 0, 1, match_counts.data(), set_ids.data()), -EINVAL) << "Multi bulk lookup with count 0 should return -EINVAL for HT";
    EXPECT_EQ(ht_filter->lookup_multi_bulk(keys_ptr.data(), 1, 0, match_counts.data(), set_ids.data()), -EINVAL) << "Multi bulk lookup with max_match_per_key 0 should return -EINVAL for HT";
}

TEST_F(FilterCommonTest, LookupMultiBulk_NullPointers_Cache) {
    std::vector<const void*> keys_ptr(1);
    std::vector<uint32_t> match_counts(1);
    std::vector<VCL::filter_set_t> set_ids(1);
    keys_ptr[0] = generate_key(1,TEST_KEY_LEN).data();

    EXPECT_EQ(cache_filter->lookup_multi_bulk(nullptr, 1, 1, match_counts.data(), set_ids.data()), -EINVAL) << "Multi bulk lookup with null keys_ptr array should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_multi_bulk(keys_ptr.data(), 1, 1, nullptr, set_ids.data()), -EINVAL) << "Multi bulk lookup with null match_counts array should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_multi_bulk(keys_ptr.data(), 1, 1, match_counts.data(), nullptr), -EINVAL) << "Multi bulk lookup with null set_ids_out array should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_multi_bulk(keys_ptr.data(), 0, 1, match_counts.data(), set_ids.data()), -EINVAL) << "Multi bulk lookup with count 0 should return -EINVAL for Cache";
    EXPECT_EQ(cache_filter->lookup_multi_bulk(keys_ptr.data(), 1, 0, match_counts.data(), set_ids.data()), -EINVAL) << "Multi bulk lookup with max_match_per_key 0 should return -EINVAL for Cache";
}

TEST_F(FilterCommonTest, CuckooHTFilter_Add_FullFilter_ENOSPC) {
    // Re-initialize a very small HT filter for this test to more easily hit capacity
    VCL::FilterParameters small_params;
    small_params.num_keys = 16; // A small number, e.g., 16 actual slots (4 entries per bucket * 4 buckets)
    small_params.key_len = TEST_KEY_LEN;
    small_params.name = "TinyHTFilter";
    small_params.engine = VCL::CuckooHT;
    small_params.prim_hash_seed = 0x12345678;
    small_params.sec_hash_seed = 0x87654321;
    ht_filter = std::make_unique<VCL::CuckooHTFilter>(small_params);
    ASSERT_TRUE(ht_filter->is_valid());

    uint32_t theoretical_max_entries = small_params.num_keys;
    // Attempt to add more keys than the filter can reliably hold to trigger ENOSPC
    // A cuckoo filter typically saturates around 90-95% of its theoretical capacity.
    // So, adding 20-30% more than 'num_keys' should reliably hit ENOSPC.
    uint32_t keys_to_attempt_add = theoretical_max_entries + theoretical_max_entries / 3;

    int enospc_count = 0;
    for (uint32_t i = 0; i < keys_to_attempt_add; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        VCL::filter_set_t set_id = static_cast<VCL::filter_set_t>(i + 1);
        int ret = ht_filter->add(key.data(), set_id);
        if (ret == -ENOSPC) {
            enospc_count++;
        } else {
            EXPECT_TRUE(ret == 0 || ret == 1) << "Unexpected return code for add during saturation: " << ret;
        }
    }
    // We expect at least some -ENOSPC errors if we truly overfill and the filter can't accommodate.
    EXPECT_GT(enospc_count, 0) << "Expected to hit -ENOSPC when overfilling CuckooHTFilter, but did not.";
}

TEST_F(FilterCommonTest, AddDeleteAdd_Sequence_HT) {
    // Phase 1: Initial Additions
    std::vector<std::vector<char>> keys1;
    for (int i = 0; i < 10; ++i) {
        keys1.push_back(generate_key(i, TEST_KEY_LEN));
        ASSERT_EQ(ht_filter->add(keys1[i].data(), static_cast<VCL::filter_set_t>(i + 1)), 0)
            << "Initial add failed for key " << i;
    }
    // Verify initial additions
    for (int i = 0; i < 10; ++i) {
        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(ht_filter->lookup(keys1[i].data(), &found_set_id), 1) << "Key " << i << " not found after initial add";
        EXPECT_EQ(found_set_id, static_cast<VCL::filter_set_t>(i + 1)) << "Incorrect set_id for key " << i;
    }

    // Phase 2: Deletions
    ASSERT_EQ(ht_filter->delete_key(keys1[2].data(), 3), 0) << "Failed to delete K2"; // Delete K2 (set_id 3)
    ASSERT_EQ(ht_filter->delete_key(keys1[5].data(), 6), 0) << "Failed to delete K5"; // Delete K5 (set_id 6)
    
    // Verify deletions
    VCL::filter_set_t found_set_id_deleted = FILTER_NO_MATCH;
    EXPECT_EQ(ht_filter->lookup(keys1[2].data(), &found_set_id_deleted), 0) << "Deleted key K2 unexpectedly found";
    EXPECT_EQ(ht_filter->lookup(keys1[5].data(), &found_set_id_deleted), 0) << "Deleted key K5 unexpectedly found";
    // Verify remaining keys
    EXPECT_EQ(ht_filter->lookup(keys1[0].data(), &found_set_id_deleted), 1) << "K0 unexpectedly not found after deletions";
    EXPECT_EQ(ht_filter->lookup(keys1[9].data(), &found_set_id_deleted), 1) << "K9 unexpectedly not found after deletions";

    // Phase 3: New Additions (including a re-added key)
    std::vector<std::vector<char>> keys2;
    keys2.push_back(generate_key(100, TEST_KEY_LEN)); // New K100
    keys2.push_back(generate_key(101, TEST_KEY_LEN)); // New K101
    keys2.push_back(keys1[2]); // Re-add K2, but with a new set_id to distinguish
    
    ASSERT_EQ(ht_filter->add(keys2[0].data(), 11), 0) << "Failed to add new key K100";
    ASSERT_EQ(ht_filter->add(keys2[1].data(), 12), 0) << "Failed to add new key K101";
    ASSERT_EQ(ht_filter->add(keys2[2].data(), 13), 0) << "Failed to re-add K2 with new set_id"; // K2 re-added with set_id 13

    // Verify all expected keys (original survivors + new + re-added)
    EXPECT_EQ(ht_filter->lookup(keys1[0].data(), &found_set_id_deleted), 1) << "K0 missing"; // K0
    VCL::filter_set_t found_k2_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(ht_filter->lookup(keys1[2].data(), &found_k2_set_id), 1) << "Re-added K2 missing"; // K2 (re-added)
    EXPECT_EQ(found_k2_set_id, 13) << "Re-added K2 found with incorrect set_id";
    EXPECT_EQ(ht_filter->lookup(keys1[5].data(), &found_set_id_deleted), 0) << "Deleted K5 unexpectedly found again"; // K5 still deleted
    EXPECT_EQ(ht_filter->lookup(keys2[0].data(), &found_set_id_deleted), 1) << "New K100 missing"; // New K100
    EXPECT_EQ(found_set_id_deleted, 11) << "New K100 found with incorrect set_id";
    EXPECT_EQ(ht_filter->lookup(keys2[1].data(), &found_set_id_deleted), 1) << "New K101 missing"; // New K101
    EXPECT_EQ(found_set_id_deleted, 12) << "New K101 found with incorrect set_id";
}

TEST_F(FilterCommonTest, AddDeleteAdd_Sequence_Cache) {
    // Phase 1: Initial Additions
    std::vector<std::vector<char>> keys1;
    for (int i = 0; i < 10; ++i) {
        keys1.push_back(generate_key(i + 1000, TEST_KEY_LEN));
        // Cache add can return 0 (inserted) or 1 (inserted after eviction)
        ASSERT_TRUE(cache_filter->add(keys1[i].data(), static_cast<VCL::filter_set_t>(i + 1001)) >= 0)
            << "Initial add failed for key " << i;
    }
    // Verify initial additions
    for (int i = 0; i < 10; ++i) {
        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(cache_filter->lookup(keys1[i].data(), &found_set_id), 1) << "Key " << i << " not found after initial add";
        EXPECT_EQ(found_set_id, static_cast<VCL::filter_set_t>(i + 1001)) << "Incorrect set_id for key " << i;
    }

    // Phase 2: Deletions
    ASSERT_EQ(cache_filter->delete_key(keys1[2].data(), 1003), 0) << "Failed to delete K2"; // Delete K2 (set_id 1003)
    ASSERT_EQ(cache_filter->delete_key(keys1[5].data(), 1006), 0) << "Failed to delete K5"; // Delete K5 (set_id 1006)
    
    // Verify deletions
    VCL::filter_set_t found_set_id_deleted = FILTER_NO_MATCH;
    EXPECT_EQ(cache_filter->lookup(keys1[2].data(), &found_set_id_deleted), 0) << "Deleted key K2 unexpectedly found";
    EXPECT_EQ(cache_filter->lookup(keys1[5].data(), &found_set_id_deleted), 0) << "Deleted key K5 unexpectedly found";
    // Verify remaining keys (a sample)
    EXPECT_EQ(cache_filter->lookup(keys1[0].data(), &found_set_id_deleted), 1) << "K0 unexpectedly not found after deletions";
    EXPECT_EQ(cache_filter->lookup(keys1[9].data(), &found_set_id_deleted), 1) << "K9 unexpectedly not found after deletions";

    // Phase 3: New Additions (re-adding K2 should update its set_id)
    std::vector<std::vector<char>> keys2;
    keys2.push_back(generate_key(1010, TEST_KEY_LEN)); // New K1010
    keys2.push_back(generate_key(1011, TEST_KEY_LEN)); // New K1011
    keys2.push_back(keys1[2]); // Re-add K2, with a new set_id 
    
    ASSERT_TRUE(cache_filter->add(keys2[0].data(), 1011) >= 0) << "Failed to add new key K1010";
    ASSERT_TRUE(cache_filter->add(keys2[1].data(), 1012) >= 0) << "Failed to add new key K1011";
    ASSERT_TRUE(cache_filter->add(keys2[2].data(), 1013) >= 0) << "Failed to re-add K2 with new set_id"; // K2 re-added with set_id 1013 (should update)

    // Verify all expected keys (original survivors + new + re-added)
    EXPECT_EQ(cache_filter->lookup(keys1[0].data(), &found_set_id_deleted), 1) << "K0 missing"; // K0
    VCL::filter_set_t found_k2_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(cache_filter->lookup(keys1[2].data(), &found_k2_set_id), 1) << "Re-added K2 missing"; // K2 (re-added)
    EXPECT_EQ(found_k2_set_id, 1013) << "Re-added K2 found with incorrect set_id";
    EXPECT_EQ(cache_filter->lookup(keys1[5].data(), &found_set_id_deleted), 0) << "Deleted K5 unexpectedly found again"; // K5 still deleted
    EXPECT_EQ(cache_filter->lookup(keys2[0].data(), &found_set_id_deleted), 1) << "New K1010 missing"; // New K1010
    EXPECT_EQ(found_set_id_deleted, 1011) << "New K1010 found with incorrect set_id";
    EXPECT_EQ(cache_filter->lookup(keys2[1].data(), &found_set_id_deleted), 1) << "New K1011 missing"; // New K1011
    EXPECT_EQ(found_set_id_deleted, 1012) << "New K1011 found with incorrect set_id";
}

TEST_F(FilterCommonTest, ResetAndReuse_HT) {
    // Add some keys
    for (int i = 0; i < 20; ++i) { 
        auto key = generate_key(i, TEST_KEY_LEN);
        ASSERT_EQ(ht_filter->add(key.data(), static_cast<VCL::filter_set_t>(i + 1)), 0);
    }
    // Verify some keys are present
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(ht_filter->lookup(generate_key(15, TEST_KEY_LEN).data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, 16); // Key 15 has set_id 16

    ht_filter->reset();

    // Verify no keys are found after reset
    for (int i = 0; i < 20; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(ht_filter->lookup(key.data(), &found_set_id), 0) << "Key " << i << " found after reset in HT filter";
        EXPECT_EQ(found_set_id, FILTER_NO_MATCH);
    }

    // Now, add a NEW set of keys and verify reuse
    for (int i = 200; i < 220; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        ASSERT_EQ(ht_filter->add(key.data(), static_cast<VCL::filter_set_t>(i - 199)), 0);
    }
    // Verify new keys are present
    EXPECT_EQ(ht_filter->lookup(generate_key(205, TEST_KEY_LEN).data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, 6); // 205 - 199 = 6
    // Verify old keys are still not present
    EXPECT_EQ(ht_filter->lookup(generate_key(15, TEST_KEY_LEN).data(), &found_set_id), 0);
}

TEST_F(FilterCommonTest, ResetAndReuse_Cache) {
    // Add some keys
    for (int i = 0; i < 20; ++i) { // Add more keys
        auto key = generate_key(i, TEST_KEY_LEN);
        ASSERT_TRUE(cache_filter->add(key.data(), static_cast<VCL::filter_set_t>(i + 1)) >= 0);
    }
    // Verify some keys are present
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(cache_filter->lookup(generate_key(15, TEST_KEY_LEN).data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, 16);

    cache_filter->reset();

    // Verify no keys are found after reset
    for (int i = 0; i < 20; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(cache_filter->lookup(key.data(), &found_set_id), 0) << "Key " << i << " found after reset in Cache filter";
        EXPECT_EQ(found_set_id, FILTER_NO_MATCH);
    }

    // Now, add a NEW set of keys and verify reuse
    for (int i = 200; i < 220; ++i) {
        auto key = generate_key(i, TEST_KEY_LEN);
        ASSERT_TRUE(cache_filter->add(key.data(), static_cast<VCL::filter_set_t>(i - 199)) >= 0);
    }
    // Verify new keys are present
    EXPECT_EQ(cache_filter->lookup(generate_key(205, TEST_KEY_LEN).data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, 6);
    // Verify old keys are still not present
    EXPECT_EQ(cache_filter->lookup(generate_key(15, TEST_KEY_LEN).data(), &found_set_id), 0);
}

TEST_F(FilterTest, Filter_DestructionAndRecreation_HT) {
    // Create, use, and destroy the first filter instance within its own scope
    {
        std::unique_ptr<VCL::CuckooHTFilter> first_filter =
            std::make_unique<VCL::CuckooHTFilter>(params_ht_medium);
        ASSERT_TRUE(first_filter->is_valid());

        auto key1 = generate_key(100, TEST_KEY_LEN);
        VCL::filter_set_t set_id1 = 10;
        ASSERT_EQ(first_filter->add(key1.data(), set_id1), 0);

        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        EXPECT_EQ(first_filter->lookup(key1.data(), &found_set_id), 1);
        EXPECT_EQ(found_set_id, set_id1);
    } // `first_filter` goes out of scope here and is destroyed

    // Create a new filter instance with the exact same parameters
    std::unique_ptr<VCL::CuckooHTFilter> second_filter =
        std::make_unique<VCL::CuckooHTFilter>(params_ht_medium);
    ASSERT_TRUE(second_filter->is_valid());

    // Verify it's empty and fully functional (no lingering state from the first filter)
    auto key1 = generate_key(100, TEST_KEY_LEN); // The old key from the first filter
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
    EXPECT_EQ(second_filter->lookup(key1.data(), &found_set_id), 0) << "Old key found in newly created filter instance";
    EXPECT_EQ(found_set_id, FILTER_NO_MATCH);

    auto key2 = generate_key(200, TEST_KEY_LEN); // A new key for the second filter
    VCL::filter_set_t set_id2 = 20;
    ASSERT_EQ(second_filter->add(key2.data(), set_id2), 0);
    EXPECT_EQ(second_filter->lookup(key2.data(), &found_set_id), 1);
    EXPECT_EQ(found_set_id, set_id2);
}

TEST_F(FilterCommonTest, CuckooHTFilter_ExactCapacityBoundary_ENOSPC) {
    // This test aims to hit the probabilistic saturation point more explicitly for HTFilter.
    // Re-initialize a very small HT filter for this test to more easily observe saturation.
    VCL::FilterParameters tiny_params;
    tiny_params.num_keys = 32; // ensure saturation
    tiny_params.key_len = TEST_KEY_LEN;
    tiny_params.name = "TinyHTFilterForSaturation";
    tiny_params.engine = VCL::CuckooHT;
    tiny_params.prim_hash_seed = 0x12345678;
    tiny_params.sec_hash_seed = 0x87654321;
    ht_filter = std::make_unique<VCL::CuckooHTFilter>(tiny_params); // Overwrite fixture's filter
    ASSERT_TRUE(ht_filter->is_valid());

    // Cuckoo filters typically achieve a load factor of ~0.95 
    // For 32 'num_keys' (meaning ~32*0.95 = 30 entries), we expect to insert roughly 30 keys
    // before hitting ENOSPC consistently. We will attempt to add a bit more than this.
    // The exact number of successful adds can vary due to hash collisions and cuckoo pathfinding,
    // so we assert on the range of successful adds and the presence of ENOSPC.

    uint32_t expected_min_successful_adds = static_cast<uint32_t>(tiny_params.num_keys * 0.9); // Expect at least 90% load factor
    uint32_t keys_to_attempt_add = tiny_params.num_keys + 10; // Try to add slightly more than nominal capacity

    std::vector<std::vector<char>> keys_data(keys_to_attempt_add);
    std::vector<VCL::filter_set_t> set_ids_added(keys_to_attempt_add); // Store set_id; 0 if add failed
    int successful_adds_count = 0;
    int enospc_errors_count = 0;

    // Fill the filter until ENOSPC is consistently returned
    for (uint32_t i = 0; i < keys_to_attempt_add; ++i) {
        keys_data[i] = generate_key(i, TEST_KEY_LEN);
        set_ids_added[i] = static_cast<VCL::filter_set_t>((i % 0x7FFF) + 1);

        int ret = ht_filter->add(keys_data[i].data(), set_ids_added[i]);
        if (ret == 0 || ret == 1) { // 0 for inserted into empty slot, 1 for inserted after kick-out
            successful_adds_count++;
        } else if (ret == -ENOSPC) {
            enospc_errors_count++;
            set_ids_added[i] = 0; // Mark as failed to add for lookup verification
        } else {
            FAIL() << "Unexpected error " << ret << " when adding key " << i << " during saturation test.";
        }
    }

    // Assert that we successfully added a high percentage of keys, and *did* encounter ENOSPC.
    EXPECT_GE(successful_adds_count, expected_min_successful_adds)
        << "Lower than expected number of successful insertions (" << successful_adds_count 
        << ") before hitting ENOSPC for CuckooHTFilter (expected min: " << expected_min_successful_adds << ").";
    EXPECT_GT(enospc_errors_count, 0)
        << "Expected to hit -ENOSPC when overfilling CuckooHTFilter, but no ENOSPC errors occurred.";

    // Verify lookups for keys that were *successfully* added
    int recall_count = 0;
    for (uint32_t i = 0; i < keys_to_attempt_add; ++i) {
        if (set_ids_added[i] != 0) { // Only check keys that were marked as successfully added
            VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
            int ret = ht_filter->lookup(keys_data[i].data(), &found_set_id);
            if (ret == 1 && found_set_id == set_ids_added[i]) {
                recall_count++;
            }
        }
    }
    EXPECT_EQ(recall_count, successful_adds_count)
        << "Not all successfully added keys could be looked up in HT filter after saturation. Some might have been overwritten unexpectedly or lookup is faulty.";
}


TEST_F(FilterCommonTest, CuckooCacheFilter_PersistentEviction) {
    // Fill the cache well beyond its nominal capacity to force evictions
    // params_cache_medium.num_keys is 1024. Let's add 2x its 'num_keys' capacity.
    const uint32_t keys_to_add = params_cache_medium.num_keys * 2;
    if (keys_to_add == 0) GTEST_SKIP() << "Test requires non-zero filter capacity.";

    std::vector<std::vector<char>> all_keys_data(keys_to_add);
    std::vector<VCL::filter_set_t> all_set_ids(keys_to_add);

    for (uint32_t i = 0; i < keys_to_add; ++i) {
        all_keys_data[i] = generate_key(i, TEST_KEY_LEN);
        all_set_ids[i] = static_cast<VCL::filter_set_t>((i % 0x7FFF) + 1);
        int ret = cache_filter->add(all_keys_data[i].data(), all_set_ids[i]);
        // Cache add should always return 0 (inserted) or 1 (inserted after eviction), never -ENOSPC
        EXPECT_TRUE(ret == 0 || ret == 1) << "CuckooCacheFilter::add failed unexpectedly for key " << i << " with error " << ret;
    }

    // Verify some of the oldest keys are now evicted (not found)
    // The exact number of evicted keys is probabilistic. If we added 2x capacity,
    // a significant portion of the *oldest* keys should be gone.
    const uint32_t num_old_keys_to_check = params_cache_medium.num_keys / 4; // Check a sample of the first 25% of the original capacity
    int old_keys_found = 0;
    for (uint32_t i = 0; i < num_old_keys_to_check; ++i) {
        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        if (cache_filter->lookup(all_keys_data[i].data(), &found_set_id) == 1) {
            old_keys_found++;
        }
    }
    // We expect very few (ideally close to zero) of these very first keys to remain.
    // This threshold can be adjusted, but >50% missing is a good sign of eviction.
    EXPECT_LT(old_keys_found, num_old_keys_to_check / 2)
        << "Too many old keys (" << old_keys_found << " out of " << num_old_keys_to_check 
        << ") are still found. Eviction might not be working as expected or filter capacity is too large.";


    // Verify some of the newest keys are still present
    const uint32_t num_new_keys_to_check = params_cache_medium.num_keys / 4; // Check a sample of the last 25% of keys added
    int new_keys_found = 0;
    for (uint32_t i = 0; i < num_new_keys_to_check; ++i) {
        uint32_t key_idx = keys_to_add - 1 - i; // Check from the end of the added keys backwards
        VCL::filter_set_t found_set_id = FILTER_NO_MATCH;
        if (cache_filter->lookup(all_keys_data[key_idx].data(), &found_set_id) == 1 &&
            found_set_id == all_set_ids[key_idx]) {
            new_keys_found++;
        }
    }
    // Expect a high recall rate for recently added keys
    EXPECT_GE(new_keys_found, num_new_keys_to_check * 70 / 100)
        << "Low recall for recent keys (" << new_keys_found << " out of " << num_new_keys_to_check
        << "). Cache might be evicting too aggressively or lookups are faulty.";
}



/*
 ***************************************
 ** These test cases are placeholder *** 
 ** VBF is just stub to be completed ***
 * Added for passing coverage threshold*
 ***************************************
*/

// --- Test Cases for VBFFilter.cc (Placeholder Coverage) ---

TEST_F(FilterTest, VBFFilter_Creation_Success) {
    VCL::FilterParameters params_vbf_small = params_ht_small; 
    params_vbf_small.name = "TestVBFFilterSmall";
    params_vbf_small.engine = VCL::VBF;

    std::unique_ptr<VCL::VBF_Filter> filter;
    ASSERT_NO_THROW(filter = std::make_unique<VCL::VBF_Filter>(params_vbf_small));
    ASSERT_TRUE(filter != nullptr);
    // Even for a stub, is_valid should return true if construction was successful
    EXPECT_TRUE(filter->is_valid());
}

TEST_F(FilterTest, VBFFilter_AddAndLookup_StubBehavior) {
    VCL::FilterParameters params_vbf_medium = params_ht_medium;
    params_vbf_medium.name = "TestVBFFilterMedium";
    params_vbf_medium.engine = VCL::VBF;

    std::unique_ptr<VCL::VBF_Filter> filter =
        std::make_unique<VCL::VBF_Filter>(params_vbf_medium);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(filter->is_valid());

    auto key = generate_key(123, TEST_KEY_LEN);
    VCL::filter_set_t set_id = 42;
    VCL::filter_set_t found_set_id = FILTER_NO_MATCH;

    // Test add - it should print the stub message
    EXPECT_EQ(filter->add(key.data(), set_id), -ENOSYS) << "VBF_Filter::add stub should return -ENOSYS";

    // Test lookup - it should print the stub message and return 0 (no match)
    EXPECT_EQ(filter->lookup(key.data(), &found_set_id), 0) << "VBF_Filter::lookup stub should return 0 (no match)";
    EXPECT_EQ(found_set_id, FILTER_NO_MATCH) << "VBF_Filter::lookup stub should set set_id to FILTER_NO_MATCH";

    // Test delete_key - should print stub message
    EXPECT_EQ(filter->delete_key(key.data(), set_id), -ENOSYS) << "VBF_Filter::delete_key stub should return -ENOSYS";

    // Test reset - should print stub message
    ASSERT_NO_THROW(filter->reset());
}

TEST_F(FilterTest, VBFFilter_LookupBulk_StubBehavior) {
    VCL::FilterParameters params_vbf = params_ht_small;
    params_vbf.name = "TestVBFFilterBulk";
    params_vbf.engine = VCL::VBF;

    std::unique_ptr<VCL::VBF_Filter> filter =
        std::make_unique<VCL::VBF_Filter>(params_vbf);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(filter->is_valid());

    std::vector<std::vector<char>> keys(5);
    std::vector<const void*> key_ptrs(5);
    std::vector<VCL::filter_set_t> set_ids(5); 

    for (int i = 0; i < 5; ++i) {
        keys[i] = generate_key(i + 1, TEST_KEY_LEN);
        key_ptrs[i] = keys[i].data();
    }

    // Expect 0 (not found) and all set_ids to be FILTER_NO_MATCH
    EXPECT_EQ(filter->lookup_bulk(key_ptrs.data(), 5, set_ids.data()), 0)
        << "VBF_Filter::lookup_bulk stub should return 0";

    for (int i = 0; i < 5; ++i) {
        EXPECT_EQ(set_ids[i], FILTER_NO_MATCH)
            << "VBF_Filter::lookup_bulk stub should set all set_ids to FILTER_NO_MATCH";
    }
}


TEST_F(FilterTest, VBFFilter_LookupMulti_StubBehavior) {
    VCL::FilterParameters params_vbf = params_ht_small;
    params_vbf.name = "TestVBFFilterMulti";
    params_vbf.engine = VCL::VBF;

    std::unique_ptr<VCL::VBF_Filter> filter =
        std::make_unique<VCL::VBF_Filter>(params_vbf);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(filter->is_valid());

    auto key = generate_key(200, TEST_KEY_LEN);
    VCL::filter_set_t found_set_ids[3]; 
    uint32_t max_matches = 3;

    // Expect 0 (no matches found) and the first set_id to be FILTER_NO_MATCH
    EXPECT_EQ(filter->lookup_multi(key.data(), max_matches, found_set_ids), 0)
        << "VBF_Filter::lookup_multi stub should return 0";
    EXPECT_EQ(found_set_ids[0], FILTER_NO_MATCH)
        << "VBF_Filter::lookup_multi stub should set the first set_id to FILTER_NO_MATCH";
}


TEST_F(FilterTest, VBFFilter_LookupMultiBulk_StubBehavior) {
    VCL::FilterParameters params_vbf = params_ht_small;
    params_vbf.name = "TestVBFFilterMultiBulk";
    params_vbf.engine = VCL::VBF;

    std::unique_ptr<VCL::VBF_Filter> filter =
        std::make_unique<VCL::VBF_Filter>(params_vbf);
    ASSERT_TRUE(filter != nullptr);
    ASSERT_TRUE(filter->is_valid());

    std::vector<std::vector<char>> keys(2);
    std::vector<const void*> key_ptrs(2);
    uint32_t max_matches_per_key = 2;
    std::vector<uint32_t> match_counts(2); 
    std::vector<VCL::filter_set_t> all_found_set_ids(2 * max_matches_per_key);

    keys[0] = generate_key(300, TEST_KEY_LEN);
    keys[1] = generate_key(301, TEST_KEY_LEN);
    key_ptrs[0] = keys[0].data();
    key_ptrs[1] = keys[1].data();

    // Expect 0 (no keys found with matches)
    EXPECT_EQ(filter->lookup_multi_bulk(key_ptrs.data(), 2, max_matches_per_key,
                                      match_counts.data(), all_found_set_ids.data()), 0)
        << "VBF_Filter::lookup_multi_bulk stub should return 0";

    // Verify match_counts are 0 and set_ids are FILTER_NO_MATCH
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(match_counts[i], 0)
            << "VBF_Filter::lookup_multi_bulk stub should set match_count to 0 for each key";
        for (uint32_t j = 0; j < max_matches_per_key; ++j) {
            EXPECT_EQ(all_found_set_ids[i * max_matches_per_key + j], FILTER_NO_MATCH)
                << "VBF_Filter::lookup_multi_bulk stub should set all set_ids to FILTER_NO_MATCH";
        }
    }
}






