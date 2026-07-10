#include <gtest/gtest.h>
#include "stards.h"
#include <sstream>



using namespace star;

class NdarrayTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup code if needed
    }

    void TearDown() override {
        // Cleanup code if needed
    }
};


TEST_F(NdarrayTest, ConstructorBasic) {
    // Test default construction with shape
    NDArray<int> arr1({3, 4});
    EXPECT_EQ(arr1.dimension(), 2);
    EXPECT_EQ(arr1.shape(0), 3);
    EXPECT_EQ(arr1.shape(1), 4);
    EXPECT_EQ(arr1.size(), 12);

    // Test 1D array
    NDArray<double> arr2({10});
    EXPECT_EQ(arr2.dimension(), 1);
    EXPECT_EQ(arr2.shape(0), 10);
    EXPECT_EQ(arr2.size(), 10);

    // Test 3D array
    NDArray<float> arr3({2, 3, 4});
    EXPECT_EQ(arr3.dimension(), 3);
    EXPECT_EQ(arr3.shape(0), 2);
    EXPECT_EQ(arr3.shape(1), 3);
    EXPECT_EQ(arr3.shape(2), 4);
    EXPECT_EQ(arr3.size(), 24);
}


TEST_F(NdarrayTest, ConstructorWithInitialValue) {
    // Test construction with initial value
    NDArray<int> arr({5}, 42);
    EXPECT_EQ(arr.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(arr(i), 42);
    }

    // Test with float
    NDArray<double> arr2({2, 3}, 3.14);
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            EXPECT_DOUBLE_EQ(arr2(i, j), 3.14);
        }
    }
}


TEST_F(NdarrayTest, VariadicOperatorCall) {
    // Test 1D indexing
    NDArray<int> arr1d({10});
    for (size_t i = 0; i < 10; ++i) {
        arr1d(i) = i * 2;
    }
    EXPECT_EQ(arr1d(0), 0);
    EXPECT_EQ(arr1d(5), 10);
    EXPECT_EQ(arr1d(9), 18);

    // Test 2D indexing
    NDArray<double> arr2d({3, 4});
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            arr2d(i, j) = i * 10.0 + j;
        }
    }
    EXPECT_DOUBLE_EQ(arr2d(0, 0), 0.0);
    EXPECT_DOUBLE_EQ(arr2d(1, 2), 12.0);
    EXPECT_DOUBLE_EQ(arr2d(2, 3), 23.0);

    // Test 3D indexing
    NDArray<float> arr3d({2, 3, 4});
    arr3d(1, 2, 3) = 123.0f;
    EXPECT_FLOAT_EQ(arr3d(1, 2, 3), 123.0f);
}


TEST_F(NdarrayTest, FlatIndexing) {
    NDArray<int> arr({2, 3});

    // Set values using flat indexing
    for (size_t i = 0; i < 6; ++i) {
        arr.flat(i) = i * 10;
    }

    // Verify with multi-dimensional indexing
    EXPECT_EQ(arr(0, 0), 0);
    EXPECT_EQ(arr(0, 1), 10);
    EXPECT_EQ(arr(0, 2), 20);
    EXPECT_EQ(arr(1, 0), 30);
    EXPECT_EQ(arr(1, 1), 40);
    EXPECT_EQ(arr(1, 2), 50);

    // Verify consistency
    EXPECT_EQ(arr.flat(5), 50);
}


TEST_F(NdarrayTest, ShapeAccessors) {
    NDArray<double> arr({2, 3, 4});

    // Test shape() returning vector
    const auto& shape_vec = arr.shape();
    EXPECT_EQ(shape_vec.size(), 3);
    EXPECT_EQ(shape_vec[0], 2);
    EXPECT_EQ(shape_vec[1], 3);
    EXPECT_EQ(shape_vec[2], 4);

    // Test shape(dim) returning single dimension
    EXPECT_EQ(arr.shape(0), 2);
    EXPECT_EQ(arr.shape(1), 3);
    EXPECT_EQ(arr.shape(2), 4);

    // Test dimension()
    EXPECT_EQ(arr.dimension(), 3);

    // Test size()
    EXPECT_EQ(arr.size(), 24);
}


TEST_F(NdarrayTest, DataAccessors) {
    NDArray<int> arr({5});
    for (size_t i = 0; i < 5; ++i) {
        arr(i) = i;
    }

    // Test data() returns reference to underlying vector
    auto& data_vec = arr.data();
    EXPECT_EQ(data_vec.size(), 5);
    EXPECT_EQ(data_vec[0], 0);
    EXPECT_EQ(data_vec[4], 4);

    // Test data_ptr() returns raw pointer
    auto* ptr = arr.data_ptr();
    EXPECT_NE(ptr, nullptr);
    EXPECT_EQ(ptr[0], 0);
    EXPECT_EQ(ptr[4], 4);
}


