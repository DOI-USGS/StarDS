"""Canonical NumPy <-> STAR DataType mapping.

Single source of truth for the type table that used to be duplicated across
dataset.py, ndarray.py and metadata.py. Everything type-related keys off this.

The heavy per-type dispatch (NumPy array <-> C++ NDArray<T>, and put/get for
every element type) now lives in C++ (see swig/dispatch.i), reached through the
generic ``star_put`` / ``star_get`` / ``star_meta_to_numpy`` helpers. This module
only needs the small amount of mapping that remains Python-side.
"""
import warnings

import numpy as np

# STAR-supported numeric dtypes as (kind, itemsize) pairs, so the check is
# independent of byte order.
_SUPPORTED_NUMERIC = {
    ("i", 1), ("i", 2), ("i", 4), ("i", 8),
    ("u", 1), ("u", 2), ("u", 4), ("u", 8),
    ("f", 4), ("f", 8),
}


def as_supported_numpy(value):
    """Coerce a Python value to a NumPy array with a STAR-supported dtype.

    Accepts NumPy arrays, Python scalars, lists/tuples, and strings. Strings and
    string sequences become ``object`` arrays (handled element-wise in C++);
    everything else keeps its numeric dtype, falling back to float64 for dtypes
    STAR doesn't support.
    """
    if not isinstance(value, np.ndarray):
        if isinstance(value, str):
            value = np.array(value, dtype=object)
        elif isinstance(value, (list, tuple)):
            if len(value) > 0 and isinstance(value[0], str):
                value = np.array(value, dtype=object)
            else:
                value = np.array(value)
        else:
            value = np.array(value)

    # Fixed-width string dtypes -> object (C++ side handles object arrays).
    if value.dtype.kind in ("U", "S"):
        value = value.astype(object)

    # Supported numeric types keyed by (kind, itemsize) so byte order is ignored;
    # a big-endian int64 is still an int64. Unsupported numeric dtypes -> float64.
    if value.dtype != object:
        if (value.dtype.kind, value.dtype.itemsize) in _SUPPORTED_NUMERIC:
            native = value.dtype.newbyteorder("=")
            if value.dtype != native:
                value = value.astype(native)
        else:
            # Complex loses the imaginary part on this cast; warn explicitly.
            if value.dtype.kind == "c":
                warnings.warn(
                    f"STAR does not support {value.dtype}; coercing to float64 "
                    "and discarding the imaginary part.",
                    stacklevel=2,
                )
            value = value.astype(np.float64)

    # Contiguity: required by the C++ buffer path. 0-d arrays are already
    # contiguous; leave their shape untouched.
    if value.ndim > 0:
        value = np.ascontiguousarray(value)
    return value
