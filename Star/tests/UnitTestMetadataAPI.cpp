#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "star.h"
#include <cmath>


using namespace star;

class MetadataAPITest : public ::testing::Test {
protected:
    std::string testFile;

    void SetUp() override {
        testFile = "/tmp/test_metadata_api.star";

        // Remove the file if it already exists
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }
    }

    void TearDown() override {
        // Clean up test file
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }
    }
};

TEST_F(MetadataAPITest, TypeDiscoveryWithoutKnowingType) {
    auto store = StarDataset::create(testFile);

    // Write different types
    store->meta.put("scalar_int", NDArray<int64_t>({}, 42));
    store->meta.put("scalar_double", NDArray<double>({}, 3.14));
    store->meta.put("scalar_string", NDArray<std::string>({}, "hello"));
    store->flush();

    // Read without knowing the type
    auto meta_int = store->meta.get("scalar_int");
    ASSERT_NE(meta_int, nullptr);
    EXPECT_EQ(meta_int->type_name(), "int64");
    EXPECT_EQ(meta_int->dtype, DataType::INT64);
    EXPECT_EQ(meta_int->as<int64_t>().data()[0], 42);

    // Evaluate type and handle accordingly
    auto meta_unknown = store->meta.get("scalar_double");
    ASSERT_NE(meta_unknown, nullptr);
    if (meta_unknown->dtype == DataType::FLOAT64) {
        EXPECT_NEAR(meta_unknown->as<double>().data()[0], 3.14, 1e-10);
    }

    // Safe casting with try_as
    auto meta_str = store->meta.get("scalar_string");
    ASSERT_NE(meta_str, nullptr);
    if (auto str_arr = meta_str->try_as<std::string>()) {
        EXPECT_EQ(str_arr->data()[0], "hello");
    }
}

TEST_F(MetadataAPITest, BatchOperations) {
    auto store = StarDataset::create(testFile);

    // Batch put
    std::map<std::string, NDArray<double>> batch = {
        {"a", NDArray<double>({}, 1.0)},
        {"b", NDArray<double>({}, 2.0)},
        {"c", NDArray<double>({}, 3.0)}
    };
    store->meta.put_batch(batch);
    store->flush();

    // Batch get
    auto results = store->meta.get_batch({"a", "b", "c"});
    EXPECT_EQ(results.size(), 3);
    EXPECT_NEAR(results["a"].as<double>().data()[0], 1.0, 1e-10);
    EXPECT_NEAR(results["b"].as<double>().data()[0], 2.0, 1e-10);
    EXPECT_NEAR(results["c"].as<double>().data()[0], 3.0, 1e-10);
}

TEST_F(MetadataAPITest, RemoveAndClear) {
    auto store = StarDataset::create(testFile);

    store->meta.put("temp", NDArray<int32_t>({}, 100));
    EXPECT_TRUE(store->meta.contains("temp"));

    store->meta.remove("temp");
    EXPECT_FALSE(store->meta.contains("temp"));

    store->meta.put("a", NDArray<int32_t>({}, 1));
    store->meta.put("b", NDArray<int32_t>({}, 2));
    store->meta.clear();
    EXPECT_FALSE(store->meta.contains("a"));
    EXPECT_FALSE(store->meta.contains("b"));
}

// NOTE: BackwardsCompatibility and AutoRoutingStillWorks tests were removed
// because they tested the old put()/get() API which was removed in the simplification.
// Only the store->meta API is now supported.

TEST_F(MetadataAPITest, CheckScalarVsArray) {
    auto store = StarDataset::create(testFile);

    // Store scalar
    store->meta.put("scalar", NDArray<int64_t>({}, 42));

    // Store 1D array
    NDArray<double> arr1d({10});
    for (size_t i = 0; i < 10; ++i) {
        arr1d.flat(i) = static_cast<double>(i);
    }
    store->meta.put("array1d", arr1d);

    // Store 2D array
    NDArray<float> arr2d({5, 3});
    for (size_t i = 0; i < arr2d.size(); ++i) {
        arr2d.flat(i) = static_cast<float>(i);
    }
    store->meta.put("array2d", arr2d);

    store->flush();

    // Check scalar
    auto scalar = store->meta.get("scalar");
    ASSERT_NE(scalar, nullptr);
    EXPECT_TRUE(scalar->is_scalar());
    EXPECT_FALSE(scalar->is_array());
    EXPECT_EQ(scalar->ndim(), 0);
    EXPECT_EQ(scalar->size(), 1);

    // Check 1D array
    auto array1d = store->meta.get("array1d");
    ASSERT_NE(array1d, nullptr);
    EXPECT_FALSE(array1d->is_scalar());
    EXPECT_TRUE(array1d->is_array());
    EXPECT_EQ(array1d->ndim(), 1);
    EXPECT_EQ(array1d->size(), 10);
    EXPECT_EQ(array1d->shape[0], 10);

    // Check 2D array
    auto array2d = store->meta.get("array2d");
    ASSERT_NE(array2d, nullptr);
    EXPECT_FALSE(array2d->is_scalar());
    EXPECT_TRUE(array2d->is_array());
    EXPECT_EQ(array2d->ndim(), 2);
    EXPECT_EQ(array2d->size(), 15);
    EXPECT_EQ(array2d->shape[0], 5);
    EXPECT_EQ(array2d->shape[1], 3);
}

