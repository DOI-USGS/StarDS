# Cloud Storage

A `.star` file can live locally or in the cloud. Prefix a path with a VSI-style
scheme and the same API reads (and, for S3, writes) remotely:

- **S3** — `/vsis3/bucket-name/path/to/file.star` (read and write)
- **HTTP** — `/vsicurl/https://example.com/path/data.star` (read-only)

These require the library to be built with `STAR_ENABLE_CURL` (HTTP) and
`STAR_ENABLE_S3` (S3) — both on by default; see
[Installation](../getting-started/installation.md).

## Python

```python
import os
from pystar import StarDataset

os.environ["AWS_PROFILE"] = "my-profile"

# Read from S3
with StarDataset.open("/vsis3/my-bucket/data.star", mode="r") as ds:
    data = ds["array_name"]

# Write to S3
with StarDataset.create("/vsis3/my-bucket/output.star") as ds:
    ds["results"] = processed_data

# Read from HTTP (read-only)
with StarDataset.open("/vsicurl/https://example.com/data.star", mode="r") as ds:
    data = ds["array_name"]
```

## C++

```cpp
#include "star.h"
using namespace star;

// Read from S3
auto store = StarDataset::open("/vsis3/my-bucket/data.star", "r");
auto data = store->get<double>("sensor_data");

// Write to S3
auto output = StarDataset::create("/vsis3/output-bucket/results.star");
output->put("results", std::move(processed_data));
output->flush();

// Read from HTTP (read-only), then save a local copy
auto http = StarDataset::open("/vsicurl/https://example.com/data.star", "r");
http->saveTo("/tmp/local-copy.star");
```

`saveTo()` writes a local copy of a remote (or read-only) dataset — useful when
you've opened an HTTP source you can't write back to.

## S3 authentication

The S3 URL format is `/vsis3/bucket-name/path/to/file.star`. Credentials are
resolved from the standard AWS sources — choose whichever fits your environment:

**Environment variables**

```bash
export AWS_ACCESS_KEY_ID=KEY
export AWS_SECRET_ACCESS_KEY=SECRET
export AWS_DEFAULT_REGION=us-east-1
```

**AWS SSO**

```bash
aws sso login --profile my-profile
export AWS_PROFILE=my-profile
```

**Credentials file** (`~/.aws/credentials`)

```ini
[default]
aws_access_key_id = KEY
aws_secret_access_key = SECRET
```

## Notes

- HTTP sources are **read-only**. Use `saveTo()` (C++) to persist a local copy.
- A full runnable example (with graceful fallback to a local file when no AWS
  credentials are present) ships at `bindings/python/examples/s3_example.py`.
