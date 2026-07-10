---
title: StarDS
template: home.html
hide:
  - navigation
  - toc
---

## What is StarDS?

**StarDS** — *Simple Tensors, Arrays & Rasters* — is a header-only C++ library
(with Python bindings via SWIG) for **persistent N-dimensional arrays**. It gives
you a single `.stards` file that stores typed arrays and metadata together, with
optional compression, cloud storage, layered versioning, and efficient partial
reads.

<div class="grid cards" markdown>

-   :material-cube-outline: **N-dimensional arrays**

    Store `int8`–`int64`, `uint8`–`uint64`, `float32/64`, and string arrays of
    any shape, round-tripping cleanly with NumPy.

-   :material-zip-box-outline: **Built-in compression**

    Choose GZIP, LZ4, or none — block-based so you can seek into large
    arrays without decompressing everything.

-   :material-cloud-outline: **Cloud storage**

    Read and write directly over S3 (`/vsis3/`) and HTTP (`/vsicurl/`) paths.

-   :material-layers-outline: **Layers with optional inheritance**

    Keep multiple versions of the same data — layers override only what changes
    and can inherit the rest from the base (opt-in).

-   :material-file-document-outline: **Specification**

    The complete `.stards` binary format — header, key registry, layer metadata,
    index entries, and data blocks.

    [:octicons-arrow-right-24: Format Specification](reference/format-spec.md)

</div>

## Quick Example

=== "Python"

    ```python
    import numpy as np
    from pystards import StarDataset

    # Create a dataset and store arrays + metadata
    with StarDataset.create("data.stards") as ds:
        ds["matrix"] = np.random.rand(100, 100)   # array namespace
        ds["vector"] = np.arange(1000)
        ds.meta["sensor_id"] = 12345              # metadata namespace
        ds.meta["timestamp"] = "2024-04-21"

    # Read it back
    with StarDataset.open("data.stards", mode="r") as ds:
        matrix = ds["matrix"]
        sensor = ds.meta["sensor_id"]
        for key in ds:
            print(f"{key}: {ds[key].shape}")
    ```

=== "C++"

    ```cpp
    #include "stards.h"
    using namespace star;

    // Create a dataset and store an array + metadata
    auto store = StarDataset::create("data.stards");
    store->put("matrix", NDArray<double>::zeros({100, 100}));
    store->meta.put("timestamp", NDArray<int64_t>({}, {1234567890}));
    store->flush();

    // Read it back
    auto store2 = StarDataset::open("data.stards");
    auto matrix = store2->get<double>("matrix");
    auto ts = store2->meta.get("timestamp")->as<int64_t>();
    ```

## Where to Next

- **[Installation](getting-started/installation.md)** — build from source with CMake.
- **[Quick Start](getting-started/quickstart.md)** — a 5-minute tour in Python.
- **[Concepts](getting-started/concepts.md)** — arrays vs. metadata, namespaces, layers, and the `.stards` model.
- **[Guides](guides/layers.md)** — layers, compression, cloud storage, slicing, and threading.
- **[Python API](python-api/index.md)** / **[C++ API](cpp-api/index.md)** — full reference.

!!! note "Project status"
    StarDS is developed by the [USGS Astrogeology Science Center](https://astrogeology.usgs.gov/)
    and is released into the public domain (CC0 1.0).
