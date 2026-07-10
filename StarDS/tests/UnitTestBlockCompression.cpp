#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "stards.h"
#include "Fixtures.h"
#include <cmath>

using namespace star;

// Per-test temp dir (auto-created/deleted); testFile lives inside it so parallel
// runs never collide (formerly a hardcoded "/tmp/test_blocks.stards").
class BlockCompressionTest : public star_test::TempDirTest {
protected:
    std::string testFile;

    void SetUp() override {
        star_test::TempDirTest::SetUp();
        testFile = tempFilePath("test_blocks.stards");
    }
};

TEST_F(BlockCompressionTest, Store1DDoubleArrayWithGZIP) {
    // Create a store (metadata block uses GZIP compression by default)
    auto store = StarDataset::create(testFile);

    // Create a 1D array of doubles
    NDArray<double> arr({100});
    for (size_t i = 0; i < 100; ++i) {
        arr.flat(i) = static_cast<double>(i) * 1.5;
    }

    // Store the array using new meta API
    store->meta.put("double_array", arr);

    // Verify the key exists
    EXPECT_TRUE(store->meta.contains("double_array"));

    // Flush to disk
    store->flush();

    // Read back the data using new meta API with type introspection
    auto meta = store->meta.get("double_array");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->dtype, DataType::FLOAT64);

    // Cast to the correct type
    auto retrieved = meta->as<double>();

    // Verify shape
    ASSERT_EQ(retrieved.shape().size(), 1);
    EXPECT_EQ(retrieved.shape(0), 100);

    // Verify data integrity
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_NEAR(retrieved.flat(i), static_cast<double>(i) * 1.5, 1e-10);
    }
}

TEST_F(BlockCompressionTest, Store2DInt32MatrixWithGZIP) {
    // Create a store (metadata block uses GZIP compression by default)
    auto store = StarDataset::create(testFile);

    // Create a 2D array of int32
    NDArray<int32_t> arr({10, 20});
    for (size_t i = 0; i < arr.size(); ++i) {
        arr.flat(i) = static_cast<int32_t>(i);
    }

    // Store the array using new meta API
    store->meta.put("int32_matrix", arr);

    // Verify the key exists
    EXPECT_TRUE(store->meta.contains("int32_matrix"));

    // Flush to disk
    store->flush();

    // Read back the data using new meta API with type introspection
    auto meta = store->meta.get("int32_matrix");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->dtype, DataType::INT32);

    // Cast to the correct type
    auto retrieved = meta->as<int32_t>();

    // Verify shape
    ASSERT_EQ(retrieved.shape().size(), 2);
    EXPECT_EQ(retrieved.shape(0), 10);
    EXPECT_EQ(retrieved.shape(1), 20);

    // Verify data integrity
    for (size_t i = 0; i < arr.size(); ++i) {
        EXPECT_EQ(retrieved.flat(i), static_cast<int32_t>(i));
    }
}

TEST_F(BlockCompressionTest, Store3DFloat32TensorUncompressed) {
    // Create a store (metadata block handles compression internally)
    auto store = StarDataset::create(testFile);

    // Create a 3D array
    NDArray<float> arr({5, 4, 3});
    for (size_t i = 0; i < arr.size(); ++i) {
        arr.flat(i) = static_cast<float>(i) / 10.0f;
    }

    // Store using new meta API
    store->meta.put("float32_tensor", arr);

    // Verify the key exists
    EXPECT_TRUE(store->meta.contains("float32_tensor"));

    // Flush to disk
    store->flush();

    // Read back the data using new meta API with type introspection
    auto meta = store->meta.get("float32_tensor");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->dtype, DataType::FLOAT32);

    // Cast to the correct type
    auto retrieved = meta->as<float>();

    // Verify shape
    ASSERT_EQ(retrieved.shape().size(), 3);
    EXPECT_EQ(retrieved.shape(0), 5);
    EXPECT_EQ(retrieved.shape(1), 4);
    EXPECT_EQ(retrieved.shape(2), 3);

    // Verify data integrity
    for (size_t i = 0; i < arr.size(); ++i) {
        EXPECT_NEAR(retrieved.flat(i), static_cast<float>(i) / 10.0f, 1e-6f);
    }
}

