# Quick Start

Get started with StarDS in 5 minutes. This guide uses the Python bindings; see
[Installation](installation.md) to build them.

## Your first dataset

=== "Python"

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

=== "JS"

    ```js
    import { loadStarDS } from './stards.mjs';
    const { Dataset, NDArray, openBytes } = await loadStarDS();
    
    // 1. Create dataset
    const ds = await new Dataset('data.stards', 'w');

    // 2. Store arrays
    ds.put('temperatures', NDArray(new Float64Array([20.5, 21.3, 19.8, 22.1])));
    ds.put('measurements', NDArray.zeros([100, 50], 'float64'));

    // 3. Add metadata
    ds.metaPut('sensor_id', 'TEMP-001');
    ds.metaPut('location', 'Lab A');
    ds.metaPut('date', '2024-04-21');

    // 4. Save
    const dataSetBytes = ds.writeBytes();
    ds.close();

    console.info("✓ Dataset created!");
    ```

=== "C++"

    ```cpp
    #include "stards.h"
    #include <stdio.h>
    using namespace star;

    void main() {
         // 1. Create Dataset
        auto ds = StarDataset::create("data.stards");

        // 2. Store Arrays
        ds->put("temperatures", NDArray<double>(std::vector<double>{20.4, 21.3, 19.8, 22.1}, {4}));
        ds->put("measurements", NDArray<double>::zeros({100, 50}));

        // 3. Add Metadata
        ds->meta.put("sensor_id", NDArray<std::string>({}, "TEMP-001"));
        ds->meta.put("location", NDArray<std::string>({}, "Lab A"));
        ds->meta.put("date", NDArray<std::string>({}, "2024-04-21"));

        // 4. Save
        ds->flush();
        ds->close();

        printf("✓ Dataset created! \n");
    }
    ```

## Reading data

=== "Python"

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

=== "JS"

    ```js
    // Open Dataset from Byte object
    const ds = await openBytes(dataSetBytes);

    // Access Arrays
    const temps = ds.getArray('temperatures');
    console.info('temperatures', temps.data());

    // Access Metadata
    const sensor = ds.metaGet('sensor_id');
    console.info('sensor_id', sensor);

    // List all arrays
    ds.keys().forEach((key, idx, arr) => {
        console.info(key + ": shape:", ds.shape(key));
    });
    ```

=== "C++"

    ```cpp
    // Open for reading
    auto ds = StarDataset::open("data.stards", "r");

    // Access Arrays
    auto temps = ds->get<double>("temperatures");
    for(int i = 0; i < temps.size(); i++) {
        printf("Temp %d: %f\n", i, temps(i));
    }

    // Access Metadata
    auto sensor = ds->meta.get("sensor_id")->as<std::string>().flat(0);
    printf("Sensor ID: %s\n", sensor.c_str());

    ds->close();
    ```

## Common patterns

### Context manager (recommended)

=== "Python"

    ```python
    with StarDataset.create("data.stards") as ds:
        ds["data"] = np.arange(1000)
        ds.meta["note"] = "Auto-flushed on exit"
    # File automatically closed and flushed
    ```

### Array slicing

=== "Python"

    ```python
    # Store large array
    with StarDataset.create("large.stards") as ds:
        ds["big_matrix"] = np.random.rand(10000, 10000)

    # Read only a slice (efficient!)
    with StarDataset.open("large.stards", mode="r") as ds:
        subset = ds.get_slice("big_matrix", [(0, 100), (0, 100)])
        print(subset.shape)  # (100, 100)
    ```

=== "JS"

    ```js

    const ds = await new Dataset('data.stards', 'r');

    // Read a 1-D Slice (use getSlice for convenience)
    console.info('Slice', await ds.getSlice('m.sample', 0, 6));
    
    // Read a 2-D Slice:
    // from [32, 64] to [48, 96],
    // at 1/4 resolution
    // (step 4 gets every 4th value, step 1 would gets full resolution).
    console.info('2D Slice', await rectReader.getSliceND('big-matrix', [[32, 48, 4],[64, 96, 4]]));

    // use yourdataset.getSliceNDArray() to get an NDArray instead of a JS Object.
    ```