TEST_F(NdarrayTest, IteratorForward) {
    NDArray<int> arr({10});

    // Initialize using operator()
    for (size_t i = 0; i < 10; ++i) {
        arr(i) = i * 3;
    }

    // Test range-based for loop
    std::vector<int> collected;
    for (const auto& val : arr) {
        collected.push_back(val);
    }

    EXPECT_EQ(collected.size(), 10);
    EXPECT_EQ(collected[0], 0);
    EXPECT_EQ(collected[5], 15);
    EXPECT_EQ(collected[9], 27);

    // Test explicit iterator
    auto it = arr.begin();
    EXPECT_EQ(*it, 0);
    ++it;
    EXPECT_EQ(*it, 3);

    // Test const iterator
    auto cit = arr.cbegin();
    EXPECT_EQ(*cit, 0);
}


TEST_F(NdarrayTest, IteratorReverse) {
    NDArray<int> arr({5});
    for (size_t i = 0; i < 5; ++i) {
        arr(i) = i * 2;
    }

    // Test reverse iterator
    std::vector<int> reversed;
    for (auto it = arr.rbegin(); it != arr.rend(); ++it) {
        reversed.push_back(*it);
    }

    EXPECT_EQ(reversed.size(), 5);
    EXPECT_EQ(reversed[0], 8); // Last element (4*2)
    EXPECT_EQ(reversed[1], 6);
    EXPECT_EQ(reversed[2], 4);
    EXPECT_EQ(reversed[3], 2);
    EXPECT_EQ(reversed[4], 0); // First element
}


TEST_F(NdarrayTest, IteratorModification) {
    NDArray<double> arr({4});

    // Modify using iterator
    for (auto& val : arr) {
        val = 1.5;
    }

    // Verify modification
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_DOUBLE_EQ(arr(i), 1.5);
    }

    // Modify using explicit iterator
    for (auto it = arr.begin(); it != arr.end(); ++it) {
        *it *= 2.0;
    }

    // Verify
    for (size_t i = 0; i < 4; ++i) {
        EXPECT_DOUBLE_EQ(arr(i), 3.0);
    }
}


TEST_F(NdarrayTest, StaticFactoryZeros) {
    // Test 1D zeros
    auto zeros1d = NDArray<double>::zeros({10});
    EXPECT_EQ(zeros1d.size(), 10);
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_DOUBLE_EQ(zeros1d(i), 0.0);
    }

    // Test 2D zeros
    auto zeros2d = NDArray<int>::zeros({3, 4});
    EXPECT_EQ(zeros2d.size(), 12);
    for (size_t i = 0; i < 3; ++i) {
        for (size_t j = 0; j < 4; ++j) {
            EXPECT_EQ(zeros2d(i, j), 0);
        }
    }
}


TEST_F(NdarrayTest, StaticFactoryOnes) {
    // Test 1D ones
    auto ones1d = NDArray<int>::ones({8});
    EXPECT_EQ(ones1d.size(), 8);
    for (size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(ones1d(i), 1);
    }

    // Test 3D ones
    auto ones3d = NDArray<double>::ones({2, 2, 2});
    EXPECT_EQ(ones3d.size(), 8);
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            for (size_t k = 0; k < 2; ++k) {
                EXPECT_DOUBLE_EQ(ones3d(i, j, k), 1.0);
            }
        }
    }
}


TEST_F(NdarrayTest, StaticFactoryFull) {
    // Test full with integer
    auto full_int = NDArray<int>::full({5}, 42);
    EXPECT_EQ(full_int.size(), 5);
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(full_int(i), 42);
    }

    // Test full with float
    auto full_float = NDArray<double>::full({2, 3}, 3.14159);
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            EXPECT_DOUBLE_EQ(full_float(i, j), 3.14159);
        }
    }
}


TEST_F(NdarrayTest, StaticFactoryEmpty) {
    // Test empty (uninitialized)
    auto empty = NDArray<int>::empty({100});
    EXPECT_EQ(empty.size(), 100);
    EXPECT_EQ(empty.shape(0), 100);
    // Values are uninitialized, so we just check structure
}


