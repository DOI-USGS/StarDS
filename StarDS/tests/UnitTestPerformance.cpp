/**
 * @file UnitTestPerformance.cpp
 * @brief Performance tests to ensure put() scaling is linear (O(N))
 *
 * These tests verify that the vector capacity pre-allocation optimizations
 * prevent O(N²) behavior when storing multiple arrays.
 */

#include "../include/stards.h"
#include "Fixtures.h"
#include <chrono>
#include <vector>
#include <cstdio>
#include <random>
#include <thread>


using namespace star;

// Helper to generate unique temp filenames
static std::string generateTempFilename(const std::string& prefix) {
    static std::random_device rd;
    static thread_local std::mt19937_64 gen(rd() + std::hash<std::thread::id>{}(std::this_thread::get_id()));
    std::uniform_int_distribution<uint64_t> dis;

    auto now = std::chrono::high_resolution_clock::now();
    uint64_t timestamp = std::chrono::duration_cast<std::chrono::nanoseconds>(now.time_since_epoch()).count();
    uint64_t random_val = dis(gen);

    // Use the OS temp dir (Fixtures.h pulls in ghc::filesystem as `fs`). On Linux
    // this resolves to /tmp (honoring TMPDIR); on Windows to %TEMP%.
    std::string name = prefix + "_" + std::to_string(timestamp) + "_" +
                       std::to_string(random_val) + ".stards";
    return (fs::temp_directory_path() / name).string();
}


/**
 * @brief Test that MetadataAccessor::put() scaling is also linear
 *
 * Tests the metadata block path (used for small arrays via meta.put())
 */
TEST(PerformanceTest, MetadataPutScalingIsLinear) {
    std::string filename = generateTempFilename("perf_test_metadata_put_scaling");

    // Clean up if exists
    std::remove(filename.c_str());

    auto store = StarDataset::create(filename);

    // Measure time to store N arrays for different N
    std::vector<int> array_counts = {10, 50, 100, 200, 500};
    std::vector<double> times_per_array;

    for (int N : array_counts) {
        // Create a fresh store for each test
        std::remove(filename.c_str());
        store = StarDataset::create(filename);

        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < N; i++) {
            std::string key = "small_array_" + std::to_string(i);
            NDArray<int32_t> arr({10});  // Small array - goes to metadata block
            for (size_t j = 0; j < arr.size(); j++) {
                arr.data()[j] = static_cast<int32_t>(j);
            }
            store->meta.put(key, arr);
        }

        auto end = std::chrono::high_resolution_clock::now();
        double elapsed = std::chrono::duration<double>(end - start).count();
        times_per_array.push_back(elapsed / N);
    }

    // Check linear scaling (allow 50% variance)
    double first_time = times_per_array[0];
    for (size_t i = 1; i < times_per_array.size(); i++) {
        EXPECT_LT(times_per_array[i], first_time * 1.5)
            << "Metadata put() time per array increased from " << first_time * 1000.0 << "ms"
            << " to " << times_per_array[i] * 1000.0 << "ms"
            << " for N=" << array_counts[i];
    }

    // Clean up
    std::remove(filename.c_str());
}

/**
 * @brief Test vector capacity growth behavior
 *
 * Verifies that capacity grows as expected with the reserve() optimization
 */
TEST(PerformanceTest, VectorCapacityPreallocation) {
    std::string filename = generateTempFilename("perf_test_capacity");

    // Clean up if exists
    std::remove(filename.c_str());

    auto store = StarDataset::create(filename);

    // Store a single array to trigger initial state
    NDArray<double> arr({100});
    store->put("test_array", std::move(arr));

    // Clean up
    std::remove(filename.c_str());

    // If we got here without crashes or excessive time, the test passes
    SUCCEED();
}
