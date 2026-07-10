# Quick Start

Get started with StarDS in 5 minutes. This guide uses the Python bindings; see
[Installation](installation.md) to build them.

## Your first dataset

```python
import numpy as np
from pystards import StarDataset

# 1. Create dataset
ds = StarDataset.create("mydata.stards")

# 2. Store arrays
ds["temperatures"] = np.array([20.5, 21.3, 19.8, 22.1])
ds["measurements"] = np.random.rand(100, 50)

# 3. Add metadata
ds.meta["sensor_id"] = "TEMP-001"
ds.meta["location"] = "Lab A"
ds.meta["date"] = "2024-04-21"

# 4. Save
ds.flush()
ds.close()

print("✓ Dataset created!")
```

## Reading data

```python
# Open for reading
ds = StarDataset.open("mydata.stards", mode="r")

# Access arrays
temps = ds["temperatures"]
print(f"Temperatures: {temps}")

# Access metadata
sensor = ds.meta["sensor_id"]
print(f"Sensor: {sensor}")

# List all arrays
for key in ds:
    print(f"  - {key}: shape {ds[key].shape}")

ds.close()
```

## Common patterns

### Context manager (recommended)

```python
with StarDataset.create("data.stards") as ds:
    ds["data"] = np.arange(1000)
    ds.meta["note"] = "Auto-flushed on exit"
# File automatically closed and flushed
```

### Array slicing

```python
# Store large array
with StarDataset.create("large.stards") as ds:
    ds["big_matrix"] = np.random.rand(10000, 10000)

# Read only a slice (efficient!)
with StarDataset.open("large.stards", mode="r") as ds:
    subset = ds.get_slice("big_matrix", [(0, 100), (0, 100)])
    print(subset.shape)  # (100, 100)
```

See the [Slicing guide](../guides/slicing.md) for more.

### Cloud storage (S3)

```python
import os
os.environ["AWS_PROFILE"] = "my-profile"

# Read from S3
with StarDataset.open("/vsis3/my-bucket/data.stards", mode="r") as ds:
    data = ds["array_name"]

# Write to S3
with StarDataset.create("/vsis3/my-bucket/output.stards") as ds:
    ds["results"] = processed_data
```

See the [Cloud Storage guide](../guides/cloud-storage.md) for authentication details.

## Array vs. metadata: when to use which?

| Storage | Use for | Example |
|---------|---------|---------|
| `ds["key"]` | Large arrays, numerical data, anything you'll slice | Image data, sensor readings, matrices |
| `ds.meta["key"]` | Small data, scalars, strings, configuration | IDs, dates, labels, parameters |

Arrays and metadata use **separate namespaces**, so the same key can live in
both:

```python
ds["matrix"] = np.random.rand(100, 100)   # store array
ds.meta["matrix"] = "Covariance matrix"   # store metadata about it

print(ds["matrix"].shape)  # (100, 100)
print(ds.meta["matrix"])   # "Covariance matrix"
```

See [Concepts](concepts.md) for the full model.

## Layers — multiple versions of data

Store multiple versions of the same data in separate layers, with **opt-in
inheritance** from the base:

```python
ds = StarDataset.create("data.stards")
ds["image"] = raw_image
ds["metadata"] = calibration_data
ds["wavelengths"] = [400, 500, 600]

# Processed version in a layer
processed = ds.create_layer("processed")
processed["image"] = filtered_image  # override: different data, same key!
# "metadata" and "wavelengths" NOT set → inherited from base if inheritance is on

ds.flush()

# Inheritance is OFF by default — enable it to fall back to base keys
ds2 = StarDataset.open("data.stards")
ds2.set_layer_inheritance(True)
proc = ds2.get_layer("processed")
proc_img = proc["image"]          # processed (overridden)
proc_meta = proc["metadata"]      # inherited from base!
proc_waves = proc["wavelengths"]  # inherited from base!
```

See the [Layers guide](../guides/layers.md) for a full walkthrough.

## Error handling

Raw scalars and strings must go in metadata; the array namespace requires arrays:

```python
ds["count"] = 5           # ✗ Error: raw scalar
ds.meta["count"] = 5      # ✓ OK
ds["count"] = [5]         # ✓ OK (wrapped in array)
ds["count"] = np.array(5) # ✓ OK (0-d array)
```

## Next steps

- Read the [Python API reference](../python-api/index.md).
- Explore the [Guides](../guides/layers.md) for compression, cloud storage, slicing, and threading.
- Browse the [CLI tools](../cli/starls.md) for inspecting and converting files.
