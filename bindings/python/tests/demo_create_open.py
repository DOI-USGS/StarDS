"""Demonstration of create() and open() behavior"""
import pystar
import numpy as np
import os


def demo_behavior():
    """Demonstrate the behavior of create() and open()"""

    print("=" * 70)
    print("StarDataset.create() and .open() Behavior Demo")
    print("=" * 70)

    filepath = "/tmp/demo_behavior.star"

    # Clean up
    if os.path.exists(filepath):
        os.remove(filepath)

    # =========================================================================
    print("\n1. create() on non-existing file → Creates new file")
    print("-" * 70)
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()
    print(f"✓ Created {filepath}")

    # =========================================================================
    print("\n2. open(mode='r') on existing file → Opens read-only")
    print("-" * 70)
    store = pystar.StarDataset.open(filepath, mode="r")
    print(f"✓ Opened (read-only: {store.is_read_only()})")
    print(f"  Data: {store['data']}")
    store.close()

    # =========================================================================
    print("\n3. open(mode='rw') on existing file → Opens read-write")
    print("-" * 70)
    store = pystar.StarDataset.open(filepath, mode="rw")
    print(f"✓ Opened (read-only: {store.is_read_only()})")
    print(f"  Data: {store['data']}")
    store.close()

    # =========================================================================
    print("\n4. create() on existing file → Overwrites file")
    print("-" * 70)
    print(f"  Before: file exists with 'data' key")
    store = pystar.StarDataset.create(filepath)
    store["new_data"] = np.array([10, 20, 30])
    store.flush()
    store.close()
    print(f"✓ Overwrote file")

    # Verify
    store = pystar.StarDataset.open(filepath, mode="r")
    print(f"  After: keys = {list(store.keys())} (old 'data' key is gone)")
    store.close()

    # Clean up for next demo
    os.remove(filepath)

    # =========================================================================
    print("\n5. open(mode='rw') on non-existing file → Creates new file")
    print("-" * 70)
    store = pystar.StarDataset.open(filepath, mode="rw")
    store["created_via_open"] = np.array([100, 200])
    store.flush()
    store.close()
    print(f"✓ Created {filepath} via open()")

    # Verify
    store = pystar.StarDataset.open(filepath, mode="r")
    print(f"  Keys: {list(store.keys())}")
    store.close()

    # =========================================================================
    print("\n6. open(mode='r') on non-existing file → Error!")
    print("-" * 70)
    os.remove(filepath)  # Make sure it doesn't exist
    try:
        store = pystar.StarDataset.open(filepath, mode="r")
        print("✗ Should have failed!")
    except RuntimeError as e:
        print(f"✓ Correctly raised error: {e}")

    # =========================================================================
    print("\n7. open() on corrupt file → Error!")
    print("-" * 70)
    # Create corrupt file
    with open(filepath, 'wb') as f:
        f.write(b"This is not a STAR file")

    try:
        store = pystar.StarDataset.open(filepath, mode="r")
        print("✗ Should have failed!")
    except RuntimeError as e:
        print(f"✓ Correctly raised error: {e}")

    # Clean up
    if os.path.exists(filepath):
        os.remove(filepath)

    print("\n" + "=" * 70)
    print("Summary:")
    print("=" * 70)
    print("• create() always creates/overwrites")
    print("• open(mode='rw') creates if missing, opens if exists")
    print("• open(mode='r') fails if file missing or corrupt")
    print("=" * 70)


if __name__ == "__main__":
    demo_behavior()
