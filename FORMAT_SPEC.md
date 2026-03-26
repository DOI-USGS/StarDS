# STARDS Binary Format Specification

This document describes the binary structure of STARDS (Simple Tensors Arrays and Rasters Data Store) files.

## File Extension

`.star` (recommended)

## Binary Structure

```
┌─────────────────────────────────────────────────────────┐
│ HEADER SECTION                                          │
├─────────────────────────────────────────────────────────┤
│ Magic String (6 bytes)              │ "STARDS"          │
│ Format Version (1 byte)             │ Current: 2        │
│ Library Version Major (2 bytes)     │ e.g., 1           │
│ Library Version Minor (2 bytes)     │ e.g., 0           │
│ Library Version Patch (2 bytes)     │ e.g., 0           │
│ Header Size (8 bytes)               │ Size in bytes     │
│ Entry Count (8 bytes)               │ Number of arrays  │
├─────────────────────────────────────────────────────────┤
│ INDEX ENTRIES (per array)                               │
│   ├─ Key Length (8 bytes)                               │
│   ├─ Key String (variable)                              │
│   ├─ Data Type (1 byte)            │ INT64, FLOAT64...  │
│   ├─ Num Dimensions (8 bytes)                           │
│   ├─ Shape (8 bytes × ndim)                             │
│   ├─ Compression Type (1 byte)     │ NONE, GZIP, LZ4    │
│   ├─ Block Size (8 bytes)                               │
│   ├─ File Position (8 bytes)                            │
│   ├─ Total Bytes (4 bytes)                              │
│   ├─ Uncompressed Size (4 bytes)                        │
│   └─ Block Metadata (per block)                         │
│       ├─ Offset (8 bytes)                               │
│       ├─ Compressed Size (4 bytes)                      │
│       └─ Uncompressed Size (4 bytes)                    │
├─────────────────────────────────────────────────────────┤
│ DATA SECTION                                            │
├─────────────────────────────────────────────────────────┤
│ Block 1 Data (compressed or raw)                        │
│ Block 2 Data (compressed or raw)                        │
│ ...                                                     │
│ Block N Data (compressed or raw)                        │
└─────────────────────────────────────────────────────────┘
```

## Header Section

### File Header (29 bytes, fixed size)

The file header can be read in a single operation:

| Field | Type | Size | Offset | Description |
|-------|------|------|--------|-------------|
| Magic String | char[6] | 6 bytes | 0 | File identifier: `"STARDS"` |
| Format Version | uint8 | 1 byte | 6 | Format version: `2` |
| Library Version Major | uint16 | 2 bytes | 7 | Major version (e.g., 1) |
| Library Version Minor | uint16 | 2 bytes | 9 | Minor version (e.g., 0) |
| Library Version Patch | uint16 | 2 bytes | 11 | Patch version (e.g., 0) |
| Header Size | uint64 | 8 bytes | 13 | Total size of header section in bytes |
| Entry Count | uint64 | 8 bytes | 21 | Number of arrays stored in file |

**Total:** 29 bytes

### Index Entry (per array)

Each array has an index entry in the header:

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Key Length | uint64 | 8 bytes | Length of key string |
| Key String | ASCII | variable | Array identifier (name) |
| Data Type | uint8 | 1 byte | Type enum (see Data Types) |
| Num Dimensions | uint64 | 8 bytes | Number of array dimensions |
| Shape | uint64[] | 8 × ndim bytes | Size of each dimension |
| Compression Type | uint8 | 1 byte | Compression enum (see Compression) |
| Block Size | uint64 | 8 bytes | Size of compression blocks (0 if uncompressed) |
| File Position | uint64 | 8 bytes | Byte offset to data in file |
| Total Bytes | uint32 | 4 bytes | Total size of stored data |
| Uncompressed Size | uint32 | 4 bytes | Size of data when decompressed |
| Block Metadata | BlockMeta[] | variable | Metadata for each block (if compressed) |

### Block Metadata (per block, if compressed)

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Offset | uint64 | 8 bytes | Byte offset from data start |
| Compressed Size | uint32 | 4 bytes | Size of compressed block |
| Uncompressed Size | uint32 | 4 bytes | Size when decompressed |

## Data Types

Data type is stored as a single byte enum:

| Value | Type | Size per Element | Description |
|-------|------|------------------|-------------|
| 0 | INT8 | 1 byte | Signed 8-bit integer |
| 1 | INT16 | 2 bytes | Signed 16-bit integer |
| 2 | INT32 | 4 bytes | Signed 32-bit integer |
| 3 | INT64 | 8 bytes | Signed 64-bit integer |
| 4 | UINT8 | 1 byte | Unsigned 8-bit integer |
| 5 | UINT16 | 2 bytes | Unsigned 16-bit integer |
| 6 | UINT32 | 4 bytes | Unsigned 32-bit integer |
| 7 | UINT64 | 8 bytes | Unsigned 64-bit integer |
| 8 | FLOAT32 | 4 bytes | 32-bit floating point (IEEE 754) |
| 9 | FLOAT64 | 8 bytes | 64-bit floating point (IEEE 754) |
| 10 | STRING | variable | UTF-8 string |

