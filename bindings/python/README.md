# STAR Python Bindings

Python bindings for the STAR library - a STAR (Simple Tensors Arrays and Rasters) for persistent N-dimensional arrays.

## Features

- **NumPy Integration**: Seamless conversion between NumPy arrays and STAR storage
- **Type Safe**: Supports all numeric types (int8-64, uint8-64, float32/64) and strings
- **Efficient Slicing**: Load subsets of large arrays without reading entire file
- **Cloud Storage**: Direct S3 and HTTP support via VSI paths
- **Compression**: Optional zlib compression for reduced file sizes
- **Simple API**: Pythonic interface with context managers

## Installation

### From Source

```bash
# Clone the repository
git clone https://code.usgs.gov/astrogeology/stards.git
cd stards

# Create build directory
mkdir build && cd build

# Configure with Python bindings enabled
cmake .. -DSTAR_BUILD_PYTHON_BINDINGS=ON

# Build and install
make -j$(nproc)
make install
```

### Requirements

- Python >= 3.8
- NumPy >= 1.20.0
- SWIG >= 4.0 (for building from source)
- CMake >= 3.14 (for building from source)

## Quick Start

```python
import numpy as np
from pystards import StarDataset

# Create a new dataset
with StarDataset.create("data.stards") as ds:
    # Dictionary-style access (recommended)
    ds["matrix"] = np.random.rand(100, 100)
    ds["vector"] = np.arange(1000)
    
    # Metadata for scalars and small data
    ds.meta["version"] = 1
    ds.meta["author"] = "Alice"
    
    # Data auto-flushed on exit

# Read back
with StarDataset.open("data.stards", mode="r") as ds:
    matrix = ds["matrix"]
    
    # Iterate over keys
    for key in ds:
        print(f"{key}: shape {ds[key].shape}")
```

## Usage Examples

### Basic Operations

```python
import numpy as np
from pystards import StarDataset

# Create/open dataset
ds = StarDataset.create("example.stards")

# Store arrays - dictionary style
ds["integers"] = np.array([1, 2, 3, 4, 5])
ds["floats"] = np.random.rand(100)
ds["matrix"] = np.ones((10, 10))

# Store metadata
ds.meta["created"] = "2024-04-21"
ds.meta["version"] = 1

ds.flush()

# Retrieve arrays
data = ds["integers"]  # Returns NumPy array

# Check if key exists
if "matrix" in ds:
    matrix = ds["matrix"]

# Iterate over keys
for key in ds:
    print(key)

# Get metadata
version = ds.meta["version"]

# Iterate over metadata
for key in ds.meta:
    print(f"Meta: {key}")
    
ds.close()
```

### Array vs Metadata Storage

**Array Storage** (`ds["key"]`):
- For large arrays and data that needs slicing
- Stored separately with compression support
- **Rule:** Must be arrays - raw Python scalars/strings not allowed

```python
ds["data"] = [1, 2, 3]              # ✓ List
ds["data"] = np.array([1, 2, 3])    # ✓ NumPy array
ds["data"] = np.array(5)            # ✓ 0-d array
ds["data"] = 5                      # ✗ Error: raw scalar
ds["data"] = "hello"                # ✗ Error: raw string
```

**Metadata Storage** (`ds.meta["key"]`):
- For scalars, strings, and small data
- Stored in metadata block (fast access)
- **Accepts any type** including raw scalars

```python
ds.meta["count"] = 42               # ✓ Raw scalar
ds.meta["name"] = "experiment"      # ✓ Raw string
ds.meta["tags"] = ["a", "b", "c"]   # ✓ List
ds.meta["values"] = np.array([1,2]) # ✓ Array
```

**When to use which:**
- Use `ds[]` for: Large arrays, data you'll slice, numerical datasets
- Use `ds.meta[]` for: Configuration, metadata, labels, small scalars/strings

### Iteration

Both StarDataset and metadata support iteration:

```python
# Iterate over all arrays
for key in ds:
    print(f"{key}: {ds[key].shape}")

# Iterate over metadata only
for key in ds.meta:
    value = ds.meta[key]
    print(f"Meta {key}: {value}")

# Get all metadata at once (returns dict with numpy arrays)
all_meta = ds.get_all_metadata()
for key, value in all_meta.items():
    print(f"{key}: {value}")
```

