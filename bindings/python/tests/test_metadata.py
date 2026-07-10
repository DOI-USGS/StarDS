import pytest
import numpy as np
from pystards import StarDataset


def test_metadata_shape(store):
    """Test metadata shape property"""
    arr = np.zeros((3, 4, 5))
    store.meta["test_shape"] = arr
    store.flush()

    # Get the metadata value
    meta = store._store.meta.get("test_shape")
    assert meta is not None
    assert tuple(meta.shape) == (3, 4, 5)


def test_metadata_dtype(store):
    """Test metadata dtype property"""
    arr = np.array([1, 2, 3], dtype=np.int32)
    store.meta["test_dtype"] = arr
    store.flush()

    meta = store._store.meta.get("test_dtype")
    assert meta is not None
    # DataType.INT32 == 2
    from pystards import DataType
    assert meta.dtype == DataType.INT32


def test_metadata_size(store):
    """Test metadata size property"""
    arr = np.zeros((4, 5, 6))
    store.meta["test_size"] = arr
    store.flush()

    meta = store._store.meta.get("test_size")
    assert meta is not None
    assert meta.size() == 4 * 5 * 6


def test_metadata_ndim(store):
    """Test metadata ndim property"""
    arr = np.zeros((2, 3, 4, 5))
    store.meta["test_ndim"] = arr
    store.flush()

    meta = store._store.meta.get("test_ndim")
    assert meta is not None
    assert meta.ndim() == 4


def test_metadata_is_scalar(store):
    """Test is_scalar for scalar values"""
    # Single element
    scalar = np.array(42.0)
    store.meta["scalar"] = scalar
    store.flush()

    meta = store._store.meta.get("scalar")
    assert meta.is_scalar()


def test_metadata_is_array(store):
    """Test is_array for array values"""
    arr = np.array([1, 2, 3, 4, 5])
    store.meta["array"] = arr
    store.flush()

    meta = store._store.meta.get("array")
    assert meta.is_array()
