#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "star.h"
#include <thread>
#include <vector>
#include <random>

using namespace star;

class ThreadingTest : public ::testing::Test {
protected:
    std::string testFile;

    void SetUp() override {
        testFile = "/tmp/test_threading.star";

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

    // Helper to generate test data
    std::vector<double> generateTestData(size_t size, unsigned seed = 42) {
        std::mt19937 gen(seed);
        std::uniform_real_distribution<> dis(0.0, 1000.0);

        std::vector<double> data(size);
        for (size_t i = 0; i < size; ++i) {
            data[i] = dis(gen);
        }
        return data;
    }
};

// Test 1: Global Configuration API
TEST_F(ThreadingTest, GlobalConfigurationAPI) {
    // Test getting default values
    size_t default_threads = getNumThreads();
    EXPECT_EQ(default_threads, 0);  // Default is auto-detect

    // Test setting thread count
    setNumThreads(4);
    EXPECT_EQ(getNumThreads(), 4);

    // Test setting thresholds
    setMinBlocksForThreading(8);
    setMinBytesForThreading(512 * 1024);

    // Reset to defaults
    setNumThreads(0);
    setMinBlocksForThreading(4);
    setMinBytesForThreading(256 * 1024);
}

// Test 2: ThreadPool Basic Functionality
TEST_F(ThreadingTest, ThreadPoolConstruction) {
    // Test auto-detect (0 threads)
    {
        ThreadPool pool(0);
        EXPECT_GT(pool.size(), 0);  // Should detect at least 1 thread
    }

    // Test explicit thread count
    {
        ThreadPool pool(4);
        EXPECT_EQ(pool.size(), 4);
    }

    // Test single-threaded
    {
        ThreadPool pool(1);
        EXPECT_EQ(pool.size(), 1);
    }
}

// Test 3: ThreadPool parallel_for Correctness
TEST_F(ThreadingTest, ThreadPoolParallelForCorrectness) {
    ThreadPool pool(4);

    // Test parallel_for produces correct results
    std::vector<int> results(100, 0);

    pool.parallel_for(0, 100, [&](size_t i) {
        results[i] = i * 2;
    });

    // Verify all elements computed correctly
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_EQ(results[i], static_cast<int>(i * 2));
    }
}

// Test 4: ThreadPool Task Ordering Independence
TEST_F(ThreadingTest, ThreadPoolTaskOrderingIndependence) {
    ThreadPool pool(4);

    // Run same computation multiple times, verify consistent results
    for (int trial = 0; trial < 10; ++trial) {
        std::vector<int> results(100, 0);

        pool.parallel_for(0, 100, [&](size_t i) {
            // Some computation
            results[i] = (i * i) % 97;
        });

        // Verify results are correct
        for (size_t i = 0; i < 100; ++i) {
            EXPECT_EQ(results[i], static_cast<int>((i * i) % 97));
        }
    }
}

// Test 5: StarDataset Threading Initialization
TEST_F(ThreadingTest, DatasetThreadingInitialization) {
    // Test with single-threaded mode
    setNumThreads(1);
    auto store1 = StarDataset::create(testFile);
    // Thread pool should be nullptr in single-threaded mode

    // Test with multi-threaded mode
    setNumThreads(4);
    std::string testFile2 = "/tmp/test_threading2.star";
    auto store2 = StarDataset::create(testFile2);
    // Thread pool should be initialized

    // Clean up
    if (fs::exists(testFile2)) {
        fs::remove(testFile2);
    }

    // Reset to default
    setNumThreads(0);
}

// Test 6: Correctness - Small Data (Single-Threaded Path)
TEST_F(ThreadingTest, SmallDataSingleThreadedPath) {
    setNumThreads(4);  // Enable threading

    auto store = StarDataset::create(testFile);

    // Create small array (should use single-threaded path due to heuristic)
    NDArray<double> small_arr({10});
    for (size_t i = 0; i < 10; ++i) {
        small_arr.flat(i) = static_cast<double>(i);
    }

    store->meta.put("small_array", small_arr);
    store->flush();

    // Read back and verify
    auto meta = store->meta.get("small_array");
    ASSERT_NE(meta, nullptr);
    auto retrieved = meta->as<double>();

    for (size_t i = 0; i < 10; ++i) {
        EXPECT_DOUBLE_EQ(retrieved.flat(i), static_cast<double>(i));
    }

    setNumThreads(0);  // Reset
}

