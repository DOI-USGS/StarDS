"""Pythonic wrapper for StarDataset.

Type dispatch (NumPy dtype <-> C++ element type) is done in C++ via the generic
helpers in swig/dispatch.i (star_put / star_get / star_get_slice / star_put_meta
/ star_meta_to_numpy / star_layer_*). This module is therefore thin: it adds the
Pythonic sugar (dict-style access, context manager, namespace separation) and
delegates the actual work to those helpers, with no per-element-type branching.
"""
try:
    from . import pystards as _star
    from . import _pystards as _pystar
except ImportError:  # pragma: no cover
    import pystards as _star
    import _pystards as _pystar

import numpy as np
from typing import List, Dict, Any, Union

from ._dtypes import as_supported_numpy
from .metadata import MetadataValue


def _to_python_scalar(arr: np.ndarray):
    """Collapse a 0-d / 1-element metadata array to a native Python scalar."""
    if arr.ndim == 0:
        value = arr.item()
        if isinstance(value, (np.str_, np.bytes_)):
            return str(value)
        return value
    if arr.shape == (1,) and arr.dtype == np.object_ and isinstance(arr[0], str):
        return arr[0]
    return arr


class MetadataAccessor:
    """Dictionary-like access to the metadata namespace: ``ds.meta["key"] = value``.

    Works for both the base ``MetadataAccessor`` and a layer's
    ``LayerMetadataAccessor`` (the C++ ``star_layer_put_meta`` variant is used for
    the latter).
    """

    def __init__(self, cpp_meta, parent_dataset, owner=None):
        self._cpp_meta = cpp_meta
        self._parent = parent_dataset
        # cpp_meta is a pointer to a C++ member (e.g. the LayerMetadataAccessor
        # inside a LayerView). SWIG does not keep that owner alive, so retain it
        # here — otherwise the owner is freed and cpp_meta dangles (use-after-free).
        self._owner = owner
        self._is_layer = type(cpp_meta).__name__ == "LayerMetadataAccessor"

    def __setitem__(self, key: str, value):
        arr = as_supported_numpy(value)
        if self._is_layer:
            _star.star_layer_put_meta(self._cpp_meta, key, arr)
        else:
            _star.star_put_meta(self._cpp_meta, key, arr)

    def __getitem__(self, key: str):
        meta = self._cpp_meta.get(key)
        if meta is None:
            raise KeyError(f"Key not found: {key}")
        return _to_python_scalar(MetadataValue(meta).to_numpy())

    def __contains__(self, key: str) -> bool:
        return self._cpp_meta.contains(key)

    def keys(self) -> List[str]:
        if self._is_layer:
            return list(self._cpp_meta.keys())
        return self._parent.get_metadata_keys()

    def __len__(self) -> int:
        # Keep len() consistent with keys()/membership. For a layer accessor
        # get_metadata_count() would report only the base-layer count.
        return len(self.keys())

    def __iter__(self):
        return iter(self.keys())

    def get(self, key: str):
        """Direct access to the C++ MetadataValue (or None if absent)."""
        return self._cpp_meta.get(key)

    def remove(self, key: str):
        self._cpp_meta.remove(key)

    def clear(self):
        self._cpp_meta.clear()


class LayerView:
    """View into a dataset layer, with inheritance from the base layer.

    Same API as :class:`StarDataset` for a single layer; keys not set in this
    layer fall back to the base.
    """

    def __init__(self, cpp_layer_view, parent_dataset):
        self._cpp_view = cpp_layer_view
        self._parent = parent_dataset
        # Pass cpp_layer_view as owner: cpp_layer_view.meta points into it, so the
        # accessor must keep the view alive to avoid a dangling C++ reference.
        self._meta_accessor = MetadataAccessor(
            cpp_layer_view.meta, parent_dataset, owner=cpp_layer_view)

    @property
    def meta(self):
        # Return a wrapper that keeps this LayerView alive for temporary access
        # patterns like ds.get_layer("l").meta["k"].
        accessor, layer_view = self._meta_accessor, self

        class _MetaRef:
            def __getitem__(self, key):
                return accessor[key]

            def __setitem__(self, key, value):
                accessor[key] = value

            def __contains__(self, key):
                return key in accessor

            def __len__(self):
                return len(accessor)

            def __iter__(self):
                return iter(accessor)

            def get(self, key):
                return accessor.get(key)

            def keys(self):
                return accessor.keys()

            def remove(self, key):
                accessor.remove(key)

        return _MetaRef()

    def __contains__(self, key: str) -> bool:
        return self._cpp_view.contains(key)

    def __getitem__(self, key: str) -> np.ndarray:
        return self.get(key)

    def __setitem__(self, key: str, value):
        self.put(key, value)

    def get(self, key: str) -> np.ndarray:
        try:
            return _star.star_layer_get(self._cpp_view, key)
        except RuntimeError:
            raise KeyError(f"Key not found: {key}")

    def put(self, key: str, value):
        _star.star_layer_put(self._cpp_view, key, as_supported_numpy(value))

    def keys(self) -> List[str]:
        return list(self._cpp_view.keys())

    def name(self) -> str:
        return self._cpp_view.name()

    def base(self):
        return self._parent


