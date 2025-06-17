#include "gtest/gtest.h"
#include "vcl/CuckooHTFilter.h"
#include "vcl/CuckooCacheFilter.h"
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

    void SetUp() override {
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
        // No explicit teardown needed for unique_ptr or stack objects
    }
};


/*
 ********************************
 * I. Filter Creation Tests     *
 ********************************
*/

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

/*
 ********************************************
 * II.    Add/Lookup/Delete/Reset Tests     *
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
 * III. Bulk Operations Tests   *
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
 * IV. Stress/Edge Cases        *
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

// New fixture for max capacity tests
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