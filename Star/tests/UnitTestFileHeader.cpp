/**
 * @file UnitTestFileHeader.cpp
 * @brief Unit tests for FileHeader struct and version API
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
// Test Fixture for FileHeader Tests
//==============================================================================

class FileHeaderTest : public ::testing::Test {
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

    std::string createTempFile(const std::string& prefix = "test_header") {
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
// FileHeader Struct Tests
//==============================================================================

TEST_F(FileHeaderTest, HeaderSizeIs23Bytes) {
    // Test that FileHeader::size() returns exactly 23 bytes
    EXPECT_EQ(FileHeader::size(), 23);

    // Verify the calculation matches the struct layout:
    // 6 (magic) + 1 (format_version) + 8 (header_size) + 8 (entry_count) = 23
    size_t expected = 6 + sizeof(uint8_t) + 2 * sizeof(uint64_t);
    EXPECT_EQ(FileHeader::size(), expected);
}

TEST_F(FileHeaderTest, MagicStringValidation) {
    FileHeader header;

    // Set valid magic string
    std::memcpy(header.magic, "STARDS", 6);
    EXPECT_TRUE(header.isValid());

    // Test invalid magic strings
    std::memcpy(header.magic, "STAR++", 6);
    EXPECT_FALSE(header.isValid());

    std::memcpy(header.magic, "STARXX", 6);
    EXPECT_FALSE(header.isValid());

    std::memcpy(header.magic, "ABCDEF", 6);
    EXPECT_FALSE(header.isValid());
}

TEST_F(FileHeaderTest, FormatVersionStringFormatting) {
    FileHeader header;

    // Test format version 2
    header.format_version = 2;
    EXPECT_EQ(header.getVersionString(), "Format v2");

    // Test format version 1
    header.format_version = 1;
    EXPECT_EQ(header.getVersionString(), "Format v1");

    // Test format version 10
    header.format_version = 10;
    EXPECT_EQ(header.getVersionString(), "Format v10");
}

TEST_F(FileHeaderTest, ReadWriteRoundTrip) {
    FileHeader original;

    // Populate with test data
    std::memcpy(original.magic, "STARDS", 6);
    original.format_version = 2;
    original.header_size = 1024;
    original.entry_count = 5;

    // Write to stream
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    original.write(ss);

    // Read back
    ss.seekg(0);
    FileHeader read_back;
    read_back.read(ss);

    // Verify all fields match
    EXPECT_EQ(std::string(read_back.magic, 6), std::string(original.magic, 6));
    EXPECT_EQ(read_back.format_version, original.format_version);
    EXPECT_EQ(read_back.header_size, original.header_size);
    EXPECT_EQ(read_back.entry_count, original.entry_count);
    EXPECT_TRUE(read_back.isValid());
    EXPECT_EQ(read_back.getVersionString(), "Format v2");
}

TEST_F(FileHeaderTest, BinaryLayoutConsistency) {
    FileHeader header;

    // Populate with known values
    std::memcpy(header.magic, "STARDS", 6);
    header.format_version = 2;
    header.header_size = 999;
    header.entry_count = 42;

    // Write to stringstream
    std::stringstream ss(std::ios::binary | std::ios::in | std::ios::out);
    header.write(ss);

    // Verify exactly 23 bytes were written
    ss.seekg(0, std::ios::end);
    EXPECT_EQ(ss.tellg(), 23);

    // Read raw bytes and verify layout
    ss.seekg(0);
    char buffer[23];
    ss.read(buffer, 23);

    // Check magic at offset 0
    EXPECT_EQ(std::string(&buffer[0], 6), "STARDS");

    // Check format version at offset 6
    EXPECT_EQ(static_cast<uint8_t>(buffer[6]), 2);

    // Check header_size at offset 7 (little-endian)
    uint64_t* header_size_ptr = reinterpret_cast<uint64_t*>(&buffer[7]);
    EXPECT_EQ(header_size_ptr[0], 999);

    // Check entry_count at offset 15 (little-endian)
    uint64_t* entry_count_ptr = reinterpret_cast<uint64_t*>(&buffer[15]);
    EXPECT_EQ(entry_count_ptr[0], 42);
}

//==============================================================================
// StarDataset Integration Tests
//==============================================================================

TEST_F(FileHeaderTest, DatasetWritesCorrectHeader) {
    std::string filename = createTempFile("header_write");

    // Create and write a dataset
    {
        auto store = StarDataset::create(filename);
        store->meta.put("test_scalar", NDArray<int64_t>({}, 42));
        store->flush();
    }

    // Open file and read header manually
    std::ifstream file(filename, std::ios::binary);
    ASSERT_TRUE(file.is_open());

    FileHeader header;
    header.read(file);

    // Verify header fields
    EXPECT_TRUE(header.isValid());
    EXPECT_EQ(std::string(header.magic, 6), "STARDS");
    EXPECT_EQ(header.format_version, 2);
    EXPECT_GT(header.header_size, 23);  // Should be larger than just fixed header
    EXPECT_GT(header.entry_count, 0);   // Should have at least metadata block
}

TEST_F(FileHeaderTest, DatasetReadsHeaderCorrectly) {
    std::string filename = createTempFile("header_read");

    // Create a test file
    {
        auto store = StarDataset::create(filename);
        store->meta.put("version_test", NDArray<int64_t>({}, 100));
        store->meta.put("another_key", NDArray<double>({}, 3.14));
        store->flush();
    }

    // Open and verify header can be read
    {
        auto store = StarDataset::open(filename, "r");
        const FileHeader& header = store->getFileHeader();

        // Check header fields
        EXPECT_EQ(std::string(header.magic, 6), "STARDS");
        EXPECT_EQ(header.format_version, 2);

        // Verify format version accessor returns format version string
        std::string format_str = header.getVersionString();
        EXPECT_EQ(format_str, "Format v2");

        // Verify data is accessible
        auto val = store->meta.get("version_test");
        ASSERT_TRUE(val);
        EXPECT_EQ(val->as<int64_t>().flat(0), 100);
    }
}

TEST_F(FileHeaderTest, GetFileHeaderAccessor) {
    std::string filename = createTempFile("header_accessor");

    // Create a dataset with some data
    {
        auto store = StarDataset::create(filename);
        // NDArray constructor: data first, shape second
        NDArray<double> data({1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0}, {3, 3});
        store->put("matrix", data);
        store->flush();
    }

    // Open and use getFileHeader() accessor
    {
        auto store = StarDataset::open(filename, "r");
        const FileHeader& header = store->getFileHeader();

        // Verify we can access all header fields
        EXPECT_TRUE(header.isValid());
        EXPECT_EQ(header.format_version, 2);
        EXPECT_GT(header.header_size, 0);
        EXPECT_GT(header.entry_count, 0);

        // Verify header data is consistent with file
        EXPECT_EQ(store->m_header_size, header.header_size);
    }
}

TEST_F(FileHeaderTest, GetLibraryVersionAccessor) {
    // Test standalone getLibraryVersion() function
    std::string lib_version = getLibraryVersion();

    // Should be in format "X.Y.Z"
    EXPECT_FALSE(lib_version.empty());

    // Should match the STAR_VERSION constants
    std::string expected = std::to_string(STAR_VERSION_MAJOR) + "." +
                          std::to_string(STAR_VERSION_MINOR) + "." +
                          std::to_string(STAR_VERSION_PATCH);
    EXPECT_EQ(lib_version, expected);
}

TEST_F(FileHeaderTest, VersionConsistencyAcrossWrites) {
    std::string filename = createTempFile("version_consistency");

    // Write data
    {
        auto store = StarDataset::create(filename);
        store->meta.put("key1", NDArray<int64_t>({}, 100));
        store->flush();
    }

    // Read and verify format version
    uint8_t format_version1;
    {
        auto store = StarDataset::open(filename, "r");
        format_version1 = store->getFileHeader().format_version;
        EXPECT_EQ(format_version1, 2);
    }

    // Update data
    {
        auto store = StarDataset::create(filename);
        store->meta.put("key2", NDArray<int64_t>({}, 200));
        store->flush();
    }

    // Read again and verify format version is consistent
    {
        auto store = StarDataset::open(filename, "r");
        uint8_t format_version2 = store->getFileHeader().format_version;
        EXPECT_EQ(format_version1, format_version2);
        EXPECT_EQ(format_version2, 2);
    }
}

TEST_F(FileHeaderTest, MultipleEntriesHeaderCount) {
    std::string filename = createTempFile("entry_count");

    // Create dataset with multiple arrays
    {
        auto store = StarDataset::create(filename);

        // Add several arrays
        NDArray<int> arr1({5});
        for (int i = 0; i < 5; i++) arr1.data()[i] = i + 1;

        NDArray<double> arr2({3, 2});
        std::vector<double> data2 = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6};
        std::copy(data2.begin(), data2.end(), arr2.data().begin());

        NDArray<float> arr3({10});
        for (int i = 0; i < 10; i++) arr3.data()[i] = (i + 1) * 0.1f;

        store->put("array1", arr1);
        store->put("array2", arr2);
        store->put("array3", arr3);

        // Add some metadata
        store->meta.put("meta1", NDArray<int64_t>({}, 42));
        store->meta.put("meta2", NDArray<double>({}, 3.14));

        store->flush();
    }

    // Verify header entry count
    {
        auto store = StarDataset::open(filename, "r");
        const FileHeader& header = store->getFileHeader();

        // Should have entries (including metadata block)
        EXPECT_GT(header.entry_count, 0);

        // Verify all arrays are accessible
        auto arr1 = store->get<int>("array1");
        auto arr2 = store->get<double>("array2");
        auto arr3 = store->get<float>("array3");

        EXPECT_EQ(arr1.size(), 5);
        EXPECT_EQ(arr2.size(), 6);
        EXPECT_EQ(arr3.size(), 10);
    }
}

TEST_F(FileHeaderTest, HeaderSizeReflectsContent) {
    std::string filename1 = createTempFile("header_small");
    std::string filename2 = createTempFile("header_large");

    size_t header_size1, header_size2;

    // Create small dataset
    {
        auto store = StarDataset::create(filename1);
        store->meta.put("key", NDArray<int64_t>({}, 1));
        store->flush();
    }
    {
        auto store = StarDataset::open(filename1, "r");
        header_size1 = store->getFileHeader().header_size;
    }

    // Create larger dataset with more entries
    {
        auto store = StarDataset::create(filename2);
        for (int i = 0; i < 20; i++) {
            std::string key = "key" + std::to_string(i);
            store->meta.put(key, NDArray<int64_t>({}, i * 100));
        }
        store->flush();
    }
    {
        auto store = StarDataset::open(filename2, "r");
        header_size2 = store->getFileHeader().header_size;
    }

    // Header size grows with number of entries (each entry has an index entry in the header)
    // File 2 has 20 entries vs file 1 with 1 entry, so header should be larger
    EXPECT_GT(header_size2, header_size1) << "Header with more entries should be larger";

    // Both should be at least the fixed header size
    EXPECT_GE(header_size1, FileHeader::size());
    EXPECT_GE(header_size2, FileHeader::size());

    // Verify header size difference is reasonable (roughly proportional to entry count)
    size_t size_per_entry_approx = (header_size2 - header_size1) / 19;  // 19 additional entries
    EXPECT_GT(size_per_entry_approx, 0);
    EXPECT_LT(size_per_entry_approx, 200);  // Each index entry should be under 200 bytes

    // Verify that both files can be read and contain their metadata
    {
        auto store1 = StarDataset::open(filename1, "r");
        EXPECT_TRUE(store1->meta.contains("key"));
        auto val1 = store1->meta.get("key");
        EXPECT_NE(val1, nullptr);
    }
    {
        auto store2 = StarDataset::open(filename2, "r");
        EXPECT_TRUE(store2->meta.contains("key0"));
        EXPECT_TRUE(store2->meta.contains("key19"));
        auto val0 = store2->meta.get("key0");
        auto val19 = store2->meta.get("key19");
        EXPECT_NE(val0, nullptr);
        EXPECT_NE(val19, nullptr);
    }
}

//==============================================================================
// Edge Cases and Error Handling
//==============================================================================

TEST_F(FileHeaderTest, EmptyFileRejectsInvalidMagic) {
    std::string filename = createTempFile("invalid_magic");

    // Write a file with invalid magic string
    {
        std::ofstream file(filename, std::ios::binary);
        FileHeader header;
        std::memcpy(header.magic, "BADMAG", 6);
        header.format_version = 2;
        header.header_size = 100;
        header.entry_count = 0;
        header.write(file);
    }

    // Try to open - should throw due to invalid magic
    EXPECT_THROW({
        auto store = StarDataset::open(filename, "r");
    }, std::runtime_error);
}

TEST_F(FileHeaderTest, FormatVersionZeroIsValid) {
    FileHeader header;
    header.format_version = 0;

    EXPECT_EQ(header.getVersionString(), "Format v0");
}

TEST_F(FileHeaderTest, MaxFormatVersionValue) {
    FileHeader header;
    header.format_version = 255;  // max uint8_t

    std::string version = header.getVersionString();
    EXPECT_EQ(version, "Format v255");
}
