#include <gtest/gtest.h>
#include <ghc/fs_std.hpp>
#include "scs.h"


// TEST(UnitTestSCS, TestWrite) {
//     try {
//         // Create a temporary file for testing
//         std::string testFile = "/tmp/test_ndarray_write.cld"; //  "/vsicurl/https://asc-isisdata.s3.us-west-2.amazonaws.com/test_ndarray_write.cld";
        
//         // Remove the file if it already exists
//         if (fs::exists(testFile)) {
//             fs::remove(testFile);
//         }
        
//         // Create a new SCStore
//         SCStore store(testFile, "CLDG");
        
//         // Create various NDArrays to test with
        
//         // 1D integer array
//         NDArray<int> intArray({5}, 0);
//         for (size_t i = 0; i < 5; i++) {
//             intArray.data[i] = i * 10;
//         }
        
//         // 2D float array
//         NDArray<float> floatArray({3, 4}, 0.0f);
//         for (size_t i = 0; i < 3; i++) {
//             for (size_t j = 0; j < 4; j++) {
//                 floatArray.at({i, j}) = i * 10.0f + j;
//             }
//         }
        
//         // 3D double array
//         NDArray<double> doubleArray({2, 2, 2}, 0.0);
//         for (size_t i = 0; i < 2; i++) {
//             for (size_t j = 0; j < 2; j++) {
//                 for (size_t k = 0; k < 2; k++) {
//                     doubleArray.at({i, j, k}) = i * 100.0 + j * 10.0 + k;
//                 }
//             }
//         }
        
//         // String array
//         NDArray<std::string> stringArray({2, 3}, "");
//         for (size_t i = 0; i < 2; i++) {
//             for (size_t j = 0; j < 3; j++) {
//                 stringArray.at({i, j}) = "str_" + std::to_string(i) + "_" + std::to_string(j);
//             }
//         }
        
//         // Store the arrays
//         store.put("int_array", intArray);
//         store.put("float_array", floatArray);
//         store.put("double_array", doubleArray);
//         store.put("string_array", stringArray);
        
//         // Verify the store contains the keys
//         EXPECT_TRUE(store.contains("int_array"));
//         EXPECT_TRUE(store.contains("float_array"));
//         EXPECT_TRUE(store.contains("double_array"));
//         EXPECT_TRUE(store.contains("string_array"));

//         // Retrieve the arrays and verify their contents
//         auto retrievedIntArray = store.get<NDArray<int>>("int_array");
//         ASSERT_NE(retrievedIntArray, nullptr);
//         EXPECT_EQ(retrievedIntArray->shape.size(), 1);
//         EXPECT_EQ(retrievedIntArray->shape[0], 5);
//         for (size_t i = 0; i < 5; i++) {
//             EXPECT_EQ(retrievedIntArray->at({i}), i * 10);
//         }
        
//         auto retrievedFloatArray = store.get<NDArray<float>>("float_array");
//         ASSERT_NE(retrievedFloatArray, nullptr);
//         EXPECT_EQ(retrievedFloatArray->shape.size(), 2);
//         EXPECT_EQ(retrievedFloatArray->shape[0], 3);
//         EXPECT_EQ(retrievedFloatArray->shape[1], 4);
//         for (size_t i = 0; i < 3; i++) {
//             for (size_t j = 0; j < 4; j++) {
//                 EXPECT_FLOAT_EQ(retrievedFloatArray->at({i, j}), i * 10.0f + j);
//             }
//         }
        
//         auto retrievedDoubleArray = store.get<NDArray<double>>("double_array");
//         ASSERT_NE(retrievedDoubleArray, nullptr);
//         EXPECT_EQ(retrievedDoubleArray->shape.size(), 3);
//         EXPECT_EQ(retrievedDoubleArray->shape[0], 2);
//         EXPECT_EQ(retrievedDoubleArray->shape[1], 2);
//         EXPECT_EQ(retrievedDoubleArray->shape[2], 2);
//         for (size_t i = 0; i < 2; i++) {
//             for (size_t j = 0; j < 2; j++) {
//                 for (size_t k = 0; k < 2; k++) {
//                     EXPECT_DOUBLE_EQ(retrievedDoubleArray->at({i, j, k}), i * 100.0 + j * 10.0 + k);
//                 }
//             }
//         }
        
