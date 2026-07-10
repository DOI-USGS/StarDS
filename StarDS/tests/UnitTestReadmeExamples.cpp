/**
 * @file UnitTestReadmeExamples.cpp
 * @brief Executable versions of the C++ code examples in README.md.
 *
 * Each test runs the same code shown in the README so the documentation cannot
 * silently drift from the actual API. S3 (/vsis3) and HTTP (/vsicurl) examples
 * are excluded because they require remote access; their non-remote structure
 * (open modes, saveTo to a local path) is still covered where possible.
 */

#include <gtest/gtest.h>
#include "stards.h"
#include "Fixtures.h"
#include <ghc/fs_std.hpp>
#include <random>
#include <sstream>

using namespace star;

//==============================================================================
// Fixture: each test gets its own temp directory (auto-deleted); tempFile()
// returns a unique path inside it (parallel-safe).
//==============================================================================
class ReadmeExamplesTest : public star_test::TempDirTest {
protected:
    std::string tempFile(const std::string& prefix = "readme") {
        return tempStardsFile(prefix);
    }
};

//==============================================================================
// README: "Then in your code" (Using in Your Project)
//==============================================================================
TEST_F(ReadmeExamplesTest, UsingInYourProject_CreateAndStore) {
    const std::string path = tempFile("using");

    // Create and store arrays
    NDArray<double> data = NDArray<double>::zeros({100, 100});
    auto store = star::StarDataset::create(path);
    store->put("matrix", data);
    store->flush();

    EXPECT_TRUE(fs::exists(path));

    // Sanity: read it back
    auto reopened = StarDataset::open(path, FileMode::READ_ONLY);
    auto matrix = reopened->get<double>("matrix");
    ASSERT_EQ(matrix.shape().size(), 2u);
    EXPECT_EQ(matrix.shape(0), 100u);
    EXPECT_EQ(matrix.shape(1), 100u);
    EXPECT_DOUBLE_EQ(matrix(0, 0), 0.0);
}

//==============================================================================
// README: "Basic C++ API"
//==============================================================================
TEST_F(ReadmeExamplesTest, BasicCppApi) {
    const std::string path = tempFile("basic");

    // Create arrays
    NDArray<double> matrix = NDArray<double>::zeros({100, 100});
    matrix(10, 20) = 3.14;

    // Create dataset with default compression
    auto store = StarDataset::create(path);

    // Store arrays - these go to block storage
    store->put("matrix", std::move(matrix));

    // Store metadata - separate namespace, can use same keys!
    store->meta.put("matrix", NDArray<std::string>({}, {"Matrix description"}));
    store->meta.put("timestamp", NDArray<int64_t>({}, {1234567890}));

    store->flush();  // Write to disk

    // Read back
    auto store2 = StarDataset::open(path);

    // Get array
    auto matrix_data = store2->get<double>("matrix");
    EXPECT_DOUBLE_EQ(matrix_data(10, 20), 3.14);

    // Get metadata (different namespace!)
    auto matrix_meta = store2->meta.get("matrix");
    ASSERT_NE(matrix_meta, nullptr);
    auto desc = matrix_meta->as<std::string>();
    EXPECT_EQ(desc(0), "Matrix description");

    // Check if key exists
    ASSERT_TRUE(store2->meta.contains("timestamp"));
    auto ts = store2->meta.get("timestamp")->as<int64_t>();
    EXPECT_EQ(ts(0), 1234567890);
}

//==============================================================================
// README: "Layers in C++"
//==============================================================================
TEST_F(ReadmeExamplesTest, LayersInCpp) {
    const std::string path = tempFile("layers");

    // Create dataset with base data
    auto store = StarDataset::create(path);
    store->put("image", NDArray<double>::zeros({512, 512}));
    store->put("wavelengths", NDArray<double>::zeros({300}));

    // Create layer with a different version of the image
    auto layer1 = store->create_layer("processed");
    layer1->put("image", NDArray<double>::full({512, 512}, 1.0));  // Different image
    // wavelengths inherited from base

    // Create another layer
    auto layer2 = store->create_layer("calibrated");
    layer2->put("image", NDArray<double>::full({512, 512}, 2.0));
    layer2->put("wavelengths", NDArray<double>::full({300}, 42.0));  // Override

    // Layers also have metadata with inheritance
    layer1->meta.put("description", NDArray<std::string>({}, {"Processed version"}));

    store->flush();

    // Read back. Layer inheritance is off by default; opt in so layers see
    // keys that live only on the base layer.
    auto store2 = StarDataset::open(path);
    store2->setLayerInheritance(true);

    // Get base data
    auto base_img = store2->get<double>("image");
    EXPECT_EQ(base_img.shape(0), 512u);

    // Get layer data
    auto proc_layer = store2->get_layer("processed");
    auto proc_img = proc_layer->get<double>("image");        // Different data
    auto proc_wave = proc_layer->get<double>("wavelengths"); // Inherited
    EXPECT_DOUBLE_EQ(proc_img(0, 0), 1.0);
    EXPECT_EQ(proc_wave.shape(0), 300u);  // inherited shape from base

    auto cal_layer = store2->get_layer("calibrated");
    auto cal_img = cal_layer->get<double>("image");          // Different data
    auto cal_wave = cal_layer->get<double>("wavelengths");   // Overridden
    EXPECT_DOUBLE_EQ(cal_img(0, 0), 2.0);
    EXPECT_DOUBLE_EQ(cal_wave(0), 42.0);

    // Metadata inheritance / layer-specific metadata
    auto proc_desc = proc_layer->meta.get("description");
    ASSERT_NE(proc_desc, nullptr);
    EXPECT_EQ(proc_desc->as<std::string>()(0), "Processed version");
}