## Compression Types

Compression type is stored as a single byte enum:

| Value | Type | Description |
|-------|------|-------------|
| 0 | NONE | No compression |
| 1 | GZIP | GZIP/zlib compression (RFC 1952) |

## Data Section

The data section contains the actual array data, either compressed or raw.

### Uncompressed Data

When `Compression Type = NONE`:
- Data is stored in row-major (C-style) order
- Total size = product of shape dimensions × element size
- Can be read directly from `File Position` for `Total Bytes`

### Compressed Data (Block-based)

When `Compression Type = GZIP`:
- Data is divided into fixed-size blocks
- Each block is compressed independently
- Block metadata in header allows random access to specific blocks
- Block size is configurable (default: 1024 bytes uncompressed)

**Block Layout:**
```
Block 1: [Compressed bytes]
Block 2: [Compressed bytes]
...
Block N: [Compressed bytes (may be partial)]
```

**Advantages:**
- **Efficient seeking**: Jump to specific blocks without full decompression
- **Streaming**: Decompress only needed portions
- **Parallel I/O**: Process blocks independently
- **Better ratios**: Optimize compression per block

**Recommended Block Sizes:**
- 1-4 KB: Random access patterns
- 16-64 KB: Sequential streaming
- No blocks: Small arrays (<100KB)

## Endianness

All multi-byte integers are stored in **little-endian** format.

## String Encoding

All strings (keys, string data) are encoded in **UTF-8**.

## Array Storage Order

Multi-dimensional arrays are stored in **row-major (C-style)** order:
- Last dimension varies fastest
- Example: 3D array `[z, y, x]` is stored as: `arr[0,0,0], arr[0,0,1], arr[0,0,2], ..., arr[0,1,0], arr[0,1,1], ...`

## File Size Calculation

Minimum file size:
```
Size = Header Size + Sum(Entry Data Sizes)

Where:
  Header Size = 29 + Sum(Index Entry Sizes)
  Fixed Header = 29 bytes (FileHeader struct)
  Index Entry Size = 8 + KeyLength + 1 + 8 + (8 × ndim) + 1 + 8 + 8 + 4 + 4 + (BlockCount × 16)
```

## Implementation Notes

### Reading Algorithm

1. Read fixed 29-byte FileHeader struct
2. Validate magic string `"STARDS"`
3. Check format version (must be `2`)
4. Read library version from header
5. Read variable-length index entries (header_size - 29 bytes)
6. For each entry:
   - Read key and metadata
   - Store index information
5. To retrieve array:
   - Look up entry by key
   - Seek to `File Position`
   - Read and decompress blocks as needed
   - Reshape into N-dimensional array

### Writing Algorithm

1. Write placeholder header (will update later)
2. For each array:
   - Serialize metadata to header
   - Write data (compressed or raw) to data section
   - Record file position and sizes
3. Seek back to start
4. Write complete header with all positions and sizes
5. Flush to storage

### Block Compression

When compressing data:
1. Divide flattened array into fixed-size chunks
2. Compress each chunk with GZIP
3. Store block metadata in header
4. Write compressed blocks sequentially

When decompressing:
1. Calculate which blocks contain desired data
2. Read only those blocks
3. Decompress each block
4. Concatenate and return slice

## Compatibility

- **Forward compatibility**: Readers should ignore unknown data types or compression types
- **Backward compatibility**: New versions should support reading Version 1 files
- **Cross-platform**: All numeric types use standard sizes (int32 is always 4 bytes)

## Validation

Valid STAR file requirements:
1. Starts with magic string `"STARDS"`
2. Format version is recognized (currently only `1`)
3. Header size matches actual header content
4. Entry count matches number of index entries
5. All file positions are within file bounds
6. All keys are unique within file
7. Data types and compression types are valid enums

## Example File Structure

A simple STAR file with two arrays:

```
Offset  | Content
--------|--------------------------------------------------
0x0000  | "STARDS" (magic)
0x0008  | 0x01 (version)
0x0009  | 0x0000000000000150 (header size = 336 bytes)
0x0011  | 0x0000000000000002 (2 entries)
        |
        | --- Entry 1: "matrix" ---
0x0019  | 0x0000000000000006 (key length = 6)
0x0021  | "matrix" (key)
0x0027  | 0x09 (FLOAT64)
0x0028  | 0x0000000000000002 (2 dimensions)
0x0030  | 0x0000000000000064, 0x0000000000000064 (100 × 100)
0x0040  | 0x01 (GZIP)
0x0041  | 0x0000000000000400 (block size = 1024)
0x0049  | 0x0000000000000150 (file position)
0x0051  | 0x00012C00 (total bytes)
0x0055  | 0x00013880 (uncompressed size)
0x0059  | [block metadata...]
        |
        | --- Entry 2: "vector" ---
        | [similar structure...]
        |
        | --- Data Section ---
0x0150  | [compressed data for "matrix"]
        | [compressed data for "vector"]
```

## Contact

For questions or clarifications about this specification, please open an issue at:
https://github.com/DOI-USGS/CameraStateFile/issues
