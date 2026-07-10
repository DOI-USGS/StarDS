import pytest
import numpy as np
from pystards import StarDataset


def test_put_get(store):
    """Test basic put/get operations"""
    arr = np.random.rand(10, 10)
    store.put("matrix", arr)
    store.flush()

    result = store.get("matrix")
    assert np.allclose(result, arr)


def test_put_get_int32(store):
    """Test with int32 dtype"""
    arr = np.arange(100, dtype=np.int32).reshape(10, 10)
    store.put("int_matrix", arr)
    store.flush()

    result = store.get("int_matrix")
    assert result.dtype == np.int32
    assert np.array_equal(result, arr)


def test_context_manager(temp_star_file):
    """Test context manager auto-flush"""
    arr = np.random.rand(10, 10)

    with StarDataset(temp_star_file) as store:
        store.put("data", arr)

    # File should be flushed
    with StarDataset(temp_star_file, mode="r") as store:
        result = store.get("data")
        assert np.allclose(result, arr)


def test_keys(store):
    """Test getting all keys"""
    store.put("a", np.array([1, 2, 3]))
    store.put("b", np.array([4, 5, 6]))
    store.flush()

    keys = store.keys()
    assert set(keys) == {"a", "b"}


def test_contains(store):
    """Test key existence check"""
    store.put("exists", np.array([1, 2, 3]))
    store.flush()

    assert "exists" in store
    assert "nonexistent" not in store


def test_len(store):
    """Test store length"""
    assert len(store) == 0

    store.put("a", np.array([1, 2, 3]))
    store.flush()
    assert len(store) == 1

    store.put("b", np.array([4, 5, 6]))
    store.flush()
    assert len(store) == 2


def test_read_only_mode(temp_star_file):
    """Test read-only mode"""
    # Write data first
    with StarDataset(temp_star_file) as store:
        store.put("data", np.array([1, 2, 3]))

    # Open in read-only mode
    store_ro = StarDataset(temp_star_file, mode="r")
    data = store_ro.get("data")
    assert np.array_equal(data, np.array([1, 2, 3]))


def test_multiple_dtypes(store):
    """Test storing arrays with different dtypes"""
    dtypes = [np.int8, np.int16, np.int32, np.int64,
              np.uint8, np.uint16, np.uint32, np.uint64,
              np.float32, np.float64]

    for i, dtype in enumerate(dtypes):
        arr = np.arange(10, dtype=dtype)
        store.put(f"array_{dtype.__name__}", arr)

    store.flush()

    for i, dtype in enumerate(dtypes):
        result = store.get(f"array_{dtype.__name__}")
        expected = np.arange(10, dtype=dtype)
        assert result.dtype == dtype
        assert np.array_equal(result, expected)


def test_key_not_found(store):
    """Test error when key doesn't exist"""
    with pytest.raises(KeyError):
        store.get("nonexistent")
