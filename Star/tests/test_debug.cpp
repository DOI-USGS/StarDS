#include "../include/star.h"
#include <iostream>

int main() {
    // Create test file
    SC Store store("test_debug.star");
    
    // Disable metadata block
    MetadataBlockConfig config;
    config.enabled = false;
    store->set_metadata_block_config(config);
    
    // Create array
    NDArray<int64_t> data({10000});
    for (size_t i = 0; i < 10000; ++i) {
        data.flat(i) = static_cast<int64_t>(i);
    }
    
    std::cout << "Writing array...\n";
    store->meta.put("data", data);
    store->flush();
    
    // Read back full array first
    std::cout << "Reading back full array...\n";
    auto readback = store->meta.get<NDArray<int64_t>>("data");
    std::cout << "Readback first 5: ";
    for (size_t i = 0; i < 5; ++i) {
        std::cout << readback.flat(i) << " ";
    }
    std::cout << "\n";
    std::cout << "Readback at 5000-5004: ";
    for (size_t i = 5000; i < 5005; ++i) {
        std::cout << readback.flat(i) << " ";
    }
    std::cout << "\n";
    
    return 0;
}
