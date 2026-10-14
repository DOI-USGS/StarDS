# Cloud Storage

A `.stards` file can live locally or in the cloud. Point the same API at a remote
path and it reads (and, for S3, writes) remotely. Each remote scheme accepts two
equivalent forms — a plain URL/URI, or the GDAL virtual-filesystem prefix:

- **S3** — `s3://bucket-name/path/to/file.stards` or
  `/vsis3/bucket-name/path/to/file.stards` (read and write)
- **HTTP** — `https://example.com/path/data.stards` or
  `/vsicurl/https://example.com/path/data.stards` (read-only)

The plain `s3://` / `https://` forms and the `/vsis3/` / `/vsicurl/` prefixes are
interchangeable; use whichever your tooling already speaks. Detection is by path
format: an `s3://` URI is treated as S3, an `http(s)://` URL as HTTP, and anything
else as a local file.

These require the library to be built with `STARDS_ENABLE_CURL` (HTTP) and
`STARDS_ENABLE_S3` (S3) — both on by default; see
[Installation](../getting-started/installation.md).

=== "Python"

    ```python
    import os
    from pystards import StarDataset

    os.environ["AWS_PROFILE"] = "my-profile"

    # Read from S3 (s3:// URI or the /vsis3/ prefix — both work)
    with StarDataset.open("s3://my-bucket/data.stards", mode="r") as ds:
        data = ds["array_name"]

    # Write to S3
    with StarDataset.create("s3://my-bucket/output.stards") as ds:
        ds["results"] = processed_data

    # Read from HTTP (read-only) — plain URL or the /vsicurl/ prefix
    with StarDataset.open("https://example.com/data.stards", mode="r") as ds:
        data = ds["array_name"]
    ```

=== "JS"

    To read/write with an S3 bucket, configure your AWS Credentials in  
    `loadStarDS({ env: { AWS_... } })`:

    ```js title="Using Credentials and Writing to S3"
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

    ```js title="Reading Public URLs"
    import { loadStarDS } from './stards.mjs';
    const { Dataset } = await loadStarDS();

    const URL = 'https://asc-isisdata.s3.amazonaws.com/cnf_test_data/largenet.stards';
    const remoteReader = await new Dataset(URL);
    console.info('Shape of remote array', remoteReader.shape('m.sample'));
    ```

    ```js title="Downloading a File"

    // HTML should have a download button for this example: <button id="download">
    // For security, browsers require the user to click something to trigger a download.

    import { loadStarDS } from './stards.mjs';
    const { Dataset } = await loadStarDS();

    const ds = await new Dataset("https://asc-isisdata.s3.amazonaws.com/cnf_test_data/test123456.stards");

    function downloadBytes(bytes, filename) {
      // Copy out the Uint8Array off the WASM heap into a Blob
      const blob = new Blob([bytes], { type: 'application/octet-stream' });
      const href = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = href;
      a.download = filename;   // instructs the browser to save, not navigate
      document.body.appendChild(a);
      a.click();
      a.remove();
      setTimeout(() => URL.revokeObjectURL(href), 0);  // free the blob once handled
    }

    // Download Button Click Listener
    document.querySelector('#download').addEventListener('click', async () => {
      const bytes = await ds.writeBytes(); // await — writeBytes() returns a Promise
      downloadBytes(bytes, "test123456.stards");
    });
    ```
    

=== "C++"

    ```cpp
    #include "stards.h"
    using namespace star;

    // Read from S3 (s3:// URI or the /vsis3/ prefix — both work)
    auto store = StarDataset::open("s3://my-bucket/data.stards", "r");
    auto data = store->get<double>("sensor_data");

    // Write to S3
    auto output = StarDataset::create("s3://output-bucket/results.stards");
    output->put("results", std::move(processed_data));
    output->flush();

    // Read from HTTP (read-only), then save a local copy
    auto http = StarDataset::open("https://example.com/data.stards", "r");
    http->save_to("/tmp/local-copy.stards");
    ```

`save_to()` writes a local copy of a remote (or read-only) dataset — useful when
you've opened an HTTP source you can't write back to.

## S3 authentication

Address an S3 object as either `s3://bucket-name/path/to/file.stards` or
`/vsis3/bucket-name/path/to/file.stards`. Credentials are resolved from the
standard AWS sources — choose whichever fits your environment:

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

By default an S3 path targets AWS at `https://<bucket>.s3.<region>.amazonaws.com`.
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

- HTTP sources are **read-only**. Use `save_to()` (C++) to persist a local copy.
- A full runnable example (with graceful fallback to a local file when no AWS
  credentials are present) ships at `bindings/python/examples/s3_example.py`.
