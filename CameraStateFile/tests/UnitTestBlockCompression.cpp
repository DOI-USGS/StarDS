#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "scs.h"
#include <cmath>

class BlockCompressionTest : public ::testing::Test {
protected:
    std::string testFile;

    void SetUp() override {
        testFile = "/tmp/test_blocks.scs";

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

TEST_F(BlockCompressionTest, Store1DDoubleArrayWithGZIP) {
    // Create a store with GZIP compression and 1KB block size
    SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

    // Create a 1D array of doubles
    ndarray<double> arr({100});
    for (size_t i = 0; i < 100; ++i) {
        arr.flat(i) = static_cast<double>(i) * 1.5;
    }

    // Store the array
    store.put("double_array", arr);

    // Verify the key exists
    EXPECT_TRUE(store.contains("double_array"));

    // Flush to disk
    store.flush();

    // Read back the data
    auto retrieved = store.get<ndarray<double>>("double_array");
    ASSERT_NE(retrieved, nullptr);

    // Verify shape
    ASSERT_EQ(retrieved->shape().size(), 1);
    EXPECT_EQ(retrieved->shape(0), 100);

    // Verify data integrity
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_NEAR(retrieved->flat(i), static_cast<double>(i) * 1.5, 1e-10);
    }
}

TEST_F(BlockCompressionTest, Store2DInt32MatrixWithGZIP) {
    // Create a store with GZIP compression and 1KB block size
    SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

    // Create a 2D array of int32
    ndarray<int32_t> arr({10, 20});
    for (size_t i = 0; i < arr.size(); ++i) {
        arr.flat(i) = static_cast<int32_t>(i);
    }

    // Store the array
    store.put("int32_matrix", arr);

    // Verify the key exists
    EXPECT_TRUE(store.contains("int32_matrix"));

    // Flush to disk
    store.flush();

    // Read back the data
    auto retrieved = store.get<ndarray<int32_t>>("int32_matrix");
    ASSERT_NE(retrieved, nullptr);

    // Verify shape
    ASSERT_EQ(retrieved->shape().size(), 2);
    EXPECT_EQ(retrieved->shape(0), 10);
    EXPECT_EQ(retrieved->shape(1), 20);

    // Verify data integrity
    for (size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(retrieved->flat(i), static_cast<int32_t>(i));
    }
}

TEST_F(BlockCompressionTest, Store3DFloat32TensorUncompressed) {
    // Create a store with GZIP compression and 1KB block size
    SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

    // Create a 3D array with explicit no compression
    ndarray<float> arr({5, 4, 3});
    for (size_t i = 0; i < arr.size(); ++i) {
        arr.flat(i) = static_cast<float>(i) / 10.0f;
    }

    // Store with explicit no compression
    store.put("float32_tensor", arr, CompressionAlgorithm::NONE);

    // Verify the key exists
    EXPECT_TRUE(store.contains("float32_tensor"));

    // Flush to disk
    store.flush();

    // Read back the data
    auto retrieved = store.get<ndarray<float>>("float32_tensor");
    ASSERT_NE(retrieved, nullptr);

    // Verify shape
    ASSERT_EQ(retrieved->shape().size(), 3);
    EXPECT_EQ(retrieved->shape(0), 5);
    EXPECT_EQ(retrieved->shape(1), 4);
    EXPECT_EQ(retrieved->shape(2), 3);

    // Verify data integrity
    for (size_t i = 0; i < arr.size(); ++i) {
        EXPECT_NEAR(retrieved->flat(i), static_cast<float>(i) / 10.0f, 1e-6f);
    }
}

TEST_F(BlockCompressionTest, MixedCompressionMultipleKeys) {
    // Create a store with GZIP compression and 1KB block size
    SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

    // Store multiple arrays with different compression settings
    ndarray<double> arr1({100});
    for (size_t i = 0; i < 100; ++i) {
        arr1.flat(i) = static_cast<double>(i) * 1.5;
    }
    store.put("double_array", arr1);

    ndarray<int32_t> arr2({10, 20});
    for (size_t i = 0; i < arr2.size(); ++i) {
        arr2.flat(i) = static_cast<int32_t>(i);
    }
    store.put("int32_matrix", arr2);

    ndarray<float> arr3({5, 4, 3});
    for (size_t i = 0; i < arr3.size(); ++i) {
        arr3.flat(i) = static_cast<float>(i) / 10.0f;
    }
    store.put("float32_tensor", arr3, CompressionAlgorithm::NONE);

    // Flush to disk
    store.flush();

    // Verify all keys exist
    EXPECT_TRUE(store.contains("double_array"));
    EXPECT_TRUE(store.contains("int32_matrix"));
    EXPECT_TRUE(store.contains("float32_tensor"));

    // Read back and verify all data
    auto read_arr1 = store.get<ndarray<double>>("double_array");
    ASSERT_NE(read_arr1, nullptr);
    EXPECT_EQ(read_arr1->shape().size(), 1);
    EXPECT_EQ(read_arr1->shape(0), 100);
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_NEAR(read_arr1->flat(i), static_cast<double>(i) * 1.5, 1e-10);
    }

