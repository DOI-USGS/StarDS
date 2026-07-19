/**
 * @file UnitTestBytesAPI.cpp
 * @brief Unit tests for the in-memory byte APIs: open_bytes() and write_bytes().
 *
 * These mirror the file-based open()/save_to() path but source/sink a complete
 * .stards image as a std::vector<char>, so a dataset can be built or read without
 * ever touching the filesystem.
 */

#include "stards.h"
#include <gtest/gtest.h>
#include "Fixtures.h"
#include <vector>
#include <string>
#include <fstream>

using namespace star;

// write_bytes() works entirely in memory, so no temp dir is strictly needed; we
// still derive from TempDirTest for the couple of tests that cross-check against
// a real file written by save_to().
class BytesApiTest : public star_test::TempDirTest {
protected:
    // Build a dataset in a temp file, populate it, and return the file path.
    std::string make_populated() {
        std::string path = tempFilePath("bytes_src.stards");
        auto ds = StarDataset::create(path);
        ds->put("signal", NDArray<double>::full({256}, 1.5));
        ds->put("image", NDArray<int32_t>::full({4, 5}, 7));
        ds->meta.put("instrument", NDArray<std::string>({}, std::string("AVIRIS")));
        ds->meta.put("gain", NDArray<double>({}, 2.25));
        ds->flush();
        return path;
    }

    static std::vector<char> read_all(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        return std::vector<char>((std::istreambuf_iterator<char>(in)),
                                 std::istreambuf_iterator<char>());
    }
};

TEST_F(BytesApiTest, WriteBytesThenOpenBytesRoundTrip) {
    // Create in a file, serialize to bytes, then reopen purely from bytes.
    std::string path = make_populated();
    std::vector<char> bytes;
    {
        auto ds = StarDataset::open(path, FileMode::READ_ONLY);
        bytes = ds->write_bytes();
    }
    ASSERT_FALSE(bytes.empty());

    auto ds = StarDataset::open_bytes(bytes);

    auto sig = ds->get<double>("signal");
    ASSERT_EQ(sig.size(), 256u);
    EXPECT_DOUBLE_EQ(sig(0), 1.5);
    EXPECT_DOUBLE_EQ(sig(255), 1.5);

    auto img = ds->get<int32_t>("image");
    ASSERT_EQ(img.shape(0), 4u);
    ASSERT_EQ(img.shape(1), 5u);
    EXPECT_EQ(img(3, 4), 7);

    auto inst = ds->meta.get("instrument");
    ASSERT_NE(inst, nullptr);
    EXPECT_EQ(inst->as<std::string>()(0), "AVIRIS");

    auto gain = ds->meta.get("gain");
    ASSERT_NE(gain, nullptr);
    EXPECT_DOUBLE_EQ(gain->as<double>()(0), 2.25);
}

TEST_F(BytesApiTest, OpenBytesIsReadOnly) {
    std::string path = make_populated();
    std::vector<char> bytes;
    {
        auto ds = StarDataset::open(path, FileMode::READ_ONLY);
        bytes = ds->write_bytes();
    }

    auto ds = StarDataset::open_bytes(bytes);
    EXPECT_TRUE(ds->is_read_only());
    // In-memory dataset has no backing file; an explicit flush must fail.
    ds->put("added", NDArray<int64_t>({}, 1));
    EXPECT_THROW(ds->flush(), std::runtime_error);
}

TEST_F(BytesApiTest, WriteBytesEqualsSaveToImage) {
    // write_bytes() should produce exactly the bytes save_to() would write to disk.
    std::string path = make_populated();
    auto ds = StarDataset::open(path, FileMode::READ_ONLY);

    std::vector<char> bytes = ds->write_bytes();

    std::string out_path = tempFilePath("bytes_saveto.stards");
    ds->save_to(out_path);
    std::vector<char> file_bytes = read_all(out_path);

    EXPECT_EQ(bytes.size(), file_bytes.size());
    EXPECT_EQ(bytes, file_bytes);
}

TEST_F(BytesApiTest, PointerOverloadRoundTrip) {
    std::string path = make_populated();
    std::vector<char> bytes;
    {
        auto ds = StarDataset::open(path, FileMode::READ_ONLY);
        bytes = ds->write_bytes();
    }

    // Open via the raw pointer + length overload (e.g. a C buffer / Python bytes).
    auto ds = StarDataset::open_bytes(bytes.data(), bytes.size());
    auto sig = ds->get<double>("signal");
    ASSERT_EQ(sig.size(), 256u);
    EXPECT_DOUBLE_EQ(sig(100), 1.5);
}

TEST_F(BytesApiTest, LayersSurviveByteRoundTrip) {
    std::string path = tempFilePath("bytes_layers.stards");
    {
        auto ds = StarDataset::create(path);
        ds->put("base", NDArray<double>::full({300}, 3.0));
        auto layer = ds->create_layer("proc");
        layer->put("base", NDArray<double>::full({300}, 9.0));
        ds->flush();
    }

    std::vector<char> bytes;
    {
        auto ds = StarDataset::open(path, FileMode::READ_ONLY);
        bytes = ds->write_bytes();
    }

    auto ds = StarDataset::open_bytes(bytes);
    EXPECT_TRUE(ds->has_layer("proc"));
    auto layer = ds->get_layer("proc");
    EXPECT_DOUBLE_EQ(layer->get<double>("base")(0), 9.0);
    EXPECT_DOUBLE_EQ(ds->get<double>("base")(0), 3.0);
}

TEST_F(BytesApiTest, OpenBytesRejectsGarbage) {
    std::vector<char> junk(64, '\x00');
    EXPECT_THROW(StarDataset::open_bytes(junk), std::runtime_error);

    std::vector<char> empty;
    EXPECT_THROW(StarDataset::open_bytes(empty), std::runtime_error);
}

TEST_F(BytesApiTest, ModifyInMemoryThenReserialize) {
    // Open from bytes, mutate in memory, re-serialize, and confirm the change
    // survives a second byte round-trip.
    std::string path = make_populated();
    std::vector<char> bytes;
    {
        auto ds = StarDataset::open(path, FileMode::READ_ONLY);
        bytes = ds->write_bytes();
    }

    std::vector<char> bytes2;
    {
        auto ds = StarDataset::open_bytes(bytes);
        ds->put("extra", NDArray<double>::full({128}, 42.0));
        bytes2 = ds->write_bytes();
    }

    auto ds = StarDataset::open_bytes(bytes2);
    auto extra = ds->get<double>("extra");
    ASSERT_EQ(extra.size(), 128u);
    EXPECT_DOUBLE_EQ(extra(0), 42.0);
    // Original data still present.
    EXPECT_DOUBLE_EQ(ds->get<double>("signal")(0), 1.5);
}
