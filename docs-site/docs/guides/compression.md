# Compression

StarDS compresses array and metadata blocks independently. Compression is
**block-based**, so large arrays can be seeked into (and
[sliced](slicing.md)) without decompressing the whole array.

## Algorithms

| Algorithm | Speed | Ratio | Notes |
|-----------|-------|-------|-------|
| `CompressionAlgorithm.NONE` | Fastest | — | Largest files; best for temporary or pre-compressed data |
| `CompressionAlgorithm.LZ4` | Very fast | Modest | ~3× faster than GZIP; great for fast writes |
| `CompressionAlgorithm.GZIP` | Balanced | Good | Default; widely compatible (zlib / RFC 1952) |

The native codec must be enabled at build time (`STAR_ENABLE_ZLIB` for GZIP,
`STAR_ENABLE_LZ4` for LZ4) — both on by default; see
[Installation](../getting-started/installation.md).

!!! note "ZSTD is reserved but not implemented"
    `CompressionAlgorithm.ZSTD` exists in the enum for forward compatibility, but
    the current reference library only reads and writes `NONE`, `GZIP`, and `LZ4`.
    Selecting `ZSTD` raises an "unsupported compression algorithm" error.

## Configuring compression (`StarConfig`)

Pass a `StarConfig` to `StarDataset.create()` to control the codec and block
size:

```python
from pystar import StarDataset, StarConfig, CompressionAlgorithm
import numpy as np

# High compression for archival storage
config = StarConfig()
config.compression = CompressionAlgorithm.GZIP
config.block_size = 512 * 1024               # 512 KB blocks
config.metadata_compression = CompressionAlgorithm.GZIP

store = StarDataset.create("/tmp/archive.star", config)
store["data"] = np.random.rand(1000, 1000)
store.flush()
```

```python
# Fast writes with LZ4
config = StarConfig()
config.compression = CompressionAlgorithm.LZ4
config.block_size = 2 * 1024 * 1024          # 2 MB blocks

store = StarDataset.create("/tmp/fast.star", config)
store["data"] = np.random.rand(1000, 1000)
store.flush()
```

```python
# No compression (maximum speed)
config = StarConfig()
config.compression = CompressionAlgorithm.NONE
config.block_size = 4 * 1024 * 1024          # 4 MB blocks

store = StarDataset.create("/tmp/uncompressed.star", config)
store["data"] = np.random.rand(1000, 1000)
store.flush()
```

### `StarConfig` parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `compression` | `CompressionAlgorithm` | `GZIP` | Main data compression algorithm |
| `block_size` | int (bytes) | `1048576` (1 MB) | Compression block size |
| `metadata_max_block_size` | int (bytes) | `65536` (64 KB) | Maximum size of a layer's metadata block |
| `metadata_compression` | `CompressionAlgorithm` | `GZIP` | Metadata block compression |
| `arena_chunk_size` | int (bytes) | `1048576` (1 MB) | Memory arena allocation size |

## Choosing a block size

- **256 KB – 512 KB** — better compression ratio, slower random access
- **1 MB – 2 MB** — balanced (recommended)
- **4 MB – 8 MB** — faster sequential I/O, slightly less compression

## The metadata block

StarDS keeps two distinct namespaces (see [Concepts](../getting-started/concepts.md)):

- **Arrays** — everything written via `store["key"] = …` / `store.put(...)` is
  **always stored as its own separately-compressed array**, regardless of size.
  These arrays support [slicing](slicing.md).
- **Metadata** — everything written via `store.meta["key"] = …` is packed into a
  single compressed **metadata block** per layer. Metadata values are read as a
  unit (the whole block is decompressed) and **cannot be sliced**.

So the choice of where a value lives is made by *which namespace you use*, not by
its size. Put large or sliceable data in the array namespace; put scalars, short
strings, and config in the metadata namespace.

`metadata_compression` sets the codec for the metadata block, and
`metadata_max_block_size` caps its total size:

```python
# Larger metadata block for datasets with many metadata entries
config = StarConfig()
config.metadata_max_block_size = 256 * 1024   # 256 KB (default is 64 KB)
config.metadata_compression = CompressionAlgorithm.GZIP

store = StarDataset.create("/tmp/rich_metadata.star", config)
for i in range(100):
    store.meta[f"note_{i}"] = f"observation {i}"
store.flush()
```

## Choosing the codec in code

Compression for a `.star` file is set through `StarConfig` when the dataset is
created (see the examples above). To re-encode an existing file with a different
codec or block size, open it with the desired `StarConfig` and copy its contents
into a newly created dataset.

!!! warning "`star_translate -c` / `-b` are not applied"
    The [`star_translate`](../cli/star-translate.md) CLI accepts `-c`/`--compression`
    and `-b`/`--block-size` flags, but the current tool creates its STAR output
    with the **default** `StarConfig` and does not apply these values. Use the
    `StarConfig` API above to control the codec and block size.

## Tips

- Flush after **batches** of writes, not after every `put()`.
- Use **LZ4** for fast, write-heavy workflows and **GZIP** for the best ratio.
- Use **NONE** for temporary files or data that is already compressed.
- Prefer the **smallest dtype** that fits your data before compressing.
