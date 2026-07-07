# C++ API

StarDS is a header-only C++ library. Everything lives in `star.h` under the
`star` namespace. This reference is generated from the in-source Doxygen
comments by [mkdoxy](https://github.com/JakubAndrysek/mkdoxy).

## Browse the reference

- **[Classes](../StarCPPAPI/classes.md)** — alphabetical class index
- **[Class List](../StarCPPAPI/annotated.md)** — annotated class list
- **[Namespaces](../StarCPPAPI/namespaces.md)** — namespace members
- **[Files](../StarCPPAPI/files.md)** — file-level documentation

## Core public types

| Type | Role |
|------|------|
| `StarDataset` | Main persistent store — `create`/`open`, `put`/`get`, `flush`, layers, slicing |
| `NDArray<T>` | Template N-dimensional array with factory helpers (`zeros`, `ones`, `full`, `arange`) |
| `LayerView` | Per-layer access with inheritance from the base layer |
| `MetadataAccessor` | Metadata namespace operations (`store.meta`) |
| `LayerMetadataAccessor` | Per-layer metadata operations (`layer->meta`) |
| `MetadataValue` | Type-erased metadata value (`as<T>()`, `is_scalar()`, …) |
| `StarConfig` | Compression, block size, and metadata-block configuration |
| `FileHeader` | Parsed file header (magic, version, entry count) |
| `DataType`, `CompressionAlgorithm`, `FileMode` | Enumerations |

Global threading controls (`setNumThreads`, `setMinBlocksForThreading`,
`setMinBytesForThreading`, `getNumThreads`) are documented in the
[Threading guide](../guides/threading.md).

## Minimal example

```cpp
#include "star.h"
using namespace star;

int main() {
    // Create and store an array + metadata
    auto store = StarDataset::create("data.star");
    store->put("matrix", NDArray<double>::zeros({100, 100}));
    store->meta.put("timestamp", NDArray<int64_t>({}, {1234567890}));
    store->flush();

    // Read it back
    auto store2 = StarDataset::open("data.star");
    auto matrix = store2->get<double>("matrix");
    auto ts = store2->meta.get("timestamp")->as<int64_t>();
    return 0;
}
```

!!! note
    The generated pages only include members carrying Doxygen documentation
    comments (`HIDE_UNDOC_MEMBERS`), which keeps internal helpers (S3/HTTP
    streams, the thread pool, key registry) out of the public reference.
