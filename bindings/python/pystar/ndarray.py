"""NumPy-compatible ndarray wrapper"""
try:
    from . import pystar as _star
except ImportError:
    import pystar as _star

from .enums import DataType
import numpy as np
from typing import Tuple, Union, List


# Map NumPy dtype to DataType, ndarray constructor, and vector type
DTYPE_TO_NDARRAY = {
    np.dtype('int8'): (DataType.INT8, _star.NDArrayInt8, _star.VectorInt8),
    np.dtype('int16'): (DataType.INT16, _star.NDArrayInt16, _star.VectorInt16),
    np.dtype('int32'): (DataType.INT32, _star.NDArrayInt32, _star.VectorInt32),
    np.dtype('int64'): (DataType.INT64, _star.NDArrayInt64, _star.VectorInt64),
    np.dtype('uint8'): (DataType.UINT8, _star.NDArrayUInt8, _star.VectorUInt8),
    np.dtype('uint16'): (DataType.UINT16, _star.NDArrayUInt16, _star.VectorUInt16),
    np.dtype('uint32'): (DataType.UINT32, _star.NDArrayUInt32, _star.VectorUInt32),
    np.dtype('uint64'): (DataType.UINT64, _star.NDArrayUInt64, _star.VectorUInt64),
    np.dtype('float32'): (DataType.FLOAT32, _star.NDArrayFloat32, _star.VectorFloat32),
    np.dtype('float64'): (DataType.FLOAT64, _star.NDArrayFloat64, _star.VectorFloat64),
    np.dtype('object'): (DataType.STRING, _star.NDArrayString, _star.VectorString),
}


class NDArray:
    """
    Unified wrapper for C++ ndarray template instantiations.

    Provides a NumPy-like interface while hiding template complexity.
    """

    def __init__(self, cpp_array):
        """
        Wrap a C++ ndarray instance.

        Args:
            cpp_array: C++ ndarray instance (any type)
        """
        self._impl = cpp_array

    @staticmethod
    def from_numpy(arr) -> 'NDArray':
        """
        Create NDArray from NumPy array, Python list, or string.

        Args:
            arr: NumPy array, Python list, tuple, or string

        Returns:
            NDArray: Wrapped C++ ndarray
        """
        # Handle Python lists/scalars by converting to NumPy first
        if not isinstance(arr, np.ndarray):
            if isinstance(arr, str):
                # Single string -> 1-element array
                arr = np.array([arr], dtype=object)
            elif isinstance(arr, (list, tuple)):
                # Try to infer dtype from first element
                if len(arr) > 0 and isinstance(arr[0], str):
                    arr = np.array(arr, dtype=object)
                else:
                    arr = np.array(arr)
            else:
                # Scalar
                arr = np.array(arr)

        # Convert string dtypes to object
        if arr.dtype.kind in ('U', 'S'):  # Unicode or byte strings
            arr = arr.astype(object)

        # For strings, use element-by-element copy (no buffer protocol)
        if arr.dtype == np.dtype('object'):
            # Ensure contiguous for iteration
            arr = np.ascontiguousarray(arr)

            # Create C++ NDArrayString
            shape = list(arr.shape)
            cpp_ndarray = _star.NDArrayString(shape)

            # Copy strings element-by-element using data() vector access
            flat_arr = arr.flatten()
            data_vec = cpp_ndarray.data()
            for i, val in enumerate(flat_arr):
                data_vec[i] = str(val)

            return NDArray(cpp_ndarray)

        # For numeric types, use fast buffer protocol
        # Ensure contiguous array
        arr = np.ascontiguousarray(arr)

        # Map NumPy dtype to buffer protocol function
        dtype_to_func = {
            np.dtype('int8'): _star.ndarray_from_numpy_int8,
            np.dtype('int16'): _star.ndarray_from_numpy_int16,
            np.dtype('int32'): _star.ndarray_from_numpy_int32,
            np.dtype('int64'): _star.ndarray_from_numpy_int64,
            np.dtype('uint8'): _star.ndarray_from_numpy_uint8,
            np.dtype('uint16'): _star.ndarray_from_numpy_uint16,
            np.dtype('uint32'): _star.ndarray_from_numpy_uint32,
            np.dtype('uint64'): _star.ndarray_from_numpy_uint64,
            np.dtype('float32'): _star.ndarray_from_numpy_float32,
            np.dtype('float64'): _star.ndarray_from_numpy_float64,
        }

        func = dtype_to_func.get(arr.dtype)
        if func is None:
            # Try to cast to float64 as fallback
            arr = arr.astype(np.float64)
            func = _star.ndarray_from_numpy_float64

        # Single memcpy conversion (eliminates 2-3 intermediate copies)
        cpp_arr = func(arr)
        return NDArray(cpp_arr)

    def to_numpy(self) -> np.ndarray:
        """
        Convert to NumPy array using zero-copy buffer protocol.

        Returns:
            np.ndarray: NumPy array with data copied from C++
        """
        # Map C++ type to buffer protocol function
        cpp_type = type(self._impl).__name__

        type_to_func = {
            'NDArrayInt8': _star.ndarray_to_numpy_int8,
            'NDArrayInt16': _star.ndarray_to_numpy_int16,
            'NDArrayInt32': _star.ndarray_to_numpy_int32,
            'NDArrayInt64': _star.ndarray_to_numpy_int64,
            'NDArrayUInt8': _star.ndarray_to_numpy_uint8,
            'NDArrayUInt16': _star.ndarray_to_numpy_uint16,
            'NDArrayUInt32': _star.ndarray_to_numpy_uint32,
            'NDArrayUInt64': _star.ndarray_to_numpy_uint64,
            'NDArrayFloat32': _star.ndarray_to_numpy_float32,
            'NDArrayFloat64': _star.ndarray_to_numpy_float64,
        }

        func = type_to_func.get(cpp_type)
        if func is None:
            # Fallback for string arrays (not supported by buffer protocol)
            if cpp_type == 'NDArrayString':
                cpp_data = self._impl.data()
                data = [cpp_data[i] for i in range(len(cpp_data))]
                arr = np.array(data, dtype=object)
                return arr.reshape(self.shape)
            else:
                raise ValueError(f"Unsupported array type: {cpp_type}")

        # Single memcpy conversion (eliminates element-by-element boundary crossings)
        return func(self._impl)

    @property
    def shape(self) -> Tuple[int, ...]:
        """Get array shape"""
        return tuple(self._impl.shape())

    @property
    def ndim(self) -> int:
        """Get number of dimensions"""
        return len(self.shape)

    @property
    def size(self) -> int:
        """Get total number of elements"""
        return len(self._impl.data())

    def __len__(self) -> int:
        """Get length (first dimension size)"""
        return self.shape[0] if self.shape else 0

    def __repr__(self) -> str:
        return f"NDArray(shape={self.shape}, dtype={type(self._impl).__name__})"


