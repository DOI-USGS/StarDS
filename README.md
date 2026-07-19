# StarDS (Simple Tensors Arrays and Rasters Dataset)

Header-only C++ library for persistent N-dimensional arrays with compression and cloud storage support.

---

### Generative AI Disclosure

This repository contains software code that was generated or modified with the assistance of artificial intelligence (AI) tools, in accordance with U.S. Geological Survey (USGS) disclosure requirements. All AI-generated or AI-assisted code has been reviewed and validated by USGS developers. 

See also: [DISCLAIMER.MD](./DISCLAIMER.md)

## Building

### Requirements

- C++17 compiler
- CMake 3.10+
- Optional: zlib (compression), libcurl (HTTP/S3), OpenSSL (S3)

### Build Tools with CMake

```bash
git clone https://code.usgs.gov/astrogeology/stards.git
cd stards
git submodule update --init --recursive

mkdir build && cd build
cmake .. \
  -DSTARDS_BUILD_TOOLS=ON \
  -DSTARDS_ENABLE_ZLIB=ON \
  -DSTARDS_ENABLE_LZ4=ON \
  -DSTARDS_ENABLE_CURL=ON \
  -DSTARDS_ENABLE_S3=ON

make -j$(nproc)
```

**CMake Options:**
- `STARDS_BUILD_TOOLS=ON` - Build CLI tools (stardsls, stards_translate)
- `STARDS_BUILD_PYTHON_BINDINGS=ON` - Build Python bindings
- `STARDS_ENABLE_ZLIB=ON` - Enable GZIP compression
- `STARDS_ENABLE_LZ4=ON` - Enable LZ4 compression
- `STARDS_ENABLE_CURL=ON` - Enable HTTP remote access
- `STARDS_ENABLE_S3=ON` - Enable S3 cloud storage

### Build Python Bindings with CMake

```bash
mkdir build && cd build
cmake .. \
  -DSTARDS_BUILD_PYTHON_BINDINGS=ON \
  -DSTARDS_ENABLE_ZLIB=ON \
  -DSTARDS_ENABLE_LZ4=ON \
  -DSTARDS_ENABLE_CURL=ON \
  -DSTARDS_ENABLE_S3=ON

make -j$(nproc)
sudo make install
```

**Requirements for Python bindings:**
- Python >= 3.8
- NumPy >= 1.20.0
- SWIG >= 4.0

---

## Using in Your Project

### Include Header in C++ Project

STAR is header-only. Add to your project:

```cmake
# CMakeLists.txt
# In-tree checkout: point at StarDS/include. Installed: the header lives at
# $PREFIX/include/stards/stards.h, so use $PREFIX/include/stards (or just
# find_package(STARDS), which sets the include path for you).
include_directories(/path/to/stards/StarDS/include)

# Optional: link compression/S3 support
find_package(ZLIB)
find_package(CURL)
find_package(OpenSSL)

if(ZLIB_FOUND)
    target_link_libraries(your_target PRIVATE ZLIB::ZLIB)
    target_compile_definitions(your_target PRIVATE ENABLE_ZLIB)
endif()

if(CURL_FOUND)
    target_link_libraries(your_target PRIVATE CURL::libcurl)
    target_compile_definitions(your_target PRIVATE ENABLE_CURL)
endif()

# S3 support (needs CURL + OpenSSL)
if(OPENSSL_FOUND AND CURL_FOUND)
    target_link_libraries(your_target PRIVATE OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(your_target PRIVATE ENABLE_S3)
endif()
```

Then in your code:

```cpp
#include "stards.h"

int main() {
    // Create and store arrays
    NDArray<double> data = NDArray<double>::zeros({100, 100});
    auto store = star::StarDataset::create("data.stards");
    store->put("matrix", data);
    store->flush();

    return 0;
}
```

---

## Command-Line Tools

### stardsls - Inspect Files

```bash
# List all keys in a file
stardsls data.stards

# Show detailed metadata
stardsls -v data.stards

# Print specific array data
stardsls -d matrix data.stards
```

### stards_translate - Format Conversion

```bash
# Convert JSON to STAR
stards_translate data.json data.stards

# Convert STAR to JSON
stards_translate data.stards data.json

# With compression
stards_translate -c gzip -b 4096 data.json data.stards
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

Without a spec, types will be inferred. Small values like strings and small arrays go into the `.meta` namespace. 

```json
{
  "my_array": [0.0, 1.0, ...],
  "meta" : "name"
}
```


---

## API Usage

### Basic C++ API

```cpp
#include "stards.h"
using namespace star;

// Create arrays
NDArray<double> matrix = NDArray<double>::zeros({100, 100});
matrix(10, 20) = 3.14;

// Create dataset with default compression
auto store = StarDataset::create("data.stards");

// Store arrays - these go to block storage
store->put("matrix", std::move(matrix));

// Store metadata - separate namespace, can use same keys!
store->meta.put("matrix", NDArray<std::string>({}, {"Matrix description"}));
store->meta.put("timestamp", NDArray<int64_t>({}, {1234567890}));

