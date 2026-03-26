"""
Example demonstrating new Python API features:
1. Dictionary-style accessors: ds["key"] = array
2. Logger control: pystar.set_log_level()
3. Explicit close(): ds.close()
"""
import pystar
import numpy as np
import tempfile
import os

def main():
    # Feature 1: Set logging level to see debug messages
    print("=" * 60)
    print("Feature 1: Logger Control")
    print("=" * 60)

    print(f"Default log level: {pystar.get_log_level()} (4=ERROR)")

    # Enable debug logging
    pystar.set_log_level(pystar.LogLevel.DEBUG)
    print(f"New log level: {pystar.get_log_level()} (1=DEBUG)")
    print("Debug messages will now be printed by STARDS C++ code\n")

    # Create a temporary file
    with tempfile.NamedTemporaryFile(suffix='.star', delete=False) as f:
        temp_file = f.name

    try:
        # Feature 2: Dictionary-style accessors
        print("=" * 60)
        print("Feature 2: Dictionary-Style Accessors")
        print("=" * 60)

        # Create dataset
        store = pystar.StarDataset(temp_file, "rw")

        # OLD WAY (still works):
        # store.put("old_style", np.array([1, 2, 3]))

        # NEW WAY: Dictionary-style assignment
        print("Storing arrays with dict syntax: ds['key'] = array")
        store["numbers"] = np.array([1, 2, 3, 4, 5], dtype=np.int64)
        store["matrix"] = np.random.rand(3, 3)
        store["tensor"] = np.arange(24).reshape(2, 3, 4).astype(np.float32)

        # Feature 3: Explicit close()
        print("\nClosing dataset explicitly with .close()")
        store.close()
        print("Dataset closed successfully\n")

        # Open again and read with dictionary syntax
        print("=" * 60)
        print("Reading with Dictionary-Style Accessors")
        print("=" * 60)

        store = pystar.StarDataset(temp_file, "r")

        # OLD WAY (still works):
        # numbers = store.get("numbers")

        # NEW WAY: Dictionary-style access
        print("Reading arrays with dict syntax: array = ds['key']")
        numbers = store["numbers"]
        print(f"Numbers: {numbers}")

        matrix = store["matrix"]
        print(f"\nMatrix shape: {matrix.shape}")
        print(matrix)

        tensor = store["tensor"]
        print(f"\nTensor shape: {tensor.shape}")

        # Check if key exists using 'in' operator
        print(f"\n'numbers' in store: {'numbers' in store}")
        print(f"'nonexistent' in store: {'nonexistent' in store}")

        # Close without context manager
        store.close()
        print("\nDataset closed successfully")

        # Feature 4: Context manager still works (preferred for automatic cleanup)
        print("\n" + "=" * 60)
        print("Context Manager (Automatic close)")
        print("=" * 60)

        with pystar.StarDataset(temp_file, "r") as store:
            print("Inside context manager")
            all_keys = store.keys()
            print(f"All keys: {all_keys}")

            for key in all_keys:
                data = store[key]  # Dictionary-style access
                print(f"  {key}: shape={data.shape}, dtype={data.dtype}")

        print("Context manager closed automatically\n")

        # Reset log level to default (ERROR)
        pystar.set_log_level(pystar.LogLevel.ERROR)
        print(f"Log level reset to: {pystar.get_log_level()} (4=ERROR)")

    finally:
        # Cleanup
        if os.path.exists(temp_file):
            os.unlink(temp_file)
            print(f"\nCleaned up temporary file: {temp_file}")

if __name__ == "__main__":
    main()
