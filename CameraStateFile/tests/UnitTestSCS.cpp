#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "scs.h"


TEST(SCStoreTest, BasicWriteAndRead) {
    try {
        // Create a temporary file for testing
        std::string testFile = "/tmp/test_ndarray_write.scs";

        // Remove the file if it already exists
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        // Create a new SCStore with GZIP compression
        SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

        // Create various ndarrays to test with

        // 1D integer array
        ndarray<int> intArray({5});
        for (size_t i = 0; i < 5; i++) {
            intArray(i) = i * 10;
        }

        // 2D float array
        ndarray<float> floatArray({3, 4});
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
                floatArray(i, j) = i * 10.0f + j;
            }
        }

        // 3D double array
        ndarray<double> doubleArray({2, 2, 2});
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
                for (size_t k = 0; k < 2; k++) {
                    doubleArray(i, j, k) = i * 100.0 + j * 10.0 + k;
                }
            }
        }

        // String array
        ndarray<std::string> stringArray({2, 3});
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 3; j++) {
                stringArray(i, j) = "str_" + std::to_string(i) + "_" + std::to_string(j);
            }
        }

        // Store the arrays
        store.put("int_array", intArray);
        store.put("float_array", floatArray);
        store.put("double_array", doubleArray);
        store.put("string_array", stringArray);

        // Verify the store contains the keys
        EXPECT_TRUE(store.contains("int_array"));
        EXPECT_TRUE(store.contains("float_array"));
        EXPECT_TRUE(store.contains("double_array"));
        EXPECT_TRUE(store.contains("string_array"));

        // Flush to disk
        store.flush();

        // Retrieve the arrays and verify their contents
        auto retrievedIntArray = store.get<ndarray<int>>("int_array");
        ASSERT_NE(retrievedIntArray, nullptr);
        EXPECT_EQ(retrievedIntArray->shape().size(), 1);
        EXPECT_EQ(retrievedIntArray->shape(0), 5);
        for (size_t i = 0; i < 5; i++) {
            EXPECT_EQ((*retrievedIntArray)(i), i * 10);
        }

        auto retrievedFloatArray = store.get<ndarray<float>>("float_array");
        ASSERT_NE(retrievedFloatArray, nullptr);
        EXPECT_EQ(retrievedFloatArray->shape().size(), 2);
        EXPECT_EQ(retrievedFloatArray->shape(0), 3);
        EXPECT_EQ(retrievedFloatArray->shape(1), 4);
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
                EXPECT_FLOAT_EQ((*retrievedFloatArray)(i, j), i * 10.0f + j);
            }
        }

        auto retrievedDoubleArray = store.get<ndarray<double>>("double_array");
        ASSERT_NE(retrievedDoubleArray, nullptr);
        EXPECT_EQ(retrievedDoubleArray->shape().size(), 3);
        EXPECT_EQ(retrievedDoubleArray->shape(0), 2);
        EXPECT_EQ(retrievedDoubleArray->shape(1), 2);
        EXPECT_EQ(retrievedDoubleArray->shape(2), 2);
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
                for (size_t k = 0; k < 2; k++) {
                    EXPECT_DOUBLE_EQ((*retrievedDoubleArray)(i, j, k), i * 100.0 + j * 10.0 + k);
                }
            }
        }

        auto retrievedStringArray = store.get<ndarray<std::string>>("string_array");
        ASSERT_NE(retrievedStringArray, nullptr);
        EXPECT_EQ(retrievedStringArray->shape().size(), 2);
        EXPECT_EQ(retrievedStringArray->shape(0), 2);
        EXPECT_EQ(retrievedStringArray->shape(1), 3);
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 3; j++) {
                EXPECT_EQ((*retrievedStringArray)(i, j), "str_" + std::to_string(i) + "_" + std::to_string(j));
            }
        }

        // Verify the file exists
        EXPECT_TRUE(fs::exists(testFile));

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, PersistenceTest) {
    try {
        // Create a temporary file for testing
        fs::path testFile = "/tmp/persistence_test.scs";

        // Remove the file if it already exists
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        // Create arrays and store them
        {
            SCStore store(testFile.string(), CompressionAlgorithm::GZIP, 1024);

            // 1D int array
            ndarray<int> intArray({5});
            for (size_t i = 0; i < 5; i++) {
                intArray(i) = i * 10;
            }

            // 2D float array
            ndarray<float> floatArray({2, 3});
            for (size_t i = 0; i < 2; i++) {
                for (size_t j = 0; j < 3; j++) {
                    floatArray(i, j) = i * 10.0f + j + 0.5f;
                }
            }

            // String array
            ndarray<std::string> stringArray({2, 2});
            stringArray(0, 0) = "hello";
            stringArray(0, 1) = "world";
            stringArray(1, 0) = "test";
            stringArray(1, 1) = "persistence";

            // Store the arrays
            store.put("int_array", intArray);
            store.put("float_array", floatArray);
            store.put("string_array", stringArray);

            // Flush to ensure data is written
            store.flush();

            // Store goes out of scope here and destructor is called
        }

        // Verify the file exists
        EXPECT_TRUE(fs::exists(testFile));

        // Reopen the file and verify the contents
        {
            SCStore store(testFile.string());

            // Verify the store contains the keys
            EXPECT_TRUE(store.contains("int_array"));
            EXPECT_TRUE(store.contains("float_array"));
            EXPECT_TRUE(store.contains("string_array"));

            // Retrieve and verify int array
            auto intArrayPtr = store.get<ndarray<int>>("int_array");
            EXPECT_NE(intArrayPtr, nullptr);
            EXPECT_EQ(intArrayPtr->shape().size(), 1);
            EXPECT_EQ(intArrayPtr->shape(0), 5);
            for (size_t i = 0; i < 5; i++) {
                EXPECT_EQ((*intArrayPtr)(i), i * 10);
            }

            // Retrieve and verify float array
            auto floatArrayPtr = store.get<ndarray<float>>("float_array");
            EXPECT_NE(floatArrayPtr, nullptr);
            EXPECT_EQ(floatArrayPtr->shape().size(), 2);
            EXPECT_EQ(floatArrayPtr->shape(0), 2);
            EXPECT_EQ(floatArrayPtr->shape(1), 3);
            for (size_t i = 0; i < 2; i++) {
                for (size_t j = 0; j < 3; j++) {
                    EXPECT_FLOAT_EQ((*floatArrayPtr)(i, j), i * 10.0f + j + 0.5f);
                }
            }

            // Retrieve and verify string array
            auto stringArrayPtr = store.get<ndarray<std::string>>("string_array");
            EXPECT_NE(stringArrayPtr, nullptr);
            EXPECT_EQ(stringArrayPtr->shape().size(), 2);
            EXPECT_EQ(stringArrayPtr->shape(0), 2);
            EXPECT_EQ(stringArrayPtr->shape(1), 2);
            EXPECT_EQ((*stringArrayPtr)(0, 0), "hello");
            EXPECT_EQ((*stringArrayPtr)(0, 1), "world");
            EXPECT_EQ((*stringArrayPtr)(1, 0), "test");
            EXPECT_EQ((*stringArrayPtr)(1, 1), "persistence");
        }

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, StaticFactoryMethodsTest) {
    try {
        std::string testFile = "/tmp/test_factory_methods.scs";

        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

        // Test zeros factory
        auto zeros = ndarray<double>::zeros({3, 4});
        for (size_t i = 0; i < zeros.size(); ++i) {
            EXPECT_DOUBLE_EQ(zeros.flat(i), 0.0);
        }
        store.put("zeros", zeros);

        // Test ones factory
        auto ones = ndarray<int>::ones({5});
        for (size_t i = 0; i < 5; ++i) {
            EXPECT_EQ(ones(i), 1);
        }
        store.put("ones", ones);

        // Test arange factory
        auto arange = ndarray<int>::arange(0, 10, 2);
        EXPECT_EQ(arange.size(), 5);
        EXPECT_EQ(arange(0), 0);
        EXPECT_EQ(arange(1), 2);
        EXPECT_EQ(arange(4), 8);
        store.put("arange", arange);

        store.flush();

        // Read back and verify
        auto read_zeros = store.get<ndarray<double>>("zeros");
        ASSERT_NE(read_zeros, nullptr);
        EXPECT_EQ(read_zeros->shape(0), 3);
        EXPECT_EQ(read_zeros->shape(1), 4);

        auto read_ones = store.get<ndarray<int>>("ones");
        ASSERT_NE(read_ones, nullptr);
        EXPECT_EQ(read_ones->shape(0), 5);

        auto read_arange = store.get<ndarray<int>>("arange");
        ASSERT_NE(read_arange, nullptr);
        EXPECT_EQ(read_arange->size(), 5);

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, IteratorTest) {
    try {
        std::string testFile = "/tmp/test_iterator.scs";

        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

        // Create array and populate using operator()
        ndarray<int> arr({10});
        for (size_t i = 0; i < 10; ++i) {
            arr(i) = i * 2;
        }

        // Verify using iterator
        int sum = 0;
        for (const auto& val : arr) {
            sum += val;
        }
        EXPECT_EQ(sum, 90); // 0+2+4+6+8+10+12+14+16+18 = 90

        store.put("array", arr);
        store.flush();

        // Read back and verify with iterator
        auto retrieved = store.get<ndarray<int>>("array");
        ASSERT_NE(retrieved, nullptr);

        int retrieved_sum = 0;
        for (const auto& val : *retrieved) {
            retrieved_sum += val;
        }
        EXPECT_EQ(retrieved_sum, 90);

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, MultipleDataTypesTest) {
    try {
        std::string testFile = "/tmp/test_multiple_types.scs";

        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

        // Test various integer types
        ndarray<int8_t> int8arr({5});
        ndarray<int16_t> int16arr({5});
        ndarray<int32_t> int32arr({5});
        ndarray<int64_t> int64arr({5});

        for (size_t i = 0; i < 5; ++i) {
            int8arr(i) = static_cast<int8_t>(i);
            int16arr(i) = static_cast<int16_t>(i * 100);
            int32arr(i) = static_cast<int32_t>(i * 10000);
            int64arr(i) = static_cast<int64_t>(i * 1000000);
        }

        store.put("int8", int8arr);
        store.put("int16", int16arr);
        store.put("int32", int32arr);
        store.put("int64", int64arr);

        // Test unsigned types
        ndarray<uint8_t> uint8arr({3});
        ndarray<uint16_t> uint16arr({3});
        ndarray<uint32_t> uint32arr({3});
        ndarray<uint64_t> uint64arr({3});

        for (size_t i = 0; i < 3; ++i) {
            uint8arr(i) = static_cast<uint8_t>(i + 200);
            uint16arr(i) = static_cast<uint16_t>(i + 50000);
            uint32arr(i) = static_cast<uint32_t>(i + 3000000000);
            uint64arr(i) = static_cast<uint64_t>(i + 10000000000ULL);
        }

        store.put("uint8", uint8arr);
        store.put("uint16", uint16arr);
        store.put("uint32", uint32arr);
        store.put("uint64", uint64arr);

        store.flush();

        // Verify all types can be read back
        auto read_int32 = store.get<ndarray<int32_t>>("int32");
        ASSERT_NE(read_int32, nullptr);
        EXPECT_EQ((*read_int32)(2), 20000);

        auto read_uint64 = store.get<ndarray<uint64_t>>("uint64");
        ASSERT_NE(read_uint64, nullptr);
        EXPECT_EQ((*read_uint64)(1), 10000000001ULL);

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, ReshapeAndResizeTest) {
    try {
        std::string testFile = "/tmp/test_reshape_resize.scs";

        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        SCStore store(testFile, CompressionAlgorithm::GZIP, 1024);

        // Create a 1D array
        ndarray<int> arr({12});
        for (size_t i = 0; i < 12; ++i) {
            arr(i) = i;
        }

        // Reshape to 3x4 (no reallocation)
        arr.reshape({3, 4});
        EXPECT_EQ(arr.shape(0), 3);
        EXPECT_EQ(arr.shape(1), 4);
        EXPECT_EQ(arr.size(), 12);
        EXPECT_EQ(arr(2, 3), 11); // Last element

        store.put("reshaped", arr);

        // Create another array and resize it (with reallocation)
        ndarray<double> arr2({5});
        for (size_t i = 0; i < 5; ++i) {
            arr2(i) = i * 1.5;
        }

        arr2.resize({10}, 99.0);
        EXPECT_EQ(arr2.size(), 10);
        EXPECT_DOUBLE_EQ(arr2(4), 6.0); // Original value
        EXPECT_DOUBLE_EQ(arr2(9), 99.0); // New value

        store.put("resized", arr2);
        store.flush();

        // Read back and verify
        auto read_reshaped = store.get<ndarray<int>>("reshaped");
        ASSERT_NE(read_reshaped, nullptr);
        EXPECT_EQ(read_reshaped->shape().size(), 2);
        EXPECT_EQ(read_reshaped->shape(0), 3);
        EXPECT_EQ(read_reshaped->shape(1), 4);

        auto read_resized = store.get<ndarray<double>>("resized");
        ASSERT_NE(read_resized, nullptr);
        EXPECT_EQ(read_resized->size(), 10);

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, EmptyStoreTest) {
    try {
        fs::path testFile = "/tmp/empty_store_test.scs";

        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }

        // Create a store with no values and flush it
        {
            SCStore store(testFile.string(), CompressionAlgorithm::GZIP, 1024);
            // Don't add any values
            store.flush();
        }

        // Reopen the store and verify it's empty
        {
            SCStore store(testFile.string());
            EXPECT_FALSE(store.contains("any_key"));
        }

        // Add values, flush, and verify
        {
            SCStore store(testFile.string(), CompressionAlgorithm::GZIP, 1024);

            ndarray<int> intArray({3});
            intArray(0) = 1;
            intArray(1) = 2;
            intArray(2) = 3;

            ndarray<float> floatArray({2, 3});
            for (size_t i = 0; i < floatArray.size(); ++i) {
                floatArray.flat(i) = i * 0.5f;
            }

            store.put("temp_key", intArray);
            store.put("temp_key2", floatArray);
            EXPECT_TRUE(store.contains("temp_key"));
            EXPECT_TRUE(store.contains("temp_key2"));

            store.flush();
        }

        // Reopen and verify values are persisted
        {
            SCStore store(testFile.string());
            EXPECT_TRUE(store.contains("temp_key"));
            EXPECT_TRUE(store.contains("temp_key2"));
        }

        // Clean up
        fs::remove(testFile);

    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}
