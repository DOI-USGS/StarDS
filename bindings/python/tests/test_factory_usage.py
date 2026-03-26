"""Simple usage examples demonstrating StarDataset.create() and StarDataset.open()"""
import pystar
import numpy as np
import os


def test_basic_create_and_open():
    """Basic example of create and open"""
    filepath = "/tmp/demo_factory.star"

    # Clean up if exists
    if os.path.exists(filepath):
        os.remove(filepath)

    try:
        # Create a new file
        print("Creating new file with StarDataset.create()...")
        store = pystar.StarDataset.create(filepath)
        store["data"] = np.array([1, 2, 3, 4, 5])
        store["matrix"] = np.eye(3)
        store.flush()
        store.close()
        print(f"✓ Created {filepath}")

        # Open the file
        print("\nOpening file with StarDataset.open()...")
        store = pystar.StarDataset.open(filepath, mode="r")
        print(f"✓ Opened {filepath}")
        print(f"  Keys: {list(store.keys())}")
        print(f"  Data: {store['data']}")
        print(f"  Matrix shape: {store['matrix'].shape}")
        store.close()

        print("\n✓ All operations successful!")

    finally:
        if os.path.exists(filepath):
            os.remove(filepath)
            print(f"\n✓ Cleaned up {filepath}")


def test_no_empty_file_error():
    """Demonstrates that create() works even if file doesn't exist"""
    filepath = "/tmp/demo_new_file.star"

    # Ensure file doesn't exist
    if os.path.exists(filepath):
        os.remove(filepath)

    try:
        # This should NOT cause "bad alloc" or "magic string mismatch"
        # because loadIndex() returns early if file doesn't exist
        print("Creating file that doesn't exist yet...")
        store = pystar.StarDataset.create(filepath)
        store["test"] = np.array([42])
        store.flush()
        store.close()
        print(f"✓ Successfully created {filepath}")

        # Verify we can open it
        store = pystar.StarDataset.open(filepath, mode="r")
        assert store["test"][0] == 42
        store.close()
        print("✓ Successfully opened and verified")

    finally:
        if os.path.exists(filepath):
            os.remove(filepath)


def test_factory_vs_constructor():
    """Show that factory methods work the same as constructor"""
    filepath1 = "/tmp/via_factory.star"
    filepath2 = "/tmp/via_constructor.star"

    for fp in [filepath1, filepath2]:
        if os.path.exists(fp):
            os.remove(fp)

    try:
        # Using factory method
        print("Using StarDataset.create() factory method...")
        store1 = pystar.StarDataset.create(filepath1)
        store1["data"] = np.array([1, 2, 3])
        store1.flush()
        store1.close()
        print(f"✓ Created {filepath1}")

        # Using constructor
        print("\nUsing StarDataset() constructor...")
        store2 = pystar.StarDataset(filepath2, mode="rw")
        store2["data"] = np.array([1, 2, 3])
        store2.flush()
        store2.close()
        print(f"✓ Created {filepath2}")

        # Both should work identically
        store1 = pystar.StarDataset.open(filepath1, mode="r")
        store2 = pystar.StarDataset.open(filepath2, mode="r")

        np.testing.assert_array_equal(store1["data"], store2["data"])
        print("\n✓ Both methods produce identical results!")

        store1.close()
        store2.close()

    finally:
        for fp in [filepath1, filepath2]:
            if os.path.exists(fp):
                os.remove(fp)


if __name__ == "__main__":
    print("="*60)
    print("StarDataset Factory Methods Demo")
    print("="*60)

    print("\n" + "="*60)
    print("Test 1: Basic create() and open()")
    print("="*60)
    test_basic_create_and_open()

    print("\n" + "="*60)
    print("Test 2: No empty file error")
    print("="*60)
    test_no_empty_file_error()

    print("\n" + "="*60)
    print("Test 3: Factory vs Constructor")
    print("="*60)
    test_factory_vs_constructor()

    print("\n" + "="*60)
    print("All tests passed! ✓")
    print("="*60)
