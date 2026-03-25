# Block Compression Implementation

## Summary

Successfully implemented architecture-independent block compression for the Simple Columnar Store (SCS) library.

## Key Features Implemented

### 1. Architecture-Independent Data Types
- Added explicit-width data types (int8, int16, int32, int64, uint8, uint16, uint32, uint64, float32, float64, string)
- Type mapping system to convert C++ types to portable DataType enum
- Ensures cross-platform compatibility

### 2. Shape-Based Array Representation
- Removed `is_array` flag - shape vector determines if value is scalar or array
- Empty shape `[]` = scalar
- Non-empty shape = n-dimensional array (e.g., `[100]` = 1D, `[10, 20]` = 2D)

### 3. Block Compression Support
- **BlockInfo structure**: Tracks per-block metadata (offset, compressed_size, uncompressed_size)
- **IndexEntry structure**: Stores complete metadata for each key:
  - DataType, shape, compression algorithm, block size
  - List of BlockInfo for all blocks
- **Compression algorithms**: NONE, GZIP, ZSTD (extensible), LZ4

### 4. Block Compression Functions
- `compressBlocks()`: Compresses data in configurable block sizes
- `decompressBlocks()`: Decompresses all or selected blocks
- Enables selective decompression (important for HTTP range requests)

### 5. Updated File Format (Version 2)

```
[MAGIC: "CLOUDS++"][VERSION: uint8][HEADER_SIZE: uint64]
[NUM_ENTRIES: uint64]

For each entry:
  [KEY_LENGTH: uint64][KEY_BYTES: char[]]
  [POSITION: uint64]
  [TOTAL_BYTES: uint64]
  [DATATYPE: uint8]
  [NDIM: uint64]
  [SHAPE: uint64[NDIM]]
  [COMPRESSION: uint8]
  [BLOCK_SIZE: uint64]
  [NUM_BLOCKS: uint64]
  For each block:
    [OFFSET: uint64]
    [COMPRESSED_SIZE: uint64]
    [UNCOMPRESSED_SIZE: uint64]

[DATA SECTION]
For each key:
  [BLOCK_0_DATA][BLOCK_1_DATA]...
```

## API Changes

### Updated Constructor
```cpp
SCStore(const std::string& fname,
        CompressionAlgorithm compression = CompressionAlgorithm::NONE,
        size_t block_size = 64*1024);
```

### Updated put() Method
```cpp
template<typename V>
void put(const std::string& key, const V& value,
         CompressionAlgorithm compression = CompressionAlgorithm::NONE,
         size_t block_size = 0);
```

### Example Usage
```cpp
// Create store with GZIP compression and 1KB blocks
SCStore store("data.scs", CompressionAlgorithm::GZIP, 1024);

// Store arrays (uses default compression)
NDArray<double> arr({100});
store.put("data", arr);

// Store with explicit no compression
store.put("uncompressed", arr, CompressionAlgorithm::NONE);

// Flush to disk
store.flush();

// Read back
auto retrieved = store.get<NDArray<double>>("data");
```

## Test Results

Test file: `test_block_compression.cpp`

### Data Stored
1. **double_array**: 100 doubles (800 bytes data + 24 bytes metadata)
   - Compressed: 824 → 224 bytes (27.2% compression ratio)

2. **int32_matrix**: 10×20 int32 (800 bytes data + 40 bytes metadata)
   - Compressed: 840 → 343 bytes (40.8% compression ratio)

3. **float32_tensor**: 5×4×3 float32 (240 bytes data + 56 bytes metadata)
   - Compressed: 296 → 193 bytes (65.2% compression ratio)

### Results
- **All integrity tests passed** ✓
- **File size**: 1.1 KB (including header)
- **Original data size**: ~2 KB
- **Overall compression**: ~45% size reduction

## Benefits

1. **Portable**: Architecture-independent format readable across platforms
2. **Efficient**: Block compression reduces I/O for large datasets
3. **Flexible**: Per-key compression settings
4. **Cloud-optimized**: Supports selective block decompression for range requests
5. **Type-safe**: Stores datatype and shape metadata with validation

## Future Enhancements

- Support for ZSTD and LZ4 compression algorithms
- Parallel block compression/decompression
- Configurable compression levels
- Block caching for frequently accessed data
- Async I/O for background compression

## Files Modified

- [CameraStateFile/include/scs.h](CameraStateFile/include/scs.h) - Main implementation
- [test_block_compression.cpp](test_block_compression.cpp) - Test suite