TEST_F(MetadataAPITest, WorkingWithArrayMetadata) {
    auto store = StarDataset::create(testFile);

    // Store a 2D matrix
    NDArray<double> matrix({3, 4});
    for (size_t i = 0; i < matrix.size(); ++i) {
        matrix.flat(i) = static_cast<double>(i) * 1.5;
    }
    store->meta.put("matrix", matrix);
    store->flush();

    // Read and process without knowing type ahead of time
    auto meta = store->meta.get("matrix");
    ASSERT_NE(meta, nullptr);

    // Check if it's an array
    if (meta->is_array()) {
        EXPECT_EQ(meta->ndim(), 2);
        EXPECT_EQ(meta->size(), 12);

        // Check type and cast
        if (meta->dtype == DataType::FLOAT64) {
            auto arr = meta->as<double>();
            EXPECT_EQ(arr.shape(0), 3);
            EXPECT_EQ(arr.shape(1), 4);

            // Access elements
            for (size_t i = 0; i < arr.size(); ++i) {
                EXPECT_NEAR(arr.flat(i), static_cast<double>(i) * 1.5, 1e-10);
            }
        }
    }
}

TEST_F(MetadataAPITest, IterateOverAllMetadataWithTypeChecking) {
    auto store = StarDataset::create(testFile);

    // Store mixed metadata
    store->meta.put("id", NDArray<int64_t>({}, 1001));
    store->meta.put("name", NDArray<std::string>({}, "camera_A"));

    NDArray<double> calibration({9});
    double calib_data[] = {1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0};
    for (size_t i = 0; i < 9; ++i) {
        calibration.flat(i) = calib_data[i];
    }
    store->meta.put("calibration", calibration);

    NDArray<int32_t> image_dims({2});
    image_dims.flat(0) = 1920;
    image_dims.flat(1) = 1080;
    store->meta.put("image_dims", image_dims);

    store->flush();

    // Get all keys
    std::vector<std::string> keys = {"id", "name", "calibration", "image_dims"};

    for (const auto& key : keys) {
        auto meta = store->meta.get(key);
        ASSERT_NE(meta, nullptr) << "Key not found: " << key;

        // Process based on type
        if (meta->dtype == DataType::INT64 && meta->is_scalar()) {
            EXPECT_EQ(meta->as<int64_t>().data()[0], 1001);
        } else if (meta->dtype == DataType::STRING && meta->is_scalar()) {
            EXPECT_EQ(meta->as<std::string>().data()[0], "camera_A");
        } else if (meta->dtype == DataType::FLOAT64 && meta->is_array()) {
            auto arr = meta->as<double>();
            EXPECT_EQ(arr.size(), 9);
        } else if (meta->dtype == DataType::INT32 && meta->is_array()) {
            auto arr = meta->as<int32_t>();
            EXPECT_EQ(arr.size(), 2);
            EXPECT_EQ(arr.flat(0), 1920);
            EXPECT_EQ(arr.flat(1), 1080);
        }
    }
}

TEST_F(MetadataAPITest, GetAllMetadata) {
    auto store = StarDataset::create(testFile);

    // Put several metadata entries
    store->meta.put("key1", NDArray<int32_t>({}, 42));
    store->meta.put("key2", NDArray<double>({}, 3.14));
    store->meta.put("key3", NDArray<std::string>({}, "hello"));

    NDArray<int32_t> arr({3});
    arr.flat(0) = 1; arr.flat(1) = 2; arr.flat(2) = 3;
    store->meta.put("key4", arr);

    store->flush();

    // Test get_all on same instance
    auto all_meta = store->meta.get_all();
    EXPECT_EQ(all_meta.size(), 4);
    EXPECT_TRUE(all_meta.count("key1") > 0);
    EXPECT_TRUE(all_meta.count("key2") > 0);
    EXPECT_TRUE(all_meta.count("key3") > 0);
    EXPECT_TRUE(all_meta.count("key4") > 0);

    // Verify values
    EXPECT_EQ(all_meta["key1"].as<int32_t>().flat(0), 42);
    EXPECT_NEAR(all_meta["key2"].as<double>().flat(0), 3.14, 1e-10);
    EXPECT_EQ(all_meta["key3"].as<std::string>().flat(0), "hello");
    EXPECT_EQ(all_meta["key4"].as<int32_t>().flat(0), 1);
    EXPECT_EQ(all_meta["key4"].as<int32_t>().flat(1), 2);
    EXPECT_EQ(all_meta["key4"].as<int32_t>().flat(2), 3);

    // Test get_all after reopening (verify disk read and caching)
    store.reset();

    auto store2 = StarDataset::open(testFile);
    // In v1 format, base layer metadata is eagerly loaded during open
    EXPECT_TRUE(store2->is_metadata_loaded());  // Already loaded

    auto all_meta2 = store2->meta.get_all();
    EXPECT_TRUE(store2->is_metadata_loaded());  // Still loaded
    EXPECT_EQ(all_meta2.size(), 4);

    // Verify values after reload
    EXPECT_EQ(all_meta2["key1"].as<int32_t>().flat(0), 42);
    EXPECT_NEAR(all_meta2["key2"].as<double>().flat(0), 3.14, 1e-10);

    // Call again to verify no redundant disk read (cached)
    auto all_meta3 = store2->meta.get_all();
    EXPECT_EQ(all_meta3.size(), 4);
    EXPECT_EQ(all_meta3["key1"].as<int32_t>().flat(0), 42);
}