TEST_F(NdarrayTest, StaticFactoryArange) {
    // Test basic arange
    auto arr1 = NDArray<int>::arange(0, 10, 1);
    EXPECT_EQ(arr1.size(), 10);
    for (size_t i = 0; i < 10; ++i) {
        EXPECT_EQ(arr1(i), static_cast<int>(i));
    }

    // Test with step
    auto arr2 = NDArray<int>::arange(0, 20, 2);
    EXPECT_EQ(arr2.size(), 10);
    EXPECT_EQ(arr2(0), 0);
    EXPECT_EQ(arr2(1), 2);
    EXPECT_EQ(arr2(9), 18);

    // Test with negative step
    auto arr3 = NDArray<int>::arange(10, 0, -1);
    EXPECT_EQ(arr3.size(), 10);
    EXPECT_EQ(arr3(0), 10);
    EXPECT_EQ(arr3(9), 1);

    // Test with double
    auto arr4 = NDArray<double>::arange(0.0, 1.0, 0.1);
    EXPECT_EQ(arr4.size(), 10);
    EXPECT_NEAR(arr4(0), 0.0, 1e-10);
    EXPECT_NEAR(arr4(9), 0.9, 1e-10);
}


TEST_F(NdarrayTest, Reshape) {
    // Create 1D array
    NDArray<int> arr({12});
    for (size_t i = 0; i < 12; ++i) {
        arr(i) = i;
    }

    // Reshape to 2D
    arr.reshape({3, 4});
    EXPECT_EQ(arr.dimension(), 2);
    EXPECT_EQ(arr.shape(0), 3);
    EXPECT_EQ(arr.shape(1), 4);
    EXPECT_EQ(arr.size(), 12);

    // Verify data is preserved
    EXPECT_EQ(arr(0, 0), 0);
    EXPECT_EQ(arr(0, 3), 3);
    EXPECT_EQ(arr(2, 3), 11);

    // Reshape to 3D
    arr.reshape({2, 2, 3});
    EXPECT_EQ(arr.dimension(), 3);
    EXPECT_EQ(arr.size(), 12);

    // Reshape back to 1D
    arr.reshape({12});
    EXPECT_EQ(arr.dimension(), 1);
    EXPECT_EQ(arr(11), 11);
}


TEST_F(NdarrayTest, ReshapeInvalidSize) {
    NDArray<int> arr({12});

    // Try to reshape to incompatible size
    EXPECT_THROW(arr.reshape({10}), std::runtime_error);
    EXPECT_THROW(arr.reshape({3, 5}), std::runtime_error);
}


TEST_F(NdarrayTest, Resize) {
    // Create array
    NDArray<int> arr({5});
    for (size_t i = 0; i < 5; ++i) {
        arr(i) = i * 10;
    }

    // Resize larger with default fill value
    arr.resize({10});
    EXPECT_EQ(arr.size(), 10);

    // Original values preserved
    EXPECT_EQ(arr(0), 0);
    EXPECT_EQ(arr(4), 40);

    // New values are default (0 for int)
    EXPECT_EQ(arr(5), 0);
    EXPECT_EQ(arr(9), 0);

    // Resize larger with custom fill value
    arr.resize({15}, 999);
    EXPECT_EQ(arr.size(), 15);
    EXPECT_EQ(arr(14), 999);

    // Resize smaller
    arr.resize({3});
    EXPECT_EQ(arr.size(), 3);
    EXPECT_EQ(arr(0), 0);
    EXPECT_EQ(arr(2), 20);
}


TEST_F(NdarrayTest, ResizeMultiDimensional) {
    NDArray<double> arr({2, 3});

    // Resize to different shape
    arr.resize({4, 5}, 1.5);
    EXPECT_EQ(arr.dimension(), 2);
    EXPECT_EQ(arr.shape(0), 4);
    EXPECT_EQ(arr.shape(1), 5);
    EXPECT_EQ(arr.size(), 20);
}


TEST_F(NdarrayTest, CopyConstructor) {
    NDArray<int> arr1({5});
    for (size_t i = 0; i < 5; ++i) {
        arr1(i) = i * 2;
    }

    // Copy construct
    NDArray<int> arr2 = arr1;

    EXPECT_EQ(arr2.size(), arr1.size());
    EXPECT_EQ(arr2.shape(0), arr1.shape(0));

    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(arr2(i), arr1(i));
    }

    // Modify copy, original should be unchanged
    arr2(0) = 999;
    EXPECT_EQ(arr2(0), 999);
    EXPECT_EQ(arr1(0), 0);
}


TEST_F(NdarrayTest, AssignmentOperator) {
    NDArray<int> arr1({5});
    for (size_t i = 0; i < 5; ++i) {
        arr1(i) = i * 2;
    }

    NDArray<int> arr2({3});

    // Assign
    arr2 = arr1;

    EXPECT_EQ(arr2.size(), arr1.size());
    for (size_t i = 0; i < 5; ++i) {
        EXPECT_EQ(arr2(i), arr1(i));
    }
}


