# SCS Command-Line Tools

Command-line utilities for working with Simple Columnar Store (SCS) files.

## Tools

### scs_inspect

A command-line tool to inspect and read SCS file contents.

#### Features

- List all keys in an SCS file
- Display metadata (type, shape, compression, block info)
- Print data for specific keys
- Verbose mode with detailed block compression statistics
- Architecture-independent data type display

#### Installation

```bash
# Build with Makefile
make -f Makefile.tools

# Or manually
g++ -std=c++20 -I. -DENABLE_CURL -DENABLE_ZLIB scs_inspect.cpp -lz -lcurl -o scs_inspect

# Optional: Install system-wide
sudo make -f Makefile.tools install
```

#### Usage

```bash
# Basic usage - list all keys
./scs_inspect data.scs

# Verbose mode - show detailed metadata
./scs_inspect -v data.scs

# Print specific key data
./scs_inspect -d mykey data.scs

# Verbose data for specific key
./scs_inspect -v -d mykey data.scs

# Print all data (WARNING: may be large)
./scs_inspect -a data.scs
```

#### Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-k, --keys` | List keys only (default) |
| `-d, --data <key>` | Print data for specific key |
| `-a, --all` | Print all data (may be large) |
| `-v, --verbose` | Verbose output with detailed metadata |

#### Example Output

**Basic listing:**
```
Opening SCS file: test_blocks.scs

╔═══════════════════════════════════════════════════════════════╗
║                      SCS File Contents                        ║
╚═══════════════════════════════════════════════════════════════╝

File: test_blocks.scs
Keys: 3
Header size: 333 bytes
Total data: 760 bytes, 360 elements

┌────────────────────────────────────────────────────────────┐
│ Keys                                                       │
├────────────────────────────────────────────────────────────┤
│ [ 0] double_array                                        │
│     float64[100], 224 bytes                             │
│ [1 ] float32_tensor                                      │
│     float32[5,4,3], 193 bytes                           │
│ [2 ] int32_matrix                                        │
│     int32[10,20], 343 bytes                             │
└────────────────────────────────────────────────────────────┘
```

**Verbose listing:**
```
┌────────────────────────────────────────────────────────────┐
│ Keys                                                       │
├────────────────────────────────────────────────────────────┤
│ [ 0] double_array                                        │
│     Type: float64                                       │
│     Shape: [100]                                      │
│     Elements: 100                                      │
│     Size: 224 bytes                                    │
│     Compression: GZIP (1 blocks)                       │
├────────────────────────────────────────────────────────────┤
```

**Printing specific key data:**
```
./scs_inspect -v -d int32_matrix test_blocks.scs

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
    [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, ...]
```

#### Supported Data Types

The tool automatically detects and displays data in the correct format:

- **Integers**: int8, int16, int32, int64, uint8, uint16, uint32, uint64
- **Floating point**: float32 (with 3 decimal precision), float64 (with 3 decimal precision)
- **Strings**: UTF-8 strings (limited to first 20 elements)
- **Arrays**: N-dimensional arrays of any supported type

#### Data Display Limits

To prevent overwhelming output, the tool limits display:
- **Numeric arrays**: Up to 100 elements (with "... (N more)" indicator)
- **String arrays**: Up to 20 elements
- Use `-a` flag cautiously with large files

#### Error Handling

The tool provides clear error messages for common issues:

```bash
# File not found
./scs_inspect nonexistent.scs
# Error: Failed to open file

# Invalid key
./scs_inspect -d badkey data.scs
# Error: Key 'badkey' not found in file
# Available keys:
#   - key1
#   - key2
```

### test_block_compression

A test program demonstrating block compression functionality.

#### Usage

```bash
./test_block_compression
```

Runs a comprehensive test suite that:
1. Creates arrays with different types and dimensions
2. Stores them with various compression settings
3. Reads back and verifies data integrity
4. Reports compression ratios and performance

## Building All Tools

```bash
# Build everything
make -f Makefile.tools

# Clean build artifacts
make -f Makefile.tools clean

# Show available targets
make -f Makefile.tools help
```

## Dependencies

- C++20 compatible compiler (g++, clang++)
- zlib (for GZIP compression)
- libcurl (for HTTP streaming support)

### Installing Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential libz-dev libcurl4-openssl-dev
```

**macOS:**
```bash
brew install zlib curl
```

**Fedora/RHEL:**
```bash
sudo dnf install gcc-c++ zlib-devel libcurl-devel
```

## Integration Examples

### Python Integration

```python
import subprocess
import json

def inspect_scs(filename):
    """Inspect SCS file from Python"""
    result = subprocess.run(
        ['./scs_inspect', filename],
        capture_output=True,
        text=True
    )
    return result.stdout

# Get file info
info = inspect_scs('data.scs')
print(info)
```

### Shell Scripts

```bash
#!/bin/bash
# Process multiple SCS files

for file in *.scs; do
    echo "Inspecting: $file"
    ./scs_inspect -v "$file" > "${file}.txt"
done
```

## File Format

SCS files use format version 2 with the following structure:

```
[MAGIC: "CLOUDS++"][VERSION: 2][HEADER_SIZE][ENTRIES]...[DATA]
```

See [BLOCK_COMPRESSION_IMPLEMENTATION.md](BLOCK_COMPRESSION_IMPLEMENTATION.md) for detailed format specification.

## Troubleshooting

### Compilation Issues

**Missing zlib:**
```
undefined reference to `compress2'
```
Solution: Install zlib-dev package

**Missing libcurl:**
```
undefined reference to `curl_easy_init'
```
Solution: Install libcurl-dev package

**C++20 not supported:**
```
error: unrecognized command line option '-std=c++20'
```
Solution: Update to GCC 10+ or Clang 10+

### Runtime Issues

**Segmentation fault:**
- Check file exists and is readable
- Verify file is valid SCS format (check magic string)
- Run with verbose logging: Set `logger::set_log_level(logger::TRACE)`

**Corrupted data:**
- Check file wasn't modified externally
- Verify compression matches what was written
- Use `-v` to inspect block metadata

## Contributing

To add new features to scs_inspect:

1. Add option parsing in main()
2. Implement feature function
3. Update print_usage() help text
4. Add example to this README
5. Test with various file types

## License

See [LICENSE.md](LICENSE.md) for license information.

## See Also

- [README.md](README.md) - Main SCS library documentation
- [BLOCK_COMPRESSION_IMPLEMENTATION.md](BLOCK_COMPRESSION_IMPLEMENTATION.md) - Implementation details
- [CameraStateFile/include/scs.h](CameraStateFile/include/scs.h) - API reference
