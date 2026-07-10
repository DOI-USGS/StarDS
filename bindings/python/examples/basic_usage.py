#!/usr/bin/env python3
"""
Basic usage example for STARDS Python bindings.

This example demonstrates:
- Creating a dataset
- Storing and retrieving arrays
- Using metadata for scalars and strings
- Namespace separation (arrays vs metadata)
- Context manager usage
- Iteration over keys
"""

import numpy as np
from pystards import StarDataset

def main():
    # Create a new dataset
    print("Creating dataset...")
    with StarDataset.create("example_basic.stards") as ds:
        # Store arrays using dictionary syntax
        print("\nStoring arrays...")
        ds["integers"] = np.array([1, 2, 3, 4, 5], dtype=np.int32)
        ds["floats"] = np.random.rand(10)
        ds["matrix"] = np.ones((5, 5), dtype=np.float64)
        ds["3d_array"] = np.zeros((3, 4, 5))

        # Store metadata for scalars and small data
        # Metadata has separate namespace - same keys are allowed!
        print("\nStoring metadata...")
        ds.meta["experiment_id"] = 12345
        ds.meta["timestamp"] = "2024-04-21"
        ds.meta["tags"] = ["test", "example", "basic"]
        ds.meta["pi"] = 3.14159

        # You can use the same key in both namespaces
        ds.meta["matrix"] = "5x5 matrix of ones"  # Metadata about the array

        # Dataset is automatically flushed on exit

    # Reopen and read
    print("\nReading from dataset...")
    with StarDataset.open("example_basic.stards", mode="r") as ds:
        print(f"Dataset contains {len(ds)} arrays")

        # Iterate over all arrays
        print("\nArrays:")
        for key in ds:
            array = ds[key]
            print(f"  {key}: shape={array.shape}, dtype={array.dtype}")

        # Access specific arrays
        integers = ds["integers"]
        print(f"\nIntegers: {integers}")

        matrix = ds["matrix"]
        print(f"\nMatrix shape: {matrix.shape}")
        print(f"  First row: {matrix[0, :]}")

        # Access metadata
        print("\nMetadata:")
        for key in ds.meta:
            print(f"  {key}: {ds.meta[key]}")

        # Demonstrate namespace separation
        print("\nNamespace separation:")
        print(f"  ds['matrix'] is array: shape={ds['matrix'].shape}")
        print(f"  ds.meta['matrix'] is metadata: {ds.meta['matrix']}")
        print("  (same key, different namespaces!)")

    print("\n✓ Basic usage example complete!")

if __name__ == "__main__":
    main()