//         auto retrievedStringArray = store.get<NDArray<std::string>>("string_array");
//         ASSERT_NE(retrievedStringArray, nullptr);
//         EXPECT_EQ(retrievedStringArray->shape.size(), 2);
//         EXPECT_EQ(retrievedStringArray->shape[0], 2);
//         EXPECT_EQ(retrievedStringArray->shape[1], 3);
//         for (size_t i = 0; i < 2; i++) {
//             for (size_t j = 0; j < 3; j++) {
//                 EXPECT_EQ(retrievedStringArray->at({i, j}), "str_" + std::to_string(i) + "_" + std::to_string(j));
//             }
//         }


//         // Verify the store size
//         EXPECT_EQ(store.size(), 4);
//         store.flush();
//         // Verify the file exists
//         EXPECT_TRUE(fs::exists(testFile));
        
//     } catch (const std::exception& e) {
//         FAIL() << "Exception occurred: " << e.what();
//     }
// }


// TEST(SCStoreTest, LargeArrayAndStringTest) {
//     try {
//         fs::path testFile = "/tmp/large_test_9999999_CLDG.cld";
//         if (fs::exists(testFile)) {
//             fs::remove(testFile);
//         }
//         size_t num_elements = 10000000;
        
//         // 1 element array of type string, with a 1 million character string
//         NDArray<std::string> bigStringArray({1});
//         std::string bigString(num_elements, 'x');
//         for (size_t i = 0; i < bigString.size(); ++i) {
//             bigString[i] = 'a' + (i % 26);
//         }
//         bigStringArray.at({0}) = bigString;

//         NDArray<int> bigArray({num_elements}, 0);
//         for (size_t i = 0; i < num_elements; ++i) {
//             bigArray.at({i}) = static_cast<int>(i);
//         }

//         // Write phase
//         {
//             SCStore store(testFile.string(), "CLDG");

//             store.put("big_array", bigArray);
//             store.put("big_string", bigStringArray);

//             store.flush();
//         }

//         // Read phase
//         {
//             SCStore store(testFile.string());

//             // Read and check array
//             auto retrievedArray = store.get<NDArray<int>>("big_array");
//             ASSERT_NE(retrievedArray, nullptr);
//             ASSERT_EQ(retrievedArray->shape.size(), 1);
//             ASSERT_EQ(retrievedArray->shape[0], num_elements);
//             for (size_t i = 0; i < num_elements; ++i) {
//                 EXPECT_EQ(retrievedArray->at({i}), static_cast<int>(i));
//             }

//             // Read and check string
//             auto retrievedString = store.get<NDArray<std::string>>("big_string");
//             ASSERT_NE(retrievedString, nullptr);
//             ASSERT_EQ(retrievedString->shape.size(), 1);
//             ASSERT_EQ(retrievedString->shape[0], 1);
//             EXPECT_EQ(retrievedString->at({0}), bigString);
//             }

//         // Clean up
//         // if (fs::exists(testFile)) {
//         //     fs::remove(testFile);
//         // }
//     } catch (const std::exception& e) {
//         FAIL() << "Exception occurred: " << e.what();
//     }
// }


// TEST(SCStoreTest, ReadTest) {
//     try {
//         // Create a temporary file for testing
//         fs::path testFile = "/tmp/persistence_test.cld";
        
//         // Remove the file if it already exists
//         if (fs::exists(testFile)) {
//             fs::remove(testFile);
//         }
        
//         // Create arrays and store them
//         {
//             SCStore store(testFile.string(), "CLDG");
            
//             // 1D int array
//             NDArray<int> intArray({5}, 0);
//             for (size_t i = 0; i < 5; i++) {
//                 intArray.at({i}) = i * 10;
//             }
            
