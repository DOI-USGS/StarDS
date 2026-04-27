/**
 * @file UnitTestCloseReopenAPI.cpp
 * @brief Unit tests for explicit close functionality
 *
 * Tests the new close() functionality that provides explicit
 * resource management with proper state tracking.
 */

#include "star.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <filesystem>
#include <random>
#include <chrono>

using namespace star;

// Helper to create temporary test files
class CloseTest : public ::testing::Test {
protected:
    std::string test_file;

    void SetUp() override {
        // Use thread-safe random generator with high-resolution timestamp
        static std::random_device rd;
        static thread_local std::mt19937_64 gen(rd() + std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_int_distribution<uint64_t> dis;

        auto now = std::chrono::high_resolution_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        uint64_t random_val = dis(gen);

        test_file = "/tmp/test_close_" +
                    std::to_string(timestamp) + "_" +
                    std::to_string(random_val) + ".star";
    }

    void TearDown() override {
        if (std::filesystem::exists(test_file)) {
            std::filesystem::remove(test_file);
        }
    }
};

/**
 * Test 1: Explicit Close (Now Just Flushes)
 * Verify that close() flushes data and dataset remains usable
 */
TEST_F(CloseTest, ExplicitClose) {
    // Create dataset and add data
    auto ds = StarDataset::create(test_file);
    ds->put("data", NDArray<double>::zeros({100}));
    ds->flush();

    // Close the dataset (just flushes)
    ds->close();

    // All operations should still work after close()
    EXPECT_NO_THROW(ds->get<double>("data"));
    EXPECT_NO_THROW(ds->put("other", NDArray<double>::zeros({50})));
    EXPECT_NO_THROW(ds->flush());
    EXPECT_NO_THROW(ds->get_all_keys());
    EXPECT_NO_THROW(ds->get_metadata_keys());

    // Metadata operations should also work
    EXPECT_NO_THROW(ds->meta.put("version", NDArray<int>::full({1}, 1)));
    EXPECT_NO_THROW(ds->meta.get("version"));
    EXPECT_NO_THROW(ds->meta.contains("version"));
}

/**
 * Test 2: Idempotent Close
 * Verify that calling close() multiple times is safe
 */
TEST_F(CloseTest, IdempotentClose) {
    auto ds = StarDataset::create(test_file);
    ds->put("data", NDArray<int>::ones({50}));

    // Close multiple times - should not throw
    EXPECT_NO_THROW(ds->close());
    EXPECT_NO_THROW(ds->close());
    EXPECT_NO_THROW(ds->close());

    // Operations should still work after multiple closes
    EXPECT_NO_THROW(ds->get<int>("data"));
}

/**
 * Test 3: Destructor Closes Automatically
 * Verify that RAII pattern still works (destructor calls close)
 */
TEST_F(CloseTest, DestructorClosesAutomatically) {
    {
        auto ds = StarDataset::create(test_file);
        ds->put("data", NDArray<int>::full({50}, 123));
        // No explicit close - destructor should handle it
    }

    // File should be valid and readable
    auto ds = StarDataset::open(test_file);
    EXPECT_NO_THROW({
        auto data = ds->get<int>("data");
        EXPECT_EQ(data.size(), 50);
        EXPECT_EQ(data(0), 123);
    });
}

/**
 * Test 4: Thread-Safe Close
 * Verify that concurrent close() calls don't cause crashes
 */
TEST_F(CloseTest, ThreadSafeClose) {
    auto ds = StarDataset::create(test_file);
    ds->put("data", NDArray<double>::zeros({100}));

    // Spawn multiple threads that all try to close
    std::vector<std::thread> threads;
    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&ds]() {
            ds->close();  // Multiple threads calling close (flush)
        });
    }

    // Wait for all threads to finish
    for (auto& t : threads) {
        t.join();
    }

    // Should still be usable without crashes
    EXPECT_NO_THROW(ds->get_all_keys());
}

/**
 * Test 5: Close Flushes Pending Changes
 * Verify that close() properly flushes unflushed data
 */
TEST_F(CloseTest, CloseFlushesData) {
    {
        auto ds = StarDataset::create(test_file);
        ds->put("unflushed_data", NDArray<double>::full({100}, 3.14));
        // Don't call flush() explicitly - close() should do it
        ds->close();
    }

    // Reopen file and verify data was persisted
    auto ds = StarDataset::open(test_file);
    auto data = ds->get<double>("unflushed_data");
    EXPECT_EQ(data.size(), 100);
    EXPECT_DOUBLE_EQ(data(0), 3.14);
}

/**
 * Test 6: Error Message Quality
 * Verify that the error message is clear for missing keys
 */
TEST_F(CloseTest, ClearErrorMessages) {
    auto ds = StarDataset::create(test_file);
    ds->close();  // Just flushes

    // Should get key not found error, not closed error
    try {
        ds->get<double>("nonexistent");
        FAIL() << "Expected std::runtime_error";
    } catch (const std::runtime_error& e) {
        std::string msg = e.what();
        // Error should mention "not found" or similar
        EXPECT_TRUE(msg.find("not found") != std::string::npos ||
                    msg.find("Key not found") != std::string::npos);
    }
}

/**
 * Test 7: Smart Pointer Cleanup
 * Verify that using smart pointers for automatic cleanup still works
 */
TEST_F(CloseTest, SmartPointerCleanup) {
    {
        auto ds = StarDataset::create(test_file);
        ds->put("data", NDArray<float>::zeros({50}));
        // Smart pointer goes out of scope - should auto-close
    }

    // File should be properly closed and readable
    EXPECT_NO_THROW({
        auto ds = StarDataset::open(test_file);
        auto data = ds->get<float>("data");
        EXPECT_EQ(data.size(), 50);
    });
}

/**
 * Test 8: New Object Can Open After Close
 * Verify that creating a new object works after closing another
 */
TEST_F(CloseTest, NewObjectAfterClose) {
    // Create and close first object
    {
        auto ds = StarDataset::create(test_file);
        ds->put("data", NDArray<int>::arange(0, 100));
        ds->close();
    }

    // New object can open the file
    auto ds = StarDataset::open(test_file);
    auto data = ds->get<int>("data");
    EXPECT_EQ(data.size(), 100);
    EXPECT_EQ(data(0), 0);
    EXPECT_EQ(data(99), 99);
}