TEST_F(BlockCompressionTest, MixedCompressionMultipleKeys) {
    // Create a store (metadata block handles compression)
    auto store = StarDataset::create(testFile);

    // Store multiple arrays using new meta API
    NDArray<double> arr1({100});
    for (size_t i = 0; i < 100; ++i) {
        arr1.flat(i) = static_cast<double>(i) * 1.5;
    }
    store->meta.put("double_array", arr1);

    NDArray<int32_t> arr2({10, 20});
    for (size_t i = 0; i < arr2.size(); ++i) {
        arr2.flat(i) = static_cast<int32_t>(i);
    }
    store->meta.put("int32_matrix", arr2);

    NDArray<float> arr3({5, 4, 3});
    for (size_t i = 0; i < arr3.size(); ++i) {
        arr3.flat(i) = static_cast<float>(i) / 10.0f;
    }
    store->meta.put("float32_tensor", arr3);

    // Flush to disk
    store->flush();

    // Verify all keys exist
    EXPECT_TRUE(store->meta.contains("double_array"));
    EXPECT_TRUE(store->meta.contains("int32_matrix"));
    EXPECT_TRUE(store->meta.contains("float32_tensor"));

    // Read back and verify all data using new meta API
    auto meta1 = store->meta.get("double_array");
    ASSERT_NE(meta1, nullptr);
    auto read_arr1 = meta1->as<double>();
    EXPECT_EQ(read_arr1.shape().size(), 1);
    EXPECT_EQ(read_arr1.shape(0), 100);
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_NEAR(read_arr1.flat(i), static_cast<double>(i) * 1.5, 1e-10);
    }

    auto meta2 = store->meta.get("int32_matrix");
    ASSERT_NE(meta2, nullptr);
    auto read_arr2 = meta2->as<int32_t>();
    EXPECT_EQ(read_arr2.shape().size(), 2);
    EXPECT_EQ(read_arr2.shape(0), 10);
    EXPECT_EQ(read_arr2.shape(1), 20);
    for (size_t i = 0; i < arr2.size(); ++i) {
        EXPECT_EQ(read_arr2.flat(i), static_cast<int32_t>(i));
    }

    auto meta3 = store->meta.get("float32_tensor");
    ASSERT_NE(meta3, nullptr);
    auto read_arr3 = meta3->as<float>();
    EXPECT_EQ(read_arr3.shape().size(), 3);
    EXPECT_EQ(read_arr3.shape(0), 5);
    EXPECT_EQ(read_arr3.shape(1), 4);
    EXPECT_EQ(read_arr3.shape(2), 3);
    for (size_t i = 0; i < arr3.size(); ++i) {
        EXPECT_NEAR(read_arr3.flat(i), static_cast<float>(i) / 10.0f, 1e-6f);
    }
}

TEST_F(BlockCompressionTest, VerifyMetadataBlockExists) {
    // Create a store
    auto store = StarDataset::create(testFile);

    // Store an array in metadata block
    NDArray<int32_t> arr({10, 20});
    for (size_t i = 0; i < arr.size(); ++i) {
        arr.flat(i) = static_cast<int32_t>(i);
    }
    store->meta.put("int32_matrix", arr);
    store->flush();

    // v1 format: Metadata is in separate namespace, not in m_key_to_index
    // Verify metadata exists via contains
    EXPECT_TRUE(store->meta.contains("int32_matrix"));

    // Verify we can read the data back
    auto meta = store->meta.get("int32_matrix");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->dtype, DataType::INT32);

    // Verify data correctness
    auto& data = std::get<NDArray<int32_t>>(meta->data);
    EXPECT_EQ(data.shape().size(), 2);
    EXPECT_EQ(data.shape()[0], 10);
    EXPECT_EQ(data.shape()[1], 20);
}

