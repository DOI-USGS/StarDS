#!/usr/bin/env python3
"""
NumPy interoperability example for CameraStateFile Python bindings.

This example demonstrates:
- Seamless NumPy integration
- Array slicing for large datasets
- Different data types
- Array creation helpers
"""

import numpy as np
from pystar import StarDataset, zeros, ones, arange, full

def main():
    print("NumPy Interoperability Example\n" + "="*50)

    # Create store
    with StarDataset("example_numpy.star") as store:
        # Store NumPy arrays directly
        print("\n1. Storing NumPy arrays...")
        random_data = np.random.rand(100, 100)
        store.put("random_matrix", random_data)

        int_data = np.arange(10000, dtype=np.int64)
        store.put("large_array", int_data)

        # Store arrays with different dtypes
        print("\n2. Storing different data types...")
        for dtype in [np.float32, np.float64, np.int32, np.int64]:
            arr = np.arange(100, dtype=dtype)
            store.put(f"array_{dtype.__name__}", arr)

        print("   Flushing to disk...")

    # Reopen and demonstrate slicing
    print("\n3. Demonstrating array slicing...")
    with StarDataset("example_numpy.star") as store:
        # Check if array is sliceable
        if store.is_sliceable("large_array"):
            print("   ✓ large_array is sliceable")

            # Get a subset without loading entire array
            subset = store.get_slice("large_array", [(1000, 2000)])
            print(f"   Slice [1000:2000]: shape={subset.shape}")
            print(f"   First 10 elements: {subset[:10]}")

            # Verify correctness
            expected = np.arange(1000, 2000, dtype=np.int64)
            assert np.array_equal(subset, expected)
            print("   ✓ Slice data verified correct")

        # Retrieve random matrix
        retrieved = store.get("random_matrix")
        print(f"\n   Retrieved matrix shape: {retrieved.shape}")
        print(f"   dtype: {retrieved.dtype}")

    # Demonstrate array creation helpers
    print("\n4. Using array creation helpers...")
    with StarDataset("example_helpers.star") as store:
        # Create arrays using helper functions
        store.put("zeros_5x5", zeros((5, 5)))
        store.put("ones_3x4", ones((3, 4)))
        store.put("range_100", arange(100))
        store.put("filled_7s", full((10, 10), 7.0))

        print("   Created arrays with helpers")

    # Verify
    with StarDataset("example_helpers.star", mode="r") as store:
        zeros_arr = store.get("zeros_5x5")
        print(f"\n   zeros array: shape={zeros_arr.shape}, all zero? {np.all(zeros_arr == 0)}")

        ones_arr = store.get("ones_3x4")
        print(f"   ones array: shape={ones_arr.shape}, all one? {np.all(ones_arr == 1)}")

        range_arr = store.get("range_100")
        print(f"   range array: shape={range_arr.shape}, matches arange? {np.array_equal(range_arr, np.arange(100))}")

        filled_arr = store.get("filled_7s")
        print(f"   filled array: shape={filled_arr.shape}, all 7.0? {np.all(filled_arr == 7.0)}")

    print("\n✓ NumPy interoperability example complete!")

if __name__ == "__main__":
    main()