//==============================================================================
// README: "File Open Modes"
//==============================================================================
TEST_F(ReadmeExamplesTest, FileOpenModes) {
    const std::string path = tempFile("modes");

    // Create a new file (README: StarDataset::create)
    auto store7 = StarDataset::create(path);
    store7->put("data", NDArray<int64_t>({3}, 7));
    store7->flush();

    // String modes
    auto store1 = StarDataset::open(path, "r");   // Read-only
    EXPECT_TRUE(store1->isReadOnly());
    auto store2 = StarDataset::open(path, "w");   // Read-write (create if missing)
    EXPECT_FALSE(store2->isReadOnly());
    auto store3 = StarDataset::open(path, "rw");  // Read-write (explicit)
    EXPECT_FALSE(store3->isReadOnly());

    // Enum modes (explicit, type-safe)
    auto store5 = StarDataset::open(path, FileMode::READ_ONLY);
    EXPECT_TRUE(store5->isReadOnly());
    auto store6 = StarDataset::open(path, FileMode::READ_WRITE);
    EXPECT_FALSE(store6->isReadOnly());

    // Value survives a read-only reopen.
    EXPECT_EQ(store1->get<int64_t>("data")(0), 7);
}

//==============================================================================
// README: "HTTP Remote Access" — remote read is excluded (needs network), but
// the local half of the example — saveTo() to a local path — is covered here
// using a local source instead of /vsicurl.
//==============================================================================
TEST_F(ReadmeExamplesTest, SaveToLocalCopy) {
    const std::string src = tempFile("savesrc");
    const std::string dst = tempFile("savedst");

    {
        auto store = StarDataset::create(src);
        store->put("sensor_data", NDArray<double>({4}, 1.5));
        store->flush();
    }

    // Open read-only and save a copy elsewhere (README: store->saveTo(...)).
    auto store = StarDataset::open(src, "r");
    auto data = store->get<double>("sensor_data");
    EXPECT_EQ(data.shape(0), 4u);
    store->saveTo(dst);

    // The saved copy is a complete, readable dataset.
    auto copy = StarDataset::open(dst, FileMode::READ_ONLY);
    auto copied = copy->get<double>("sensor_data");
    ASSERT_EQ(copied.shape(0), 4u);
    EXPECT_DOUBLE_EQ(copied(0), 1.5);
}

//==============================================================================
// README: "NDArray Factories"
//==============================================================================
TEST_F(ReadmeExamplesTest, NDArrayFactories) {
    // Create arrays
    auto zeros = NDArray<double>::zeros({100, 100});
    auto ones = NDArray<int>::ones({50});
    auto filled = NDArray<float>::full({10, 10}, 3.14f);
    auto range = NDArray<int>::arange(0, 10, 2);  // [0, 2, 4, 6, 8]

    EXPECT_EQ(zeros.shape(0), 100u);
    EXPECT_DOUBLE_EQ(zeros(0, 0), 0.0);

    EXPECT_EQ(ones.shape(0), 50u);
    EXPECT_EQ(ones(0), 1);

    EXPECT_EQ(filled.shape(0), 10u);
    EXPECT_FLOAT_EQ(filled(5, 5), 3.14f);

    ASSERT_EQ(range.size(), 5u);
    EXPECT_EQ(range(0), 0);
    EXPECT_EQ(range(1), 2);
    EXPECT_EQ(range(2), 4);
    EXPECT_EQ(range(3), 6);
    EXPECT_EQ(range(4), 8);
}
