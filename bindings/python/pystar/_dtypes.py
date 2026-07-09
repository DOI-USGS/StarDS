"""Canonical NumPy <-> STAR DataType mapping.

Single source of truth for the type table that used to be duplicated across
dataset.py, ndarray.py and metadata.py. Everything type-related keys off this.

The heavy per-type dispatch (NumPy array <-> C++ NDArray<T>, and put/get for
every element type) now lives in C++ (see swig/dispatch.i), reached through the
generic ``star_put`` / ``star_get`` / ``star_meta_to_numpy`` helpers. This module
only needs the small amount of mapping that remains Python-side.
"""
try:
    from . import pystar as _star
except ImportError:  # pragma: no cover - direct (non-package) import fallback
    import pystar as _star

import numpy as np

# DataType enum value -> NumPy dtype. STRING maps to object arrays (Python str).
DATATYPE_TO_NUMPY = {
    _star.DataType_INT8:    np.int8,
    _star.DataType_INT16:   np.int16,
    _star.DataType_INT32:   np.int32,
    _star.DataType_INT64:   np.int64,
    _star.DataType_UINT8:   np.uint8,
    _star.DataType_UINT16:  np.uint16,
    _star.DataType_UINT32:  np.uint32,
    _star.DataType_UINT64:  np.uint64,
    _star.DataType_FLOAT32: np.float32,
    _star.DataType_FLOAT64: np.float64,
    _star.DataType_STRING:  object,
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

    # Any unsupported numeric dtype -> float64.
    if value.dtype != object and value.dtype not in (
        np.dtype("int8"), np.dtype("int16"), np.dtype("int32"), np.dtype("int64"),
        np.dtype("uint8"), np.dtype("uint16"), np.dtype("uint32"), np.dtype("uint64"),
        np.dtype("float32"), np.dtype("float64"),
    ):
        value = value.astype(np.float64)

    # Contiguity: required by the C++ buffer path. 0-d arrays are already
    # contiguous; leave their shape untouched.
    if value.ndim > 0:
        value = np.ascontiguousarray(value)
    return value