store->flush();  // Write to disk

// Read back
auto store2 = StarDataset::open("data.stards");

// Get array
auto matrix_data = store2->get<double>("matrix");
std::cout << matrix_data(10, 20) << std::endl;  // 3.14

// Get metadata (different namespace!)
auto matrix_meta = store2->meta.get("matrix");
if (matrix_meta) {
    auto desc = matrix_meta->as<std::string>();
    std::cout << desc(0) << std::endl;  // "Matrix description"
}

// Check if key exists
if (store2->meta.contains("timestamp")) {
    auto ts = store2->meta.get("timestamp")->as<int64_t>();
    std::cout << "Timestamp: " << ts(0) << std::endl;
}
```

### Layers in C++

```cpp
#include "stards.h"
using namespace star;

// Create dataset with base data
auto store = StarDataset::create("data.stards");
store->put("image", NDArray<double>::zeros({512, 512}));
store->put("wavelengths", NDArray<double>::zeros({300}));

// Create layer with a different version of the image
auto layer1 = store->create_layer("processed");
layer1->put("image", NDArray<double>::full({512, 512}, 1.0));  // Different image
// wavelengths inherited from base

// Create another layer
auto layer2 = store->create_layer("calibrated");
layer2->put("image", NDArray<double>::full({512, 512}, 2.0));
layer2->put("wavelengths", NDArray<double>::full({300}, 42.0));  // Override

store->flush();

// Read back. Layer inheritance is OFF by default: a key absent from a layer is
// a miss, not a fall-through to the base. Opt in when you want base fallback,
// either at open time via OpenOptions...
OpenOptions opts;
opts.layer_inheritance = true;
auto store2 = StarDataset::open("data.stards", FileMode::READ_ONLY, opts);
// ...or after opening with the post-open setter on any already-open dataset:
store2->set_layer_inheritance(true);

// Get base data
auto base_img = store2->get<double>("image");

// Get layer data
auto proc_layer = store2->get_layer("processed");
auto proc_img = proc_layer->get<double>("image");      // Different data
auto proc_wave = proc_layer->get<double>("wavelengths"); // Inherited (needs inheritance on)

auto cal_layer = store2->get_layer("calibrated");
auto cal_img = cal_layer->get<double>("image");        // Different data
auto cal_wave = cal_layer->get<double>("wavelengths");  // Overridden

// Layers also have metadata with inheritance
layer1->meta.put("description", NDArray<std::string>({}, {"Processed version"}));
```

### File Open Modes

STAR supports both **string modes** (Python-style) and **enum modes** for opening files:

```cpp
// String modes
auto store1 = StarDataset::open("data.stards", "r");   // Read-only
auto store2 = StarDataset::open("data.stards", "w");   // Read-write (create if missing)
auto store3 = StarDataset::open("data.stards", "rw");  // Read-write (explicit)

// Enum modes (explicit, type-safe)
auto store5 = StarDataset::open("data.stards", FileMode::READ_ONLY);
auto store6 = StarDataset::open("data.stards", FileMode::READ_WRITE);

// Create a new file
auto store7 = StarDataset::create("data.stards");
```

**Supported string modes:**
- `"r"` - Read-only (cannot modify file)
- `"w"`, `"rw"`, `"a"` - Read-write (can modify file)

### In-Memory (Byte Array) I/O

Read and write a complete `.stards` image as a byte array — no filesystem needed
(e.g. data received over a socket or stored in a database):

```cpp
// Serialize a dataset to bytes (the same image save_to() would write to disk).
auto store = StarDataset::open("data.stards", "r");
std::vector<char> bytes = store->write_bytes();

// Open a dataset directly from bytes (read-only; use write_bytes() to persist edits).
auto in_mem = StarDataset::open_bytes(bytes);
auto data = in_mem->get<double>("sensor_data");

// Also accepts a raw pointer + length.
auto in_mem2 = StarDataset::open_bytes(bytes.data(), bytes.size());
```

In Python:

```python
blob = ds.write_bytes()               # -> bytes
ds2  = StarDataset.open_bytes(blob)   # accepts bytes / bytearray / memoryview / uint8 ndarray
```

### HTTP Remote Access

```cpp
// Read from HTTP (read-only)
auto store = StarDataset::open("/vsicurl/https://example.com/data.stards", "r");
auto data = store->get<double>("sensor_data");

// Cannot write to HTTP source, but can save locally
store->save_to("/tmp/local-copy.stards");
```

### S3 Cloud Storage

```cpp
// Read from S3 (both modes work)
auto store = StarDataset::open("/vsis3/my-bucket/data.stards", "r");  // String mode
// or: StarDataset::open("/vsis3/my-bucket/data.stards", FileMode::READ_ONLY);
auto data = store->get<double>("sensor_data");

// Write to S3
auto output = StarDataset::open("/vsis3/output-bucket/results.stards", "w");
output->put("results", NDArray<double>::zeros({256, 256}));
output->flush();

