/**
 * @file UnitTestLayers.cpp
 * @brief Unit tests for layer functionality and inheritance
 */

#include <gtest/gtest.h>
#include "stards.h"
#include "Fixtures.h"
#include <iostream>
#include <sstream>
#include <ghc/fs_std.hpp>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>

using namespace star;

//==============================================================================
// Test Fixture for Layer Tests
//==============================================================================

// Each test runs in its own temp directory (auto-deleted); createTempFile()
// returns a unique path inside it (parallel-safe).
class LayerTest : public star_test::TempDirTest {
protected:
    std::string createTempFile(const std::string& prefix = "test_layer") {
        return tempStardsFile(prefix);
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
    // base() returns a shared_ptr<StarDataset>; compare raw pointers so the
    // check works regardless of shared_ptr control-block identity.
    EXPECT_EQ(layer1->base().get(), store.get());
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
    store->setLayerInheritance(true);  // this test exercises base-layer inheritance

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
    store->setLayerInheritance(true);  // this test exercises base-layer inheritance

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
    store->setLayerInheritance(true);  // this test exercises base-layer inheritance

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

    // Reopen and verify (opt into inheritance so base keys are visible to layers)
    {
        OpenOptions opts;
        opts.layer_inheritance = true;
        auto store = StarDataset::open(filename, "r", opts);

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
    store->setLayerInheritance(true);  // this test exercises base-layer inheritance

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

    // Reopen and verify inheritance persists (opt into base-layer inheritance)
    OpenOptions reopen_opts;
    reopen_opts.layer_inheritance = true;
    store = StarDataset::open(filename, "r", reopen_opts);

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

// Regression: layer metadata written after a prior flush must still persist.
// (LayerMetadataAccessor::put previously left m_flushed set, so the second
// flush was a no-op and the write was dropped.)
TEST_F(LayerTest, LayerMetaPutAfterFlushPersists) {
    std::string testFile = createTempFile("layer_meta_persist");
    {
        auto store = StarDataset::create(testFile);
        store->meta.put("base_key", NDArray<int64_t>({}, 1));
        store->flush();  // sets m_flushed = true

        auto layer = store->create_layer("layer_1");
        layer->meta.put("layer_key", NDArray<int64_t>({}, 99));
        store->flush();  // must NOT be skipped
    }
    {
        auto store = StarDataset::open(testFile);
        auto layer = store->get_layer("layer_1");
        auto mv = layer->meta.get("layer_key");
        ASSERT_NE(mv, nullptr);
        EXPECT_EQ(mv->as<int64_t>().data()[0], 99);
    }
}

// Regression: removing a layer metadata key must actually remove it.
// (LayerMetadataAccessor::remove previously targeted a "__layer_"-prefixed key
// in the base layer, so it was a silent no-op.)
TEST_F(LayerTest, LayerMetaRemoveActuallyRemoves) {
    std::string testFile = createTempFile("layer_meta_remove");
    auto store = StarDataset::create(testFile);
    auto layer = store->create_layer("layer_1");

    layer->meta.put("doomed", NDArray<int64_t>({}, 7));
    ASSERT_NE(layer->meta.get("doomed"), nullptr);

    layer->meta.remove("doomed");
    EXPECT_EQ(layer->meta.get("doomed"), nullptr);
}

//==============================================================================
// OpenOptions: layer-inheritance opt-in (default OFF)
//==============================================================================

// Writes a base key "A" plus a layer "L" that owns key "B", then reopens the
// file with the given OpenOptions and returns the layer view for assertions.
namespace {
void write_base_and_layer(const std::string& filename) {
    auto store = StarDataset::create(filename);
    store->put("A", NDArray<double>::full({8}, 1.0));   // base-only key
    auto layer = store->create_layer("L");
    layer->put("B", NDArray<double>::full({8}, 2.0));   // layer-owned key
    store->flush();
    store->close();
}
}  // namespace

// By default (inheritance OFF), a key that lives only on the base layer is NOT
// visible through a layer view.
TEST_F(LayerTest, InheritanceOffByDefault) {
    std::string filename = createTempFile("inherit_default_off");
    write_base_and_layer(filename);

    auto store = StarDataset::open(filename, "r");
    EXPECT_FALSE(store->layerInheritance());  // default

    auto layer = store->get_layer("L");
    // Layer's own key resolves regardless of inheritance.
    EXPECT_NO_THROW(layer->get<double>("B"));
    EXPECT_DOUBLE_EQ(layer->get<double>("B")(0), 2.0);
    // Base-only key is a miss without inheritance.
    EXPECT_FALSE(layer->contains("A"));
    EXPECT_THROW(layer->get<double>("A"), std::runtime_error);
    // keys() excludes the base-only key.
    auto keys = layer->keys();
    EXPECT_EQ(std::find(keys.begin(), keys.end(), "A"), keys.end());
}

// Opening with OpenOptions{layer_inheritance=true} restores base fallback.
TEST_F(LayerTest, InheritanceOptInViaOpenOptions) {
    std::string filename = createTempFile("inherit_opt_in");
    write_base_and_layer(filename);

    OpenOptions opts;
    opts.layer_inheritance = true;
    auto store = StarDataset::open(filename, "r", opts);
    EXPECT_TRUE(store->layerInheritance());

    auto layer = store->get_layer("L");
    EXPECT_TRUE(layer->contains("A"));  // inherited from base
    EXPECT_DOUBLE_EQ(layer->get<double>("A")(0), 1.0);
    EXPECT_DOUBLE_EQ(layer->get<double>("B")(0), 2.0);
    auto keys = layer->keys();
    EXPECT_NE(std::find(keys.begin(), keys.end(), "A"), keys.end());
}

// The post-open setter toggles inheritance live: existing layer views observe
// the change because they read the flag off the owning dataset.
TEST_F(LayerTest, InheritancePostOpenToggle) {
    std::string filename = createTempFile("inherit_toggle");
    write_base_and_layer(filename);

    auto store = StarDataset::open(filename, "r");
    auto layer = store->get_layer("L");

    // Default OFF: base-only key invisible.
    EXPECT_FALSE(layer->contains("A"));

    // Turn inheritance ON — the same layer view now sees the base key.
    store->setLayerInheritance(true);
    EXPECT_TRUE(store->layerInheritance());
    EXPECT_TRUE(layer->contains("A"));
    EXPECT_DOUBLE_EQ(layer->get<double>("A")(0), 1.0);

    // Turn it back OFF — the base key disappears again.
    store->setLayerInheritance(false);
    EXPECT_FALSE(store->layerInheritance());
    EXPECT_FALSE(layer->contains("A"));
    EXPECT_THROW(layer->get<double>("A"), std::runtime_error);
}
