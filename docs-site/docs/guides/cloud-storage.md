# Cloud Storage

A `.stards` file can live locally or in the cloud. Prefix a path with a VSI-style
scheme and the same API reads (and, for S3, writes) remotely:

- **S3** — `/vsis3/bucket-name/path/to/file.stards` (read and write)
- **HTTP** — `/vsicurl/https://example.com/path/data.stards` (read-only)

These require the library to be built with `STARDS_ENABLE_CURL` (HTTP) and
`STARDS_ENABLE_S3` (S3) — both on by default; see
[Installation](../getting-started/installation.md).

## Python

```python
import os
from pystards import StarDataset

os.environ["AWS_PROFILE"] = "my-profile"

# Read from S3
with StarDataset.open("/vsis3/my-bucket/data.stards", mode="r") as ds:
    data = ds["array_name"]

# Write to S3
with StarDataset.create("/vsis3/my-bucket/output.stards") as ds:
    ds["results"] = processed_data

# Read from HTTP (read-only)
with StarDataset.open("/vsicurl/https://example.com/data.stards", mode="r") as ds:
    data = ds["array_name"]
```

## C++

```cpp
#include "stards.h"
using namespace star;

// Read from S3
auto store = StarDataset::open("/vsis3/my-bucket/data.stards", "r");
auto data = store->get<double>("sensor_data");

// Write to S3
auto output = StarDataset::create("/vsis3/output-bucket/results.stards");
output->put("results", std::move(processed_data));
output->flush();

// Read from HTTP (read-only), then save a local copy
auto http = StarDataset::open("/vsicurl/https://example.com/data.stards", "r");
http->saveTo("/tmp/local-copy.stards");
```

`saveTo()` writes a local copy of a remote (or read-only) dataset — useful when
you've opened an HTTP source you can't write back to.

## S3 authentication

The S3 URL format is `/vsis3/bucket-name/path/to/file.stards`. Credentials are
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

## Custom S3 endpoints (MinIO, S3-compatible services, testing)

By default `/vsis3/` targets AWS at `https://<bucket>.s3.<region>.amazonaws.com`.
To use an S3-compatible service (e.g. MinIO) or a local test server, set these
environment variables (GDAL-compatible names):

```bash
export AWS_S3_ENDPOINT=minio.example.com:9000  # host[:port] to use instead of AWS
export AWS_VIRTUAL_HOSTING=FALSE               # path-style URLs (endpoint/bucket/key)
export AWS_HTTPS=NO                            # use http:// instead of https://
```

- `AWS_S3_ENDPOINT` — replaces the AWS host. Setting it defaults to **path-style**
  addressing (`endpoint/bucket/key`), which most self-hosted servers expect.
- `AWS_VIRTUAL_HOSTING` — `FALSE`/`NO` forces path-style; `TRUE`/`YES` forces
  virtual-hosted (`bucket.endpoint/key`).
- `AWS_HTTPS` — `NO`/`FALSE` selects `http://` (useful for local/plaintext endpoints).

Requests are still signed with AWS Signature V4; the signed host header and
canonical path are derived from the same settings so signatures stay valid. With
none of these set, behavior is identical to standard AWS S3.

## Notes

- HTTP sources are **read-only**. Use `saveTo()` (C++) to persist a local copy.
- A full runnable example (with graceful fallback to a local file when no AWS
  credentials are present) ships at `bindings/python/examples/s3_example.py`.
