#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "scs.h"
// #include "deflate.h"

TEST(UnitTestSCS, TestWrite) {
    try {
        // Create a temporary file for testing
        std::string testFile = "/tmp/test_ndarray_write.scs";
        
        // Remove the file if it already exists
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }
        
        // Create a new SCStore
        SCStore store(testFile);
        
        // Create various NDArrays to test with
        
        // 1D integer array
        NDArray<int> intArray({5}, 0);
        for (size_t i = 0; i < 5; i++) {
            intArray.data[i] = i * 10;
        }
        
        // 2D float array
        NDArray<float> floatArray({3, 4}, 0.0f);
        for (size_t i = 0; i < 3; i++) {
            for (size_t j = 0; j < 4; j++) {
                floatArray.at({i, j}) = i * 10.0f + j;
            }
        }
        
        // 3D double array
        NDArray<double> doubleArray({2, 2, 2}, 0.0);
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 2; j++) {
                for (size_t k = 0; k < 2; k++) {
                    doubleArray.at({i, j, k}) = i * 100.0 + j * 10.0 + k;
                }
            }
        }
        
        // String array
        NDArray<std::string> stringArray({2, 3}, "");
        for (size_t i = 0; i < 2; i++) {
            for (size_t j = 0; j < 3; j++) {
                stringArray.at({i, j}) = "str_" + std::to_string(i) + "_" + std::to_string(j);
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
        
        // Verify the store size
        EXPECT_EQ(store.size(), 4);
        store.flush();
        // Verify the file exists
        EXPECT_TRUE(fs::exists(testFile));
        
    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}

TEST(SCStoreTest, ReadTest) {
    try {
        // Create a temporary file for testing
        fs::path testFile = "/tmp/persistence_test.scs";
        
        // Remove the file if it already exists
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }
        
        // Create arrays and store them
        {
            SCStore store(testFile.string());
            
            // 1D int array
            NDArray<int> intArray({5}, 0);
            for (size_t i = 0; i < 5; i++) {
                intArray.at({i}) = i * 10;
            }
            
            // 2D float array
            NDArray<float> floatArray({2, 3}, 0.0f);
            for (size_t i = 0; i < 2; i++) {
                for (size_t j = 0; j < 3; j++) {
                    floatArray.at({i, j}) = i * 10.0f + j + 0.5f;
                }
            }
            
            // String array
            NDArray<std::string> stringArray({2, 2}, "");
            stringArray.at({0, 0}) = "hello";
            stringArray.at({0, 1}) = "world";
            stringArray.at({1, 0}) = "test";
            stringArray.at({1, 1}) = "persistence";
            
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
            auto intArrayPtr = store.get<NDArray<int>>("int_array");
            EXPECT_NE(intArrayPtr, nullptr);
            EXPECT_EQ(intArrayPtr->shape.size(), 1);
            EXPECT_EQ(intArrayPtr->shape[0], 5);
            for (size_t i = 0; i < 5; i++) {
                EXPECT_EQ(intArrayPtr->at({i}), i * 10);
            }
            
            // Retrieve and verify float array
            auto floatArrayPtr = store.get<NDArray<float>>("float_array");
            EXPECT_NE(floatArrayPtr, nullptr);
            EXPECT_EQ(floatArrayPtr->shape.size(), 2);
            EXPECT_EQ(floatArrayPtr->shape[0], 2);
            EXPECT_EQ(floatArrayPtr->shape[1], 3);
            for (size_t i = 0; i < 2; i++) {
                for (size_t j = 0; j < 3; j++) {
                    EXPECT_FLOAT_EQ(floatArrayPtr->at({i, j}), i * 10.0f + j + 0.5f);
                }
            }
            
            // Retrieve and verify string array
            auto stringArrayPtr = store.get<NDArray<std::string>>("string_array");
            EXPECT_NE(stringArrayPtr, nullptr);
            EXPECT_EQ(stringArrayPtr->shape.size(), 2);
            EXPECT_EQ(stringArrayPtr->shape[0], 2);
            EXPECT_EQ(stringArrayPtr->shape[1], 2);
            EXPECT_EQ(stringArrayPtr->at({0, 0}), "hello");
            EXPECT_EQ(stringArrayPtr->at({0, 1}), "world");
            EXPECT_EQ(stringArrayPtr->at({1, 0}), "test");
            EXPECT_EQ(stringArrayPtr->at({1, 1}), "persistence");
        }
        
        // Clean up
        fs::remove(testFile);
        
    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}


TEST(SCStoreTest, EmptyIndexTest) {
    try {
        // Create a temporary file path
        fs::path testFile = "/tmp/empty_index_test.scs";
        
        // Clean up any existing test file
        if (fs::exists(testFile)) {
            fs::remove(testFile);
        }
        
        // Create a store with no values and flush it
        {
            SCStore store(testFile.string());
            // Don't add any values
            store.flush();
        }
        
        // Reopen the store and verify it's empty
        {
            SCStore store(testFile.string());
            EXPECT_EQ(store.size(), 0);
            EXPECT_FALSE(store.contains("any_key"));
        }
        
        // Add a single value, flush, and remove it
        {
            SCStore store(testFile.string());
            
            // Add a value
            NDArray<int> intArray({3}, 42);
            NDArray<float> floatArray({2, 3}, 0.0f);
            store.put("temp_key", intArray);
            store.put("temp_key2", floatArray);
            EXPECT_EQ(store.size(), 2);
            EXPECT_TRUE(store.contains("temp_key"));
            EXPECT_TRUE(store.contains("temp_key2"));
            
            // Flush to ensure the empty index is written
            store.flush();
        }
        
        // Reopen and verify it's still empty
        {
            SCStore store(testFile.string());
            EXPECT_EQ(store.m_index.size(), 2);
            EXPECT_TRUE(store.contains("temp_key"));
            EXPECT_TRUE(store.contains("temp_key2"));
            EXPECT_EQ(store.size(), 2);
        }
        
        // Clean up
        fs::remove(testFile);
        
    } catch (const std::exception& e) {
        FAIL() << "Exception occurred: " << e.what();
    }
}