### Logger Control

Control STARDS logging level at runtime:

```python
import pystards

# Check current log level
print(f"Current level: {pystards.get_log_level()}")  # Default: 4 (ERROR)

# Enable debug logging to see internal operations
pystards.set_log_level(pystards.LogLevel.DEBUG)

# Available levels:
# pystards.LogLevel.TRACE   (0) - Most verbose
# pystards.LogLevel.DEBUG   (1) - Debug messages
# pystards.LogLevel.INFO    (2) - Info messages
# pystards.LogLevel.WARN    (3) - Warnings
# pystards.LogLevel.ERROR   (4) - Errors only (default)

# Or use string names
pystards.set_log_level("DEBUG")

# Or use integers directly
pystards.set_log_level(1)  # DEBUG

# Reset to default (errors only)
pystards.set_log_level(pystards.LogLevel.ERROR)
```

### Explicit Close (New!)

Close datasets explicitly when not using context managers:

```python
# Without context manager
store = StarDataset("data.stards")
store["data"] = np.array([1, 2, 3])
store.close()  # Explicitly flush and close

# Context manager still recommended (auto-close)
with StarDataset("data.stards") as store:
    store["data"] = np.array([1, 2, 3])
    # Automatically closed on exit
```

### Array Slicing (for Large Arrays)

```python
# Store a large array
large_data = np.arange(1_000_000)
store.put("big_array", large_data)
store.flush()

# Check if sliceable
if store.is_sliceable("big_array"):
    # Load only a subset [1000:2000]
    subset = store.get_slice("big_array", [(1000, 2000)])
    print(subset.shape)  # (1000,)

# 2D slicing
matrix = np.arange(10000).reshape(100, 100)
store.put("big_matrix", matrix)
store.flush()

# Get slice [10:20, 30:40]
subset_2d = store.get_slice("big_matrix", [(10, 20), (30, 40)])
print(subset_2d.shape)  # (10, 10)

# 3D slicing with step
array_3d = np.arange(8000).reshape(20, 20, 20)
store.put("3d_data", array_3d)
store.flush()

# Get every 2nd element in first dimension
subset_3d = store.get_slice("3d_data", [(0, 20, 2), (5, 15), (0, 20)])
```

### Cloud Storage (S3)

```python
# Write to S3 (requires AWS credentials)
with StarDataset("/vsis3/my-bucket/data.stards") as store:
    store.put("cloud_data", np.random.rand(1000, 100))

# Read from S3
with StarDataset("/vsis3/my-bucket/data.stards", mode="r") as store:
    data = store.get("cloud_data")

# Read from HTTP (read-only)
with StarDataset("/vsicurl/https://example.com/data.stards", mode="r") as store:
    data = store.get("some_array")
```

### Array Creation Helpers

```python
from pystards import zeros, ones, arange, full

# Create arrays similar to NumPy
z = zeros((5, 5))           # 5x5 array of zeros
o = ones((3, 4))            # 3x4 array of ones
r = arange(100)             # 0 to 99
f = full((10, 10), 7.5)     # 10x10 array filled with 7.5

# Store them
store.put("zeros", z)
store.put("ones", o)
```

### File Configuration (StarConfig)

Customize compression, block sizes, and metadata storage when creating files:

