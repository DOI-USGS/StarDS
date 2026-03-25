# Simple Columnar Store (SCS)

A high-performance, header-only C++ library for storing multi-dimensional arrays with optional block-based compression. SCS provides a modern, xtensor-inspired API for N-dimensional arrays and a persistent key-value store optimized for scientific computing and data processing workloads.

## Features

### Core Capabilities
- **Modern ndarray API**: xtensor-like interface with `operator()` multi-dimensional indexing
- **Block Compression**: Optional GZIP compression with configurable block sizes for efficient storage
- **Zero Dependencies**: Header-only library (compression optional via zlib)
- **Type Safe**: Template-based with compile-time type checking
- **Persistent Storage**: Automatic serialization to disk with efficient binary format
- **RAII**: Automatic flush on scope exit - no manual cleanup required
- **Iterator Support**: Range-based for loops over store keys and metadata
- **Thread Safe**: Read-write locking for concurrent access
- **Remote Access**: Built-in support for reading from HTTP/S3 via `/vsicurl/` paths (requires libcurl)
- **Format Conversion**: Built-in tools to convert between SCS, JSON, and MessagePack formats

### Array Features
- **Static Factories**: `zeros()`, `ones()`, `arange()`, `full()`, `empty()`
- **Flexible Indexing**: Multi-dimensional `arr(i, j, k)` or flat `arr.flat(idx)`
- **Iterator Support**: STL-compatible iterators for range-based for loops
- **Shape Manipulation**: `reshape()` (no realloc) and `resize()` (with realloc)
- **Serialization**: Efficient binary serialization with shape preservation

### Supported Data Types
- **Integers**: int8, int16, int32, int64, uint8, uint16, uint32, uint64
- **Floats**: float32 (float), float64 (double)
- **Strings**: std::string arrays

---

## Quick Start

### Basic Usage

```cpp
#include "scs.h"

// Create store with GZIP compression and 1KB block size
SCStore store("data.scs", CompressionAlgorithm::GZIP, 1024);

// Create arrays using static factories
auto zeros = ndarray<double>::zeros({100, 100});
auto ones = ndarray<int>::ones({50});
auto range = ndarray<int>::arange(0, 10, 2);  // [0, 2, 4, 6, 8]

// Multi-dimensional indexing
ndarray<double> matrix({3, 4});
for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 4; ++j) {
        matrix(i, j) = i * 10.0 + j;
    }
}

// Store arrays
store.put("matrix", matrix);
store.put("zeros", zeros);
store.flush();

// Retrieve arrays
auto retrieved = store.get<ndarray<double>>("matrix");
if (retrieved) {
    std::cout << "Value at (2,3): " << (*retrieved)(2, 3) << std::endl;
}

// Check existence
if (store.contains("matrix")) {
    std::cout << "Matrix exists!" << std::endl;
}
```

### Working with Iterators

```cpp
ndarray<int> arr({100});
for (size_t i = 0; i < 100; ++i) {
    arr(i) = i;
}

// Range-based for loop
int sum = 0;
for (const auto& val : arr) {
    sum += val;
}

// STL algorithms
auto max_val = *std::max_element(arr.begin(), arr.end());
```

### Iterating Through Store Keys

```cpp
SCStore store("data.scs", CompressionAlgorithm::GZIP, 1024);

// Add some arrays
store.put("array1", ndarray<int>::zeros({100}));
store.put("array2", ndarray<double>::ones({50}));
store.put("array3", ndarray<float>::arange(0.0f, 10.0f, 0.1f));

// Iterate through all keys
for (const auto& [key, entry] : store) {
    std::cout << "Key: " << key << std::endl;
    std::cout << "  Type: " << datatype_to_string(entry.datatype) << std::endl;
    std::cout << "  Size: " << entry.total_bytes << " bytes" << std::endl;
    std::cout << "  Compression: " << (entry.compression == CompressionAlgorithm::GZIP ? "GZIP" : "None") << std::endl;

    // Load and process array
    auto data = store.get<ndarray<int>>(key);
    if (data) {
        std::cout << "  Elements: " << data->size() << std::endl;
    }
}
```