TEST_F(NdarrayTest, SerializationRoundTrip) {
    // Create original array
    NDArray<double> original({2, 3});
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            original(i, j) = i * 10.0 + j + 0.5;
        }
    }

    // Serialize
    std::ostringstream oss;
    oss << original;

    // Deserialize
    NDArray<double> restored;
    std::istringstream iss(oss.str());
    iss >> restored;

    // Verify
    EXPECT_EQ(restored.dimension(), original.dimension());
    EXPECT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored.shape(0), original.shape(0));
    EXPECT_EQ(restored.shape(1), original.shape(1));

    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 3; ++j) {
            EXPECT_DOUBLE_EQ(restored(i, j), original(i, j));
        }
    }
}


TEST_F(NdarrayTest, SerializationDifferentTypes) {
    // Test int32
    NDArray<int32_t> int_arr({3});
    int_arr(0) = -100;
    int_arr(1) = 0;
    int_arr(2) = 100;

    std::ostringstream oss1;
    oss1 << int_arr;
    NDArray<int32_t> int_restored;
    std::istringstream iss1(oss1.str());
    iss1 >> int_restored;

    EXPECT_EQ(int_restored(0), -100);
    EXPECT_EQ(int_restored(1), 0);
    EXPECT_EQ(int_restored(2), 100);

    // Test float
    NDArray<float> float_arr({2});
    float_arr(0) = 3.14f;
    float_arr(1) = -2.71f;

    std::ostringstream oss2;
    oss2 << float_arr;
    NDArray<float> float_restored;
    std::istringstream iss2(oss2.str());
    iss2 >> float_restored;

    EXPECT_FLOAT_EQ(float_restored(0), 3.14f);
    EXPECT_FLOAT_EQ(float_restored(1), -2.71f);
}


TEST_F(NdarrayTest, StringArraySupport) {
    NDArray<std::string> arr({2, 2});
    arr(0, 0) = "hello";
    arr(0, 1) = "world";
    arr(1, 0) = "foo";
    arr(1, 1) = "bar";

    EXPECT_EQ(arr(0, 0), "hello");
    EXPECT_EQ(arr(1, 1), "bar");

    // Test with iterator
    std::vector<std::string> collected;
    for (const auto& s : arr) {
        collected.push_back(s);
    }
    EXPECT_EQ(collected.size(), 4);
    EXPECT_EQ(collected[0], "hello");
    EXPECT_EQ(collected[3], "bar");
}


TEST_F(NdarrayTest, LargeArray) {
    // Test with larger array
    size_t size = 10000;
    NDArray<int> arr({size});

    for (size_t i = 0; i < size; ++i) {
        arr(i) = i;
    }

    EXPECT_EQ(arr.size(), size);
    EXPECT_EQ(arr(0), 0);
    EXPECT_EQ(arr(size-1), static_cast<int>(size-1));

    // Test with iterator
    int sum = 0;
    for (const auto& val : arr) {
        sum += val;
    }
    // Sum of 0 to 9999 = 49995000
    EXPECT_EQ(sum, 49995000);
}


TEST_F(NdarrayTest, MultiDimensionalLargeArray) {
    NDArray<double> arr({100, 100});
    EXPECT_EQ(arr.size(), 10000);
    EXPECT_EQ(arr.dimension(), 2);

    // Set diagonal elements
    for (size_t i = 0; i < 100; ++i) {
        arr(i, i) = i * 1.5;
    }

    // Verify diagonal
    for (size_t i = 0; i < 100; ++i) {
        EXPECT_DOUBLE_EQ(arr(i, i), i * 1.5);
    }
}


TEST_F(NdarrayTest, EdgeCaseSingleElement) {
    NDArray<int> arr({1});
    arr(0) = 42;

    EXPECT_EQ(arr.size(), 1);
    EXPECT_EQ(arr.dimension(), 1);
    EXPECT_EQ(arr(0), 42);
    EXPECT_EQ(arr.flat(0), 42);

    // Test iterator
    int count = 0;
    for (const auto& val : arr) {
        EXPECT_EQ(val, 42);
        count++;
    }
    EXPECT_EQ(count, 1);
}


TEST_F(NdarrayTest, HighDimensionalArray) {
    // Test 5D array
    NDArray<int> arr({2, 2, 2, 2, 2});
    EXPECT_EQ(arr.dimension(), 5);
    EXPECT_EQ(arr.size(), 32);

    // Set and verify specific element
    arr(1, 1, 1, 1, 1) = 999;
    EXPECT_EQ(arr(1, 1, 1, 1, 1), 999);
}
