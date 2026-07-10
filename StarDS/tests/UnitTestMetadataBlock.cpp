/**
 * @file UnitTestMetadataBlock.cpp
 * @brief Unit tests for metadata block functionality
 */

#include <gtest/gtest.h>
#include "stards.h"
#include "Fixtures.h"
#include <sstream>
#include <iostream>
#include <ghc/fs_std.hpp>
#include <random>


using namespace star;

//==============================================================================
// Test Fixture for Metadata Block Tests
//==============================================================================

/**
 * @brief Test fixture that manages temporary files for metadata block tests
 *
 * This fixture creates unique temporary file names for each test and
 * automatically cleans them up in TearDown to prevent file pollution.
 */
// Each test gets its own temp directory (auto-deleted); createTempFile() returns a
// unique path inside it, so tests are safe under parallel execution.
class MetadataBlockTest : public star_test::TempDirTest {
protected:
    std::string createTempFile(const std::string& prefix = "test_metadata") {
        return tempStardsFile(prefix);
    }
};

//==============================================================================
// Phase 1 Tests: Core Serialization/Deserialization
//==============================================================================

TEST_F(MetadataBlockTest, SerializationRoundTrip) {
    std::cerr << "[TEST] Starting SerializationRoundTrip" << std::endl;
    std::string testFile = createTempFile("roundtrip");
    std::cerr << "[TEST] Created temp file: " << testFile << std::endl;

    // Write metadata using new API
    {
        std::cerr << "[TEST] Creating dataset..." << std::endl;
        auto store_write = StarDataset::create(testFile);
        std::cerr << "[TEST] Putting scalar_int..." << std::endl;
        store_write->meta.put("scalar_int", NDArray<int64_t>({}, 42));
        std::cerr << "[TEST] Putting scalar_float..." << std::endl;
        store_write->meta.put("scalar_float", NDArray<double>({}, 3.14159));
        std::cerr << "[TEST] Flushing..." << std::endl;
        store_write->flush();
        std::cerr << "[TEST] Done with write block" << std::endl;
    }
    std::cerr << "[TEST] Opening dataset..." << std::endl;

    // Read back using new API
    auto store_read = StarDataset::open(testFile);

    // Verify loaded metadata - use public API
    ASSERT_EQ(store_read->get_metadata_count(), 2);

    // Verify keys exist in metadata
    auto keys = store_read->get_metadata_keys();
    ASSERT_TRUE(std::find(keys.begin(), keys.end(), "scalar_int") != keys.end());
    ASSERT_TRUE(std::find(keys.begin(), keys.end(), "scalar_float") != keys.end());

    // Verify values
    auto int_arr = store_read->meta.get("scalar_int")->as<int64_t>();
    EXPECT_EQ(int_arr.data()[0], 42);

    auto float_arr = store_read->meta.get("scalar_float")->as<double>();
    EXPECT_DOUBLE_EQ(float_arr.data()[0], 3.14159);
}

TEST_F(MetadataBlockTest, MixedTypes) {
    std::string testFile = createTempFile("mixed");

    // Write metadata using new API
    {
        auto store_write = StarDataset::create(testFile);
        store_write->meta.put("int8_val", NDArray<int8_t>({}, 127));
        store_write->meta.put("int16_val", NDArray<int16_t>({}, 32000));
        store_write->meta.put("int32_val", NDArray<int32_t>({}, 2000000));
        store_write->meta.put("int64_val", NDArray<int64_t>({}, 9000000000LL));
        store_write->meta.put("float_val", NDArray<float>({}, 2.71828f));
        store_write->meta.put("double_val", NDArray<double>({}, 1.41421356));
        store_write->meta.put("string_val", NDArray<std::string>({}, "test_string"));
        store_write->flush();
    }

    // Read back using new API
    auto store_read = StarDataset::open(testFile);

    // Verify all types - use public API
    size_t metadata_count = store_read->get_metadata_count();
    ASSERT_EQ(metadata_count, 7);

    auto int8_arr = store_read->meta.get("int8_val")->as<int8_t>();
    EXPECT_EQ(int8_arr.data()[0], 127);

    auto int16_arr = store_read->meta.get("int16_val")->as<int16_t>();
    EXPECT_EQ(int16_arr.data()[0], 32000);

    auto int32_arr = store_read->meta.get("int32_val")->as<int32_t>();
    EXPECT_EQ(int32_arr.data()[0], 2000000);

    auto int64_arr = store_read->meta.get("int64_val")->as<int64_t>();
    EXPECT_EQ(int64_arr.data()[0], 9000000000LL);

    auto float_arr = store_read->meta.get("float_val")->as<float>();
    EXPECT_FLOAT_EQ(float_arr.data()[0], 2.71828f);

    auto double_arr = store_read->meta.get("double_val")->as<double>();
    EXPECT_DOUBLE_EQ(double_arr.data()[0], 1.41421356);

    auto string_arr = store_read->meta.get("string_val")->as<std::string>();
    EXPECT_EQ(string_arr.data()[0], "test_string");
}