// Test 7: Correctness - Large Data (Multi-Threaded Path)
TEST_F(ThreadingTest, LargeDataMultiThreadedPath) {
    setNumThreads(4);  // Enable threading

    auto store = StarDataset::create(testFile);

    // Create large array (should trigger multi-threaded path)
    size_t size = 1000000;  // 1M elements = 8MB
    auto data = generateTestData(size);

    NDArray<double> large_arr({size});
    for (size_t i = 0; i < size; ++i) {
        large_arr.flat(i) = data[i];
    }

    store->put("large_array", std::move(large_arr));
    store->flush();

    // Read back and verify
    auto store2 = StarDataset::open(testFile);
    auto retrieved = store2->get<double>("large_array");

    ASSERT_EQ(retrieved.shape(0), size);

    // Verify all data matches
    for (size_t i = 0; i < size; ++i) {
        EXPECT_DOUBLE_EQ(retrieved.flat(i), data[i]);
    }

    setNumThreads(0);  // Reset
}

// Test 8: Thread Safety - Concurrent Reads
TEST_F(ThreadingTest, ConcurrentReads) {
    setNumThreads(4);

    // Create store with some data
    auto store = StarDataset::create(testFile);

    NDArray<int32_t> arr({100});
    for (size_t i = 0; i < 100; ++i) {
        arr.flat(i) = static_cast<int32_t>(i);
    }

    store->put("test_array", std::move(arr));
    store->flush();

    // Open multiple times and read concurrently
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int t = 0; t < 8; ++t) {
        threads.emplace_back([&, t]() {
            try {
                auto reader = StarDataset::open(testFile, FileMode::READ_ONLY);
                auto data = reader->get<int32_t>("test_array");

                // Verify data
                bool correct = true;
                for (size_t i = 0; i < 100; ++i) {
                    if (data.flat(i) != static_cast<int32_t>(i)) {
                        correct = false;
                        break;
                    }
                }

                if (correct) {
                    success_count++;
                }
            } catch (...) {
                // Thread failed
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_EQ(success_count.load(), 8);  // All threads should succeed

    setNumThreads(0);  // Reset
}

// Test 9: Edge Case - Zero Threads (Auto-Detect)
TEST_F(ThreadingTest, ZeroThreadsAutoDetect) {
    setNumThreads(0);  // Auto-detect

    auto store = StarDataset::create(testFile);

    NDArray<double> arr({1000});
    for (size_t i = 0; i < 1000; ++i) {
        arr.flat(i) = static_cast<double>(i);
    }

    store->meta.put("array", arr);
    store->flush();

    auto meta = store->meta.get("array");
    ASSERT_NE(meta, nullptr);
    auto retrieved = meta->as<double>();

    for (size_t i = 0; i < 1000; ++i) {
        EXPECT_DOUBLE_EQ(retrieved.flat(i), static_cast<double>(i));
    }
}

// Test 10: Edge Case - Single Block
TEST_F(ThreadingTest, SingleBlockCompression) {
    setNumThreads(4);

    auto store = StarDataset::create(testFile);

    // Create array that fits in single block (< 1MB default block size)
    size_t size = 50000;  // 400KB for doubles
    NDArray<double> arr({size});
    for (size_t i = 0; i < size; ++i) {
        arr.flat(i) = static_cast<double>(i) * 0.5;
    }

    store->put("single_block_array", std::move(arr));
    store->flush();

    // Read back and verify
    auto store2 = StarDataset::open(testFile);
    auto retrieved = store2->get<double>("single_block_array");

    for (size_t i = 0; i < size; ++i) {
        EXPECT_DOUBLE_EQ(retrieved.flat(i), static_cast<double>(i) * 0.5);
    }

    setNumThreads(0);  // Reset
}

// Test 11: Compression Ratios Unchanged
TEST_F(ThreadingTest, CompressionRatiosUnchanged) {
    // Compare file sizes with single-threaded vs multi-threaded
    size_t size = 1000000;
    auto data = generateTestData(size);

    // Single-threaded
    setNumThreads(1);
    std::string file1 = "/tmp/test_threading_st.star";
    {
        auto store = StarDataset::create(file1);
        NDArray<double> arr({size});
        for (size_t i = 0; i < size; ++i) {
            arr.flat(i) = data[i];
        }
        store->put("data", std::move(arr));
        store->flush();
    }

    size_t size_st = fs::file_size(file1);

    // Multi-threaded
    setNumThreads(4);
    std::string file2 = "/tmp/test_threading_mt.star";
    {
        auto store = StarDataset::create(file2);
        NDArray<double> arr({size});
        for (size_t i = 0; i < size; ++i) {
            arr.flat(i) = data[i];
        }
        store->put("data", std::move(arr));
        store->flush();
    }

    size_t size_mt = fs::file_size(file2);

    // File sizes should be identical (bit-for-bit same compression)
    EXPECT_EQ(size_st, size_mt);

    // Clean up
    fs::remove(file1);
    fs::remove(file2);

    setNumThreads(0);  // Reset
}