    auto read_arr2 = store.get<ndarray<int32_t>>("int32_matrix");
    ASSERT_NE(read_arr2, nullptr);
    EXPECT_EQ(read_arr2->shape().size(), 2);
    EXPECT_EQ(read_arr2->shape(0), 10);
    EXPECT_EQ(read_arr2->shape(1), 20);
    for (size_t i = 0; i < arr2.size(); ++i) {
        EXPECT_EQ(read_arr2->flat(i), static_cast<int32_t>(i));
    }

    auto read_arr3 = store.get<ndarray<float>>("float32_tensor");
    ASSERT_NE(read_arr3, nullptr);
    EXPECT_EQ(read_arr3->shape().size(), 3);
    EXPECT_EQ(read_arr3->shape(0), 5);
    EXPECT_EQ(read_arr3->shape(1), 4);
    EXPECT_EQ(read_arr3->shape(2), 3);
    for (size_t i = 0; i < arr3.size(); ++i) {
        EXPECT_NEAR(read_arr3->flat(i), static_cast<float>(i) / 10.0f, 1e-6f);
    }
}

TEST_F(BlockCompressionTest, VerifyBlockMetadata) {
    // Create a store with GZIP compression and 1KB block size
    SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

    // Store an array
    ndarray<int32_t> arr({10, 20});
    for (size_t i = 0; i < arr.size(); ++i) {
        arr.flat(i) = static_cast<int32_t>(i);
    }
    store.put("int32_matrix", arr);
    store.flush();

    // Verify metadata is present
    ASSERT_TRUE(store.m_index.find("int32_matrix") != store.m_index.end());

    const auto& entry = store.m_index["int32_matrix"];

    // Check datatype
    EXPECT_EQ(entry.datatype, DataType::INT32);

    // Check shape
    ASSERT_EQ(entry.shape.size(), 2);
    EXPECT_EQ(entry.shape[0], 10);
    EXPECT_EQ(entry.shape[1], 20);

    // Check compression
    EXPECT_EQ(entry.compression, CompressionAlgorithm::GZIP);

    // Check block size
    EXPECT_EQ(entry.block_size, 1024);

    // Check that blocks were created
    EXPECT_GT(entry.blocks.size(), 0);

    // Verify block metadata
    for (const auto& block : entry.blocks) {
        EXPECT_GT(block.compressed_size, 0);
        EXPECT_GT(block.uncompressed_size, 0);
        // Compressed should be smaller or equal to uncompressed
        EXPECT_LE(block.compressed_size, block.uncompressed_size);
    }
}

TEST_F(BlockCompressionTest, CompressionRatio) {
    // Create a store with GZIP compression and 1KB block size
    SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

    // Store a large array with pattern data (compressible)
    ndarray<double> arr({1000});
    for (size_t i = 0; i < 1000; ++i) {
        arr.flat(i) = static_cast<double>(i % 10);  // Repeating pattern
    }
    store.put("compressible_data", arr);
    store.flush();

    // Get metadata
    const auto& entry = store.m_index["compressible_data"];

    // Calculate total compressed and uncompressed sizes
    size_t total_compressed = 0;
    size_t total_uncompressed = 0;
    for (const auto& block : entry.blocks) {
        total_compressed += block.compressed_size;
        total_uncompressed += block.uncompressed_size;
    }

    // Verify compression is actually happening
    EXPECT_LT(total_compressed, total_uncompressed);

    // Calculate compression ratio (should be significant for this pattern)
    double ratio = static_cast<double>(total_compressed) / total_uncompressed;
    EXPECT_LT(ratio, 0.8);  // Should compress to less than 80% of original
}
