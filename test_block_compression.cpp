#include "CameraStateFile/include/scs.h"
#include <iostream>

int main() {
    try {
        // Create a store with GZIP compression and 1KB block size
        SCStore store("test_blocks.scs", CompressionAlgorithm::GZIP, 1024);

        // Test 1: Store a simple 1D array of doubles
        std::cout << "Test 1: Storing 1D double array..." << std::endl;
        NDArray<double> arr1({100});
        for (size_t i = 0; i < 100; ++i) {
            arr1.data[i] = static_cast<double>(i) * 1.5;
        }
        store.put("double_array", arr1);

        // Test 2: Store a 2D array of int32
        std::cout << "Test 2: Storing 2D int32 array..." << std::endl;
        NDArray<int32_t> arr2({10, 20});
        for (size_t i = 0; i < arr2.data.size(); ++i) {
            arr2.data[i] = static_cast<int32_t>(i);
        }
        store.put("int32_matrix", arr2);

        // Test 3: Store a 3D array with explicit no compression
        std::cout << "Test 3: Storing 3D float32 array (uncompressed)..." << std::endl;
        NDArray<float> arr3({5, 4, 3});
        for (size_t i = 0; i < arr3.data.size(); ++i) {
            arr3.data[i] = static_cast<float>(i) / 10.0f;
        }
        store.put("float32_tensor", arr3, CompressionAlgorithm::NONE);

        // Flush to disk
        std::cout << "Flushing to disk..." << std::endl;
        store.flush();

        // Print header info
        std::cout << "\n=== Store Header Info ===" << std::endl;
        store.printHeader();

        // Test 4: Read back the data
        std::cout << "\nTest 4: Reading back data..." << std::endl;
        auto read_arr1 = store.get<NDArray<double>>("double_array");
        if (read_arr1) {
            std::cout << "  double_array: First element = " << read_arr1->data[0]
                      << ", Last element = " << read_arr1->data[99] << std::endl;
            std::cout << "  Shape: [";
            for (size_t i = 0; i < read_arr1->shape.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << read_arr1->shape[i];
            }
            std::cout << "]" << std::endl;
        }

        auto read_arr2 = store.get<NDArray<int32_t>>("int32_matrix");
        if (read_arr2) {
            std::cout << "  int32_matrix: First element = " << read_arr2->data[0]
                      << ", Last element = " << read_arr2->data[read_arr2->data.size()-1] << std::endl;
            std::cout << "  Shape: [";
            for (size_t i = 0; i < read_arr2->shape.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << read_arr2->shape[i];
            }
            std::cout << "]" << std::endl;
        }

        auto read_arr3 = store.get<NDArray<float>>("float32_tensor");
        if (read_arr3) {
            std::cout << "  float32_tensor: First element = " << read_arr3->data[0]
                      << ", Last element = " << read_arr3->data[read_arr3->data.size()-1] << std::endl;
            std::cout << "  Shape: [";
            for (size_t i = 0; i < read_arr3->shape.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << read_arr3->shape[i];
            }
            std::cout << "]" << std::endl;
        }

        // Test 5: Verify data integrity
        std::cout << "\nTest 5: Verifying data integrity..." << std::endl;
        bool test1_pass = true;
        for (size_t i = 0; i < 100; ++i) {
            if (std::abs(read_arr1->data[i] - static_cast<double>(i) * 1.5) > 1e-10) {
                test1_pass = false;
                break;
            }
        }
        std::cout << "  Test 1 (double_array): " << (test1_pass ? "PASS" : "FAIL") << std::endl;

        bool test2_pass = true;
        for (size_t i = 0; i < arr2.data.size(); ++i) {
            if (read_arr2->data[i] != static_cast<int32_t>(i)) {
                test2_pass = false;
                break;
            }
        }
        std::cout << "  Test 2 (int32_matrix): " << (test2_pass ? "PASS" : "FAIL") << std::endl;

        bool test3_pass = true;
        for (size_t i = 0; i < arr3.data.size(); ++i) {
            if (std::abs(read_arr3->data[i] - static_cast<float>(i) / 10.0f) > 1e-6f) {
                test3_pass = false;
                break;
            }
        }
        std::cout << "  Test 3 (float32_tensor): " << (test3_pass ? "PASS" : "FAIL") << std::endl;

        std::cout << "\n=== All tests completed ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
