# Concepts

This page explains the data model behind a `.stards` file so the rest of the API
makes sense.

## The `.stards` file

A StarDS dataset is a single `.stards` file that contains:

- a **header** with a magic string, format version, and an index of every entry;
- an **array section** holding (optionally compressed) N-dimensional array data;
- a **metadata block** holding small scalars and strings for fast access;
- optional **per-layer metadata blocks** for layered datasets.

For the exact byte layout, see the [Format Specification](../reference/format-spec.md).

## Arrays

Arrays are typed, N-dimensional, and stored under the **array namespace** via
`ds["key"]` (Python) or `store.put(key, ndarray)` (C++). Supported element types:

- signed integers: `int8`, `int16`, `int32`, `int64`
- unsigned integers: `uint8`, `uint16`, `uint32`, `uint64`
- floating point: `float32`, `float64`
- `string`

Arrays are the right home for anything large or anything you want to
[slice](../guides/slicing.md). In Python they round-trip with NumPy; the array
namespace requires an actual array (a raw Python scalar raises an error — wrap it
as `np.array(5)` or `[5]`).

## Metadata

Metadata lives in a separate **metadata namespace** via `ds.meta["key"]` (Python)
or `store.meta.put(key, value)` (C++). It's intended for scalars, short strings,
and configuration — small values grouped into a compressed metadata block for
quick access. Unlike the array namespace, metadata accepts raw Python scalars and
strings directly.

## Namespace separation

Arrays and metadata are **independent namespaces**, so the same key can exist in
both without conflict:

=== "Python"

    ```python
    ds["matrix"] = np.random.rand(100, 100)   # array namespace
    ds.meta["matrix"] = "Covariance matrix"   # metadata namespace

    ds["matrix"].shape   # (100, 100)
    ds.meta["matrix"]    # "Covariance matrix"
    ```

=== "C++"

    ```cpp
    store->put("matrix", NDArray<double>::zeros({100, 100}));
    store->meta.put("matrix", NDArray<std::string>({}, {"Covariance matrix"}));

    auto arr  = store2->get<double>("matrix");
    auto note = store2->meta.get("matrix")->as<std::string>();
    ```

| Namespace | Python | C++ | Best for |
|-----------|--------|-----|----------|
| Array | `ds["key"]` | `store.put(key, arr)` | Large / numeric / sliceable data |
| Metadata | `ds.meta["key"]` | `store.meta.put(key, v)` | Scalars, strings, config |

## Layers

**Layers** store multiple versions of the same logical data in one file. Each
layer has **isolated storage** and **opt-in inheritance** from the base:

- Setting a key in a layer overrides it **for that layer only** — it never
  overwrites the base or other layers.
- Accessing a key that a layer did *not* set **falls back to the base layer**
  only when inheritance is enabled. Inheritance is **off by default**; without
  it, a missing key raises. Enable it with `ds.set_layer_inheritance(True)` (or
  at open time via `OpenOptions`).

With inheritance on you only store what changes in each layer; everything else
is inherited. Layers apply to both arrays and metadata.

```python
ds["cube"] = raw_cube
ds["wavelengths"] = np.linspace(400, 2500, 300)

processed = ds.create_layer("processed")
processed["cube"] = corrected_cube   # override
# "wavelengths" not set → inherited from base when inheritance is on

ds.set_layer_inheritance(True)       # off by default
layer = ds.get_layer("processed")
layer["cube"]         # processed data
layer["wavelengths"]  # inherited base wavelengths
```

Common uses: hyperspectral pipelines (raw → corrected → subset), multi-temporal
datasets, A/B experiments, and dataset version control. See the
[Layers guide](../guides/layers.md).

## Compression

Array and metadata blocks can be compressed independently with **GZIP**,
**LZ4**, or **none**. Compression is block-based, so large arrays can be seeked
into without decompressing the whole thing. Configure it through `StarConfig`;
see the [Compression guide](../guides/compression.md).

## Cloud storage

A `.stards` file can live locally or in the cloud. Use an `s3://` URI (or the
`/vsis3/` prefix) for S3, or an `https://` URL (or `/vsicurl/`) for HTTP, and the
same API reads and (for S3) writes remotely.
See the [Cloud Storage guide](../guides/cloud-storage.md).