=== "C++"

    ```cpp
    // this file should have "big_matrix" that you are slicing from
    auto ds = StarDataset::open("data.stards", "r");

    // Slice specs are per-dimension {start, stop[, step]}
    // subset will be is a 100x100 NDArray
    auto subset = ds->get_slice<double>("big_matrix", {{0, 100}, {0, 100}});
    
    ```

See the [Slicing guide](../guides/slicing.md) for more.

### Cloud Storage & S3

=== "Python"

    ```python
    import os
    os.environ["AWS_PROFILE"] = "my-profile"

    # Read from S3 (s3:// URI or the /vsis3/ prefix)
    with StarDataset.open("s3://my-bucket/data.stards", mode="r") as ds:
        data = ds["array_name"]

    # Write to S3
    with StarDataset.create("s3://my-bucket/output.stards") as ds:
        ds["results"] = processed_data
    ```

=== "JS"

    ```js title="Reading Public URLs"
    const URL = 'https://asc-isisdata.s3.amazonaws.com/cnf_test_data/largenet.stards';
    const remoteReader = await new Dataset(URL);
    console.info('Shape of remote array', remoteReader.shape('m.sample'));
    ```

    For reading or writing to an S3 bucket, configure your AWS Credentials in  
    `loadStarDS({ env: { AWS_... } })`:

    ```js title="S3 Read & Write"
    import { loadStarDS } from './stards.mjs';

    const { Dataset } = await loadStarDS({
    env: {
        AWS_ACCESS_KEY_ID: '...',
        AWS_SECRET_ACCESS_KEY: '...',
        AWS_SESSION_TOKEN: '...',        // only for STS/temporary creds
        AWS_DEFAULT_REGION: 'us-west-2',

        // Optional — for S3-compatible stores (MinIO, R2, etc.):
        // ENV.AWS_S3_ENDPOINT     = 'localhost:9000';
        // ENV.AWS_VIRTUAL_HOSTING = 'FALSE';       // path-style: endpoint/bucket/key
        // ENV.AWS_HTTPS           = 'NO';          // http:// instead of https://…
    },
    });

    const ds = await new Dataset('s3://your-s3-bucket/path/yourfile.stards', 'w');
    ds.put('elevation', new Float32Array([1,2,3,4,5,6]));
    await ds.flush();
    ds.close();
    ds.delete();
    ```

See the [Cloud Storage guide](../guides/cloud-storage.md) for authentication details.

## Array vs. metadata: when to use which?

| Storage | Use for | Example |
|---------|---------|---------|
| `ds["key"]` | Large arrays, numerical data, anything you'll slice | Image data, sensor readings, matrices |
| `ds.meta["key"]` | Small data, scalars, strings, configuration | IDs, dates, labels, parameters |

Arrays and metadata use **separate namespaces**, so the same key can live in
both:

=== "Python"

    ```python
    ds["matrix"] = np.random.rand(100, 100)   # store array
    ds.meta["matrix"] = "Covariance matrix"   # store metadata about it

    print(ds["matrix"].shape)  # (100, 100)
    print(ds.meta["matrix"])   # "Covariance matrix"
    ```

=== "JS"

    ```js
    const ds = await new Dataset("data.stards", 'w');
    ds.put('matrix', NDArray.zeros([100, 100], 'float64')); // array
    ds.metaPut('matrix', 'Covariance Matrix');              // metadata about the array

    console.info(ds.shape('matrix'));   //  Shape of array
    console.info(ds.metaGet('matrix')); // "Covariance Matrix"
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
- Browse the [CLI tools](../cli/stardsls.md) for inspecting and converting files.
