meta:
  id: stards
  title: StarDS (Simple Tensors Arrays and Rasters) container, on-disk format v1
  file-extension: stards
  endian: le
  encoding: UTF-8
  # This Kaitai Struct spec describes the StarDS **format version 1** layout
  # ("1.0"). The on-disk format version is a single byte in the fixed header
  # (`file_header.format_version`) and is asserted to be 1 below, so this .ksy is
  # pinned to the 1.x format line. When the format changes incompatibly, copy this
  # file to a new versioned spec (e.g. stards_v2.ksy) and bump the assertion there,
  # rather than editing this one — that keeps each version's structure described in
  # exactly one place.
  #
  # NOTE: the *library* version (currently 1.0.0) is NOT stored in the file; only
  # the one-byte format version is persisted.

doc: |
  Binary layout of a `.stards` file as written/read by the reference
  implementation in `StarDS/include/stards.h`.

  A file is a HEADER SECTION followed by a DATA SECTION:

    * Header section: fixed file header, then the global key registry, the
      per-layer metadata registry, the per-layer presence bitmaps, and one index
      entry per stored array. `file_header.header_size` is the total byte size of
      this section, so a reader can fetch it in one read and parse the rest from
      memory.
    * Data section: the array data blocks (compressed or raw) and the per-layer
      metadata blocks. These are referenced by absolute byte offsets stored in the
      index entries and the layer metadata registry (`*_position` fields), so this
      spec exposes the header structures and leaves the offset-addressed payloads
      to the reader (they overlap/are addressed out of order).

  All multi-byte integers are little-endian; all strings are UTF-8; arrays are
  stored row-major (C order).

seq:
  - id: file_header
    type: file_header
    doc: Fixed 31-byte header (magic, format version, sizes/counts).
  - id: key_registry
    type: key_entry
    repeat: expr
    repeat-expr: file_header.key_registry_count
    doc: Global registry of every unique key name; referenced elsewhere by index.
  - id: layer_registry
    type: layer_meta_entry
    repeat: expr
    repeat-expr: file_header.total_layers
    doc: One entry per layer (base layer `__base__` first, then each named layer).
  - id: layer_presence
    type: presence_bitmap
    repeat: expr
    repeat-expr: file_header.total_layers
    doc: >
      One presence bitmap per layer, same order as layer_registry. Each bitmap has
      ceil(entry_count / 64) 64-bit words; bit i set = array index-entry i is
      present in that layer.
  - id: index_entries
    type: index_entry
    repeat: expr
    repeat-expr: file_header.entry_count
    doc: One descriptor per stored data array (metadata values live in the data-section metadata blocks, not here).

types:
  file_header:
    doc: Fixed-size file header (31 bytes in format v1).
    seq:
      - id: magic
        contents: "STARDS"
        doc: File identifier, the 6 ASCII bytes "STARDS".
      - id: format_version
        type: u1
        valid: 1
        doc: On-disk format version. This spec describes version 1 only.
      - id: header_size
        type: u8
        doc: Total size of the header section (this struct through the last index entry), in bytes.
      - id: entry_count
        type: u8
        doc: Number of stored data arrays (== number of index_entries).
      - id: layer_count
        type: u4
        doc: Number of NAMED layers, excluding the implicit `__base__` layer.
      - id: key_registry_count
        type: u4
        doc: Number of unique keys in the global key registry.
    instances:
      total_layers:
        value: layer_count + 1
        doc: Layer count including the implicit base layer (`__base__`).
      bitmap_words:
        value: (entry_count + 63) / 64
        doc: Number of 64-bit words in each layer presence bitmap.

  key_entry:
    doc: One entry in the global key registry — a key name plus its precomputed hash.
    seq:
      - id: key_len
        type: u2
        doc: Length of the key string in bytes.
      - id: key
        type: str
        size: key_len
        doc: The key name (UTF-8).
      - id: hash
        type: u8
        doc: Precomputed std::hash of the key name, for O(1) lookup.

  layer_meta_entry:
    doc: >
      One layer's metadata-block descriptor: where its metadata block lives in the
      data section, and which registry keys it holds.
    seq:
      - id: name_len
        type: u2
        doc: Length of the layer name in bytes.
      - id: name
        type: str
        size: name_len
        doc: Layer name (`__base__` for the implicit base layer).
      - id: block_position
        type: u8
        doc: Absolute byte offset of this layer's metadata block in the data section.
      - id: block_size
        type: u4
        doc: Size of this layer's metadata block in bytes (0 = no block).
      - id: compression
        type: u1
        enum: compression_algorithm
        doc: Compression algorithm of the metadata block.
      - id: metadata_key_count
        type: u2
        doc: Number of metadata keys belonging to this layer.
      - id: key_indices
        type: u2
        repeat: expr
        repeat-expr: metadata_key_count
        doc: Registry indices (into key_registry) of this layer's metadata keys, ascending.

  presence_bitmap:
    doc: Bit i (LSB-first within each word) marks whether index-entry i exists in this layer.
    seq:
      - id: words
        type: u8
        repeat: expr
        repeat-expr: _root.file_header.bitmap_words

  index_entry:
    doc: >
      Descriptor for one stored data array: a u2 key index into the global
      registry, followed by the serialized array descriptor. The array's bytes
      live in the data section at `position` (see the block table for layout).
    seq:
      - id: key_index
        type: u2
        doc: Index into key_registry identifying this array's key.
      - id: position
        type: u8
        doc: Absolute byte offset of this array's data in the data section.
      - id: total_bytes
        type: u8
        doc: Total stored size of the array data across all blocks.
      - id: datatype
        type: u1
        enum: data_type
        doc: Element type of the array.
      - id: num_dimensions
        type: u8
        doc: Number of dimensions (0 = scalar).
      - id: shape
        type: u8
        repeat: expr
        repeat-expr: num_dimensions
        doc: Size of each dimension (row-major; last dimension varies fastest).
      - id: compression
        type: u1
        enum: compression_algorithm
        doc: Compression algorithm applied to this array's blocks.
      - id: block_size
        type: u8
        doc: Uncompressed size of each block (0 = not blocked).
      - id: num_blocks
        type: u8
        doc: Number of data blocks for this array.
      - id: blocks
        type: block_info
        repeat: expr
        repeat-expr: num_blocks
        doc: Per-block offsets/sizes, enabling random block access.
      - id: stored_in_metadata
        type: u1
        doc: Always 0 for index entries in format v1 (metadata values are not listed here).

  block_info:
    doc: One compressed/raw block of an array's data.
    seq:
      - id: offset
        type: u8
        doc: Byte offset of this block within the array's data (relative to index_entry.position).
      - id: compressed_size
        type: u8
        doc: Stored (compressed) size of the block; equals uncompressed_size when compression is none.
      - id: uncompressed_size
        type: u8
        doc: Size of the block after decompression.

enums:
  data_type:
    0: int8
    1: int16
    2: int32
    3: int64
    4: uint8
    5: uint16
    6: uint32
    7: uint64
    8: float32
    9: float64
    10: string

  compression_algorithm:
    0: none
    1: gzip
    2: zstd       # reserved in the enum; not implemented by the reference library
    3: lz4
    4: gzip_shuffle
    5: lz4_shuffle
