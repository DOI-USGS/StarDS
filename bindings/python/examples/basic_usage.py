#!/usr/bin/env python3
"""
Basic usage example for CameraStateFile Python bindings.

This example demonstrates:
- Creating a Store
- Storing and retrieving arrays
- Using different data types
- Context manager usage
"""

import numpy as np
from pystar import StarDataset

def main():
    # Create a new store (or open existing)
    print("Creating store...")
    with StarDataset("example_basic.star") as store:
        # Store some arrays
        print("\nStoring arrays...")
        store.put("integers", np.array([1, 2, 3, 4, 5], dtype=np.int32))
        store.put("floats", np.random.rand(10))
        store.put("matrix", np.ones((5, 5), dtype=np.float64))
        store.put("3d_array", np.zeros((3, 4, 5)))

        # Store is automatically flushed on exit

    # Reopen and read
    print("\nReading from store...")
    with StarDataset("example_basic.star", mode="r") as store:
        print(f"Store contains {len(store)} arrays")
        print(f"Keys: {store.keys()}")

        # Retrieve arrays
        integers = store.get("integers")
        print(f"\nIntegers: {integers}")
        print(f"  dtype: {integers.dtype}")
        print(f"  shape: {integers.shape}")

        matrix = store.get("matrix")
        print(f"\nMatrix shape: {matrix.shape}")
        print(f"  First row: {matrix[0, :]}")

        array_3d = store.get("3d_array")
        print(f"\n3D array shape: {array_3d.shape}")

    print("\n✓ Basic usage example complete!")

if __name__ == "__main__":
    main()