TEST_F(MetadataBlockTest, StringHandling) {
    std::string testFile = createTempFile("strings");

    // Write metadata using new API
    {
        auto store_write = StarDataset::create(testFile);
        store_write->meta.put("empty_string", NDArray<std::string>({}, ""));
        store_write->meta.put("short_string", NDArray<std::string>({}, "hi"));
        store_write->meta.put("long_string", NDArray<std::string>({},
            "This is a much longer string to test serialization of variable-length data"));
        store_write->meta.put("special_chars", NDArray<std::string>({},
            "Special: !@#$%^&*()_+-=[]{}|;':\",./<>?"));
        store_write->flush();
    }

    // Read back using new API
    auto store_read = StarDataset::open(testFile);

    // Verify strings - use public API
    size_t metadata_count = store_read->get_metadata_count();
    ASSERT_EQ(metadata_count, 4);

    auto empty = store_read->meta.get("empty_string")->as<std::string>();
    EXPECT_EQ(empty.data()[0], "");

    auto short_str = store_read->meta.get("short_string")->as<std::string>();
    EXPECT_EQ(short_str.data()[0], "hi");

    auto long_str = store_read->meta.get("long_string")->as<std::string>();
    EXPECT_EQ(long_str.data()[0],
        "This is a much longer string to test serialization of variable-length data");

    auto special = store_read->meta.get("special_chars")->as<std::string>();
    EXPECT_EQ(special.data()[0], "Special: !@#$%^&*()_+-=[]{}|;':\",./<>?");
}

TEST_F(MetadataBlockTest, EmptyBlock) {
    std::string testFile = createTempFile("empty");

    // Write empty file using new API
    {
        auto store_write = StarDataset::create(testFile);
        // Don't add any metadata
        store_write->flush();
    }

    // Read back using new API
    auto store_read = StarDataset::open(testFile);

    // Verify empty - use public API
    size_t metadata_count = store_read->get_metadata_count();
    EXPECT_EQ(metadata_count, 0);
}

TEST_F(MetadataBlockTest, SmallArrays) {
    std::string testFile = createTempFile("arrays");

    // Write metadata using new API
    {
        auto store_write = StarDataset::create(testFile);

        // Add small arrays (should fit in metadata block)
        NDArray<int64_t> small_1d({5});
        for (size_t i = 0; i < 5; ++i) {
            small_1d.data()[i] = static_cast<int64_t>(i * 10);
        }
        store_write->meta.put("small_array_1d", std::move(small_1d));

        NDArray<double> small_2d({2, 3});
        for (size_t i = 0; i < 6; ++i) {
            small_2d.data()[i] = static_cast<double>(i) * 1.5;
        }
        store_write->meta.put("small_array_2d", std::move(small_2d));
        store_write->flush();
    }

    // Read back using new API
    auto store_read = StarDataset::open(testFile);

    // Verify arrays - count entries with stored_in_metadata_flags = 1
    size_t metadata_count = 0;
    // Use public API
    metadata_count = store_read->get_metadata_count();
    ASSERT_EQ(metadata_count, 2);

    auto arr_1d = store_read->meta.get("small_array_1d")->as<int64_t>();
    EXPECT_EQ(arr_1d.shape().size(), 1);
    EXPECT_EQ(arr_1d.shape()[0], 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(arr_1d.data()[i], static_cast<int64_t>(i * 10));
    }

    auto arr_2d = store_read->meta.get("small_array_2d")->as<double>();
    EXPECT_EQ(arr_2d.shape().size(), 2);
    EXPECT_EQ(arr_2d.shape()[0], 2);
    EXPECT_EQ(arr_2d.shape()[1], 3);
    for (size_t i = 0; i < 6; ++i) {
        EXPECT_DOUBLE_EQ(arr_2d.data()[i], static_cast<double>(i) * 1.5);
    }
}

