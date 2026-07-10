# Layers

Layers let you store **multiple versions of the same data** in one `.stards` file.
Each layer has isolated storage and can optionally inherit anything it doesn't
override from the base layer.

## Key ideas

- **Isolation** — setting a key in a layer never overwrites the base or other
  layers. Each layer keeps its own copy of the keys it defines.
- **Opt-in inheritance** — inheritance is **off by default**. A key a layer did
  not set is reported as missing. Turn inheritance on (see below) to make a
  layer fall back to the base for keys it didn't override.
- **Selective override** — with inheritance on, override just the keys that
  differ; the rest are inherited.
- **Works for metadata too** — layer metadata inherits from base metadata the
  same way when inheritance is enabled.

## Enabling inheritance

Inheritance is a **read-time** setting, controlled by `OpenOptions` (distinct
from `StarConfig`, which configures how a new file is written). Nothing about it
is persisted to the file — it only affects how an open dataset resolves keys.

You can enable it two ways:

=== "Python"

    ```python
    import pystards
    from pystards import StarDataset

    # At open time
    opts = pystards.OpenOptions()
    opts.layer_inheritance = True
    ds = StarDataset.open("data.stards", mode="r", options=opts)

    # ...or after opening (works on any already-open dataset)
    ds = StarDataset.open("data.stards")
    ds.set_layer_inheritance(True)
    print(ds.layer_inheritance())  # True
    ```

=== "C++"

    ```cpp
    // At open time
    OpenOptions opts;
    opts.layer_inheritance = true;
    auto ds = StarDataset::open("data.stards", FileMode::READ_ONLY, opts);

    // ...or after opening
    auto ds2 = StarDataset::open("data.stards");
    ds2->setLayerInheritance(true);
    bool on = ds2->layerInheritance();  // true
    ```

With inheritance **off** (the default), a layer lookup for a base-only key raises
(Python `KeyError` / C++ `std::runtime_error`) instead of returning the base
value. The examples below assume inheritance has been enabled.

## Python

```python
import numpy as np
from pystards import StarDataset

# Base data
ds = StarDataset.create("data.stards")
ds["image"] = raw_image
ds["metadata"] = calibration_data
ds["wavelengths"] = [400, 500, 600]

# Processed version in a layer
processed = ds.create_layer("processed")
processed["image"] = filtered_image  # override: different data, same key!
# "metadata" and "wavelengths" NOT set → inherited from base

# Another version
enhanced = ds.create_layer("enhanced")
enhanced["image"] = enhanced_image
enhanced["wavelengths"] = [450, 550, 650]  # override wavelengths
# "metadata" inherited from base

ds.flush()

# Read back — enable inheritance so layers fall back to base keys
ds2 = StarDataset.open("data.stards")
ds2.set_layer_inheritance(True)
base_img = ds2["image"]                     # raw

proc = ds2.get_layer("processed")
proc_img = proc["image"]                    # processed (overridden)
proc_meta = proc["metadata"]                # inherited from base!
proc_waves = proc["wavelengths"]            # inherited from base!

enh = ds2.get_layer("enhanced")
enh_img = enh["image"]                      # enhanced (overridden)
enh_waves = enh["wavelengths"]              # overridden
enh_meta = enh["metadata"]                  # inherited from base!
```

Layers also carry metadata with inheritance:

```python
ds.meta["config"] = "default config"

layer = ds.create_layer("experiment")
layer.meta["experiment_id"] = "exp_001"     # layer-specific
config = layer.meta["config"]               # inherited: "default config"
```

## C++

```cpp
#include "stards.h"
using namespace star;

// Create dataset with base data
auto store = StarDataset::create("data.stards");
store->put("image", base_image);
store->put("wavelengths", wavelengths);

// Create a layer with a different version
auto layer1 = store->create_layer("processed");
layer1->put("image", processed_image);   // different data
// wavelengths inherited from base

// Another layer
auto layer2 = store->create_layer("calibrated");
layer2->put("image", calibrated_image);
layer2->put("wavelengths", adjusted_wavelengths);  // override

store->flush();

// Read back — enable inheritance so layers fall back to base keys
auto store2 = StarDataset::open("data.stards");
store2->setLayerInheritance(true);
auto base_img = store2->get<double>("image");
auto proc_img = store2->get_layer("processed")->get<double>("image");
auto cal_img  = store2->get_layer("calibrated")->get<double>("image");

// Layers have metadata with inheritance too
layer1->meta.put("description", NDArray<std::string>({}, {"Processed version"}));
```

## Worked example: hyperspectral pipeline

A full runnable example ships in the repository at
`bindings/python/examples/layers_example.py`. It builds a raw hyperspectral cube,
then adds two layers:

```python
import numpy as np
from pystards import StarDataset

ds = StarDataset.create("hyperspectral.stards")

# Base: raw data
raw_cube = np.random.rand(512, 512, 300).astype(np.float32)
wavelengths = np.linspace(400, 2500, 300)
ds["cube"] = raw_cube
ds["wavelengths"] = wavelengths
ds.meta["description"] = "Raw hyperspectral cube"
ds.meta["instrument"] = "AVIRIS-NG"

# Layer 1: atmospherically corrected (wavelengths inherited)
atm = ds.create_layer("atm_corrected")
atm["cube"] = raw_cube + 0.05
atm.meta["description"] = "Atmospherically corrected"

# Layer 2: VNIR spectral subset (wavelengths overridden)
vnir_mask = wavelengths < 1000
vnir = ds.create_layer("vnir_only")
vnir["cube"] = raw_cube[:, :, vnir_mask]
vnir["wavelengths"] = wavelengths[vnir_mask]

ds.flush()
```

On read-back (with inheritance enabled via `set_layer_inheritance(True)` or
`OpenOptions`), `atm_corrected["wavelengths"]` returns the full 300-band base
wavelengths (inherited), while `vnir_only["wavelengths"]` returns the overridden
subset. Without inheritance, `atm_corrected["wavelengths"]` would raise, since
that layer never set its own `wavelengths`.

## When to use layers

- **Hyperspectral imaging** — raw, atmospherically corrected, and subset versions
- **Multi-temporal datasets** — different time slices
- **A/B testing** — variant versions for experiments
- **Version control** — track dataset evolution
- **Processing pipelines** — raw → intermediate → final

## Implementation note

Internally, layers use prefixed keys so each layer's storage is independent while
the user-facing API keeps the same logical key names:

- Base layer: `"data"` → stored as `"data"`
- Layer `processed`: `"data"` → stored as `"__layer_processed__:data"`
- Layer `calibrated`: `"data"` → stored as `"__layer_calibrated__:data"`
