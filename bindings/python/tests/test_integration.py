import pytest
import numpy as np
from pystards import StarDataset


def test_mixed_scalars_and_arrays(store):
    """Test storing both scalars and arrays"""
    # Scalar values
    store.put("scalar_int", np.array(42, dtype=np.int32))
    store.put("scalar_float", np.array(3.14, dtype=np.float64))

    # Array values
    store.put("array_1d", np.array([1, 2, 3, 4, 5]))
    store.put("array_2d", np.ones((10, 10)))
    store.put("array_3d", np.zeros((5, 5, 5)))

    store.flush()

    # Verify scalar values
    assert store.get("scalar_int") == 42
    assert np.isclose(store.get("scalar_float"), 3.14)

    # Verify arrays
    assert np.array_equal(store.get("array_1d"), np.array([1, 2, 3, 4, 5]))
    assert np.all(store.get("array_2d") == 1.0)
    assert np.all(store.get("array_3d") == 0.0)


def test_overwrite_key(store):
    """Test overwriting an existing key"""
    # Write initial value
    store.put("key", np.array([1, 2, 3]))
    store.flush()

    # Overwrite
    store.put("key", np.array([4, 5, 6]))
    store.flush()

    # Verify new value
    result = store.get("key")
    assert np.array_equal(result, np.array([4, 5, 6]))


def test_large_and_small_arrays(sliceable_store):
    """Test mixing large and small arrays"""
    # Use a sliceable (non-shuffle) codec: the large array is asserted to be
    # sliceable below, which the default byte-shuffle codec does not support.
    store = sliceable_store
    # Small array (stored in metadata)
    small = np.arange(100)
    store.put("small", small)

    # Large array (stored separately)
    large = np.arange(100000)
    store.put("large", large)

    store.flush()

    # Verify both can be retrieved
    assert np.array_equal(store.get("small"), small)
    assert np.array_equal(store.get("large"), large)

    # Large should be sliceable
    assert store.is_sliceable("large")


def test_multiple_sessions(temp_star_file):
    """Test writing in one session and reading in another"""
    # Session 1: Write data
    with StarDataset(temp_star_file) as store1:
        store1.put("data1", np.array([1, 2, 3]))
        store1.put("data2", np.ones((10, 10)))

    # Session 2: Read and write more
    with StarDataset(temp_star_file) as store2:
        # Read existing data
        assert np.array_equal(store2.get("data1"), np.array([1, 2, 3]))
        assert np.all(store2.get("data2") == 1.0)

        # Write new data
        store2.put("data3", np.zeros((5, 5)))

    # Session 3: Verify all data
    with StarDataset(temp_star_file, mode="r") as store3:
        assert len(store3) == 3
        assert "data1" in store3
        assert "data2" in store3
        assert "data3" in store3


def test_empty_store(temp_star_file):
    """Test creating and reading empty store"""
    # Create empty store
    with StarDataset(temp_star_file) as store:
        pass

    # Read empty store
    with StarDataset(temp_star_file, mode="r") as store:
        assert len(store) == 0
        assert store.keys() == []


def test_many_small_arrays(store):
    """Test storing many small arrays"""
    n_arrays = 100

    # Store many arrays
    for i in range(n_arrays):
        arr = np.random.rand(10)
        store.put(f"array_{i}", arr)

    store.flush()

    # Verify count
    assert len(store) == n_arrays
    assert len(store.keys()) == n_arrays

    # Verify a few random ones
    for i in [0, 25, 50, 75, 99]:
        data = store.get(f"array_{i}")
        assert data.shape == (10,)


def test_filename_property(temp_star_file):
    """Test filename property"""
    store = StarDataset(temp_star_file)
    assert store.filename == temp_star_file
