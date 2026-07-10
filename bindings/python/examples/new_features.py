"""
Example demonstrating standard STARDS Python API features:
1. Dictionary-style accessors: ds["key"] = array
2. Metadata storage: ds.meta["key"] = value
3. Iteration: for key in ds
4. Logger control: pystards.set_log_level()
"""
import pystards
import numpy as np
import tempfile
import os

def main():
    # Feature 1: Set logging level to see debug messages
    print("=" * 60)
    print("Feature 1: Logger Control")
    print("=" * 60)

    print(f"Default log level: {pystards.get_log_level()} (4=ERROR)")

    # Enable debug logging
    pystards.set_log_level(pystards.LogLevel.DEBUG)
    print(f"New log level: {pystards.get_log_level()} (1=DEBUG)")
    print("Debug messages will now be printed by STARDS C++ code\n")

    # Create a temporary file
    with tempfile.NamedTemporaryFile(suffix='.stards', delete=False) as f:
        temp_file = f.name

    try:
        # Feature 2: Dictionary-style accessors and metadata
        print("=" * 60)
        print("Feature 2: Dictionary-Style Access & Metadata")
        print("=" * 60)

        # Create dataset
        store = pystards.StarDataset.create(temp_file)

        # Dictionary-style assignment for arrays
        print("Storing arrays with dict syntax: ds['key'] = array")
        store["numbers"] = np.array([1, 2, 3, 4, 5], dtype=np.int64)
        store["matrix"] = np.random.rand(3, 3)
        store["tensor"] = np.arange(24).reshape(2, 3, 4).astype(np.float32)

        # Metadata storage for scalars and strings
        print("\nStoring metadata: ds.meta['key'] = value")
        store.meta["version"] = 1
        store.meta["experiment"] = "demo"
        store.meta["tags"] = ["example", "test"]

        # Explicit close()
        print("\nClosing dataset explicitly with .close()")
        store.close()
        print("Dataset closed successfully\n")

        # Open again and read with dictionary syntax
        print("=" * 60)
        print("Feature 3: Reading & Iteration")
        print("=" * 60)

        store = pystards.StarDataset.open(temp_file, mode="r")

        # Dictionary-style access for arrays
        print("Reading arrays with dict syntax: array = ds['key']")
        numbers = store["numbers"]
        print(f"Numbers: {numbers}")

        # Iterate over all array keys
        print("\nIterating over arrays:")
        for key in store:
            data = store[key]
            print(f"  {key}: shape={data.shape}, dtype={data.dtype}")

        # Access metadata
        print("\nMetadata:")
        for key in store.meta:
            print(f"  {key}: {store.meta[key]}")

        # Get all metadata at once
        all_meta = store.get_all_metadata()
        print(f"\nAll metadata: {all_meta}")

        # Close without context manager
        store.close()
        print("\nDataset closed successfully")

        # Feature 4: Context manager still works (preferred for automatic cleanup)
        print("\n" + "=" * 60)
        print("Context Manager (Automatic close)")
        print("=" * 60)

        with pystards.StarDataset(temp_file, "r") as store:
            print("Inside context manager")
            all_keys = store.keys()
            print(f"All keys: {all_keys}")

            for key in all_keys:
                data = store[key]  # Dictionary-style access
                print(f"  {key}: shape={data.shape}, dtype={data.dtype}")

        print("Context manager closed automatically\n")

        # Reset log level to default (ERROR)
        pystards.set_log_level(pystards.LogLevel.ERROR)
        print(f"Log level reset to: {pystards.get_log_level()} (4=ERROR)")

    finally:
        # Cleanup
        if os.path.exists(temp_file):
            os.unlink(temp_file)
            print(f"\nCleaned up temporary file: {temp_file}")

if __name__ == "__main__":
    main()