```python
from pystards import StarDataset, StarConfig, CompressionAlgorithm

# Example 1: High compression for archival storage
config = StarConfig()
config.compression = CompressionAlgorithm.ZSTD
config.block_size = 512 * 1024  # 512KB blocks
config.metadata_compression = CompressionAlgorithm.ZSTD

store = StarDataset.create("/tmp/archive.stards", config)
store["data"] = np.random.rand(1000, 1000)
store.flush()

# Example 2: Fast writes with LZ4
config = StarConfig()
config.compression = CompressionAlgorithm.LZ4
config.block_size = 2 * 1024 * 1024  # 2MB blocks

store = StarDataset.create("/tmp/fast.stards", config)
store["data"] = np.random.rand(1000, 1000)
store.flush()

# Example 3: No compression (maximum speed)
config = StarConfig()
config.compression = CompressionAlgorithm.NONE
config.block_size = 4 * 1024 * 1024  # 4MB blocks

store = StarDataset.create("/tmp/uncompressed.stards", config)
store["data"] = np.random.rand(1000, 1000)
store.flush()

# Example 4: Disable metadata block (all arrays stored separately)
config = StarConfig()
config.metadata_block_enabled = False  # Makes small arrays sliceable

store = StarDataset.create("/tmp/no_metadata.stards", config)
store["small"] = np.array([1, 2, 3])  # Normally goes to metadata block
store.flush()
# Now even small arrays support slicing

# Example 5: Larger metadata block for many small arrays
config = StarConfig()
config.metadata_max_block_size = 256 * 1024  # 256KB (default is 64KB)

store = StarDataset.create("/tmp/large_metadata.stards", config)
for i in range(100):
    store[f"array_{i}"] = np.arange(100, dtype=np.float64)
store.flush()
```

**Configuration Parameters:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `compression` | CompressionAlgorithm | GZIP | Main data compression algorithm |
| `block_size` | int (bytes) | 1048576 (1MB) | Compression block size |
| `metadata_block_enabled` | bool | True | Enable shared metadata block for small arrays |
| `metadata_max_block_size` | int (bytes) | 65536 (64KB) | Maximum size of metadata block |
| `metadata_compression` | CompressionAlgorithm | GZIP | Metadata block compression |
| `arena_chunk_size` | int (bytes) | 1048576 (1MB) | Memory arena allocation size |

**Compression Algorithms:**

- `CompressionAlgorithm.NONE` - No compression (fastest, largest files)
- `CompressionAlgorithm.LZ4` - Fast compression (~3x faster than GZIP)
- `CompressionAlgorithm.GZIP` - Balanced compression/speed (default)
- `CompressionAlgorithm.ZSTD` - Best compression (slower, smallest files)

**Block Size Guidelines:**

- **256KB-512KB**: Better compression ratio, slower random access
- **1MB-2MB**: Balanced performance (recommended)
- **4MB-8MB**: Faster sequential I/O, slightly less compression

**Metadata Block:**

- **Enabled (default)**: Small arrays (<= 64KB) stored together in a compressed block
  - Faster access for many small arrays (single decompression)
  - Reduces file overhead
  - Small arrays cannot be sliced (must load entire metadata block)

- **Disabled**: All arrays stored separately
  - Enables slicing of even small arrays
  - More file overhead per array
  - Better when all arrays are large

### Different Data Types

```python
# Supported dtypes
dtypes = [
    np.int8, np.int16, np.int32, np.int64,
    np.uint8, np.uint16, np.uint32, np.uint64,
    np.float32, np.float64
]

for dtype in dtypes:
    arr = np.arange(100, dtype=dtype)
    store.put(f"array_{dtype.__name__}", arr)

store.flush()

# Retrieve with original dtype preserved
int32_data = store.get("array_int32")
print(int32_data.dtype)  # dtype('int32')
```

## API Reference

### StarDataset

Main interface for reading and writing arrays.

**Constructor:**
- `StarDataset(filename, mode="rw")` - Open or create store
  - `filename`: Path (local, /vsis3/, /vsicurl/)
  - `mode`: "rw" (read-write), "r" (read-only), "w" (write), "a" (append)

**Factory Methods:**
- `StarDataset.create(filename, config=None)` - Create new file with optional configuration
- `StarDataset.open(filename, mode="rw")` - Open existing file

**Methods:**
- `put(key, array)` - Store an array
- `get(key)` - Retrieve an array as NumPy array
- `get_slice(key, slices)` - Get array subset
- `keys()` - Get all keys
- `flush()` - Write pending changes
- `close()` - Flush and close the dataset (NEW!)
- `is_sliceable(key)` - Check if array supports slicing
- `__contains__(key)` - Check if key exists (`"key" in store`)
- `__len__()` - Number of entries (`len(store)`)
- `__getitem__(key)` - Get array using `store["key"]` (NEW!)
- `__setitem__(key, array)` - Store array using `store["key"] = array` (NEW!)