### RAII Auto-Flush

```cpp
// Store automatically flushes when going out of scope
{
    SCStore store("data.scs", CompressionAlgorithm::GZIP, 1024);

    store.put("array1", ndarray<int>::zeros({100}));
    store.put("array2", ndarray<double>::ones({50}));

    // No need to call flush() explicitly
} // Destructor automatically flushes data to disk

// Data is now persisted
SCStore store2("data.scs");
assert(store2.contains("array1")); // true
assert(store2.contains("array2")); // true
```

### Shape Manipulation

```cpp
// Create 1D array
ndarray<int> arr({12});
for (size_t i = 0; i < 12; ++i) {
    arr(i) = i;
}

// Reshape to 2D (no reallocation)
arr.reshape({3, 4});
std::cout << arr(2, 3) << std::endl;  // 11

// Resize to larger (with reallocation)
arr.resize({5, 5}, 99);  // Fill new elements with 99
```

### Remote File Access

```cpp
// Read from S3 (requires libcurl)
SCStore remote_store("/vsicurl/https://example.com/data.scs");

auto data = remote_store.get<ndarray<double>>("key");
```

---

## API Reference

### ndarray Class

#### Construction
```cpp
// Shape-based construction
ndarray<T>(const std::vector<size_t>& shape);
ndarray<T>(const std::vector<size_t>& shape, const T& fill_value);

// Static factories
static ndarray<T> zeros(const std::vector<size_t>& shape);
static ndarray<T> ones(const std::vector<size_t>& shape);
static ndarray<T> full(const std::vector<size_t>& shape, const T& value);
static ndarray<T> empty(const std::vector<size_t>& shape);
static ndarray<T> arange(T start, T stop, T step = T{1});  // Arithmetic types only
```

#### Indexing
```cpp
// Multi-dimensional indexing
T& operator()(Indices... indices);
const T& operator()(Indices... indices) const;

// Flat (linear) indexing
T& flat(size_t index);
const T& flat(size_t index) const;
```

#### Shape & Size
```cpp
const std::vector<size_t>& shape() const;      // Get all dimensions
size_t shape(size_t dim) const;                // Get single dimension
size_t dimension() const;                       // Number of dimensions
size_t size() const;                            // Total number of elements
```

#### Iterators
```cpp
iterator begin();
iterator end();
const_iterator begin() const;
const_iterator end() const;
const_iterator cbegin() const;
const_iterator cend() const;
reverse_iterator rbegin();
reverse_iterator rend();
```

#### Shape Manipulation
```cpp
void reshape(const std::vector<size_t>& new_shape);  // No reallocation
void resize(const std::vector<size_t>& new_shape, const T& fill_value = T{});  // With reallocation
```

#### Data Access
```cpp
std::vector<T>& data();                        // Get underlying vector
const std::vector<T>& data() const;
T* data_ptr();                                 // Get raw pointer
const T* data_ptr() const;
```

### SCStore Class

#### Construction
```cpp
// Create new store or open existing
SCStore(const std::string& filename,
        CompressionAlgorithm compression = CompressionAlgorithm::NONE,
        size_t block_size = 0);
```

#### Storage Operations
```cpp
// Store array (uses store's default compression)
template<typename T>
void put(const std::string& key, const ndarray<T>& value);

// Store array with specific compression
template<typename T>
void put(const std::string& key, const ndarray<T>& value,
         CompressionAlgorithm compression);

// Retrieve array
template<typename T>
std::shared_ptr<ndarray<T>> get(const std::string& key);

// Check existence
bool contains(const std::string& key);

// Flush to disk (called automatically by destructor)
void flush();
```

#### Lifecycle (RAII)
```cpp
// Constructor - opens or creates file
SCStore(const std::string& filename, ...);

// Destructor - automatically flushes to disk
~SCStore();  // RAII: No need to call flush() explicitly
```

#### Iterator Support
```cpp
// Type aliases
using iterator = std::map<std::string, IndexEntry>::iterator;
using const_iterator = std::map<std::string, IndexEntry>::const_iterator;

// Iterator methods
iterator begin();
iterator end();
const_iterator begin() const;
const_iterator end() const;
const_iterator cbegin() const;
const_iterator cend() const;

// Usage: Iterate through all keys
for (const auto& [key, entry] : store) {
    // Access key and IndexEntry metadata
    auto data = store.get<ndarray<double>>(key);
}
```

