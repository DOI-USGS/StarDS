"""Metadata value wrapper for STAR."""
try:
    from . import pystards as _star
except ImportError:  # pragma: no cover
    import pystards as _star

import numpy as np


class MetadataValue:
    """Wrapper for a C++ MetadataValue with NumPy conversion."""

    def __init__(self, meta_value):
        self._meta = meta_value

    @property
    def dtype(self):
        """Get the STAR DataType."""
        return self._meta.dtype

    @property
    def shape(self):
        """Get shape as a tuple."""
        return tuple(self._meta.shape)

    @property
    def ndim(self) -> int:
        """Number of dimensions."""
        return self._meta.ndim()

    @property
    def size(self) -> int:
        """Total number of elements."""
        return self._meta.size()

    def is_scalar(self) -> bool:
        return self._meta.is_scalar()

    def is_array(self) -> bool:
        return self._meta.is_array()

    def to_numpy(self) -> np.ndarray:
        """Convert to a NumPy array.

        The element-type switch happens in C++ (star_meta_to_numpy): numeric
        types round-trip via the zero-copy buffer protocol, strings via an
        object array.
        """
        return _star.star_meta_to_numpy(self._meta)


__all__ = ["MetadataValue"]
