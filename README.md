# Simple Columnar Store: Efficient Binary Key-Value Store

Simple Columnar Store (SCS) is a C++ template library that provides a persistent binary key-value store with transparent serialization capabilities. It offers an efficient way to store and retrieve various data types with disk persistence while maintaining an in-memory cache for fast access. The store is optimized for cloud environments, allowing efficient data transfer and storage in distributed systems.

## Features

- **Type-Safe Storage**: Store multiple value types using C++17 variadic templates and `std::variant`
- **Disk Persistence**: Automatically persists data to disk
- **Memory Caching**: Maintains an in-memory cache for fast access to frequently used values
- **Efficient Array Handling**: Special handling for large arrays with direct disk streaming
- **Multi-dimensional Array Support**: Handles n-dimensional arrays (represented as nested vectors)
- **Simple API**: Easy-to-use interface for storing and retrieving values

## Usage Example

```cpp
#include "include/scs.h"
#include <string>
#include <vector>

int main() {
    // Create a key-value store supporting int, double, std::string, and vector<int>
    SCStore<int, double, std::string, std::vector<int>> store("data.bin");
    
    // Store different value types
    store.put("int_value", 42);
    store.put("double_value", 3.14159);
    store.put("string_value", "Hello, World!");
    store.put("array_value", std::vector<int>{1, 2, 3, 4, 5});
    
    // Retrieve values
    auto int_val = store.get<int>("int_value");
    auto double_val = store.get<double>("double_value");
    auto string_val = store.get<std::string>("string_value");
    auto array_val = store.get<std::vector<int>>("array_value");
    
    // Check if a key exists
    if (store.contains("int_value")) {
        std::cout << "int_value exists" << std::endl;
    }
    
    // Remove a key
    store.remove("double_value");
    
    // Get store size
    std::cout << "Store size: " << store.size() << std::endl;
    
    return 0;
}
```

## Implementation Details

- Uses template metaprogramming for type safety and serialization
- Automatically flattens and reconstructs multi-dimensional arrays
- Optimizes storage of large arrays by bypassing text serialization
- Maintains an index of stored keys and their file positions
- Lazily loads values from disk only when needed

## Requirements

- C++17 compatible compiler
- Standard library with support for `std::variant`, `std::visit`, and SFINAE

## Performance Considerations

- Automatically uses binary streaming for large arrays (>1024 bytes) for better performance
- In-memory caching reduces disk I/O for frequently accessed values
- All changes are persisted to disk when the store is destroyed or when `writeAll()` is called

