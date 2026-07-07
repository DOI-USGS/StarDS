# STAR (Simple Tensors Arrays and Rasters)

Header-only C++ library for persistent N-dimensional arrays with compression and cloud storage support.

---

### Generative AI Disclosure

This repository contains software code that was generated or modified with the assistance of artificial intelligence (AI) tools, in accordance with U.S. Geological Survey (USGS) disclosure requirements. All AI-generated or AI-assisted code has been reviewed and validated by USGS developers. 

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
  -DSTAR_BUILD_TOOLS=ON \
  -DSTAR_ENABLE_ZLIB=ON \
  -DSTAR_ENABLE_LZ4=ON \
  -DSTAR_ENABLE_CURL=ON \
  -DSTAR_ENABLE_S3=ON

make -j$(nproc)
```

**CMake Options:**
- `STAR_BUILD_TOOLS=ON` - Build CLI tools (starls, star_translate)
- `STAR_BUILD_PYTHON_BINDINGS=ON` - Build Python bindings
- `STAR_ENABLE_ZLIB=ON` - Enable GZIP compression
- `STAR_ENABLE_LZ4=ON` - Enable LZ4 compression
- `STAR_ENABLE_CURL=ON` - Enable HTTP remote access
- `STAR_ENABLE_S3=ON` - Enable S3 cloud storage

### Build Python Bindings with CMake

```bash
mkdir build && cd build
cmake .. \
  -DSTAR_BUILD_PYTHON_BINDINGS=ON \
  -DSTAR_ENABLE_ZLIB=ON \
  -DSTAR_ENABLE_LZ4=ON \
  -DSTAR_ENABLE_CURL=ON \
  -DSTAR_ENABLE_S3=ON

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
include_directories(/path/to/stards/Star/include)

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
using namespace star;

// Create arrays
NDArray<double> matrix = NDArray<double>::zeros({100, 100});
matrix(10, 20) = 3.14;

// Create dataset with default compression
auto store = StarDataset::create("data.star");

// Store arrays - these go to block storage
store->put("matrix", std::move(matrix));

// Store metadata - separate namespace, can use same keys!
store->meta.put("matrix", NDArray<std::string>({}, {"Matrix description"}));
store->meta.put("timestamp", NDArray<int64_t>({}, {1234567890}));

store->flush();  // Write to disk

// Read back
auto store2 = StarDataset::open("data.star");

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
#include "star.h"
using namespace star;

// Create dataset with base data
auto store = StarDataset::create("data.star");
store->put("image", NDArray<double>({512, 512}));
store->put("wavelengths", NDArray<double>({300}));

// Create layer with different version of data
auto layer1 = store->create_layer("processed");
layer1->put("image", processed_image);  // Different image
// wavelengths inherited from base

// Create another layer
auto layer2 = store->create_layer("calibrated");
layer2->put("image", calibrated_image);
layer2->put("wavelengths", adjusted_wavelengths);  // Override

store->flush();

// Read back
auto store2 = StarDataset::open("data.star");

// Get base data
auto base_img = store2->get<double>("image");

// Get layer data
auto proc_layer = store2->get_layer("processed");
auto proc_img = proc_layer->get<double>("image");      // Different data
auto proc_wave = proc_layer->get<double>("wavelengths"); // Inherited

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
    # Dictionary-style access for arrays
    ds["matrix"] = np.random.rand(100, 100)
    ds["vector"] = np.arange(1000)
    
    # Metadata storage (for scalars, small data)
    # Metadata has separate namespace - same keys allowed!
    ds.meta["matrix"] = "This is array metadata"
    ds.meta["sensor_id"] = 12345
    ds.meta["timestamp"] = "2024-04-21"

# Read back
with StarDataset.open("data.star", mode="r") as ds:
    matrix = ds["matrix"]           # Gets the array
    matrix_meta = ds.meta["matrix"] # Gets the metadata (different!)
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

### Layers - Multi-Version Data Storage

STAR supports **layers** for storing multiple versions of the same data. Each layer can have its own version of an array, or inherit from the base layer.

**Key Concept:** When you access a key in a layer that wasn't explicitly set in that layer, it **automatically falls back to the base layer**. You only override what changes.

```python
import numpy as np
from pystar import StarDataset

# Create dataset with base data
ds = StarDataset.create("hyperspectral.star")
ds["image"] = np.random.rand(512, 512, 300)  # Base hyperspectral cube
ds["wavelengths"] = np.linspace(400, 2500, 300)
ds["metadata"] = calibration_info

# Create layer for processed version
processed = ds.create_layer("processed")
processed["image"] = median_filter(ds["image"])  # Override: Different image data
# "wavelengths" and "metadata" NOT set → will inherit from base automatically!

# Create layer for calibrated version
calibrated = ds.create_layer("calibrated")
calibrated["image"] = calibrate(ds["image"])     # Override: Another version
calibrated["wavelengths"] = adjusted_wavelengths # Override: Different wavelengths
# "metadata" NOT set → will inherit from base automatically!

ds.flush()

# Read back - inheritance happens automatically
ds2 = StarDataset.open("hyperspectral.star")

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
layer["data"] = modified_array                    # Override data
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