TEST_F(MetadataBlockTest, LargeMetadataBlock) {
    std::string testFile = createTempFile("large");

    // Write metadata using new API
    {
        auto store_write = StarDataset::create(testFile);

        // Add 100 scalar entries
        for (int i = 0; i < 100; ++i) {
            std::string key = "scalar_" + std::to_string(i);
            store_write->meta.put(key, NDArray<int64_t>({}, i * 100));
        }
        store_write->flush();
    }

    // Read back using new API
    auto store_read = StarDataset::open(testFile);

    // Verify all 100 entries - use public API
    size_t metadata_count = store_read->get_metadata_count();
    ASSERT_EQ(metadata_count, 100);

    for (int i = 0; i < 100; ++i) {
        std::string key = "scalar_" + std::to_string(i);
        // v1 format: Metadata is in separate namespace, use contains() API
        ASSERT_TRUE(store_read->meta.contains(key));

        auto arr = store_read->meta.get(key)->as<int64_t>();
        EXPECT_EQ(arr.data()[0], i * 100);
    }
}

TEST_F(MetadataBlockTest, MagicHeaderValidation) {
    std::string testFile = createTempFile("magic");

    // Create a valid file
    {
        auto store_write = StarDataset::create(testFile);
        store_write->meta.put("test", NDArray<int64_t>({}, 123));
        store_write->flush();
    }

    // Read and corrupt the file's magic string
    std::ifstream in(testFile, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    in.close();

    std::string file_content = buffer.str();
    file_content[0] = 'X';  // Corrupt first byte of magic

    // Write corrupted content back
    std::ofstream out(testFile, std::ios::binary | std::ios::trunc);
    out.write(file_content.data(), file_content.size());
    out.close();

    // Try to open corrupted file
    EXPECT_THROW({
        auto store_read = StarDataset::open(testFile);
    }, std::runtime_error);
}

TEST_F(MetadataBlockTest, VersionCheck) {
    std::string testFile = createTempFile("version");

    // Create a valid file with current format version (1)
    {
        auto store_write = StarDataset::create(testFile);
        store_write->meta.put("test", NDArray<int64_t>({}, 123));
        store_write->flush();
    }

    // Verify the file has format version 1
    {
        auto store_read = StarDataset::open(testFile);
        const auto& header = store_read->getFileHeader();
        EXPECT_EQ(header.format_version, 1);
    }

    // Read and corrupt the format version to an unsupported version
    std::ifstream in(testFile, std::ios::binary);
    std::stringstream buffer;
    buffer << in.rdbuf();
    in.close();

    std::string file_content = buffer.str();
    file_content[6] = 99;  // Format version is at offset 6 (after 6-byte magic), set to unsupported version

    // Write corrupted content back
    std::ofstream out(testFile, std::ios::binary | std::ios::trunc);
    out.write(file_content.data(), file_content.size());
    out.close();

    // Try to open file with unsupported version - should succeed but data may not be readable
    // (Our current implementation doesn't validate format version beyond checking magic)
    // This test just verifies the format version field works
    auto store_read = StarDataset::open(testFile);
    EXPECT_EQ(store_read->getFileHeader().format_version, 99);
}

//==============================================================================
// Phase 2 Tests: Classification Logic
//==============================================================================

// NOTE: These tests were removed because they tested classify_value() which was part
// of the old auto-routing API that was removed in the simplification. Now all data
// goes to the metadata block via store->meta.put(), making classification unnecessary.

// TEST_F(MetadataBlockTest, Classification) - REMOVED (tested old classify_value API)
// TEST_F(MetadataBlockTest, ForceSeparate) - REMOVED (tested old classify_value API)
// TEST_F(MetadataBlockTest, ThresholdBoundaries) - REMOVED (tested old classify_value API)

// NOTE: ConfigurationMethods test removed - MetadataBlockConfig struct and auto-routing
// were removed in simplification. Configuration now done through StarConfig at creation time.

TEST_F(MetadataBlockTest, QueryMethods) {
    std::string testFile = createTempFile("query");
    auto store = StarDataset::create(testFile);

    // Initially no metadata
    EXPECT_FALSE(store->is_metadata_loaded());
    EXPECT_EQ(store->get_metadata_count(), 0);
    EXPECT_EQ(store->get_metadata_keys().size(), 0);

    // Add some pending metadata
    store->meta.put("key1", NDArray<int64_t>({}, 1));
    store->meta.put("key2", NDArray<int64_t>({}, 2));
    store->meta.put("key3", NDArray<int64_t>({}, 3));

    EXPECT_EQ(store->get_metadata_count(), 3);
    auto keys = store->get_metadata_keys();
    EXPECT_EQ(keys.size(), 3);

    // Verify key names (order may vary)
    std::set<std::string> key_set(keys.begin(), keys.end());
    EXPECT_TRUE(key_set.find("key1") != key_set.end());
    EXPECT_TRUE(key_set.find("key2") != key_set.end());
    EXPECT_TRUE(key_set.find("key3") != key_set.end());
}
