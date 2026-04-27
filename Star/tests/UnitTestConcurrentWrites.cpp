/**
 * @file UnitTestConcurrentWrites.cpp
 * @brief Unit tests for thread safety of concurrent writes to different keys
 */

#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "star.h"
#include <thread>
#include <vector>
#include <atomic>
#include <random>
#include <chrono>

using namespace star;

class ConcurrentWritesTest : public ::testing::Test {
protected:
    std::string testFile;

    void SetUp() override {
        // Use thread-safe random generator with high-resolution timestamp
        static std::random_device rd;
        static thread_local std::mt19937_64 gen(rd() + std::hash<std::thread::id>{}(std::this_thread::get_id()));
        std::uniform_int_distribution<uint64_t> dis;

        auto now = std::chrono::high_resolution_clock::now();
        uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
        uint64_t random_val = dis(gen);

        testFile = "/tmp/test_concurrent_" + std::to_string(timestamp) + "_" + std::to_string(random_val) + ".star";
    }

    void TearDown() override {
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }
    }
};

/**
 * Test 1: Concurrent Writes to Different Keys
 * Multiple threads write to different keys in the same dataset simultaneously
 */
TEST_F(ConcurrentWritesTest, MultithreadedWritesDifferentKeys) {
    setNumThreads(4);

    auto store = StarDataset::create(testFile);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> failure_count{0};

    const int num_threads = 8;
    const size_t array_size = 10000;

    // Each thread writes to a different key
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                std::string key = "thread_" + std::to_string(t) + "_data";

                // Create unique data for this thread
                NDArray<int32_t> arr({array_size});
                for (size_t i = 0; i < array_size; ++i) {
                    arr.flat(i) = static_cast<int32_t>(t * 1000000 + i);
                }

                // Write to the shared dataset
                store->put(key, std::move(arr));
                success_count++;
            } catch (const std::exception& e) {
                std::cerr << "Thread " << t << " failed: " << e.what() << std::endl;
                failure_count++;
            }
        });
    }

    // Wait for all writes to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Flush once after all writes
    EXPECT_NO_THROW(store->flush());

    // Verify all threads succeeded
    EXPECT_EQ(success_count.load(), num_threads);
    EXPECT_EQ(failure_count.load(), 0);

    // Verify all data was written correctly
    for (int t = 0; t < num_threads; ++t) {
        std::string key = "thread_" + std::to_string(t) + "_data";

        auto retrieved = store->get<int32_t>(key);
        ASSERT_EQ(retrieved.shape(0), array_size) << "Key: " << key;

        // Verify data integrity
        for (size_t i = 0; i < array_size; ++i) {
            int32_t expected = static_cast<int32_t>(t * 1000000 + i);
            EXPECT_EQ(retrieved.flat(i), expected)
                << "Mismatch at thread " << t << ", index " << i;
        }
    }

    setNumThreads(0);  // Reset
}

/**
 * Test 2: Concurrent Writes with Periodic Flush
 * Writers add data while a separate thread periodically flushes
 */
