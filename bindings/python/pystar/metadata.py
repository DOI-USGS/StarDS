"""Metadata wrapper for STAR"""
try:
    from . import pystar as _star
except ImportError:
    import pystar as _star

from .enums import DataType
import numpy as np
from typing import Union


# Map DataType to NumPy dtype
DTYPE_MAP = {
    DataType.INT8: np.int8,
    DataType.INT16: np.int16,
    DataType.INT32: np.int32,
    DataType.INT64: np.int64,
    DataType.UINT8: np.uint8,
    DataType.UINT16: np.uint16,
    DataType.UINT32: np.uint32,
    DataType.UINT64: np.uint64,
    DataType.FLOAT32: np.float32,
    DataType.FLOAT64: np.float64,
    DataType.STRING: object,
}


class MetadataValue:
    """Wrapper for C++ MetadataValue with NumPy conversion"""

    def __init__(self, meta_value):
        """
        Wrap a C++ MetadataValue.

        Args:
            meta_value: C++ MetadataValue object from _star
        """
        self._meta = meta_value

    @property
    def dtype(self):
        """Get DataType"""
        return self._meta.dtype

    @property
    def shape(self):
        """Get shape as tuple"""
        return tuple(self._meta.shape)

    @property
    def ndim(self):
        """Get number of dimensions"""
        return self._meta.ndim()

    @property
    def size(self):
        """Get total number of elements"""
        return self._meta.size()

    def is_scalar(self):
        """Check if this is a scalar value"""
        return self._meta.is_scalar()

    def is_array(self):
        """Check if this is an array"""
        return self._meta.is_array()

    def to_numpy(self) -> np.ndarray:
        """
        Convert to NumPy array using efficient buffer protocol.

        Returns:
            np.ndarray: NumPy array with appropriate dtype
        """
        # Map dtype to (as_method, conversion_func)
        dtype_to_converter = {
            DataType.INT8: (self._meta.as_int8, _star.ndarray_to_numpy_int8),
            DataType.INT16: (self._meta.as_int16, _star.ndarray_to_numpy_int16),
            DataType.INT32: (self._meta.as_int32, _star.ndarray_to_numpy_int32),
            DataType.INT64: (self._meta.as_int64, _star.ndarray_to_numpy_int64),
            DataType.UINT8: (self._meta.as_uint8, _star.ndarray_to_numpy_uint8),
            DataType.UINT16: (self._meta.as_uint16, _star.ndarray_to_numpy_uint16),
            DataType.UINT32: (self._meta.as_uint32, _star.ndarray_to_numpy_uint32),
            DataType.UINT64: (self._meta.as_uint64, _star.ndarray_to_numpy_uint64),
            DataType.FLOAT32: (self._meta.as_float32, _star.ndarray_to_numpy_float32),
            DataType.FLOAT64: (self._meta.as_float64, _star.ndarray_to_numpy_float64),
        }

        converter = dtype_to_converter.get(self.dtype)
        if converter is not None:
            as_method, numpy_func = converter
            cpp_arr = as_method()
            return numpy_func(cpp_arr)

        # Fallback for strings (no buffer protocol support)
        if self.dtype == DataType.STRING:
            cpp_arr = self._meta.as_string()
            cpp_data = cpp_arr.data()
            data = [cpp_data[i] for i in range(len(cpp_data))]
            arr = np.array(data, dtype=object)
            return arr.reshape(self.shape)

        raise ValueError(f"Unsupported dtype: {self.dtype}")


__all__ = ['MetadataValue']
