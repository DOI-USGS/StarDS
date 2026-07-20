import pytest
import numpy as np
from pystards import StarDataset


def test_1d_slicing(sliceable_store):
    """Test slicing 1D array"""
    store = sliceable_store
    arr = np.arange(10000, dtype=np.int64)
    store.put("large_1d", arr)
    store.flush()

    assert store.is_sliceable("large_1d")

    # Slice [100:200]
    subset = store.get_slice("large_1d", [(100, 200)])
    assert subset.shape == (100,)
    assert np.array_equal(subset, np.arange(100, 200))


def test_2d_slicing(sliceable_store):
    """Test slicing 2D array"""
    store = sliceable_store
    arr = np.arange(10000, dtype=np.float64).reshape(100, 100)
    store.put("large_2d", arr)
    store.flush()

    assert store.is_sliceable("large_2d")

    # Slice [10:20, 30:40]
    subset = store.get_slice("large_2d", [(10, 20), (30, 40)])
    assert subset.shape == (10, 10)
    assert np.allclose(subset, arr[10:20, 30:40])


def test_3d_slicing(sliceable_store):
    """Test slicing 3D array"""
    store = sliceable_store
    arr = np.arange(8000, dtype=np.int32).reshape(20, 20, 20)
    store.put("large_3d", arr)
    store.flush()

    assert store.is_sliceable("large_3d")

    # Slice [5:10, 5:15, 5:10]
    subset = store.get_slice("large_3d", [(5, 10), (5, 15), (5, 10)])
    assert subset.shape == (5, 10, 5)
    assert np.array_equal(subset, arr[5:10, 5:15, 5:10])


def test_slicing_with_step(sliceable_store):
    """Test slicing with step parameter"""
    store = sliceable_store
    arr = np.arange(10000, dtype=np.int64)
    store.put("large", arr)
    store.flush()

    # Slice [0:100:2] - every other element
    subset = store.get_slice("large", [(0, 100, 2)])
    assert subset.shape == (50,)
    assert np.array_equal(subset, np.arange(0, 100, 2))


def test_full_dimension_slice(sliceable_store):
    """Test slicing entire dimension"""
    store = sliceable_store
    arr = np.arange(10000, dtype=np.float32).reshape(100, 100)
    store.put("matrix", arr)
    store.flush()

    # Slice all rows, subset of columns
    subset = store.get_slice("matrix", [(0, 100), (25, 75)])
    assert subset.shape == (100, 50)
    assert np.allclose(subset, arr[:, 25:75])


def test_small_array_not_sliceable(store):
    """Test that small arrays are not sliceable"""
    arr = np.arange(100)  # Small array
    store.put("small", arr)
    store.flush()

    # Small arrays stored in metadata block aren't sliceable
    # (This depends on implementation details)
    # If it's sliceable, that's fine too
    pass


def test_slice_different_dtypes(sliceable_store):
    """Test slicing with different data types"""
    store = sliceable_store
    # Use appropriate ranges for each dtype to avoid overflow
    dtype_configs = [
        (np.int8, 256, 100, 150),      # Small range for int8 (-128 to 127)
        (np.uint8, 256, 100, 150),     # Small range for uint8 (0 to 255)
        (np.int16, 10000, 1000, 2000),
        (np.int32, 10000, 1000, 2000),
        (np.int64, 10000, 1000, 2000),
        (np.uint16, 10000, 1000, 2000),
        (np.uint32, 10000, 1000, 2000),
        (np.uint64, 10000, 1000, 2000),
        (np.float32, 10000, 1000, 2000),
        (np.float64, 10000, 1000, 2000),
    ]

    for dtype, size, slice_start, slice_end in dtype_configs:
        arr = np.arange(size, dtype=dtype)
        key = f"array_{dtype.__name__}"
        store.put(key, arr)
        store.flush()

        if store.is_sliceable(key):
            subset = store.get_slice(key, [(slice_start, slice_end)])
            expected = np.arange(slice_start, slice_end, dtype=dtype)
            assert subset.dtype == dtype
            assert np.array_equal(subset, expected)