def zeros(shape: Union[int, Tuple[int, ...]], dtype=np.float64) -> NDArray:
    """
    Create an array of zeros.

    Args:
        shape: Shape of the array
        dtype: NumPy dtype (default: np.float64)

    Returns:
        NDArray: Array filled with zeros
    """
    if isinstance(shape, int):
        shape = (shape,)
    arr = np.zeros(shape, dtype=dtype)
    return NDArray.from_numpy(arr)


def ones(shape: Union[int, Tuple[int, ...]], dtype=np.float64) -> NDArray:
    """
    Create an array of ones.

    Args:
        shape: Shape of the array
        dtype: NumPy dtype (default: np.float64)

    Returns:
        NDArray: Array filled with ones
    """
    if isinstance(shape, int):
        shape = (shape,)
    arr = np.ones(shape, dtype=dtype)
    return NDArray.from_numpy(arr)


def arange(start: float, stop: float = None, step: float = 1, dtype=None) -> NDArray:
    """
    Create an array with evenly spaced values.

    Args:
        start: Start value (or stop if stop is None)
        stop: Stop value (optional)
        step: Step size (default: 1)
        dtype: NumPy dtype (optional)

    Returns:
        NDArray: Array with evenly spaced values
    """
    if stop is None:
        stop = start
        start = 0
    arr = np.arange(start, stop, step, dtype=dtype)
    return NDArray.from_numpy(arr)


def full(shape: Union[int, Tuple[int, ...]], fill_value: float, dtype=None) -> NDArray:
    """
    Create an array filled with a constant value.

    Args:
        shape: Shape of the array
        fill_value: Fill value
        dtype: NumPy dtype (optional)

    Returns:
        NDArray: Array filled with fill_value
    """
    if isinstance(shape, int):
        shape = (shape,)
    arr = np.full(shape, fill_value, dtype=dtype)
    return NDArray.from_numpy(arr)


__all__ = ['NDArray', 'zeros', 'ones', 'arange', 'full']