//             // 2D float array
//             NDArray<float> floatArray({2, 3}, 0.0f);
//             for (size_t i = 0; i < 2; i++) {
//                 for (size_t j = 0; j < 3; j++) {
//                     floatArray.at({i, j}) = i * 10.0f + j + 0.5f;
//                 }
//             }
            
//             // String array
//             NDArray<std::string> stringArray({2, 2}, "");
//             stringArray.at({0, 0}) = "hello";
//             stringArray.at({0, 1}) = "world";
//             stringArray.at({1, 0}) = "test";
//             stringArray.at({1, 1}) = "persistence";
            
//             // Store the arrays
//             store.put("int_array", intArray);
//             store.put("float_array", floatArray);
//             store.put("string_array", stringArray);
            
//             // Flush to ensure data is written
//             store.flush();
            
//             // Store goes out of scope here and destructor is called
//         }
        
//         // Verify the file exists
//         EXPECT_TRUE(fs::exists(testFile));
        
//         // Reopen the file and verify the contents
//         {
//             SCStore store(testFile.string());
            
//             // Verify the store contains the keys
//             EXPECT_TRUE(store.contains("int_array"));
//             EXPECT_TRUE(store.contains("float_array"));
//             EXPECT_TRUE(store.contains("string_array"));
            
//             // Retrieve and verify int array
//             auto intArrayPtr = store.get<NDArray<int>>("int_array");
//             EXPECT_NE(intArrayPtr, nullptr);
//             EXPECT_EQ(intArrayPtr->shape.size(), 1);
//             EXPECT_EQ(intArrayPtr->shape[0], 5);
//             for (size_t i = 0; i < 5; i++) {
//                 EXPECT_EQ(intArrayPtr->at({i}), i * 10);
//             }
            
//             // Retrieve and verify float array
//             auto floatArrayPtr = store.get<NDArray<float>>("float_array");
//             EXPECT_NE(floatArrayPtr, nullptr);
//             EXPECT_EQ(floatArrayPtr->shape.size(), 2);
//             EXPECT_EQ(floatArrayPtr->shape[0], 2);
//             EXPECT_EQ(floatArrayPtr->shape[1], 3);
//             for (size_t i = 0; i < 2; i++) {
//                 for (size_t j = 0; j < 3; j++) {
//                     EXPECT_FLOAT_EQ(floatArrayPtr->at({i, j}), i * 10.0f + j + 0.5f);
//                 }
//             }
            
//             // Retrieve and verify string array
//             auto stringArrayPtr = store.get<NDArray<std::string>>("string_array");
//             EXPECT_NE(stringArrayPtr, nullptr);
//             EXPECT_EQ(stringArrayPtr->shape.size(), 2);
//             EXPECT_EQ(stringArrayPtr->shape[0], 2);
//             EXPECT_EQ(stringArrayPtr->shape[1], 2);
//             EXPECT_EQ(stringArrayPtr->at({0, 0}), "hello");
//             EXPECT_EQ(stringArrayPtr->at({0, 1}), "world");
//             EXPECT_EQ(stringArrayPtr->at({1, 0}), "test");
//             EXPECT_EQ(stringArrayPtr->at({1, 1}), "persistence");
//         }
        
//         // Clean up
//         fs::remove(testFile);
        
//     } catch (const std::exception& e) {
//         FAIL() << "Exception occurred: " << e.what();
//     }
// }


// TEST(SCStoreTest, HeaderReadWriteTest) {
//     try {
//         // Create a temporary file path
//         fs::path testFile = "/tmp/header_rw_test.cld";

//         // Clean up any existing test file
//         if (fs::exists(testFile)) {
//             fs::remove(testFile);
//         }

//         // Write a store with a few keys, but don't write any data (just header/index)
//         {
//             SCStore store(testFile.string(), "CLDG");
//             NDArray<int> intArray({2}, 1);
//             NDArray<float> floatArray({1}, 2.0f);
//             store.put("header_key1", intArray);
//             store.put("header_key2", floatArray);
//             // Only flush, don't access or write any values
//             store.flush();
//         }

