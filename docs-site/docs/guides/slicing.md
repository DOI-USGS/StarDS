# Slicing

Slicing loads only the portion of an array you ask for, so you can work with
arrays far larger than memory without reading the whole file. Because
compression is [block-based](compression.md), StarDS decompresses only the
blocks a slice touches.

## Reading a slice

Pass a list of `(start, stop)` or `(start, stop, step)` tuples — one per
dimension:

```python
import numpy as np
from pystar import StarDataset

# Store a large array
with StarDataset.create("large.star") as ds:
    ds["big_matrix"] = np.random.rand(10000, 10000)

# Read only a 100×100 corner (efficient!)
with StarDataset.open("large.star", mode="r") as ds:
    subset = ds.get_slice("big_matrix", [(0, 100), (0, 100)])
    print(subset.shape)  # (100, 100)
```

### 1D, 2D, and 3D examples

```python
# 1D slice [1000:2000]
subset = store.get_slice("large_array", [(1000, 2000)])
print(subset.shape)  # (1000,)

# 2D slice [10:20, 30:40]
subset_2d = store.get_slice("big_matrix", [(10, 20), (30, 40)])
print(subset_2d.shape)  # (10, 10)

# 3D slice with a step in the first dimension
subset_3d = store.get_slice("3d_data", [(0, 20, 2), (5, 15), (0, 20)])
```

## Checking whether an array is sliceable

Every value in the **array namespace** (`ds["key"] = …` / `store.put(...)`) is
stored as its own array and is sliceable regardless of size. Only values written
to the **metadata namespace** (`ds.meta["key"] = …`) live in the shared
[metadata block](compression.md#the-metadata-block) and **cannot** be sliced.

`is_sliceable()` reports this — check first if you're unsure which namespace a key
came from:

```python
if store.is_sliceable("large_array"):
    subset = store.get_slice("large_array", [(1000, 2000)])
```

So to keep something sliceable, simply store it as an array (the default) rather
than as metadata.

## C++

```cpp
#include "star.h"
using namespace star;

auto store = StarDataset::open("large.star", "r");
// Slice specs are per-dimension {start, stop[, step]}
auto subset = store->get_slice<double>("big_matrix", {{0, 100}, {0, 100}});
```

## Notes and limits

- Slicing currently supports **1D, 2D, and 3D** arrays.
- Arrays are stored in **row-major (C-style)** order.
- A full runnable example ships at `bindings/python/examples/numpy_interop.py`.
