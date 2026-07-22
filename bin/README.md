# STAR Command-Line Tools

This directory contains command-line tools for working with STAR files.

## Tools

### 1. stardsls
**Purpose**: Inspect and list contents of STAR files

**Usage**:
```bash
# List all keys
stardsls data.stards

# Verbose output with metadata
stardsls -v data.stards

# Print specific key data
stardsls -d array_name data.stards

# Print all data
stardsls -a data.stards
```

**Build**: Always built when `STARDS_BUILD_TOOLS=ON` (default)

---

### 2. stards_translate
**Purpose**: Convert between STAR and other formats (JSON, MessagePack, CSV, ISDS)

**Usage**:
```bash
# STAR to JSON
stards_translate data.stards data.json

# JSON to STAR
stards_translate data.json data.stards

# CSV to STAR (2D arrays only)
stards_translate data.csv data.stards

# MessagePack (if built with msgpack support)
stards_translate data.stards data.msgpack

# ISDS optimization: reorganize by array size
# Large arrays go to array storage, small data goes to metadata
stards_translate -f isds input.stards output.stards

# ISDS with custom threshold (default: 100 elements)
stards_translate -f isds -t 50 input.stards output.stards
```

**ISDS Conversion**:

The ISDS (ISIS Dataset) optimization reorganizes STAR files by moving large arrays to efficient block storage and keeping small metadata in the metadata block. This is useful for ISIS camera state files with mixed data sizes.

- Arrays with more elements than threshold → array storage (compressed blocks)
- Arrays with fewer elements → metadata storage (quick access)
- Default threshold: 100 elements
- Preserves all data, just reorganizes storage

Example workflow:
```bash
# Convert ISIS camera state file for optimal performance
stards_translate -f isds -t 100 camera_state.stards camera_state_optimized.stards

# Large arrays (quaternions, ephemeris) → block storage
# Small scalars (focal_length, detector_center) → metadata
```

**Build**: Controlled by `STARDS_BUILD_TRANSLATE=ON` (default)

**Dependencies**:
- Required: nlohmann/json (included as submodule)
- Optional: msgpack-c (for MessagePack support)

---

## Building

### Build All Tools

```bash
mkdir build && cd build
cmake .. \
  -DSTARDS_BUILD_TOOLS=ON \
  -DSTARDS_BUILD_TRANSLATE=ON
make
```

### Build Specific Tool

```bash
# Build only stardsls
make stardsls

# Build only stards_translate
make stards_translate
```

### Install

```bash
sudo make install
# Installs to: ${CMAKE_INSTALL_PREFIX}/bin/
```

---

## Testing

### Quick Smoke Test

Fast basic functionality test:

```bash
cd build
../bin/quick_test_translate.sh
```

**Time**: ~5 seconds
**Tests**: Help message, basic conversion with existing test data

### Comprehensive Validation

Full validation suite with all features:

```bash
cd build
../bin/validate_star_translate.sh
```

**Time**: ~30-60 seconds
**Tests**:
- Build verification
- Installation check
- STAR → JSON conversion
- JSON → STAR round-trip
- Data integrity validation
- Compression functionality
- Error handling
- JSON content validation

### CTest Integration

Run via CTest:

```bash
cd build
ctest -R star_translate_validation -V
```

Or run all tests:

```bash
ctest --output-on-failure
```

---

## Examples and Tests

### Example Programs

Located in this directory:

1. **example_json_conversion.cpp**
   - Shows programmatic JSON conversion
   - Demonstrates direct use of nlohmann/json
   - Template for custom converters

2. **test_translate_roundtrip.cpp**
   - Automated integration test
   - Validates STAR → JSON → STAR round-trip
   - Checks data integrity

Build examples:
```bash
cd build
make example_json_conversion
./bin/example_json_conversion
```

### Test Scripts

1. **quick_test_translate.sh** - Fast smoke test (~5s)
2. **validate_star_translate.sh** - Comprehensive validation (~60s)
3. **test_translate.sh** - Manual interactive testing

---

## Development

### Adding New Format Support

To add a new format (e.g., HDF5):

1. Add conversion functions to `stards_translate.cpp`:
   ```cpp
   void star_to_hdf5(const std::string& input, const std::string& output);
   void hdf5_to_star(const std::string& input, const std::string& output);
   ```

2. Update format detection:
   ```cpp
   if (ext == "h5" || ext == "hdf5") return "hdf5";
   ```

3. Add to main conversion logic:
   ```cpp
   else if (input_format == "star" && output_format == "hdf5") {
       star_to_hdf5(input_file, output_file);
   }
   ```

4. Update CMakeLists.txt for new dependencies
5. Add tests to validation script
6. Update documentation

### Debugging

The tools set the library log level to `ERROR` by default. To see more detail,
lower the level in code with `logger::set_log_level(logger::STARDS_DEBUG)` (C++) or
`set_log_level` (Python) before running the conversion.

Check conversion output:

```bash
# View JSON output
cat data.json | jq '.arrays | keys'

# Compare file sizes
ls -lh data.stards data.json
```

Validate JSON structure:

```bash
# Using Python
python3 -m json.tool data.json > /dev/null && echo "Valid JSON"

# Using jq
jq empty data.json && echo "Valid JSON"
```

---

## File Organization

```
bin/
├── CMakeLists.txt                  # Build configuration
├── README.md                       # This file
│
├── stardsls.cpp                       # STAR inspector tool
└── stards_translate.cpp               # Format converter tool
```

---

## Troubleshooting

### "stards_translate not found"

**Problem**: Tool not in PATH after installation

**Solution**:
```bash
# Check installation location
find build/install -name stards_translate

# Add to PATH or use full path
export PATH="$PATH:$(pwd)/build/install/bin"
```

### "nlohmann/json.hpp not found"

**Problem**: Submodule not initialized

**Solution**:
```bash
git submodule update --init --recursive
```

### "MessagePack support disabled"

**Problem**: msgpack-c not found

**Solution** (optional):
```bash
# Install msgpack-c
# Ubuntu/Debian:
sudo apt-get install libmsgpack-dev

# macOS:
brew install msgpack

# Conda:
conda install -c conda-forge msgpack-cxx

# Rebuild
cd build && cmake .. && make stards_translate
```

### Build Errors

**Check compiler version**:
```bash
c++ --version  # Should be GCC 7+, Clang 5+, or MSVC 2017+
```

**Check CMake version**:
```bash
cmake --version  # Should be 3.10+
```

**Clean rebuild**:
```bash
rm -rf build
mkdir build && cd build
cmake .. && make
```

---

## Performance Tips

### Large File Conversion

For large files, use the CLI's `-c`/`--compression` and `-b`/`--block-size` flags
to control the output codec and block size (see the
[Compression guide](../docs/guides/compression.md) for the available codecs).

### Batch Conversion

Convert multiple files:

```bash
# STAR to JSON
for file in *.stards; do
    stards_translate "$file" "${file%.stards}.json"
done

# JSON to STAR
for file in *.json; do
    stards_translate "$file" "${file%.json}.stards"
done
```

---

## Contributing

When adding new features to tools:

1. Update this README
2. Add tests to validation scripts
3. Update main project README.md
4. Add usage examples
5. Update TRANSLATE_TOOL_SUMMARY.md if applicable

---

## License

Same as parent project (StarDS).

**Last Updated**: 2025-03-25
