# scs_inspect Quick Start Guide

## Installation

```bash
# Build the tool
make -f Makefile.tools

# Or manually
g++ -std=c++20 -I. -DENABLE_CURL -DENABLE_ZLIB scs_inspect.cpp -lz -lcurl -o scs_inspect
```

## Common Commands

### 1. List all keys in a file
```bash
./scs_inspect data.scs
```
**Output:** Table of keys with type and size

### 2. Show detailed metadata
```bash
./scs_inspect -v data.scs
```
**Output:** Includes compression info, block count, element count

### 3. View specific key's data
```bash
./scs_inspect -d mykey data.scs
```
**Output:** Prints up to 100 data values

### 4. View specific key with full details
```bash
./scs_inspect -v -d mykey data.scs
```
**Output:** Block-by-block compression statistics + data

### 5. Print all data (use with caution)
```bash
./scs_inspect -a data.scs
```
**Warning:** May produce large output

### 6. Get help
```bash
./scs_inspect --help
```

## Output Examples

### Basic Listing
```
File: test_blocks.scs
Keys: 3
Total data: 760 bytes, 360 elements

┌─────────────────────────────────────────┐
│ Keys                                    │
├─────────────────────────────────────────┤
│ [ 0] double_array                      │
│     float64[100], 224 bytes           │
│ [1 ] int32_matrix                      │
│     int32[10,20], 343 bytes           │
└─────────────────────────────────────────┘
```

### Verbose Block Details
```
=== Key: int32_matrix ===
  Type: int32[10, 20]
  Elements: 200
  Compression: GZIP
  Block size: 1024 bytes
  Num blocks: 1
  Total bytes: 343
  Blocks:
    [0] offset=0, compressed=343, uncompressed=840 (40.8%)

  Reading data...
    Data (showing up to 100 elements):
    [0, 1, 2, 3, 4, 5, ...]
```

## Tips

- **Large files**: Use `-v` without `-a` to see metadata without loading all data
- **Compression ratios**: Look for the percentage in block details (e.g., "40.8%")
- **Shape information**: Arrays show dimensions like `[10, 20]` for 2D, `[100]` for 1D
- **Data preview**: Limited to 100 elements to avoid overwhelming output
- **Error checking**: Tool validates file format and provides clear error messages

## Scripting

### Export key list
```bash
./scs_inspect data.scs | grep '^\| \[' > keys.txt
```

### Check file size
```bash
./scs_inspect data.scs | grep 'Total data'
```

### Inspect multiple files
```bash
for f in *.scs; do
    echo "=== $f ==="
    ./scs_inspect "$f"
done
```

## Troubleshooting

**"Error: Failed to open file"**
- Check file exists and is readable
- Verify file has `.scs` extension

**"Error: Key 'xyz' not found"**
- Run without `-d` to see available keys
- Check spelling (keys are case-sensitive)

**Segmentation fault**
- File may be corrupted
- Check file was written with compatible version
- Verify file starts with "CLOUDS++" magic string

## See Also

- [TOOLS_README.md](TOOLS_README.md) - Full documentation
- [BLOCK_COMPRESSION_IMPLEMENTATION.md](BLOCK_COMPRESSION_IMPLEMENTATION.md) - File format details
