# STAR (Simple Tensors Arrays and Rasters)

Header-only C++ library for persistent N-dimensional arrays with compression and cloud storage support.

**Supported Types:** int8-64, uint8-64, float32/64, string

---

## Building

### Requirements

- C++17 compiler
- CMake 3.10+
- Optional: zlib (compression), libcurl (HTTP/S3), OpenSSL (S3)

### Build Tools with CMake

```bash
git clone https://github.com/yourusername/camerastatefile.git
cd camerastatefile
git submodule update --init --recursive

mkdir build && cd build
cmake .. \
  -DCAMERASTATEFILE_BUILD_TOOLS=ON \
  -DCAMERASTATEFILE_ENABLE_ZLIB=ON \
  -DCAMERASTATEFILE_ENABLE_CURL=ON \
  -DCAMERASTATEFILE_ENABLE_S3=ON

make -j$(nproc)
```

**CMake Options:**
- `CAMERASTATEFILE_BUILD_TOOLS=ON` - Build CLI tools (starls, star_translate)
- `CAMERASTATEFILE_BUILD_PYTHON_BINDINGS=ON` - Build Python bindings
- `CAMERASTATEFILE_ENABLE_ZLIB=ON` - Enable GZIP compression
- `CAMERASTATEFILE_ENABLE_CURL=ON` - Enable HTTP remote access
- `CAMERASTATEFILE_ENABLE_S3=ON` - Enable S3 cloud storage

### Build Python Bindings with CMake

```bash
mkdir build && cd build
cmake .. \
  -DCAMERASTATEFILE_BUILD_PYTHON_BINDINGS=ON \
  -DCAMERASTATEFILE_ENABLE_ZLIB=ON \
  -DCAMERASTATEFILE_ENABLE_CURL=ON \
  -DCAMERASTATEFILE_ENABLE_S3=ON

make -j$(nproc)
sudo make install
```

**Requirements for Python bindings:**
- Python >= 3.8
- NumPy >= 1.20.0
- SWIG >= 4.0

---

## File Format

For detailed binary format specification, see [FORMAT_SPEC.md](FORMAT_SPEC.md).

---

## Using in Your Project

### Include Header in C++ Project

STAR is header-only. Add to your project:

```cmake
# CMakeLists.txt
include_directories(/path/to/camerastatefile/Star/include)

# Optional: link compression/S3 support
find_package(ZLIB)
find_package(CURL)
find_package(OpenSSL)

if(ZLIB_FOUND)
    target_link_libraries(your_target PRIVATE ZLIB::ZLIB)
    target_compile_definitions(your_target PRIVATE HAS_ZLIB)
endif()

if(CURL_FOUND)
    target_link_libraries(your_target PRIVATE CURL::libcurl)
    target_compile_definitions(your_target PRIVATE HAS_CURL)
endif()

if(OPENSSL_FOUND)
    target_link_libraries(your_target PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(your_target PRIVATE HAS_OPENSSL)
endif()
```

Then in your code:

```cpp
#include "star.h"

int main() {
    // Create and store arrays
    NDArray<double> data = NDArray<double>::zeros({100, 100});
    StarDataset store("data.star");
    store.put("matrix", data);
    store.flush();

    return 0;
}
```

---

## Command-Line Tools

### starls - Inspect Files

```bash
# List all keys in a file
starls data.star

# Show detailed metadata
starls -v data.star

# Print specific array data
starls -d matrix data.star
```

### star_translate - Format Conversion

```bash
# Convert JSON to STAR
star_translate data.json data.star

# Convert STAR to JSON
star_translate data.star data.json

# With compression
star_translate -c gzip -b 4096 data.json data.star
```

**JSON Format Example:**
```json
{
  "arrays": {
    "my_array": {
      "dtype": "float64",
      "shape": [10, 20],
      "data": [0.0, 1.0, ...]
    }
  }
}
```

---

## API Usage

### Basic C++ API

