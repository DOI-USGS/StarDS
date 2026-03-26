#!/usr/bin/env python3
"""
S3 cloud storage example for CameraStateFile Python bindings.

This example demonstrates:
- Writing to S3 with /vsis3/ paths
- Reading from S3
- HTTP streaming with /vsicurl/

Note: Requires AWS credentials configured (e.g., via environment variables)
"""

import numpy as np
from pystar import StarDataset
import os

def main():
    print("S3 Cloud Storage Example\n" + "="*50)

    # Check if AWS credentials are available
    has_aws = os.environ.get('AWS_ACCESS_KEY_ID') is not None
    if not has_aws:
        print("\n⚠️  AWS credentials not found in environment")
        print("   Set AWS_ACCESS_KEY_ID and AWS_SECRET_ACCESS_KEY to use S3")
        print("   This example will use local file instead\n")

    # Example S3 paths (update with your bucket)
    s3_path = "/vsis3/your-bucket-name/data.star"
    local_path = "example_s3_local.star"

    # Use local path if no AWS credentials
    file_path = s3_path if has_aws else local_path

    # Write data
    print(f"\n1. Writing data to: {file_path}")
    try:
        with StarDataset(file_path) as store:
            # Store some data
            store.put("sensor_data", np.random.rand(1000, 100))
            store.put("timestamps", np.arange(1000, dtype=np.int64))
            store.put("metadata_version", np.array(1, dtype=np.int32))

            print("   ✓ Data written successfully")
    except Exception as e:
        print(f"   ✗ Write failed: {e}")
        if has_aws:
            print("   Make sure the bucket exists and you have write permissions")
        return

    # Read data back
    print(f"\n2. Reading data from: {file_path}")
    try:
        with StarDataset(file_path, mode="r") as store:
            print(f"   Store contains {len(store)} entries")
            print(f"   Keys: {store.keys()}")

            # Read a sample
            sensor_data = store.get("sensor_data")
            print(f"   Sensor data shape: {sensor_data.shape}")

            timestamps = store.get("timestamps")
            print(f"   Timestamps: {timestamps[:5]}... ({len(timestamps)} total)")

            print("   ✓ Data read successfully")
    except Exception as e:
        print(f"   ✗ Read failed: {e}")

    # Demonstrate HTTP streaming (read-only)
    print("\n3. HTTP streaming example")
    print("   (This requires a publicly accessible HTTP URL)")
    http_url = "/vsicurl/https://example.com/data.star"
    print(f"   URL: {http_url}")
    print("   Skipping - requires valid public URL")

    # Example of how to use it:
    # with StarDataset(http_url, mode="r") as store:
    #     data = store.get("some_key")

    if has_aws:
        print("\n✓ S3 example complete!")
        print(f"\nNote: S3 file written to: {s3_path}")
        print("      You may want to clean it up:")
        print(f"      aws s3 rm s3://your-bucket-name/data.star")
    else:
        print("\n✓ Local file example complete (S3 requires credentials)")
        print(f"\nLocal file: {local_path}")

if __name__ == "__main__":
    main()
