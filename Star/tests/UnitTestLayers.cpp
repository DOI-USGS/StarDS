/**
 * @file UnitTestLayers.cpp
 * @brief Unit tests for layer functionality and inheritance
 */

#include <gtest/gtest.h>
#include "star.h"
#include <iostream>
#include <sstream>
#include <ghc/fs_std.hpp>
#include <random>
#include <chrono>
#include <thread>

using namespace star;

//==============================================================================
// Test Fixture for Layer Tests
//==============================================================================

class LayerTest : public ::testing::Test {
protected:
    std::vector<std::string> temp_files;

    void SetUp() override {
        // Nothing to do in setup
    }

    void TearDown() override {
        // Clean up all temporary files created during the test
        for (const auto& file : temp_files) {
            if (fs::exists(file)) {
                try {
                    fs::remove(file);
                } catch (const std::exception& e) {
                    std::cerr << "Warning: Failed to delete temp file " << file
                              << ": " << e.what() << std::endl;
                }
            }
        }
    }

    std::string createTempFile(const std::string& prefix = "test_layer") {
        // Use thread-safe random generator with high-resolution timestamp
        static std::random_device rd;
        static thread_local std::mt19937_64 gen(rd() + std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_int_distribution<uint64_t> dis;

        auto now = std::chrono::high_resolution_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        uint64_t random_val = dis(gen);

        std::stringstream ss;
        ss << "/tmp/" << prefix << "_" << timestamp << "_" << std::hex << random_val << ".star";
        std::string filename = ss.str();

        temp_files.push_back(filename);
        return filename;
    }
};

//==============================================================================
// Basic Layer Creation and Retrieval Tests
//==============================================================================

TEST_F(LayerTest, CreateLayer) {
    std::string filename = createTempFile("create_layer");

    auto store = StarDataset::create(filename);

    // Create a layer
    auto layer1 = store->create_layer("band_0");

    // Verify layer properties
    ASSERT_NE(layer1, nullptr);
    EXPECT_EQ(layer1->name(), "band_0");
    EXPECT_EQ(layer1->base(), store.get());
}

TEST_F(LayerTest, GetExistingLayer) {
    std::string filename = createTempFile("get_layer");

    auto store = StarDataset::create(filename);
    store->create_layer("band_0");

    // Retrieve the layer
    auto layer1 = store->get_layer("band_0");

    ASSERT_NE(layer1, nullptr);
    EXPECT_EQ(layer1->name(), "band_0");
}

TEST_F(LayerTest, GetNonExistentLayerThrows) {
    std::string filename = createTempFile("get_nonexistent");

    auto store = StarDataset::create(filename);

    // Should throw when layer doesn't exist
    EXPECT_THROW({
        auto layer = store->get_layer("nonexistent");
    }, std::runtime_error);
}

TEST_F(LayerTest, CreateDuplicateLayerThrows) {
    std::string filename = createTempFile("duplicate_layer");

    auto store = StarDataset::create(filename);
    store->create_layer("band_0");

    // Should throw when creating duplicate
    EXPECT_THROW({
        store->create_layer("band_0");
    }, std::runtime_error);
}

TEST_F(LayerTest, HasLayer) {
    std::string filename = createTempFile("has_layer");

    auto store = StarDataset::create(filename);

    // Initially no layers
    EXPECT_FALSE(store->has_layer("band_0"));

    // Create layer
    store->create_layer("band_0");

    // Now exists
    EXPECT_TRUE(store->has_layer("band_0"));
    EXPECT_FALSE(store->has_layer("band_1"));
}

TEST_F(LayerTest, ListLayers) {
    std::string filename = createTempFile("list_layers");

    auto store = StarDataset::create(filename);

    // Initially empty
    auto layers = store->list_layers();
    EXPECT_EQ(layers.size(), 0);

    // Create multiple layers
    store->create_layer("band_0");
    store->create_layer("band_1");
    store->create_layer("band_2");

    // Verify all layers listed
    layers = store->list_layers();
    EXPECT_EQ(layers.size(), 3);
    EXPECT_NE(std::find(layers.begin(), layers.end(), "band_0"), layers.end());
    EXPECT_NE(std::find(layers.begin(), layers.end(), "band_1"), layers.end());
    EXPECT_NE(std::find(layers.begin(), layers.end(), "band_2"), layers.end());
}

//==============================================================================
// Layer Inheritance Tests
//==============================================================================

TEST_F(LayerTest, LayerInheritsBaseKeys) {
    std::string filename = createTempFile("inherit_keys");

    auto store = StarDataset::create(filename);

    // Add base metadata
    store->meta.put("instrument", NDArray<std::string>({}, "AVIRIS"));
    store->meta.put("date", NDArray<std::string>({}, "2026-04-22"));

    // Add base array
    NDArray<double> cube({10, 10, 10});
    for (size_t i = 0; i < cube.size(); ++i) {
        cube.data()[i] = i * 1.5;
    }
    store->put("cube", cube);

    store->flush();

    // Create layer
    auto layer1 = store->create_layer("band_0");

    // Layer should see base keys
    auto layer_keys = layer1->keys();
    EXPECT_NE(std::find(layer_keys.begin(), layer_keys.end(), "instrument"), layer_keys.end());
    EXPECT_NE(std::find(layer_keys.begin(), layer_keys.end(), "date"), layer_keys.end());
    EXPECT_NE(std::find(layer_keys.begin(), layer_keys.end(), "cube"), layer_keys.end());
}

TEST_F(LayerTest, LayerContainsBaseKeys) {
    std::string filename = createTempFile("contains_base");

    auto store = StarDataset::create(filename);

    // Add base key
    store->meta.put("base_key", NDArray<int64_t>({}, 42));
    store->flush();

    // Create layer
    auto layer1 = store->create_layer("band_0");

    // Contains should find inherited key
    EXPECT_TRUE(layer1->contains("base_key"));
}

TEST_F(LayerTest, BitMaskPresenceCheck) {
    std::string filename = createTempFile("bitmask");

    auto store = StarDataset::create(filename);

    // Add multiple keys to base
    for (int i = 0; i < 100; ++i) {
        std::string key = "key_" + std::to_string(i);
        store->meta.put(key, NDArray<int64_t>({}, i));
    }
    store->flush();

    // Create layer
    auto layer1 = store->create_layer("band_0");

    // All keys should be in base (not in layer)
    for (int i = 0; i < 100; ++i) {
        std::string key = "key_" + std::to_string(i);
        EXPECT_TRUE(store->key_in_layer(key, "__base__"));
        EXPECT_FALSE(store->key_in_layer(key, "band_0"));

        // But layer should contain them via inheritance
        EXPECT_TRUE(layer1->contains(key));
    }
}

//==============================================================================
// Persistence Tests
//==============================================================================

TEST_F(LayerTest, LayersPersistAcrossCloseReopen) {
    std::string filename = createTempFile("persist_layers");

    // Create and write
    {
        auto store = StarDataset::create(filename);
        store->meta.put("instrument", NDArray<std::string>({}, "AVIRIS"));

        store->create_layer("band_0");
        store->create_layer("band_1");
        store->create_layer("band_2");

        store->flush();
    }

    // Reopen and verify
    {
        auto store = StarDataset::open(filename, "r");

        auto layers = store->list_layers();
        EXPECT_EQ(layers.size(), 3);
        EXPECT_TRUE(store->has_layer("band_0"));
        EXPECT_TRUE(store->has_layer("band_1"));
        EXPECT_TRUE(store->has_layer("band_2"));

        // Verify inheritance still works
        auto layer1 = store->get_layer("band_1");
        EXPECT_TRUE(layer1->contains("instrument"));
    }
}

TEST_F(LayerTest, LayerInfoInFileHeader) {
    std::string filename = createTempFile("layer_header");

    // Create with layers
    {
        auto store = StarDataset::create(filename);
        store->create_layer("band_0");
        store->create_layer("band_1");
        store->flush();
    }

    // Reopen and check header
    {
        auto store = StarDataset::open(filename, "r");
        const FileHeader& header = store->getFileHeader();

        // Format version should be 1 (v1 format with per-layer metadata blocks)
        EXPECT_EQ(header.format_version, 1);

        // Layer count should be correct
        EXPECT_EQ(header.layer_count, 2);
    }
}

//==============================================================================
// Performance Tests with Many Layers
//==============================================================================

TEST_F(LayerTest, CreateManyLayers) {
    std::string filename = createTempFile("many_layers");

    auto store = StarDataset::create(filename);

    // Create 300 layers (hyperspectral use case)
    for (int i = 0; i < 300; ++i) {
        std::string layer_name = "band_" + std::to_string(i);
        auto layer = store->create_layer(layer_name);
        ASSERT_NE(layer, nullptr);
        EXPECT_EQ(layer->name(), layer_name);
    }

    // Verify all exist
    auto layers = store->list_layers();
    EXPECT_EQ(layers.size(), 300);

    // Check a few specific ones
    EXPECT_TRUE(store->has_layer("band_0"));
    EXPECT_TRUE(store->has_layer("band_149"));
    EXPECT_TRUE(store->has_layer("band_299"));
}

TEST_F(LayerTest, ManyLayersWithInheritance) {
    std::string filename = createTempFile("many_inherit");

    auto store = StarDataset::create(filename);

    // Add base metadata
    store->meta.put("instrument", NDArray<std::string>({}, "AVIRIS"));
    store->meta.put("scene", NDArray<std::string>({}, "Yellowstone"));
    store->flush();

    // Create 100 layers
    for (int i = 0; i < 100; ++i) {
        std::string layer_name = "band_" + std::to_string(i);
        auto layer = store->create_layer(layer_name);

        // Each layer should inherit base metadata
        EXPECT_TRUE(layer->contains("instrument"));
        EXPECT_TRUE(layer->contains("scene"));
    }

    // Before flushing, check layers are created
    auto layers_before_flush = store->list_layers();
    ASSERT_EQ(layers_before_flush.size(), 100) << "Expected 100 layers before flush";

    store->close();  // Explicit close (calls flush)
    store.reset();   // Delete the object

    // Reopen and verify inheritance persists
    store = StarDataset::open(filename, "r");

    // Debug: Check file header
    const FileHeader& header = store->getFileHeader();
    std::cout << "File header after reopen:" << std::endl;
    std::cout << "  format_version: " << static_cast<int>(header.format_version) << std::endl;
    std::cout << "  layer_count: " << header.layer_count << std::endl;

    // Debug: Check what layers exist
    auto layers_after_reopen = store->list_layers();
    ASSERT_GT(layers_after_reopen.size(), 50) << "Expected at least 51 layers, got " << layers_after_reopen.size();

    auto layer50 = store->get_layer("band_50");
    EXPECT_TRUE(layer50->contains("instrument"));
    EXPECT_TRUE(layer50->contains("scene"));
}

//==============================================================================
// Edge Cases and Error Handling
//==============================================================================

TEST_F(LayerTest, EmptyLayerName) {
    std::string filename = createTempFile("empty_name");

    auto store = StarDataset::create(filename);

    // Empty layer name should work (though not recommended)
    auto layer = store->create_layer("");
    EXPECT_EQ(layer->name(), "");
}

TEST_F(LayerTest, LongLayerName) {
    std::string filename = createTempFile("long_name");

    auto store = StarDataset::create(filename);

    // Very long layer name
    std::string long_name(1000, 'x');
    auto layer = store->create_layer(long_name);
    EXPECT_EQ(layer->name(), long_name);
}

TEST_F(LayerTest, SpecialCharactersInLayerName) {
    std::string filename = createTempFile("special_chars");

    auto store = StarDataset::create(filename);

    // Layer names with special characters
    auto layer1 = store->create_layer("band-0");
    auto layer2 = store->create_layer("band_0");
    auto layer3 = store->create_layer("band.0");
    auto layer4 = store->create_layer("band:0");

    EXPECT_EQ(layer1->name(), "band-0");
    EXPECT_EQ(layer2->name(), "band_0");
    EXPECT_EQ(layer3->name(), "band.0");
    EXPECT_EQ(layer4->name(), "band:0");
}

TEST_F(LayerTest, KeyInLayerWithInvalidKey) {
    std::string filename = createTempFile("invalid_key");

    auto store = StarDataset::create(filename);
    auto layer = store->create_layer("band_0");

    // Non-existent key should return false
    EXPECT_FALSE(store->key_in_layer("nonexistent", "band_0"));
    EXPECT_FALSE(store->key_in_layer("nonexistent", "__base__"));
}

TEST_F(LayerTest, KeyInLayerWithInvalidLayer) {
    std::string filename = createTempFile("invalid_layer");

    auto store = StarDataset::create(filename);
    store->meta.put("key1", NDArray<int64_t>({}, 42));

    // Valid key but invalid layer
    EXPECT_FALSE(store->key_in_layer("key1", "nonexistent_layer"));
}

//==============================================================================
// FileHeader Layer Count Tests
//==============================================================================

TEST_F(LayerTest, FileHeaderLayerCountZero) {
    std::string filename = createTempFile("layer_count_zero");

    // Create without layers
    {
        auto store = StarDataset::create(filename);
        store->meta.put("key1", NDArray<int64_t>({}, 42));
        store->flush();
    }

    // Check header
    {
        auto store = StarDataset::open(filename, "r");
        const FileHeader& header = store->getFileHeader();
        EXPECT_EQ(header.layer_count, 0);
    }
}

TEST_F(LayerTest, FileHeaderLayerCountMultiple) {
    std::string filename = createTempFile("layer_count_multiple");

    // Create with 5 layers
    {
        auto store = StarDataset::create(filename);
        for (int i = 0; i < 5; ++i) {
            store->create_layer("layer_" + std::to_string(i));
        }
        store->flush();
    }

    // Check header
    {
        auto store = StarDataset::open(filename, "r");
        const FileHeader& header = store->getFileHeader();
        EXPECT_EQ(header.layer_count, 5);
    }
}

TEST_F(LayerTest, HeaderSizeGrowsWithLayers) {
    std::string filename1 = createTempFile("header_no_layers");
    std::string filename2 = createTempFile("header_with_layers");

    size_t header_size1, header_size2;

    // File without layers
    {
        auto store = StarDataset::create(filename1);
        store->meta.put("key", NDArray<int64_t>({}, 1));
        store->flush();
    }
    {
        auto store = StarDataset::open(filename1, "r");
        header_size1 = store->getFileHeader().header_size;
    }

    // File with 10 layers
    {
        auto store = StarDataset::create(filename2);
        store->meta.put("key", NDArray<int64_t>({}, 1));
        for (int i = 0; i < 10; ++i) {
            store->create_layer("layer_" + std::to_string(i));
        }
        store->flush();
    }
    {
        auto store = StarDataset::open(filename2, "r");
        header_size2 = store->getFileHeader().header_size;
    }

    // Header with layers should be larger
    EXPECT_GT(header_size2, header_size1);
}

//==============================================================================
// Thread Safety Tests
//==============================================================================

TEST_F(LayerTest, ConcurrentLayerCreation) {
    std::string filename = createTempFile("concurrent_layers");

    auto store = StarDataset::create(filename);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> error_count{0};

    // Try to create layers from multiple threads
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([&store, i, &success_count, &error_count]() {
            try {
                std::string layer_name = "band_" + std::to_string(i);
                store->create_layer(layer_name);
                success_count++;
            } catch (const std::exception& e) {
                error_count++;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // All should succeed
    EXPECT_EQ(success_count.load(), 10);
    EXPECT_EQ(error_count.load(), 0);

    // Verify all layers exist
    auto layers = store->list_layers();
    EXPECT_EQ(layers.size(), 10);
}

TEST_F(LayerTest, LayerDataArrays) {
    std::string testFile = createTempFile("layer_data");
    
    // Create dataset with base and layer data arrays
    {
        auto store = StarDataset::create(testFile);
        
        // Base data
        NDArray<double> base_data({5});
        for (size_t i = 0; i < 5; i++) base_data.data()[i] = i * 10.0;
        store->put("base_data", std::move(base_data));
        
        // Layer data
        auto layer = store->create_layer("layer_1");
        NDArray<double> layer_data({3});
        for (size_t i = 0; i < 3; i++) layer_data.data()[i] = i * 100.0;
        layer->put("layer_data", std::move(layer_data));
        
        store->flush();
    }
    
    // Reopen and verify
    {
        auto store = StarDataset::open(testFile);
        
        // Check base data
        auto base = store->get<double>("base_data");
        EXPECT_EQ(base.size(), 5);
        EXPECT_DOUBLE_EQ(base.data()[0], 0.0);
        EXPECT_DOUBLE_EQ(base.data()[1], 10.0);
        
        // Check layer data
        auto layer = store->get_layer("layer_1");
        auto layer_arr = layer->get<double>("layer_data");
        EXPECT_EQ(layer_arr.size(), 3);
        EXPECT_DOUBLE_EQ(layer_arr.data()[0], 0.0);
        EXPECT_DOUBLE_EQ(layer_arr.data()[1], 100.0);
    }
}
