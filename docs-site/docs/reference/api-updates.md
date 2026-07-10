# API Updates — Namespace Separation & Layer Isolation

This page documents two features that shape the StarDS data model: **namespace
separation** (arrays vs. metadata) and **layer isolation** (multi-version data
storage). For the conceptual overview see [Concepts](../getting-started/concepts.md);
for a full layers walkthrough see the [Layers guide](../guides/layers.md).

## 1. Namespace separation (arrays vs. metadata)

### Problem solved

Previously, arrays and metadata shared the same key namespace, making it
impossible to store both an array and metadata about that array using the same
key name.

### Solution

Arrays and metadata now have **separate namespaces**. You can use the same key in
both without conflicts.

=== "Python"

    ```python
    import numpy as np
    from pystards import StarDataset

    ds = StarDataset.create("data.stards")
    ds["matrix"] = np.random.rand(100, 100)             # array
    ds.meta["matrix"] = "Covariance matrix from sensor A"  # metadata, same key
    ds.flush()

    ds2 = StarDataset.open("data.stards")
    array_data = ds2["matrix"]        # the 100×100 array
    metadata = ds2.meta["matrix"]     # "Covariance matrix from sensor A"
    ```

=== "C++"

    ```cpp
    #include "stards.h"
    using namespace star;

    auto store = StarDataset::create("data.stards");
    NDArray<double> matrix = NDArray<double>::zeros({100, 100});
    store->put("matrix", std::move(matrix));
    store->meta.put("matrix", NDArray<std::string>({}, {"Covariance matrix"}));
    store->flush();

    auto store2 = StarDataset::open("data.stards");
    auto array_data = store2->get<double>("matrix");
    auto metadata = store2->meta.get("matrix")->as<std::string>();
    ```

**Key points**

- Arrays: `ds["key"]` / `store->put(key, data)`
- Metadata: `ds.meta["key"]` / `store->meta.put(key, data)`
- Same keys allowed in both namespaces.

## 2. Layer isolation (multi-version data storage)

### Problem solved

Previously, storing data in a layer would overwrite base data when using the same
key name — all layers shared the same storage.

### Solution

Each layer now has **isolated storage** with **opt-in inheritance** from the base
layer. Layers can hold their own version of the same logical key without
interfering with other layers. Inheritance is **off by default**; enable it via
`OpenOptions` at open time or `set_layer_inheritance(True)` / `setLayerInheritance(true)`
afterward.

=== "Python"

    ```python
    ds = StarDataset.create("hyperspectral.stards")
    ds["cube"] = raw_cube
    ds["wavelengths"] = np.linspace(400, 2500, 300)

    processed = ds.create_layer("processed")
    processed["cube"] = processed_cube      # different data, same key
    # wavelengths inherited from base

    calibrated = ds.create_layer("calibrated")
    calibrated["cube"] = calibrated_cube
    calibrated["wavelengths"] = adjusted    # override

    ds.flush()

    # Enable inheritance (off by default) so layers fall back to base keys
    ds2 = StarDataset.open("hyperspectral.stards")
    ds2.set_layer_inheritance(True)
    base_cube = ds2["cube"]
    proc_cube = ds2.get_layer("processed")["cube"]     # different!
    proc_waves = ds2.get_layer("processed")["wavelengths"]  # inherited
    cal_waves = ds2.get_layer("calibrated")["wavelengths"]  # overridden
    ```

=== "C++"

    ```cpp
    auto store = StarDataset::create("data.stards");
    store->put("image", base_image);
    store->put("wavelengths", wavelengths);

    auto layer1 = store->create_layer("processed");
    layer1->put("image", processed_image);      // different data
    // wavelengths inherited from base (when inheritance is enabled)

    auto layer2 = store->create_layer("calibrated");
    layer2->put("image", calibrated_image);
    layer2->put("wavelengths", adjusted_wavelengths);  // override

    store->flush();

    // Inheritance is off by default; opt in at open time or afterward
    auto store2 = StarDataset::open("data.stards");
    store2->setLayerInheritance(true);
    ```

### Key features

1. **Isolation** — each layer's data is independent; `layer1["data"] = x` does not
   overwrite `base["data"]`.
2. **Opt-in inheritance** — off by default; when enabled, accessing a key not set
   in a layer falls back to the base layer transparently.
3. **Selective override** — set only the keys that differ; the rest are inherited
   (with inheritance on).
4. **Metadata support** — inheritance applies to metadata as well as arrays.

### Implementation

Layers use prefixed internal keys for storage while keeping the same user-facing
key names. Array keys use a colon separator:

- Base layer: `"data"` → stored as `"data"`
- Layer `processed`: `"data"` → stored as `"__layer_processed__:data"`
- Layer `calibrated`: `"data"` → stored as `"__layer_calibrated__:data"`

Layer *metadata* keys use the same `__layer_<name>__` prefix but without the
colon (e.g. `"__layer_processed__note"`); metadata is additionally tracked
per-layer in the metadata registry. The exact internal key scheme is an
implementation detail — user-facing key names are unchanged.

## Examples

- [`bindings/python/examples/basic_usage.py`](https://code.usgs.gov/astrogeology/stards/-/blob/main/bindings/python/examples/basic_usage.py) — namespace separation demo
- [`bindings/python/examples/layers_example.py`](https://code.usgs.gov/astrogeology/stards/-/blob/main/bindings/python/examples/layers_example.py) — complete layers tutorial