#### Compression Algorithms
```cpp
enum class CompressionAlgorithm {
    NONE,   // No compression
    GZIP,   // GZIP compression (requires zlib)
    ZSTD,   // ZSTD (not yet implemented)
    LZ4     // LZ4 (not yet implemented)
};
```

---

## File Format Specification

### SCS Binary Format (Version 2)

The SCS file format is designed for efficient storage and retrieval of multi-dimensional arrays with optional block-based compression.

#### File Structure
```
┌─────────────────────────────────────┐
│  Magic String (8 bytes)             │  "CLOUDS++"
├─────────────────────────────────────┤
│  Format Version (size_t)            │  Current: 2
├─────────────────────────────────────┤
│  Header Size (size_t)               │  Size of header in bytes
├─────────────────────────────────────┤
│  Number of Entries (size_t)         │  Count of stored arrays
├─────────────────────────────────────┤
│  Index Entries (variable)           │  Metadata for each array
│    ├── Key Length (size_t)          │
│    ├── Key String (variable)        │
│    ├── Data Type (DataType enum)    │
│    ├── Shape (vector<size_t>)       │
│    ├── Compression (enum)           │
│    ├── Block Size (size_t)          │
│    ├── Total Bytes (size_t)         │
│    ├── Num Blocks (size_t)          │
│    └── Block Metadata (per block)   │
│         ├── Offset (size_t)         │
│         ├── Compressed Size         │
│         └── Uncompressed Size       │
├─────────────────────────────────────┤
│  Data Blocks (variable)             │  Actual array data
│    ├── Block 1 (compressed/raw)     │
│    ├── Block 2 (compressed/raw)     │
│    └── ...                           │
└─────────────────────────────────────┘
```

#### Data Types
```cpp
enum class DataType {
    INT8, INT16, INT32, INT64,
    UINT8, UINT16, UINT32, UINT64,
    FLOAT32, FLOAT64,
    STRING
};
```

#### ndarray Serialization Format
Each ndarray is serialized as:
```
┌─────────────────────────────────────┐
│  Number of Dimensions (size_t)      │
├─────────────────────────────────────┤
│  Shape Vector (size_t × dims)       │
├─────────────────────────────────────┤
│  Strides Vector (size_t × dims)     │
├─────────────────────────────────────┤
│  Data Elements (T × total_size)     │
└─────────────────────────────────────┘
```

#### Block Compression
- Arrays can be split into fixed-size blocks (e.g., 1KB, 4KB)
- Each block is independently compressed
- Enables partial decompression for large arrays
- Block metadata stored in header for efficient seeking

#### Benefits
- **Efficient Seeking**: Jump directly to needed blocks
- **Streaming**: Decompress only required portions
- **Parallel I/O**: Multiple blocks can be processed concurrently
- **Compression Ratio**: Better compression for large, homogeneous arrays

---

## Building from Source

### Requirements

**Required:**
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+

**Optional Dependencies:**
- **zlib**: For GZIP compression support (enable with `-DCAMERASTATEFILE_ENABLE_ZLIB=ON`)
- **libcurl**: For HTTP/S3 remote file access (enable with `-DCAMERASTATEFILE_ENABLE_CURL=ON`)

### Build Instructions

```bash
# Clone the repository
git clone https://github.com/yourusername/camerastatefile.git
cd camerastatefile

# Initialize submodules
git submodule update --init --recursive

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCAMERASTATEFILE_BUILD_TESTS=ON \
  -DCAMERASTATEFILE_BUILD_TOOLS=ON \
  -DCAMERASTATEFILE_ENABLE_ZLIB=ON \
  -DCAMERASTATEFILE_ENABLE_CURL=ON

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure

# Install (optional)
sudo make install
```

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `CAMERASTATEFILE_BUILD_TESTS` | `ON` | Build unit tests |
| `CAMERASTATEFILE_BUILD_TOOLS` | `ON` | Build command-line tools (scsls) |
| `CAMERASTATEFILE_BUILD_TRANSLATE` | `ON` | Build format conversion tool (scs_translate) |
| `CAMERASTATEFILE_ENABLE_ZLIB` | `ON` | Enable GZIP compression |
| `CAMERASTATEFILE_ENABLE_CURL` | `ON` | Enable remote file access |