### StarConfig

Configuration for file creation. Pass to `StarDataset.create()`.

**Properties:**
- `compression` - Main data compression (CompressionAlgorithm, default: GZIP)
- `block_size` - Compression block size in bytes (int, default: 1048576)
- `metadata_block_enabled` - Enable metadata block (bool, default: True)
- `metadata_max_block_size` - Max metadata block size in bytes (int, default: 65536)
- `metadata_compression` - Metadata compression (CompressionAlgorithm, default: GZIP)
- `arena_chunk_size` - Arena allocation size in bytes (int, default: 1048576)

**Example:**
```python
config = StarConfig()
config.compression = CompressionAlgorithm.ZSTD
config.block_size = 512 * 1024
store = StarDataset.create("output.stards", config)
```

### NDArray

Wrapper for C++ ndarray with NumPy conversion.

**Methods:**
- `from_numpy(arr)` - Create from NumPy array (static)
- `to_numpy()` - Convert to NumPy array
- `shape` - Array dimensions (property)
- `ndim` - Number of dimensions (property)
- `size` - Total elements (property)

### Helper Functions

- `zeros(shape, dtype=np.float64)` - Create zero-filled array
- `ones(shape, dtype=np.float64)` - Create one-filled array
- `arange(start, stop, step, dtype)` - Create range array
- `full(shape, value, dtype)` - Create constant-filled array

### Logger Functions (New!)

Control STARDS C++ logging at runtime:

- `set_log_level(level)` - Set logging level
  - `level`: Integer 0-4, string ("TRACE", "DEBUG", "INFO", "WARN", "ERROR"), or LogLevel constant
- `get_log_level()` - Get current logging level (returns int 0-4)
- `LogLevel` - Log level constants class
  - `LogLevel.TRACE` (0)
  - `LogLevel.DEBUG` (1)
  - `LogLevel.INFO` (2)
  - `LogLevel.WARN` (3)
  - `LogLevel.ERROR` (4) - Default

### Enums

- `DataType` - Data type enumeration
- `CompressionAlgorithm` - Compression options
- `FileMode` - File open modes

## Testing

```bash
# Run tests
cd bindings/python
pytest tests/ -v

# With coverage
pytest tests/ -v --cov=pystards --cov-report=html
```

## Examples

See the `examples/` directory:
- `basic_usage.py` - Basic operations
- `numpy_interop.py` - NumPy integration and slicing
- `s3_example.py` - Cloud storage (S3/HTTP)
- `new_features.py` - Dictionary syntax, logger control, and explicit close (NEW!)

## Performance Tips

1. **Flush strategically**: Call `flush()` after batch writes, not after each `put()`
2. **Use slicing**: For large arrays, use `get_slice()` to avoid loading entire arrays
3. **Choose appropriate dtypes**: Use smallest dtype that fits your data
4. **Context managers**: Use `with StarDataset(...)` for automatic flushing
5. **Tune compression**: Use `StarConfig` to balance speed vs. file size
   - LZ4 for fast writes (~3x faster than GZIP)
   - ZSTD for maximum compression (archival storage)
   - NONE for temporary files or pre-compressed data
6. **Adjust block size**: Larger blocks (2-4MB) for sequential I/O, smaller blocks (512KB) for better compression
7. **Disable metadata block**: For all-large-array datasets, disable metadata block to enable slicing of all data

## Limitations

- **Slicing dimensions**: Currently supports 1D, 2D, and 3D arrays only
- **String arrays**: Limited support; numeric types preferred
- **Thread safety**: Not thread-safe; use separate Store instances per thread
- **In-place modification**: Not supported; arrays are immutable once stored

## Contributing

See the main [StarDS repository](https://code.usgs.gov/astrogeology/stards) for contribution guidelines.

## License

MIT License - see repository for full text.

## Support

- Issues: https://code.usgs.gov/astrogeology/stards/-/issues
- Documentation: https://code.usgs.gov/astrogeology/stards

## Version

Current version: 1.0.0
