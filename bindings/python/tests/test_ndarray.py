import pytest
import numpy as np
from pystar import NDArray, zeros, ones, arange, full


def test_from_numpy():
    """Test creating NDArray from NumPy array"""
    arr = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float64)
    nd = NDArray.from_numpy(arr)

    assert nd.shape == (2, 3)
    assert nd.ndim == 2
    assert nd.size == 6


def test_to_numpy():
    """Test converting NDArray back to NumPy"""
    arr = np.array([[1, 2, 3], [4, 5, 6]], dtype=np.float64)
    nd = NDArray.from_numpy(arr)
    result = nd.to_numpy()

    assert np.array_equal(result, arr)
    assert result.shape == arr.shape


def test_zeros():
    """Test zeros creation"""
    nd = zeros((3, 4), dtype=np.float64)
    arr = nd.to_numpy()

    assert arr.shape == (3, 4)
    assert np.all(arr == 0)


def test_ones():
    """Test ones creation"""
    nd = ones((2, 3), dtype=np.int32)
    arr = nd.to_numpy()

    assert arr.shape == (2, 3)
    assert np.all(arr == 1)


def test_arange():
    """Test arange creation"""
    nd = arange(10)
    arr = nd.to_numpy()

    assert np.array_equal(arr, np.arange(10))


def test_arange_with_start_stop():
    """Test arange with start and stop"""
    nd = arange(5, 15)
    arr = nd.to_numpy()

    assert np.array_equal(arr, np.arange(5, 15))


def test_full():
    """Test full creation"""
    nd = full((3, 3), 7.5)
    arr = nd.to_numpy()

    assert arr.shape == (3, 3)
    assert np.all(arr == 7.5)


def test_different_dtypes():
    """Test NDArray with different NumPy dtypes"""
    dtypes = [np.int8, np.int16, np.int32, np.int64,
              np.uint8, np.uint16, np.uint32, np.uint64,
              np.float32, np.float64]

    for dtype in dtypes:
        arr = np.array([1, 2, 3, 4, 5], dtype=dtype)
        nd = NDArray.from_numpy(arr)
        result = nd.to_numpy()

        assert result.dtype == dtype
        assert np.array_equal(result, arr)


def test_1d_array():
    """Test 1D array"""
    arr = np.array([1, 2, 3, 4, 5])
    nd = NDArray.from_numpy(arr)

    assert nd.shape == (5,)
    assert nd.ndim == 1
    assert len(nd) == 5


def test_3d_array():
    """Test 3D array"""
    arr = np.arange(24).reshape(2, 3, 4)
    nd = NDArray.from_numpy(arr)

    assert nd.shape == (2, 3, 4)
    assert nd.ndim == 3
    assert nd.size == 24

    result = nd.to_numpy()
    assert np.array_equal(result, arr)


def test_roundtrip():
    """Test NumPy -> NDArray -> NumPy roundtrip preserves data"""
    original = np.random.rand(5, 10, 15)
    nd = NDArray.from_numpy(original)
    result = nd.to_numpy()

    assert np.allclose(result, original)
    assert result.shape == original.shape