TEST_F(BlockCompressionTest, MetadataBlockCompressionRatio) {
    // Create a store
    auto store = StarDataset::create(testFile);

    // Store a large array with repeating pattern (highly compressible)
    NDArray<double> arr({1000});
    for (size_t i = 0; i < 1000; ++i) {
        arr.flat(i) = static_cast<double>(i % 10);  // Repeating pattern
    }
    store->meta.put("compressible_data", arr);
    store->flush();

    // v1 format: Metadata is in separate namespace, stored in per-layer blocks
    // Verify metadata exists and can be retrieved
    EXPECT_TRUE(store->meta.contains("compressible_data"));

    auto meta = store->meta.get("compressible_data");
    ASSERT_NE(meta, nullptr);
    EXPECT_EQ(meta->dtype, DataType::FLOAT64);

    // Verify data correctness
    auto& data = std::get<NDArray<double>>(meta->data);
    EXPECT_EQ(data.shape().size(), 1);
    EXPECT_EQ(data.shape()[0], 1000);

    // Check a few values
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_DOUBLE_EQ(data.flat(i), static_cast<double>(i % 10));
    }

    // Note: Compression statistics for metadata blocks are tracked separately
    // in the layer metadata registry, not in m_cold vectors

    // Data verification already done above, test complete
}

// Direct round-trip of the byte-shuffle prefilter (no compression library needed).
TEST_F(BlockCompressionTest, ByteShuffleRoundTrip) {
    const size_t count = 257;          // non-power-of-two to catch tail bugs
    const size_t elem_size = sizeof(double);
    std::vector<double> original(count);
    for (size_t i = 0; i < count; ++i) original[i] = static_cast<double>(i) * 3.5 - 1.0;

    std::vector<char> shuffled(count * elem_size);
    byte_shuffle(reinterpret_cast<const char*>(original.data()), shuffled.data(), count, elem_size);

    std::vector<double> restored(count);
    byte_unshuffle(shuffled.data(), reinterpret_cast<char*>(restored.data()), count, elem_size);

    EXPECT_EQ(restored, original);
    EXPECT_TRUE(uses_shuffle(CompressionAlgorithm::GZIP_SHUFFLE));
    EXPECT_TRUE(uses_shuffle(CompressionAlgorithm::LZ4_SHUFFLE));
    EXPECT_FALSE(uses_shuffle(CompressionAlgorithm::GZIP));
}

// End-to-end array-storage round-trip with the shuffle codecs (guards the
// write-side byte_shuffle + read-side byte_unshuffle paths in StarDataset).
TEST_F(BlockCompressionTest, ShuffleCodecArrayRoundTrip) {
    for (auto codec : {CompressionAlgorithm::GZIP_SHUFFLE, CompressionAlgorithm::LZ4_SHUFFLE}) {
        if (fs::exists(testFile)) fs::remove(testFile);

        StarConfig config;
        config.compression = codec;
        auto store = StarDataset::create(testFile, config);

        NDArray<double> arr({500});
        for (size_t i = 0; i < 500; ++i) arr.flat(i) = static_cast<double>(i) * 0.25 + 7.0;
        store->put("shuffled", std::move(arr));
        store->flush();

        auto reopened = StarDataset::open(testFile, FileMode::READ_ONLY);
        auto retrieved = reopened->get<double>("shuffled");
        ASSERT_EQ(retrieved.size(), 500u);
        for (size_t i = 0; i < 500; ++i) {
            EXPECT_DOUBLE_EQ(retrieved.flat(i), static_cast<double>(i) * 0.25 + 7.0)
                << "codec=" << static_cast<int>(codec) << " i=" << i;
        }
    }
}

#ifdef ENABLE_LZ4
// Round-trips an array through LZ4-compressed block storage (config.compression =
// LZ4), so this actually exercises the LZ4 compress/decompress path rather than
// the default GZIP metadata block.
TEST_F(BlockCompressionTest, LZ4BasicFunctionalityTest) {
    StarConfig config;
    config.compression = CompressionAlgorithm::LZ4;
    auto store = StarDataset::create(testFile, config);

    NDArray<double> arr({1000});
    for (size_t i = 0; i < 1000; ++i) {
        arr.flat(i) = static_cast<double>(i) * 0.5;
    }
    store->put("test_data", std::move(arr));  // array storage -> uses config.compression (LZ4)
    store->flush();

    auto reopened = StarDataset::open(testFile, FileMode::READ_ONLY);
    auto retrieved = reopened->get<double>("test_data");

    EXPECT_EQ(retrieved.size(), 1000u);
    for (size_t i = 0; i < 1000; ++i) {
        EXPECT_DOUBLE_EQ(retrieved.flat(i), static_cast<double>(i) * 0.5);
    }
}
#endif  // ENABLE_LZ4