class StarDataset:
    """Persistent storage for N-dimensional arrays.

    Supports local files, HTTP (``/vsicurl/``), and S3 (``/vsis3/``).

    Examples:
        >>> with StarDataset.create("data.stards") as store:
        ...     store["matrix"] = np.random.rand(100, 100)
        ...     store.meta["units"] = "meters"
        >>> store = StarDataset.open("/vsis3/bucket/data.stards", mode="r")
        >>> keys = store.keys()
    """

    def __init__(self, filename: str, mode: str = "rw", options=None):
        self._mode = mode
        if options is None:
            self._store = _star.StarDataset.open(filename, mode)
        else:
            self._store = _star.StarDataset.open(filename, mode, options)
        self.meta = MetadataAccessor(self._store.meta, self)

    @classmethod
    def create(cls, filename: str, config=None):
        """Create a new STAR dataset file."""
        instance = cls.__new__(cls)
        instance._mode = "rw"
        if config is None:
            instance._store = _star.StarDataset.create(filename)
        else:
            instance._store = _star.StarDataset.create(filename, config)
        instance.meta = MetadataAccessor(instance._store.meta, instance)
        return instance

    @classmethod
    def open(cls, filename: str, mode: str = "rw", options=None):
        """Open an existing STAR dataset file.

        Args:
            filename: Path to the ``.stards`` file.
            mode: Open mode (``"r"``, ``"rw"``, ``"w"``).
            options: Optional ``pystards.OpenOptions`` controlling read-time
                behavior such as ``layer_inheritance`` (off by default).
        """
        instance = cls.__new__(cls)
        instance._mode = mode
        if options is None:
            instance._store = _star.StarDataset.open(filename, mode)
        else:
            instance._store = _star.StarDataset.open(filename, mode, options)
        instance.meta = MetadataAccessor(instance._store.meta, instance)
        return instance

    def __enter__(self):
        return self

    def __exit__(self, *args):
        # Query actual state instead of comparing the raw mode string: aliases
        # like "read" also open read-only, and flush() throws on a read-only store.
        if not self.is_read_only():
            self.flush()

    # --- Array namespace -----------------------------------------------------
    def put(self, key: str, value):
        """Store an array (stored separately, supports slicing)."""
        _star.star_put(self._store, key, as_supported_numpy(value))

    def get(self, key: str) -> np.ndarray:
        """Retrieve an array as a NumPy array."""
        try:
            return _star.star_get(self._store, key)
        except RuntimeError:
            raise KeyError(f"Key not found: {key}")

    def get_slice(self, key: str, slices: List[tuple]) -> np.ndarray:
        """Get a slice of a large array.

        Args:
            key: Array key
            slices: List of ``(start, stop[, step])`` tuples, one per dimension.
        """
        cpp_slices = _star.VectorSlice()
        for s in slices:
            slice_obj = _star.Slice()
            slice_obj.start = s[0]
            slice_obj.stop = s[1]
            slice_obj.step = s[2] if len(s) > 2 else 1
            cpp_slices.append(slice_obj)
        return _star.star_get_slice(self._store, key, cpp_slices)

    def put_metadata(self, key: str, value):
        """Store a value in the metadata block (equivalent to ``ds.meta[key] = value``)."""
        self.meta[key] = value

    # --- Keys / iteration ----------------------------------------------------
    def keys(self) -> List[str]:
        """All array keys (excludes internal layer-prefixed keys)."""
        keys_tuple = self._store.get_all_keys()
        filtered = [k for k in keys_tuple if not k.startswith("__layer_")]
        return list(dict.fromkeys(filtered))

    def __len__(self) -> int:
        return len(self.keys())

    def __contains__(self, key: str) -> bool:
        # C++ contains() checks both the array and metadata namespaces.
        return self._store.contains(key)

    def __iter__(self):
        return iter(self.keys())

    def __getitem__(self, key: str) -> np.ndarray:
        return self.get(key)

    def __setitem__(self, key: str, value):
        """Store an array via dict syntax (separate storage, supports slicing).

        Raw Python scalars/strings are rejected -- use a NumPy array, or
        ``ds.meta[...]`` for scalar/string metadata.
        """
        if isinstance(value, (int, float, bool, np.number)):
            raise TypeError(
                f"Cannot store raw Python scalar. Either:\n"
                f"  • Use NumPy array: ds['{key}'] = np.array({value})\n"
                f"  • Use metadata: ds.meta['{key}'] = {value}  (recommended for scalars)"
            )
        if isinstance(value, str):
            raise TypeError(
                f"Cannot store raw Python string. Either:\n"
                f"  • Use array: ds['{key}'] = ['{value}']\n"
                f"  • Use metadata: ds.meta['{key}'] = '{value}'  (recommended for strings)"
            )
        self.put(key, value)

    # --- Misc / persistence --------------------------------------------------
    def flush(self):
        """Write pending changes to disk."""
        self._store.flush()

    def close(self):
        """Flush pending writes to disk (dataset remains usable)."""
        if self._store is not None:
            self._store.close()

    def is_sliceable(self, key: str) -> bool:
        """Return True if the array can be read in slices (see :meth:`get_slice`)."""
        return self._store.is_sliceable(key)

    @property
    def filename(self) -> str:
        """Path this dataset was opened from."""
        return self._store.getFilename()

    def is_metadata_loaded(self) -> bool:
        """Return True if the metadata block has been read into memory."""
        return self._store.is_metadata_loaded()

    def is_read_only(self) -> bool:
        """Return True if the dataset was opened read-only."""
        return self._store.isReadOnly()

    def save_to(self, target_path: str):
        """Save the dataset to a different file (e.g. read-only -> writable, or local <-> S3)."""
        self._store.saveTo(target_path)

    def get_file_header(self):
        return self._store.getFileHeader()

    # --- Metadata queries ----------------------------------------------------
    def get_metadata_keys(self) -> List[str]:
        return list(self._store.get_metadata_keys())

    def get_metadata_count(self) -> int:
        return self._store.get_metadata_count()

    def get_all_metadata(self) -> Dict[str, Any]:
        """All metadata entries as a dict of ``{key: np.ndarray}``."""
        result = {}
        for key in self.get_metadata_keys():
            meta = self._store.meta.get(key)
            if meta is not None:
                result[key] = MetadataValue(meta).to_numpy()
        return result

    def remove_metadata(self, key: str):
        self._store.meta.remove(key)

    def clear_metadata(self):
        self._store.meta.clear()

    # --- Layer management ----------------------------------------------------
    def get_layer(self, layer_name: str):
        """Get an existing layer view (raises RuntimeError if it doesn't exist)."""
        cpp_layer = _pystar.StarDataset_get_layer(self._store, layer_name)
        return LayerView(cpp_layer, self)

    def create_layer(self, layer_name: str):
        """Create a new layer and return its view."""
        cpp_layer = _pystar.StarDataset_create_layer(self._store, layer_name)
        return LayerView(cpp_layer, self)

    def has_layer(self, layer_name: str) -> bool:
        return _pystar.StarDataset_has_layer(self._store, layer_name)

    def list_layers(self) -> List[str]:
        return list(_pystar.StarDataset_list_layers(self._store))

    # --- Layer inheritance (read-time behavior) ------------------------------
    def set_layer_inheritance(self, on: bool) -> None:
        """Enable or disable base-layer inheritance for layer lookups.

        Inheritance is off by default: a key absent from a layer is reported as
        missing rather than falling back to the base layer. This is a read-time
        setting (it changes in-memory behavior only, never the file) and can be
        toggled at any time after opening.
        """
        self._store.setLayerInheritance(on)

    def layer_inheritance(self) -> bool:
        """Return whether base-layer inheritance is currently enabled."""
        return self._store.layerInheritance()


__all__ = ["StarDataset", "LayerView"]