```cpp
#include "star.h"

// Create arrays
NDArray<double> matrix = NDArray<double>::zeros({100, 100});
matrix(10, 20) = 3.14;

// Store with compression
StarDataset store("data.star", CompressionAlgorithm::GZIP, 1024);
store.put("matrix", matrix);
store.flush();  // Write to disk

// Retrieve
auto data = store.get<NDArray<double>>("matrix");
std::cout << (*data)(10, 20) << std::endl;  // 3.14

// Check if key exists
if (store.contains("matrix")) {
    // ...
}

// Iterate over entries
for (const auto& [key, entry] : store) {
    std::cout << key << std::endl;
}
```

### File Open Modes

STAR supports both **string modes** (Python-style) and **enum modes** for opening files:

```cpp
// String modes 
StarDataset store1("data.star", "r");   // Read-only
StarDataset store2("data.star", "w");   // Read-write (create if missing)
StarDataset store3("data.star", "rw");  // Read-write (explicit)
StarDataset store4("data.star");        // Default: read-write

// Enum modes (explicit, type-safe)
StarDataset store5("data.star", FileMode::READ_ONLY);
StarDataset store6("data.star", FileMode::READ_WRITE);
```

**Supported string modes:**
- `"r"` - Read-only (cannot modify file)
- `"w"`, `"rw"`, `"a"` - Read-write (can modify file)

### HTTP Remote Access

```cpp
// Read from HTTP (read-only)
StarDataset store("/vsicurl/https://example.com/data.star", "r");
auto data = store.get<NDArray<double>>("sensor_data");

// Cannot write to HTTP source, but can save locally
store.saveTo("/tmp/local-copy.star");
```

### S3 Cloud Storage

```cpp
// Read from S3 (both modes work)
StarDataset store("/vsis3/my-bucket/data.star", "r");  // String mode
// or: StarDataset store("/vsis3/my-bucket/data.star", FileMode::READ_ONLY);
auto data = store.get<NDArray<double>>("sensor_data");

// Write to S3
StarDataset output("/vsis3/output-bucket/results.star", "w");
output.put("results", processed_data);
output.flush();

// Save S3 file locally
store.saveTo("/tmp/local-copy.star");
```

**S3 URL Format:** `/vsis3/bucket-name/path/to/file.star`

**Authentication** (choose one method):

**Environment Variables:**
```bash
export AWS_ACCESS_KEY_ID=KEY
export AWS_SECRET_ACCESS_KEY=KEY
export AWS_DEFAULT_REGION=us-east-1
```

**AWS SSO:**
```bash
aws sso login --profile my-profile
export AWS_PROFILE=my-profile
```

**Credentials File (~/.aws/credentials):**
```ini
[default]
aws_access_key_id = KEY
aws_secret_access_key = KEY
```

### Python API

```python
import numpy as np
from pystar import StarDataset

# Create dataset and store arrays
with StarDataset.create("data.star") as ds:
    # Dictionary-style access
    ds["matrix"] = np.random.rand(100, 100)
    ds["vector"] = np.arange(1000)
    
    # Metadata storage (for scalars, small data)
    ds.meta["sensor_id"] = 12345
    ds.meta["timestamp"] = "2024-04-21"

# Read back
with StarDataset.open("data.star", mode="r") as ds:
    matrix = ds["matrix"]
    sensor_id = ds.meta["sensor_id"]
    
    # Iterate over all arrays
    for key in ds:
        print(f"{key}: {ds[key].shape}")

# S3 access
with StarDataset.open("/vsis3/my-bucket/data.star", mode="r") as ds:
    data = ds["sensor_data"]

# HTTP access
with StarDataset.open("/vsicurl/https://example.com/data.star", mode="r") as ds:
    data = ds["array_name"]
```

**See [QUICKSTART.md](QUICKSTART.md) for a complete getting started guide.**

---

## NDArray Factories

```cpp
// Create arrays
auto zeros = NDArray<double>::zeros({100, 100});
auto ones = NDArray<int>::ones({50});
auto filled = NDArray<float>::full({10, 10}, 3.14f);
auto range = NDArray<int>::arange(0, 10, 2);  // [0, 2, 4, 6, 8]
```

---

## License

See [LICENSE.md](LICENSE.md) for details.
