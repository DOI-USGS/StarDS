"""NumPy-compatible ndarray wrapper.

``NDArray`` is a thin handle around a C++ ``NDArray<T>`` instance. The NumPy
<-> C++ conversion is done by the buffer-protocol helpers generated in
swig/numpy_buffer.i (one memcpy, no per-element crossings); this module just
selects the right helper by dtype from a single table.
"""
try:
    from . import pystards as _star
except ImportError:  # pragma: no cover
    import pystards as _star

from ._dtypes import as_supported_numpy
import numpy as np
from typing import Tuple, Union


# NumPy dtype -> (C++ from_numpy helper, C++ to_numpy helper). Numeric only;
# strings (object arrays) are handled separately since they can't use the
# zero-copy buffer protocol.
_NUMERIC_CONVERTERS = {
    np.dtype("int8"):    (_star.ndarray_from_numpy_int8,    _star.ndarray_to_numpy_int8),
    np.dtype("int16"):   (_star.ndarray_from_numpy_int16,   _star.ndarray_to_numpy_int16),
    np.dtype("int32"):   (_star.ndarray_from_numpy_int32,   _star.ndarray_to_numpy_int32),
    np.dtype("int64"):   (_star.ndarray_from_numpy_int64,   _star.ndarray_to_numpy_int64),
    np.dtype("uint8"):   (_star.ndarray_from_numpy_uint8,   _star.ndarray_to_numpy_uint8),
    np.dtype("uint16"):  (_star.ndarray_from_numpy_uint16,  _star.ndarray_to_numpy_uint16),
    np.dtype("uint32"):  (_star.ndarray_from_numpy_uint32,  _star.ndarray_to_numpy_uint32),
    np.dtype("uint64"):  (_star.ndarray_from_numpy_uint64,  _star.ndarray_to_numpy_uint64),
    np.dtype("float32"): (_star.ndarray_from_numpy_float32, _star.ndarray_to_numpy_float32),
    np.dtype("float64"): (_star.ndarray_from_numpy_float64, _star.ndarray_to_numpy_float64),
}

# C++ NDArray class name -> to_numpy helper (for to_numpy(), which starts from a
# C++ instance rather than a NumPy dtype).
_TONUMPY_BY_CPP = {
    "NDArrayInt8":    _star.ndarray_to_numpy_int8,
    "NDArrayInt16":   _star.ndarray_to_numpy_int16,
    "NDArrayInt32":   _star.ndarray_to_numpy_int32,
    "NDArrayInt64":   _star.ndarray_to_numpy_int64,
    "NDArrayUInt8":   _star.ndarray_to_numpy_uint8,
    "NDArrayUInt16":  _star.ndarray_to_numpy_uint16,
    "NDArrayUInt32":  _star.ndarray_to_numpy_uint32,
    "NDArrayUInt64":  _star.ndarray_to_numpy_uint64,
    "NDArrayFloat32": _star.ndarray_to_numpy_float32,
    "NDArrayFloat64": _star.ndarray_to_numpy_float64,
}


class NDArray:
    """Unified wrapper for C++ NDArray template instantiations."""

    def __init__(self, cpp_array):
        self._impl = cpp_array

    @staticmethod
    def from_numpy(arr) -> "NDArray":
        """Create an NDArray from a NumPy array, Python list, scalar, or string."""
        arr = as_supported_numpy(arr)

        if arr.dtype == object:  # string array
            shape = list(arr.shape)
            cpp = _star.NDArrayString(shape)
            data_vec = cpp.data()
            for i, val in enumerate(arr.flatten()):
                data_vec[i] = str(val)
            return NDArray(cpp)

        return NDArray(_NUMERIC_CONVERTERS[arr.dtype][0](arr))

    def to_numpy(self) -> np.ndarray:
        """Convert to a NumPy array (single memcpy for numeric; element copy for strings)."""
        func = _TONUMPY_BY_CPP.get(type(self._impl).__name__)
        if func is not None:
            return func(self._impl)
        # String array: no buffer protocol.
        cpp_data = self._impl.data()
        data = [cpp_data[i] for i in range(len(cpp_data))]
        return np.array(data, dtype=object).reshape(self.shape)

    @property
    def shape(self) -> Tuple[int, ...]:
        return tuple(self._impl.shape())

    @property
    def ndim(self) -> int:
        return len(self.shape)

    @property
    def size(self) -> int:
        return len(self._impl.data())

    def __len__(self) -> int:
        return self.shape[0] if self.shape else 0

    def __repr__(self) -> str:
        return f"NDArray(shape={self.shape}, dtype={type(self._impl).__name__})"


def zeros(shape: Union[int, Tuple[int, ...]], dtype=np.float64) -> NDArray:
    """Create an array of zeros."""
    if isinstance(shape, int):
        shape = (shape,)
    return NDArray.from_numpy(np.zeros(shape, dtype=dtype))


def ones(shape: Union[int, Tuple[int, ...]], dtype=np.float64) -> NDArray:
    """Create an array of ones."""
    if isinstance(shape, int):
        shape = (shape,)
    return NDArray.from_numpy(np.ones(shape, dtype=dtype))


def arange(start: float, stop: float = None, step: float = 1, dtype=None) -> NDArray:
    """Create an array with evenly spaced values."""
    if stop is None:
        stop, start = start, 0
    return NDArray.from_numpy(np.arange(start, stop, step, dtype=dtype))


def full(shape: Union[int, Tuple[int, ...]], fill_value: float, dtype=None) -> NDArray:
    """Create an array filled with a constant value."""
    if isinstance(shape, int):
        shape = (shape,)
    return NDArray.from_numpy(np.full(shape, fill_value, dtype=dtype))


__all__ = ["NDArray", "zeros", "ones", "arange", "full"]
