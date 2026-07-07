# Binary Format Specification

This document describes the on-disk binary structure of StarDS (Simple Tensors
Arrays and Rasters) files, as written and read by the reference implementation in
`Star/include/star.h`.

## File extension

`.star`

## Format version

The current on-disk format is **version 1** (`format_version = 1`), which stores a
global key registry, per-layer metadata blocks, and per-layer presence bitmaps.
The **software/library version is not stored in the file** — only the one-byte
format version is persisted. (The library version, currently `1.0.0`, is available
at runtime via `getLibraryVersion()`.)

## Binary structure

A `.star` file is a **header section** followed by a **data section**:

```
┌──────────────────────────────────────────────────────────────┐
│ HEADER SECTION                                                 │
├──────────────────────────────────────────────────────────────┤
│ 1. Fixed file header (31 bytes)                                │
│ 2. Global key registry        (all unique key names + hashes)  │
│ 3. Layer metadata registry    (__base__ + each named layer)    │
│ 4. Layer presence bitmaps      (which arrays exist per layer)   │
│ 5. Index entries              (one per stored array)           │
├──────────────────────────────────────────────────────────────┤
│ DATA SECTION                                                   │
├──────────────────────────────────────────────────────────────┤
│ Array data blocks (compressed or raw), and per-layer           │
│ metadata blocks referenced by the layer metadata registry.     │
└──────────────────────────────────────────────────────────────┘
```

The fixed header records `header_size`, so a reader loads the whole header
section in one read, then parses sections 2–5 from it.

## Header section

All multi-byte integers are little-endian.

### 1. Fixed file header (31 bytes)

| Field | Type | Size | Offset | Description |
|-------|------|------|--------|-------------|
| Magic String | char[6] | 6 | 0 | File identifier: `"STARDS"` |
| Format Version | uint8 | 1 | 6 | On-disk format version: `1` |
| Header Size | uint64 | 8 | 7 | Total size of the header section in bytes |
| Entry Count | uint64 | 8 | 15 | Number of stored data arrays (index entries) |
| Layer Count | uint32 | 4 | 23 | Number of **named** layers (excludes `__base__`) |
| Key Registry Count | uint32 | 4 | 27 | Number of unique keys in the global registry |

**Total:** 31 bytes.

### 2. Global key registry

All unique key names are stored once, in a central registry; everywhere else keys
are referenced by their `uint16` index into this registry. The registry holds
`Key Registry Count` entries, each:

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Key Length | uint16 | 2 | Length of the key string in bytes |
| Key String | UTF-8 | variable | The key name |
| Hash | uint64 | 8 | Precomputed `std::hash` of the key (for O(1) lookup) |

### 3. Layer metadata registry

Contains `Layer Count + 1` entries — one for the implicit base layer (`__base__`)
plus one per named layer. Each entry describes that layer's metadata block and
which metadata keys it holds:

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Layer Name Length | uint16 | 2 | Length of the layer name |
| Layer Name | UTF-8 | variable | Layer name (`__base__` for the base layer) |
| Metadata Block Position | uint64 | 8 | Byte offset of this layer's metadata block in the data section |
| Metadata Block Size | uint32 | 4 | Size of the metadata block in bytes |
| Compression | uint8 | 1 | Compression algorithm of the metadata block (see enum) |
| Metadata Key Count | uint16 | 2 | Number of metadata keys in this layer |
| Key Indices | uint16[] | 2 × count | Registry indices of this layer's metadata keys |

### 4. Layer presence bitmaps

For each layer (again `Layer Count + 1`, base first), a bitmap marks which of the
data arrays are present in that layer. Each bitmap is
`ceil(Entry Count / 64)` words:

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Bitmap | uint64[] | 8 × ⌈entry_count/64⌉ | One bit per index entry; set = array present in this layer |

### 5. Index entries

One entry per stored data array (`Entry Count` total). Metadata values are **not**
listed here — they live in the per-layer metadata blocks (section 3). Each index
entry is a `uint16` key index followed by the serialized array descriptor:

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Key Index | uint16 | 2 | Index into the global key registry |
| File Position | uint64 | 8 | Byte offset to this array's data in the data section |
| Total Bytes | uint64 | 8 | Total stored size of the array data (all blocks) |
| Data Type | uint8 | 1 | Element type enum (see Data Types) |
| Num Dimensions | uint64 | 8 | Number of dimensions (`0` = scalar) |
| Shape | uint64[] | 8 × ndim | Size of each dimension |
| Compression | uint8 | 1 | Compression algorithm enum |
| Block Size | uint64 | 8 | Uncompressed block size (`0` = not blocked) |
| Num Blocks | uint64 | 8 | Number of data blocks |
| Block Metadata | BlockInfo[] | 24 × num_blocks | Per-block metadata (below) |
| Stored-in-metadata flag | uint8 | 1 | Always `0` for index entries in v1 |

### Block metadata (`BlockInfo`, per block)

| Field | Type | Size | Description |
|-------|------|------|-------------|
| Offset | uint64 | 8 | Byte offset of the block within this array's data section |
| Compressed Size | uint64 | 8 | Size of the stored (compressed) block |
| Uncompressed Size | uint64 | 8 | Size of the block when decompressed |

## Data types

Element type is stored as a single-byte enum:

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

## Compression types

Compression is stored as a single-byte enum:

| Value | Type | Description |
|-------|------|-------------|
| 0 | NONE | No compression |
| 1 | GZIP | GZIP/zlib compression (RFC 1952) |
| 2 | ZSTD | Zstandard — reserved; not implemented by the reference library |
| 3 | LZ4 | LZ4 compression |

!!! note
    The `ZSTD` value is reserved in the enum but the reference implementation
    currently reads and writes only `NONE`, `GZIP`, and `LZ4`. See the
    [Compression guide](../guides/compression.md).

## Data section

The data section holds the actual array data and the per-layer metadata blocks.

### Uncompressed data

When `Compression = NONE`:

- Data is stored in row-major (C-style) order.
- Total size = product of shape dimensions × element size.
- Can be read directly from `File Position` for `Total Bytes`.

### Compressed data (block-based)

When compressed:

- Data is divided into fixed-size blocks (`Block Size` uncompressed each).
- Each block is compressed independently.
- The per-block metadata in the index entry enables random access to specific
  blocks without decompressing the whole array.
- Block size is configurable (default: 1 MB uncompressed).

**Advantages:**

- **Efficient seeking** — jump to specific blocks without full decompression.
- **Streaming** — decompress only the needed portions.
- **Parallel I/O** — process blocks independently.

## Endianness and encoding

- All multi-byte integers are stored in **little-endian** format.
- All strings (key names, string data) are encoded in **UTF-8**.
- Multi-dimensional arrays are stored in **row-major (C-style)** order — the last
  dimension varies fastest. A 3D array `[z, y, x]` is stored as
  `arr[0,0,0], arr[0,0,1], …, arr[0,1,0], …`.

## Validation

A valid `.star` file must:

1. Start with the magic string `"STARDS"`.
2. Use a recognized format version.
3. Have a header size that matches the actual header content.
4. Have an entry count that matches the number of index entries.
5. Keep all file positions within file bounds.
6. Reference only valid key-registry indices.
7. Use valid data-type and compression-type enum values.

## Compatibility

- **Forward compatibility** — readers should ignore unknown data types or
  compression types they do not support.
- **Cross-platform** — all numeric fields use fixed-width types (an `int32` is
  always 4 bytes) and little-endian byte order.
