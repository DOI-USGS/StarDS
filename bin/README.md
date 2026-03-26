# STAR Command-Line Tools

This directory contains command-line tools for working with STAR files.

## Tools

### 1. starls
**Purpose**: Inspect and list contents of STAR files

**Usage**:
```bash
# List all keys
starls data.star

# Verbose output with metadata
starls -v data.star

# Print specific key data
starls -d array_name data.star

# Print all data
starls -a data.star
```

**Build**: Always built when `CAMERASTATEFILE_BUILD_TOOLS=ON` (default)

---

### 2. star_translate
**Purpose**: Convert between SCS and other formats (JSON, MessagePack)

**Usage**:
```bash
# SCS to JSON
star_translate data.star data.json

# JSON to SCS
star_translate data.json data.star

# With compression
star_translate -c gzip data.json data.star

# With custom block size
star_translate -c gzip -b 4096 data.json data.star

# MessagePack (if built with msgpack support)
star_translate data.star data.msgpack
```

**Build**: Controlled by `CAMERASTATEFILE_BUILD_TRANSLATE=ON` (default)

**Dependencies**:
- Required: nlohmann/json (included as submodule)
- Optional: msgpack-c (for MessagePack support)

---

## Building

### Build All Tools

```bash
mkdir build && cd build
cmake .. \
  -DCAMERASTATEFILE_BUILD_TOOLS=ON \
  -DCAMERASTATEFILE_BUILD_TRANSLATE=ON
make
```

### Build Specific Tool

```bash
# Build only starls
make starls

# Build only star_translate
make star_translate
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
- SCS → JSON conversion
- JSON → SCS round-trip
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
   - Validates SCS → JSON → SCS round-trip
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

1. Add conversion functions to `star_translate.cpp`:
   ```cpp
   void scs_to_hdf5(const std::string& input, const std::string& output);
   void hdf5_to_scs(const std::string& input, const std::string& output);
   ```

2. Update format detection:
   ```cpp
   if (ext == "h5" || ext == "hdf5") return "hdf5";
   ```

3. Add to main conversion logic:
   ```cpp
   else if (input_format == "scs" && output_format == "hdf5") {
       scs_to_hdf5(input_file, output_file);
   }
   ```

4. Update CMakeLists.txt for new dependencies
5. Add tests to validation script
6. Update documentation

### Debugging

Enable verbose output:

```bash
# For star_translate
export CAMERASTATEFILE_LOG_LEVEL=DEBUG
./bin/star_translate data.star data.json
```

Check conversion output:

```bash
# View JSON output
cat data.json | jq '.arrays | keys'

# Compare file sizes
ls -lh data.star data.json
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
├── starls.cpp                       # SCS inspector tool
├── star_translate.cpp               # Format converter tool
│
├── example_json_conversion.cpp     # Example program
├── test_translate_roundtrip.cpp    # Integration test
│
├── quick_test_translate.sh         # Quick smoke test
├── validate_star_translate.sh       # Full validation suite
├── test_translate.sh               # Manual test helper
│
└── TRANSLATE_TOOL_SUMMARY.md       # Detailed documentation
```

---

## Troubleshooting

### "star_translate not found"

**Problem**: Tool not in PATH after installation

**Solution**:
```bash
# Check installation location
find build/install -name star_translate

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
cd build && cmake .. && make star_translate
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

For files > 1GB:

```bash
# Use larger block size for better compression ratio
star_translate -c gzip -b 65536 large.json large.star

# Or no compression for faster conversion
star_translate -c none large.json large.star
```

### Batch Conversion

Convert multiple files:

```bash
# SCS to JSON
for file in *.star; do
    star_translate "$file" "${file%.star}.json"
done

# JSON to SCS with compression
for file in *.json; do
    star_translate -c gzip "$file" "${file%.json}.star"
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

Same as parent project (CameraStateFile).

**Last Updated**: 2025-03-25