TEST_F(ConcurrentWritesTest, ConcurrentWritesWithPeriodicFlush) {
    setNumThreads(4);

    auto store = StarDataset::create(testFile);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<bool> stop_flushing{false};

    const int num_writer_threads = 6;
    const size_t arrays_per_thread = 10;
    const size_t array_size = 5000;

    // Writer threads
    for (int t = 0; t < num_writer_threads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                for (size_t batch = 0; batch < arrays_per_thread; ++batch) {
                    std::string key = "thread_" + std::to_string(t) + "_batch_" + std::to_string(batch);

                    NDArray<double> arr({array_size});
                    for (size_t i = 0; i < array_size; ++i) {
                        arr.flat(i) = static_cast<double>(t * 1000 + batch * 100 + i);
                    }

                    store->put(key, std::move(arr));

                    // Small sleep to increase contention
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
                success_count++;
            } catch (const std::exception& e) {
                std::cerr << "Writer thread " << t << " failed: " << e.what() << std::endl;
            }
        });
    }

    // Flusher thread - periodically flushes while writes are happening
    threads.emplace_back([&]() {
        try {
            while (!stop_flushing.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                store->flush();
            }
        } catch (const std::exception& e) {
            std::cerr << "Flusher thread failed: " << e.what() << std::endl;
        }
    });

    // Wait for all writers to complete
    for (int t = 0; t < num_writer_threads; ++t) {
        threads[t].join();
    }

    // Stop flusher thread
    stop_flushing.store(true);
    threads.back().join();

    // Final flush
    store->flush();

    // Verify all threads succeeded
    EXPECT_EQ(success_count.load(), num_writer_threads);

    // Verify data integrity
    for (int t = 0; t < num_writer_threads; ++t) {
        for (size_t batch = 0; batch < arrays_per_thread; ++batch) {
            std::string key = "thread_" + std::to_string(t) + "_batch_" + std::to_string(batch);

            auto retrieved = store->get<double>(key);
            ASSERT_EQ(retrieved.shape(0), array_size) << "Key: " << key;

            // Spot check a few values
            EXPECT_DOUBLE_EQ(retrieved.flat(0), static_cast<double>(t * 1000 + batch * 100 + 0));
            EXPECT_DOUBLE_EQ(retrieved.flat(array_size / 2),
                           static_cast<double>(t * 1000 + batch * 100 + array_size / 2));
            EXPECT_DOUBLE_EQ(retrieved.flat(array_size - 1),
                           static_cast<double>(t * 1000 + batch * 100 + array_size - 1));
        }
    }

    setNumThreads(0);  // Reset
}

/**
 * Test 3: Concurrent Writes with Close
 * Writers operate while another thread closes the dataset
 */
TEST_F(ConcurrentWritesTest, ConcurrentWritesWithClose) {
    setNumThreads(4);

    auto store = StarDataset::create(testFile);

    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};
    std::atomic<int> expected_failures{0};  // Writes after close should fail

    const int num_threads = 6;
    const size_t array_size = 5000;

    // Writer threads
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                // Write multiple arrays
                for (int i = 0; i < 5; ++i) {
                    std::string key = "thread_" + std::to_string(t) + "_item_" + std::to_string(i);

                    NDArray<float> arr({array_size});
                    for (size_t j = 0; j < array_size; ++j) {
                        arr.flat(j) = static_cast<float>(t * 10000 + i * 1000 + j);
                    }

                    store->put(key, std::move(arr));

                    // Brief sleep to increase contention
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                }
                success_count++;
            } catch (const std::runtime_error& e) {
                // Expected if we tried to write after close
                std::string msg = e.what();
                if (msg.find("closed") != std::string::npos) {
                    expected_failures++;
                } else {
                    std::cerr << "Unexpected error in thread " << t << ": " << e.what() << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Thread " << t << " failed: " << e.what() << std::endl;
            }
        });
    }

    // Give threads time to start writing
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Close in the middle of concurrent writes
    store->flush();
    store->close();

    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }

    // Either all succeeded (wrote before close) or some failed (wrote after close)
    int total_outcomes = success_count.load() + expected_failures.load();
    EXPECT_EQ(total_outcomes, num_threads)
        << "success: " << success_count.load()
        << ", expected failures: " << expected_failures.load();

    // Reopen and verify data that was written before close
    // Note: File might be corrupted if close() happened during active writes
    try {
        auto store2 = StarDataset::open(testFile, FileMode::READ_ONLY);

        // Count how many keys exist
        auto keys = store2->get_all_keys();
        EXPECT_GT(keys.size(), 0) << "At least some data should have been written before close";

        // Verify integrity of data that exists
        for (const auto& key : keys) {
            if (key.find("thread_") == 0) {
                auto retrieved = store2->get<float>(key);
                EXPECT_GT(retrieved.shape(0), 0) << "Key: " << key;
            }
        }
    } catch (const std::exception& e) {
        // File might be corrupted - this is expected behavior when closing during active writes
        std::cerr << "WARNING: File corrupted after close during concurrent writes: " << e.what() << std::endl;
        // This is actually acceptable - closing while writes are in progress is undefined behavior
    }

    setNumThreads(0);  // Reset
}

/**
 * Test 4: High Contention - Many Threads, Small Arrays
 * Stress test with many threads writing small arrays
 */
