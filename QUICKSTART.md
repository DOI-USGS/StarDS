# STARDS Quick Start Guide

Get started with STARDS in 5 minutes.

## Installation

```bash
pip install pystar  # When available on PyPI
# OR build from source (see README.md)
```

## Your First Dataset

```python
import numpy as np
from pystar import StarDataset

# 1. Create dataset
ds = StarDataset.create("mydata.star")

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

## Reading Data

```python
# Open for reading
ds = StarDataset.open("mydata.star", mode="r")

# Access arrays
temps = ds["temperatures"]
print(f"Temperatures: {temps}")

# Access metadata
sensor = ds.meta["sensor_id"]
print(f"Sensor: {sensor}")

# List all arrays
print("\nAll arrays:")
for key in ds:
    print(f"  - {key}: shape {ds[key].shape}")

ds.close()
```

## Common Patterns

### Context Manager (Recommended)

```python
with StarDataset.create("data.star") as ds:
    ds["data"] = np.arange(1000)
    ds.meta["note"] = "Auto-flushed on exit"
# File automatically closed and flushed
```

### Array Slicing

```python
# Store large array
with StarDataset.create("large.star") as ds:
    ds["big_matrix"] = np.random.rand(10000, 10000)

# Read only a slice (efficient!)
with StarDataset.open("large.star", mode="r") as ds:
    subset = ds.get_slice("big_matrix", [(0, 100), (0, 100)])
    print(subset.shape)  # (100, 100)
```

### Cloud Storage (S3)

```python
# Set up AWS credentials first
import os
os.environ["AWS_PROFILE"] = "my-profile"

# Read from S3
with StarDataset.open("/vsis3/my-bucket/data.star", mode="r") as ds:
    data = ds["array_name"]

# Write to S3
with StarDataset.create("/vsis3/my-bucket/output.star") as ds:
    ds["results"] = processed_data
```

## Array vs Metadata: When to Use Which?

| Storage Type | Use For | Example |
|--------------|---------|---------|
| `ds["key"]` | Large arrays, numerical data, data you'll slice | Image data, sensor readings, matrices |
| `ds.meta["key"]` | Small data, scalars, strings, configuration | IDs, dates, labels, parameters |

## Error Handling

```python
# Raw scalars need to go in metadata
ds["count"] = 5           # ✗ Error
ds.meta["count"] = 5      # ✓ OK
ds["count"] = [5]         # ✓ OK (wrapped in array)
ds["count"] = np.array(5) # ✓ OK (0-d array)
```

## Next Steps

- Read the full [Python API documentation](bindings/python/README.md)
- Check out [examples](bindings/python/examples/)
- Learn about [compression and performance](bindings/python/README.md#performance)
- Explore [CLI tools](bin/README.md)