//         // Reopen and only check the header/index, not the data
//         {
//             SCStore store(testFile.string());
//             // The index should contain the keys
//             EXPECT_EQ(store.size(), 2);
//             EXPECT_TRUE(store.contains("header_key1"));
//             EXPECT_TRUE(store.contains("header_key2"));

//             // Check that the index entries are correct (positions and sizes are nonzero)
//             auto it1 = store.m_index.find("header_key1");
//             auto it2 = store.m_index.find("header_key2");
//             EXPECT_NE(it1, store.m_index.end());
//             EXPECT_NE(it2, store.m_index.end());

//             // The tuple is (position, bytes, dirty)
//             size_t pos1, bytes1, pos2, bytes2;
//             bool dirty1, dirty2;
//             std::tie(pos1, bytes1, dirty1) = it1->second;
//             std::tie(pos2, bytes2, dirty2) = it2->second;

//             // Positions should be >= header size, bytes should be > 0, dirty should be false
//             EXPECT_GT(bytes1, 0u);
//             EXPECT_GT(bytes2, 0u);
//             EXPECT_FALSE(dirty1);
//             EXPECT_FALSE(dirty2);

//             // The header should not be dirty after reading
//             EXPECT_FALSE(store.m_header_dirty);
//         }

//         // Clean up
//         fs::remove(testFile);

//     } catch (const std::exception& e) {
//         FAIL() << "Exception occurred: " << e.what();
//     }
// }


// TEST(SCStoreTest, EmptyIndexTest) {
//     try {
//         // Create a temporary file path
//         fs::path testFile = "/tmp/empty_index_test.cld";
        
//         // Clean up any existing test file
//         if (fs::exists(testFile)) {
//             fs::remove(testFile);
//         }
        
//         // Create a store with no values and flush it
//         {
//             SCStore store(testFile.string());
//             // Don't add any values
//             store.flush();
//         }
        
//         // Reopen the store and verify it's empty
//         {
//             SCStore store(testFile.string());
//             EXPECT_EQ(store.size(), 0);
//             EXPECT_FALSE(store.contains("any_key"));
//         }
        
//         // Add a single value, flush, and remove it
//         {
//             SCStore store(testFile.string());
            
//             // Add a value
//             NDArray<int> intArray({3}, 42);
//             NDArray<float> floatArray({2, 3}, 0.0f);
//             store.put("temp_key", intArray);
//             store.put("temp_key2", floatArray);
//             EXPECT_EQ(store.size(), 2);
//             EXPECT_TRUE(store.contains("temp_key"));
//             EXPECT_TRUE(store.contains("temp_key2"));
            
//             // Flush to ensure the empty index is written
//             store.flush();
//         }
        
//         // Reopen and verify it's still empty
//         {
//             SCStore store(testFile.string());
//             EXPECT_EQ(store.m_index.size(), 2);
//             EXPECT_TRUE(store.contains("temp_key"));
//             EXPECT_TRUE(store.contains("temp_key2"));
//             EXPECT_EQ(store.size(), 2);
//         }
        
//         // Clean up
//         fs::remove(testFile);
        
//     } catch (const std::exception& e) {
//         FAIL() << "Exception occurred: " << e.what();
//     }
// }


// TEST(SCStoreTest, ReadRemoteLargeCLD0) {
//     try {
//         // Use a remote file (via /vsicurl/ or direct HTTP if supported)
//         std::string remoteFile = "/vsicurl/https://asc-isisdata.s3.us-west-2.amazonaws.com/astro_data/cloud_tests/large_test_1000000_CLDG.cld";

//         // Open the store from the remote file
//         SCStore store(remoteFile);

//         store.printHeader();
//     } catch (const std::exception& e) {
//         FAIL() << "Exception occurred: " << e.what();
//     }
// }