### Using as Header-Only Library

Since SCS is header-only (with optional dependencies), you can include it directly:

```cmake
# In your CMakeLists.txt
include_directories(/path/to/camerastatefile/CameraStateFile/include)

# Optional: link zlib for compression
find_package(ZLIB REQUIRED)
target_link_libraries(your_target PRIVATE ${ZLIB_LIBRARIES})

# Optional: link curl for remote access
find_package(CURL REQUIRED)
target_link_libraries(your_target PRIVATE ${CURL_LIBRARIES})
```

---

## Command-Line Tools

### scsls

Inspect and view contents of SCS files.

```bash
# List all keys
scsls data.scs

# Verbose output with metadata
scsls -v data.scs

# Print specific key
scsls -d matrix data.scs

# Print all data
scsls -a data.scs
```

### scs_translate

Convert between SCS and other formats (JSON, CSV, MessagePack).

**Building with Translation Support:**
```bash
# Enable during CMake configuration
cmake .. -DCAMERASTATEFILE_BUILD_TRANSLATE=ON

# MessagePack support is optional (auto-detected)
# Install msgpack-c for MessagePack format support
```

**Basic Usage:**
```bash
# Convert SCS to JSON
scs_translate data.scs data.json

# Convert JSON to SCS
scs_translate data.json data.scs

# Convert with compression
scs_translate -c gzip data.json data.scs

# Convert with custom block size
scs_translate -c gzip -b 4096 data.json data.scs

# CSV support (2D arrays only)
scs_translate data.csv data.scs
scs_translate data.scs data.csv

# MessagePack support (if built with msgpack)
scs_translate data.scs data.msgpack
scs_translate data.msgpack data.scs
```

**JSON Formats:**

The tool supports two JSON formats:

1. **Structured Format** (explicit dtype/shape):
```json
{
  "format": "scs",
  "version": "1.0",
  "arrays": {
    "my_array": {
      "dtype": "float64",
      "shape": [10, 20],
      "data": [0.0, 1.0, 2.0, ...]
    }
  }
}
```

2. **Simple Format** (auto type detection, NEW):
```json
{
  "image_lines": 368056,
  "sensor_name": "TMC_NADIR",
  "focal_length": 0.074,
  "line_scan_rate": [0.5, -595.5, 0.003],
  "quaternions": [
    [0.965, -0.187, 0.041, -0.177],
    [0.965, -0.187, 0.042, -0.178]
  ]
}
```

**Simple Format Features:**
- ✅ Scalars automatically become 1-element arrays
- ✅ Auto type detection (int64, float64, string)
- ✅ Supports 1D and 2D arrays
- ✅ Compatible with existing JSON files
- ✅ Nested objects flattened with colon notation (e.g., `body_rotation:quaternions`)

**Options:**
- `-h, --help`: Show help message
- `-f, --format <fmt>`: Specify output format (json, msgpack)
- `-c, --compression <alg>`: Compression for SCS output (none, gzip)
- `-b, --block-size <size>`: Block size in bytes (default: 1MB)

**Use Cases:**
- **Data Exchange**: Share arrays with non-C++ environments
- **Debugging**: Inspect array contents in human-readable format
- **Archival**: Convert to JSON for long-term storage
- **Interoperability**: Exchange data with Python, JavaScript, etc.

---

## Performance Considerations

### Memory Usage
- Arrays are stored in contiguous memory (std::vector)
- Metadata overhead: ~100 bytes per array
- Block metadata: ~24 bytes per block

### Compression Trade-offs
| Block Size | Compression Ratio | Decompression Speed | Use Case |
|------------|-------------------|---------------------|----------|
| No blocks | Best | Slowest | Small arrays (<100KB) |
| 1KB | Good | Fast | Medium arrays (100KB-10MB) |
| 4KB | Good | Very Fast | Large arrays (>10MB) |
| 64KB | Fair | Very Fast | Huge arrays (>100MB) |

