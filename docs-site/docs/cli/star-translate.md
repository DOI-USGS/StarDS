# star_translate

`star_translate` converts between `.star` files and other formats (JSON,
MessagePack, CSV), and can reorganize a file for ISDS (ISIS Dataset)
optimization.

Built when `STAR_BUILD_TRANSLATE=ON` (the default); see
[Installation](../getting-started/installation.md).

## Usage

```bash
star_translate [OPTIONS] <input_file> <output_file>

# STAR ↔ JSON (format auto-detected from the file extensions)
star_translate data.star data.json
star_translate data.json data.star

# CSV to STAR (2D arrays only)
star_translate data.csv data.star

# Force the output format explicitly
star_translate -f json data.star out.json

# MessagePack (if built with msgpack support)
star_translate data.star data.msgpack
```

The output format is auto-detected from the output file's extension
(`.star`, `.json`, `.csv`, `.msgpack`/`.mp`), or you can force it with
`-f`.

## Options

| Flag | Long form | Description |
|------|-----------|-------------|
| `-h` | `--help` | Show the help message |
| `-f <fmt>` | `--format <fmt>` | Output format: `json`, `msgpack`, or `isds` (otherwise auto-detected from the extension) |
| `-c <alg>` | `--compression <alg>` | Compression for STAR output: `none`, `gzip`, `lz4`, `zstd`. Default: `lz4`. **See the note below.** |
| `-b <bytes>` | `--block-size <bytes>` | Block size for STAR compression. Default: `1048576` (1 MB). **See the note below.** |
| `-t <n>` | `--threshold <n>` | ISDS element threshold (default: `100`) |

!!! warning "`-c` and `-b` are currently not applied"
    The tool parses `--compression` and `--block-size` but writes its STAR output
    with the default `StarConfig`, so these flags have no effect on the result.
    To control the codec or block size, use the `StarConfig` API — see the
    [Compression guide](../guides/compression.md).

## JSON format

```json
{
  "arrays": {
    "my_array": {
      "dtype": "float64",
      "shape": [10, 20],
      "data": [0.0, 1.0, "..."]
    }
  }
}
```

## ISDS optimization

The ISDS (ISIS Dataset) mode reorganizes a `.star` file by size: large arrays go
to efficient block storage, while small values are kept in the metadata block for
quick access. This suits ISIS camera-state files that mix large arrays
(quaternions, ephemeris) with small scalars (focal length, detector center).

- Arrays with **more** elements than the threshold → array storage (compressed blocks)
- Arrays with the threshold count **or fewer** → metadata storage
- Default threshold: **100 elements**
- All data is preserved — only its storage location changes

```bash
# Reorganize by array size (default threshold of 100 elements)
star_translate -f isds input.star output.star

# Custom threshold
star_translate -f isds -t 50 input.star output.star
```

## Batch conversion

```bash
# STAR → JSON for every file in a directory
for file in *.star; do
    star_translate "$file" "${file%.star}.json"
done

# JSON → STAR
for file in *.json; do
    star_translate "$file" "${file%.json}.star"
done
```

## See also

- [starls](starls.md) — inspect file contents.
- [Compression guide](../guides/compression.md) — codecs and block sizes.
