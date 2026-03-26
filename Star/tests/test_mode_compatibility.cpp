/**
 * @file test_mode_compatibility.cpp
 * @brief Quick test to verify both string and enum modes work
 */

#include "star.h"
#include <iostream>
#include <ghc/fs_std.hpp>

int main() {
    std::string test_file = "test_modes.star";

    try {
        // Test 1: String modes (backward compatible)
        std::cout << "Testing string modes..." << std::endl;

        {
            StarDataset store1(test_file, "w");  // Write mode (string)
            store1->meta.put("test", 42);
            store1->flush();
        }

        {
            StarDataset store2(test_file, "r");  // Read mode (string)
            auto val = store2->meta.get("test");
            std::cout << "  String mode 'r': ✓" << std::endl;
        }

        {
            StarDataset store3(test_file, "rw");  // Read-write mode (string)
            store3.meta.put("test2", 100);
            store3.flush();
            std::cout << "  String mode 'rw': ✓" << std::endl;
        }

        // Test 2: Enum modes
        std::cout << "Testing enum modes..." << std::endl;

        {
            StarDataset store4(test_file, FileMode::READ_WRITE);  // Enum
            store4.meta.put("test3", 200);
            store4.flush();
            std::cout << "  Enum mode READ_WRITE: ✓" << std::endl;
        }

        {
            StarDataset store5(test_file, FileMode::READ_ONLY);  // Enum
            auto val = store5.meta.get("test3");
            std::cout << "  Enum mode READ_ONLY: ✓" << std::endl;
        }

        // Test 3: Default mode (should be READ_WRITE)
        {
            StarDataset store6(test_file);  // Default
            store6.meta.put("test4", 300);
            store6.flush();
            std::cout << "  Default mode: ✓" << std::endl;
        }

        // Cleanup
        if (fs::exists(test_file)) {
            fs::remove(test_file);
        }

        std::cout << "\n✅ All mode tests passed!" << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "❌ Test failed: " << e.what() << std::endl;

        // Cleanup on error
        if (fs::exists(test_file)) {
            fs::remove(test_file);
        }

        return 1;
    }
}