### Best Practices
1. **Choose appropriate block sizes**: Smaller blocks (1-4KB) for random access, larger blocks (16-64KB) for sequential access
2. **Use compression for sparse data**: Arrays with repeated patterns compress well
3. **Batch operations**: Use `flush()` after multiple `put()` operations
4. **Leverage iterators**: More efficient than indexing for sequential access
5. **Pre-allocate shapes**: Use static factories (`zeros`, `ones`) to avoid reallocation

---

## Examples

### Example 1: Image Processing

```cpp
#include "scs.h"

// Store image data
SCStore store("images.scs", CompressionAlgorithm::GZIP, 4096);

// Load RGB image as 3D array (height, width, channels)
ndarray<uint8_t> image({1920, 1080, 3});
// ... load image data ...

store.put("frame_001", image);
store.flush();

// Later: retrieve and process
auto img = store.get<ndarray<uint8_t>>("frame_001");
for (size_t y = 0; y < img->shape(0); ++y) {
    for (size_t x = 0; x < img->shape(1); ++x) {
        uint8_t r = (*img)(y, x, 0);
        uint8_t g = (*img)(y, x, 1);
        uint8_t b = (*img)(y, x, 2);
        // Process pixel...
    }
}
```

### Example 2: Scientific Computing

```cpp
#include "scs.h"

// Store simulation results
SCStore results("simulation.scs", CompressionAlgorithm::GZIP, 1024);

// Temperature field over time
ndarray<double> temperature({100, 100, 100});  // 3D grid

// Compute values
for (size_t x = 0; x < 100; ++x) {
    for (size_t y = 0; y < 100; ++y) {
        for (size_t z = 0; z < 100; ++z) {
            temperature(x, y, z) = compute_temperature(x, y, z);
        }
    }
}

results.put("temperature_t0", temperature);

// Later: analyze with iterators
auto temp = results.get<ndarray<double>>("temperature_t0");
double max_temp = *std::max_element(temp->begin(), temp->end());
double avg_temp = std::accumulate(temp->begin(), temp->end(), 0.0) / temp->size();
```

### Example 3: Data Pipeline

```cpp
#include "scs.h"

void process_batch(const std::string& input_file, const std::string& output_file) {
    SCStore input(input_file);
    SCStore output(output_file, CompressionAlgorithm::GZIP, 1024);

    // Read all arrays
    for (const auto& [key, entry] : input.m_index) {
        auto data = input.get<ndarray<double>>(key);

        // Apply transformation
        for (auto& val : *data) {
            val = std::sqrt(val * val + 1.0);  // Some operation
        }

        // Store result
        output.put(key + "_processed", *data);
    }

    output.flush();
}
```

---

## Testing

The library includes comprehensive test coverage:

- **Unit Tests**: 40+ tests covering all API features
  - `NdarrayTest`: 27 tests for ndarray class functionality
  - `SCStoreTest`: 7 tests for store operations and persistence
  - `BlockCompressionTest`: 6 tests for compression functionality

Run tests:
```bash
cd build
./CameraStateFile/tests/runCameraStateFileTests
```

Run specific test suite:
```bash
./CameraStateFile/tests/runCameraStateFileTests --gtest_filter="NdarrayTest.*"
```

---

## License

See [LICENSE.md](LICENSE.md) for details.

---

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request

---

## Roadmap

- [ ] ZSTD and LZ4 compression support
- [ ] Parallel block compression/decompression
- [ ] Memory-mapped file support for very large arrays
- [ ] Python bindings
- [ ] Advanced indexing (slicing, boolean masks)
- [ ] Lazy evaluation for array operations

---

## Acknowledgments

- Inspired by [xtensor](https://github.com/xtensor-stack/xtensor) for the ndarray API design
- Uses [gularkfilesystem](https://github.com/gulrak/filesystem) for C++17 filesystem compatibility
- Built with [GoogleTest](https://github.com/google/googletest) for unit testing