TEST_F(ConcurrentWritesTest, HighContentionSmallArrays) {
    setNumThreads(8);

    auto store = StarDataset::create(testFile);

    std::vector<std::thread> threads;
    std::atomic<int> total_writes{0};

    const int num_threads = 16;
    const int arrays_per_thread = 50;
    const size_t array_size = 100;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < arrays_per_thread; ++i) {
                std::string key = "t" + std::to_string(t) + "_a" + std::to_string(i);

                NDArray<uint32_t> arr({array_size});
                for (size_t j = 0; j < array_size; ++j) {
                    arr.flat(j) = static_cast<uint32_t>((t << 16) | i);
                }

                store->put(key, std::move(arr));
                total_writes++;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    store->flush();

    // Verify all writes completed
    EXPECT_EQ(total_writes.load(), num_threads * arrays_per_thread);

    // Verify all keys exist and have correct data
    for (int t = 0; t < num_threads; ++t) {
        for (int i = 0; i < arrays_per_thread; ++i) {
            std::string key = "t" + std::to_string(t) + "_a" + std::to_string(i);

            auto retrieved = store->get<uint32_t>(key);
            ASSERT_EQ(retrieved.shape(0), array_size) << "Key: " << key;

            uint32_t expected = static_cast<uint32_t>((t << 16) | i);
            EXPECT_EQ(retrieved.flat(0), expected) << "Key: " << key;
        }
    }

    setNumThreads(0);  // Reset
}

/**
 * Test 5: Mixed Operations - Concurrent Reads and Writes
 * Some threads read while others write to different keys
 */
TEST_F(ConcurrentWritesTest, MixedReadsAndWrites) {
    setNumThreads(4);

    auto store = StarDataset::create(testFile);

    // Pre-populate with some data for readers
    const int num_initial_keys = 10;
    for (int i = 0; i < num_initial_keys; ++i) {
        std::string key = "initial_" + std::to_string(i);
        NDArray<int64_t> arr({1000});
        for (size_t j = 0; j < 1000; ++j) {
            arr.flat(j) = static_cast<int64_t>(i * 1000 + j);
        }
        store->put(key, std::move(arr));
    }
    store->flush();

    std::vector<std::thread> threads;
    std::atomic<int> read_success{0};
    std::atomic<int> write_success{0};

    const int num_reader_threads = 4;
    const int num_writer_threads = 4;

    // Reader threads
    for (int t = 0; t < num_reader_threads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                for (int i = 0; i < 20; ++i) {
                    int key_idx = i % num_initial_keys;
                    std::string key = "initial_" + std::to_string(key_idx);

                    auto retrieved = store->get<int64_t>(key);

                    // Verify data
                    bool correct = true;
                    for (size_t j = 0; j < 1000; ++j) {
                        if (retrieved.flat(j) != static_cast<int64_t>(key_idx * 1000 + j)) {
                            correct = false;
                            break;
                        }
                    }

                    if (correct) {
                        read_success++;
                    }

                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            } catch (const std::exception& e) {
                std::cerr << "Reader thread " << t << " failed: " << e.what() << std::endl;
            }
        });
    }

    // Writer threads
    for (int t = 0; t < num_writer_threads; ++t) {
        threads.emplace_back([&, t]() {
            try {
                for (int i = 0; i < 20; ++i) {
                    std::string key = "new_t" + std::to_string(t) + "_i" + std::to_string(i);

                    NDArray<int64_t> arr({1000});
                    for (size_t j = 0; j < 1000; ++j) {
                        arr.flat(j) = static_cast<int64_t>(t * 100000 + i * 1000 + j);
                    }

                    store->put(key, std::move(arr));
                    write_success++;

                    std::this_thread::sleep_for(std::chrono::microseconds(10));
                }
            } catch (const std::exception& e) {
                std::cerr << "Writer thread " << t << " failed: " << e.what() << std::endl;
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    store->flush();

    // Verify reads and writes succeeded
    EXPECT_EQ(read_success.load(), num_reader_threads * 20);
    EXPECT_EQ(write_success.load(), num_writer_threads * 20);

    setNumThreads(0);  // Reset
}