// Save S3 file locally
store->save_to("/tmp/local-copy.stards");
```

**S3 URL Format:** `/vsis3/bucket-name/path/to/file.stards`

**Authentication** (choose one method)

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
from pystards import StarDataset

# Create dataset and store arrays
with StarDataset.create("data.stards") as ds:
    # Dictionary-style access for arrays
    ds["matrix"] = np.random.rand(100, 100)
    ds["vector"] = np.arange(1000)
    
    # Metadata storage (for scalars, small data)
    # Metadata has separate namespace - same keys allowed!
    ds.meta["matrix"] = "This is array metadata"
    ds.meta["sensor_id"] = 12345
    ds.meta["timestamp"] = "2024-04-21"

# Read back
with StarDataset.open("data.stards", mode="r") as ds:
    matrix = ds["matrix"]           # Gets the array
    matrix_meta = ds.meta["matrix"] # Gets the metadata (different!)
    sensor_id = ds.meta["sensor_id"]
    
    # Iterate over all arrays
    for key in ds:
        print(f"{key}: {ds[key].shape}")

# S3 access
with StarDataset.open("/vsis3/my-bucket/data.stards", mode="r") as ds:
    data = ds["sensor_data"]

# HTTP access
with StarDataset.open("/vsicurl/https://example.com/data.stards", mode="r") as ds:
    data = ds["array_name"]
```

### Layers - Multi-Version Data Storage

STAR supports **layers** for storing multiple versions of the same data. Each layer can have its own version of an array, or inherit from the base layer.

**Key Concept:** A key that isn't set in a layer can **fall back to the base layer** (inheritance). Inheritance is **off by default** — a missing key is reported as missing. Opt in either at open time with `OpenOptions`, or after opening with `set_layer_inheritance(True)`, when you want a layer to see base keys it didn't override.

```python
import numpy as np
from pystards import StarDataset

# Create dataset with base data
ds = StarDataset.create("hyperspectral.stards")
ds["image"] = np.random.rand(512, 512, 300)  # Base hyperspectral cube
ds["wavelengths"] = np.linspace(400, 2500, 300)
ds["metadata"] = np.array([1, 2, 3])          # Base calibration info

# Create layer for processed version
processed = ds.create_layer("processed")
processed["image"] = ds["image"] * 2          # Override: Different image data
# "wavelengths" and "metadata" NOT set → inherited from base if inheritance is on

# Create layer for calibrated version
calibrated = ds.create_layer("calibrated")
calibrated["image"] = ds["image"] + 1              # Override: Another version
calibrated["wavelengths"] = np.linspace(410, 2510, 300)  # Override: Different wavelengths
# "metadata" NOT set → inherited from base if inheritance is on

ds.flush()

# Read back. Inheritance is OFF by default, so opt in to get base fallback.
# Option 1 — at open time via OpenOptions:
import pystards
opts = pystards.OpenOptions()
opts.layer_inheritance = True
ds2 = StarDataset.open("hyperspectral.stards", mode="r", options=opts)

# Option 2 — after opening, on any dataset:
ds2 = StarDataset.open("hyperspectral.stards")
ds2.set_layer_inheritance(True)

base_img = ds2["image"]                           # Base version
base_waves = ds2["wavelengths"]                   # [400...2500]
base_meta = ds2["metadata"]                       # Base metadata

proc_layer = ds2.get_layer("processed")
proc_img = proc_layer["image"]                    # Processed (overridden)
proc_waves = proc_layer["wavelengths"]            # ← Inherited from base! Same as base_waves
proc_meta = proc_layer["metadata"]                # ← Inherited from base! Same as base_meta

cal_layer = ds2.get_layer("calibrated")
cal_img = cal_layer["image"]                      # Calibrated (overridden)
cal_waves = cal_layer["wavelengths"]              # Overridden (different from base)
cal_meta = cal_layer["metadata"]                  # ← Inherited from base! Same as base_meta

# Layers also support metadata with inheritance
layer = ds.create_layer("variant")
layer["image"] = ds["image"] * 0.5                # Override data
layer.meta["variant_info"] = "Layer-specific"     # Layer-specific metadata
# layer.meta["description"] would inherit from base if base had it
```

**Key Features:**
- **Separate namespaces** - Arrays and metadata don't conflict (same keys allowed)
- **Isolated storage** - Each layer's data is independent (no overwriting)
- **Automatic inheritance** - Missing keys default to base layer (no duplication needed)
- **Selective override** - Only store what's different in each layer
- **Metadata inheritance** - Works for both arrays and metadata

**When to use layers:**
- Hyperspectral imaging: raw, atmospherically corrected, subsets
- Multi-temporal datasets: different time periods
- A/B testing: variant versions for experiments
- Version control: track dataset evolution
- Processing pipelines: raw → intermediate → final

**See the [Quick Start guide](docs-site/docs/getting-started/quickstart.md) for a complete getting started guide.**

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
