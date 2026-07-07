"""Pythonic wrapper for StarDataset"""
try:
    from . import pystar as _star
    from . import _pystar
except ImportError:
    import pystar as _star
    import _pystar

from .enums import DataType, FileMode
import numpy as np
from typing import Optional, Union, List, Dict, Any
from .ndarray import NDArray
from .metadata import MetadataValue


class MetadataAccessor:
    """
    Pythonic wrapper for C++ MetadataAccessor.

    Provides dictionary-like access to metadata: ds.meta["key"] = value
    """

    def __init__(self, cpp_meta, parent_dataset):
        """
        Args:
            cpp_meta: C++ MetadataAccessor or LayerMetadataAccessor instance
            parent_dataset: Parent StarDataset instance (for put_metadata/get logic)
        """
        self._cpp_meta = cpp_meta
        self._parent = parent_dataset
        # Check if this is a LayerMetadataAccessor
        self._is_layer = type(cpp_meta).__name__ == 'LayerMetadataAccessor'

    def __setitem__(self, key: str, value: Union[np.ndarray, NDArray, list, str, int, float]):
        """Store value in metadata block: ds.meta["key"] = value"""
        if self._is_layer:
            # For LayerMetadataAccessor, call put methods directly on the C++ accessor
            self._put_via_cpp_accessor(key, value)
        else:
            # For base MetadataAccessor, use parent's put_metadata
            self._parent.put_metadata(key, value)

    def _put_via_cpp_accessor(self, key: str, value: Union[np.ndarray, NDArray, list, str, int, float]):
        """Store value using C++ accessor's put methods (for LayerMetadataAccessor)"""
        # Convert Python types to NDArray
        if isinstance(value, (list, str, int, float)):
            value = NDArray.from_numpy(value)
        elif isinstance(value, np.ndarray):
            value = NDArray.from_numpy(value)

        # Call appropriate C++ put method
        cpp_type = type(value._impl).__name__

        if cpp_type == 'NDArrayInt8':
            self._cpp_meta.put_int8(key, value._impl)
        elif cpp_type == 'NDArrayInt16':
            self._cpp_meta.put_int16(key, value._impl)
        elif cpp_type == 'NDArrayInt32':
            self._cpp_meta.put_int32(key, value._impl)
        elif cpp_type == 'NDArrayInt64':
            self._cpp_meta.put_int64(key, value._impl)
        elif cpp_type == 'NDArrayUInt8':
            self._cpp_meta.put_uint8(key, value._impl)
        elif cpp_type == 'NDArrayUInt16':
            self._cpp_meta.put_uint16(key, value._impl)
        elif cpp_type == 'NDArrayUInt32':
            self._cpp_meta.put_uint32(key, value._impl)
        elif cpp_type == 'NDArrayUInt64':
            self._cpp_meta.put_uint64(key, value._impl)
        elif cpp_type == 'NDArrayFloat32':
            self._cpp_meta.put_float32(key, value._impl)
        elif cpp_type == 'NDArrayFloat64':
            self._cpp_meta.put_float64(key, value._impl)
        elif cpp_type == 'NDArrayString':
            self._cpp_meta.put_string(key, value._impl)
        else:
            raise ValueError(f"Unsupported array type: {cpp_type}")

    def __getitem__(self, key: str):
        """
        Retrieve value from metadata block: value = ds.meta["key"]

        Returns native Python types for scalars:
        - Scalar string -> str
        - Scalar int -> int
        - Scalar float -> float
        - Arrays -> np.ndarray
        """
        meta = self._cpp_meta.get(key)
        if meta is None:
            raise KeyError(f"Key not found: {key}")

        arr = MetadataValue(meta).to_numpy()

        # Convert scalar arrays to native Python types
        if arr.ndim == 0:  # Scalar
            value = arr.item()
            # For string scalars, convert to str
            if isinstance(value, (np.str_, np.bytes_)):
                return str(value)
            return value

        # For 1-element arrays, check if it's a string and should be scalar
        if arr.shape == (1,) and arr.dtype == np.object_:
            # This might be a scalar string stored as 1-element array
            # Return as string if it's a single string
            if isinstance(arr[0], str):
                return arr[0]

        return arr

    def __contains__(self, key: str) -> bool:
        """Check if key exists: "key" in ds.meta"""
        return self._cpp_meta.contains(key)

    def keys(self) -> List[str]:
        """Get all metadata keys"""
        if self._is_layer:
            # For LayerMetadataAccessor, use C++ accessor's keys method
            return list(self._cpp_meta.keys())
        else:
            # For base MetadataAccessor, use parent's filtered method
            return self._parent.get_metadata_keys()

    def __len__(self) -> int:
        """Get count of metadata entries"""
        return self._parent.get_metadata_count()

    def __iter__(self):
        """Iterate over metadata keys: for key in ds.meta"""
        return iter(self.keys())

    # Expose C++ methods for direct access if needed
    def get(self, key: str):
        """Direct access to C++ get() method"""
        return self._cpp_meta.get(key)

    def put(self, key: str, value):
        """Direct access to C++ put() methods - use __setitem__ instead"""
        raise NotImplementedError("Use ds.meta['key'] = value instead")

    def remove(self, key: str):
        """Remove metadata entry"""
        self._cpp_meta.remove(key)

    def clear(self):
        """Clear all metadata"""
        self._cpp_meta.clear()


class LayerView:
    """
    View into a specific dataset layer with inheritance from base.

    Provides same API as StarDataset but operates on a single layer.
    Keys not found in this layer automatically fall back to the base layer.

    Example:
        >>> ds = StarDataset.create("file.star")
        >>> ds.meta["instrument"] = "AVIRIS"
        >>>
        >>> layer = ds.create_layer("band_0")
        >>> layer.meta["wavelength"] = 450.5  # Layer-specific
        >>> layer.meta["instrument"]  # "AVIRIS" (inherited from base)
    """

    def __init__(self, cpp_layer_view, parent_dataset):
        """
        Args:
            cpp_layer_view: C++ LayerView instance
            parent_dataset: Parent StarDataset instance
        """
        self._cpp_view = cpp_layer_view
        self._parent = parent_dataset
        # Store metadata accessor but don't expose directly
        # Use property to keep LayerView alive
        self._meta_accessor = MetadataAccessor(cpp_layer_view.meta, parent_dataset)

    @property
    def meta(self):
        """
        Access layer metadata with inheritance.

        This property ensures the LayerView stays alive even in temporary access patterns
        like: ds.get_layer("layer1").meta["key"]
        """
        # Return wrapper that keeps self alive
        class MetadataAccessorWithLayerRef:
            def __init__(self, accessor, layer_view):
                self._accessor = accessor
                self._layer_view = layer_view  # Keep LayerView alive

            def __getitem__(self, key):
                return self._accessor.__getitem__(key)

            def __setitem__(self, key, value):
                return self._accessor.__setitem__(key, value)

            def __contains__(self, key):
                return self._accessor.__contains__(key)

            def get(self, key, default=None):
                return self._accessor.get(key, default)

            def keys(self):
                return self._accessor.keys()

            def __repr__(self):
                return self._accessor.__repr__()

        return MetadataAccessorWithLayerRef(self._meta_accessor, self)

    def __contains__(self, key: str) -> bool:
        """Check if key exists in this layer or base"""
        return self._cpp_view.contains(key)

    def __getitem__(self, key: str) -> np.ndarray:
        """
        Get array from this layer (with inheritance from base).

        If key exists in this layer, returns the layer-specific value.
        Otherwise falls back to base layer.

        Args:
            key: Array key

        Returns:
            NumPy array

        Example:
            >>> layer = ds.create_layer("band_0")
            >>> data = layer["array_key"]  # Gets from layer or base
        """
        return self.get(key)

    def __setitem__(self, key: str, value: Union[np.ndarray, list, int, float, str]):
        """
        Store array in this layer.

        Marks the layer as having this key in the layer presence bitmap.
        Data is physically stored in the base dataset, but logically belongs to this layer.

        Args:
            key: Array key
            value: Data to store (NumPy array, list, scalar)

        Example:
            >>> layer = ds.create_layer("band_0")
            >>> layer.meta["wavelength"] = 450.5          # Layer-specific metadata
            >>> layer["calibration"] = np.array([1, 2, 3])  # Layer-specific data array
        """
        self.put(key, value)

    def get(self, key: str) -> np.ndarray:
        """
        Retrieve array from this layer (with inheritance from base).

        Args:
            key: Array key

        Returns:
            NumPy array
        """
        # Try each data type until one works, then fall back to metadata
        get_methods = [
            'get_float64', 'get_int64', 'get_float32', 'get_int32',
            'get_int16', 'get_int8', 'get_uint64', 'get_uint32',
            'get_uint16', 'get_uint8', 'get_string'
        ]

        for method_name in get_methods:
            try:
                result = getattr(self._cpp_view, method_name)(key)
                return NDArray(result).to_numpy()
            except (RuntimeError, AttributeError):
                continue

        # If no data array found, raise error
        # Note: Use layer.meta["key"] to access metadata
        raise KeyError(f"Key not found: {key}")

    def put(self, key: str, value: Union[np.ndarray, list, int, float, str]):
        """
        Store array in this layer.

        Args:
            key: Array key
            value: Data to store
        """
        # Convert Python types to NDArray
        if isinstance(value, (list, str, int, float)):
            value = NDArray.from_numpy(value)
        elif isinstance(value, np.ndarray):
            value = NDArray.from_numpy(value)

        # Determine the dtype and call appropriate LayerView put method
        cpp_type = type(value._impl).__name__

        if cpp_type == 'NDArrayInt8':
            self._cpp_view.put_int8(key, value._impl)
        elif cpp_type == 'NDArrayInt16':
            self._cpp_view.put_int16(key, value._impl)
        elif cpp_type == 'NDArrayInt32':
            self._cpp_view.put_int32(key, value._impl)
        elif cpp_type == 'NDArrayInt64':
            self._cpp_view.put_int64(key, value._impl)
        elif cpp_type == 'NDArrayUInt8':
            self._cpp_view.put_uint8(key, value._impl)
        elif cpp_type == 'NDArrayUInt16':
            self._cpp_view.put_uint16(key, value._impl)
        elif cpp_type == 'NDArrayUInt32':
            self._cpp_view.put_uint32(key, value._impl)
        elif cpp_type == 'NDArrayUInt64':
            self._cpp_view.put_uint64(key, value._impl)
        elif cpp_type == 'NDArrayFloat32':
            self._cpp_view.put_float32(key, value._impl)
        elif cpp_type == 'NDArrayFloat64':
            self._cpp_view.put_float64(key, value._impl)
        elif cpp_type == 'NDArrayString':
            self._cpp_view.put_string(key, value._impl)
        else:
            raise ValueError(f"Unsupported array type: {cpp_type}")

    def keys(self) -> List[str]:
        """Get all keys (local + inherited)"""
        return list(self._cpp_view.keys())

    def name(self) -> str:
        """Get layer name"""
        return self._cpp_view.name()

    def base(self):
        """Get parent dataset"""
        return self._parent


class StarDataset:
    """
    Persistent storage for N-dimensional arrays.

    Supports local files, HTTP (/vsicurl/), and S3 (/vsis3/).

    Examples:
        >>> with StarDataset("data.star") as store:
        ...     store.put("matrix", np.random.rand(100, 100))
        ...     data = store.get("matrix")

        >>> store = StarDataset("/vsis3/bucket/data.star", mode="r")
        >>> keys = store.keys()
    """

    def __init__(self, filename: str, mode: str = "rw"):
        """
        Open or create a STAR file.

        Args:
            filename: Path to file (local, /vsicurl/, /vsis3/)
            mode: "rw" for read-write, "r" for read-only, "w" for write, "a" for append
        """
        self._mode = mode
        # Use the open static factory method
        # SWIG exposes static methods as class methods
        self._store = _star.StarDataset.open(filename, mode)
        # Wrap the C++ MetadataAccessor with Python wrapper for ds.meta["key"] = value
        self.meta = MetadataAccessor(self._store.meta, self)

    @classmethod
    def create(cls, filename: str, config=None):
        """
        Create a new STAR dataset file.

        Args:
            filename: Path to file (local or /vsis3/)
            config: Optional StarConfig object

        Returns:
            StarDataset instance
        """
        instance = cls.__new__(cls)
        instance._mode = "rw"
        if config is None:
            instance._store = _star.StarDataset.create(filename)
        else:
            instance._store = _star.StarDataset.create(filename, config)
        # Wrap the C++ MetadataAccessor
        instance.meta = MetadataAccessor(instance._store.meta, instance)
        return instance

    @classmethod
    def open(cls, filename: str, mode: str = "rw"):
        """
        Open an existing STAR dataset file.

        Args:
            filename: Path to file (local, /vsicurl/, /vsis3/)
            mode: "rw" for read-write, "r" for read-only

        Returns:
            StarDataset instance
        """
        instance = cls.__new__(cls)
        instance._mode = mode
        instance._store = _star.StarDataset.open(filename, mode)
        # Wrap the C++ MetadataAccessor
        instance.meta = MetadataAccessor(instance._store.meta, instance)
        return instance

    def __enter__(self):
        return self

    def __exit__(self, *args):
        if self._mode != "r":
            self.flush()

    def put(self, key: str, value: Union[np.ndarray, NDArray, list, str, int, float]):
        """Store an array (stored separately, not in metadata block)"""
        # Convert Python types to NDArray
        if isinstance(value, (list, str, int, float)):
            value = NDArray.from_numpy(value)
        elif isinstance(value, np.ndarray):
            value = NDArray.from_numpy(value)

        # Determine the dtype and call appropriate put_array method
        # These methods store arrays separately (not in metadata block) for slicing support
        cpp_type = type(value._impl).__name__

        if cpp_type == 'NDArrayInt8':
            self._store.put_array_int8(key, value._impl)
        elif cpp_type == 'NDArrayInt16':
            self._store.put_array_int16(key, value._impl)
        elif cpp_type == 'NDArrayInt32':
            self._store.put_array_int32(key, value._impl)
        elif cpp_type == 'NDArrayInt64':
            self._store.put_array_int64(key, value._impl)
        elif cpp_type == 'NDArrayUInt8':
            self._store.put_array_uint8(key, value._impl)
        elif cpp_type == 'NDArrayUInt16':
            self._store.put_array_uint16(key, value._impl)
        elif cpp_type == 'NDArrayUInt32':
            self._store.put_array_uint32(key, value._impl)
        elif cpp_type == 'NDArrayUInt64':
            self._store.put_array_uint64(key, value._impl)
        elif cpp_type == 'NDArrayFloat32':
            self._store.put_array_float32(key, value._impl)
        elif cpp_type == 'NDArrayFloat64':
            self._store.put_array_float64(key, value._impl)
        elif cpp_type == 'NDArrayString':
            self._store.put_array_string(key, value._impl)
        else:
            raise ValueError(f"Unsupported array type: {cpp_type}")

    def put_metadata(self, key: str, value: Union[np.ndarray, NDArray, list, str, int, float]):
        """Store an array in metadata block (for small arrays/scalars)"""
        # Convert Python types to NDArray
        if isinstance(value, (list, str, int, float)):
            value = NDArray.from_numpy(value)
        elif isinstance(value, np.ndarray):
            value = NDArray.from_numpy(value)

        # Determine the dtype and call appropriate meta.put method
        # These methods store in metadata block
        cpp_type = type(value._impl).__name__

        if cpp_type == 'NDArrayInt8':
            self._store.meta.put_int8(key, value._impl)
        elif cpp_type == 'NDArrayInt16':
            self._store.meta.put_int16(key, value._impl)
        elif cpp_type == 'NDArrayInt32':
            self._store.meta.put_int32(key, value._impl)
        elif cpp_type == 'NDArrayInt64':
            self._store.meta.put_int64(key, value._impl)
        elif cpp_type == 'NDArrayUInt8':
            self._store.meta.put_uint8(key, value._impl)
        elif cpp_type == 'NDArrayUInt16':
            self._store.meta.put_uint16(key, value._impl)
        elif cpp_type == 'NDArrayUInt32':
            self._store.meta.put_uint32(key, value._impl)
        elif cpp_type == 'NDArrayUInt64':
            self._store.meta.put_uint64(key, value._impl)
        elif cpp_type == 'NDArrayFloat32':
            self._store.meta.put_float32(key, value._impl)
        elif cpp_type == 'NDArrayFloat64':
            self._store.meta.put_float64(key, value._impl)
        elif cpp_type == 'NDArrayString':
            self._store.meta.put_string(key, value._impl)
        else:
            raise ValueError(f"Unsupported array type: {cpp_type}")

    def get(self, key: str) -> np.ndarray:
        """Retrieve an array as NumPy array"""
        # Try each data type until one works, then fall back to metadata
        # This is inefficient but works without needing get_dtype
        get_methods = [
            'get_float64', 'get_int64', 'get_float32', 'get_int32',
            'get_int16', 'get_int8', 'get_uint64', 'get_uint32',
            'get_uint16', 'get_uint8', 'get_string'
        ]

        for method_name in get_methods:
            try:
                result = getattr(self._store, method_name)(key)
                return NDArray(result).to_numpy()
            except (RuntimeError, AttributeError):
                continue

        # If no data array found, raise error
        # Note: Use ds.meta["key"] to access metadata
        raise KeyError(f"Key not found: {key}")

    def get_slice(self, key: str, slices: List[tuple]) -> np.ndarray:
        """
        Get a slice of a large array.

        Args:
            key: Array key
            slices: List of (start, stop[, step]) tuples

        Example:
            >>> subset = store.get_slice("large", [(100, 200), (0, 50)])
        """
        cpp_slices = _star.VectorSlice()
        for s in slices:
            slice_obj = _star.Slice()
            slice_obj.start = s[0]
            slice_obj.stop = s[1]
            slice_obj.step = s[2] if len(s) > 2 else 1
            cpp_slices.append(slice_obj)

        # Get metadata to determine dtype
        meta = self._store.meta.get(key)
        if meta is None:
            raise KeyError(f"Key not found: {key}")

        dtype = meta.dtype

        # Dispatch to appropriate template
        if dtype == DataType.FLOAT64:
            result = self._store.get_slice_float64(key, cpp_slices)
        elif dtype == DataType.INT64:
            result = self._store.get_slice_int64(key, cpp_slices)
        elif dtype == DataType.FLOAT32:
            result = self._store.get_slice_float32(key, cpp_slices)
        elif dtype == DataType.INT32:
            result = self._store.get_slice_int32(key, cpp_slices)
        elif dtype == DataType.INT16:
            result = self._store.get_slice_int16(key, cpp_slices)
        elif dtype == DataType.INT8:
            result = self._store.get_slice_int8(key, cpp_slices)
        elif dtype == DataType.UINT64:
            result = self._store.get_slice_uint64(key, cpp_slices)
        elif dtype == DataType.UINT32:
            result = self._store.get_slice_uint32(key, cpp_slices)
        elif dtype == DataType.UINT16:
            result = self._store.get_slice_uint16(key, cpp_slices)
        elif dtype == DataType.UINT8:
            result = self._store.get_slice_uint8(key, cpp_slices)
        else:
            raise ValueError(f"Unsupported dtype for slicing: {dtype}")

        return NDArray(result).to_numpy()

    def keys(self) -> List[str]:
        """Get all keys in store (excludes internal layer-prefixed keys)"""
        # Convert tuple to list and remove duplicates
        keys_tuple = self._store.get_all_keys()
        # Filter out layer-prefixed internal keys
        filtered_keys = [k for k in keys_tuple if not k.startswith('__layer_')]
        return list(dict.fromkeys(filtered_keys))

    def flush(self):
        """Write pending changes to disk"""
        self._store.flush()

    def is_sliceable(self, key: str) -> bool:
        """Check if array supports slicing"""
        return self._store.is_sliceable(key)

    @property
    def filename(self) -> str:
        """Get filename"""
        return self._store.getFilename()

    def __len__(self) -> int:
        """Number of user entries (excludes internal metadata)"""
        return len(self.keys())

    def __contains__(self, key: str) -> bool:
        """Check if key exists"""
        return self._store.contains(key)

    def __iter__(self):
        """Iterate over all keys: for key in ds"""
        return iter(self.keys())

    def __getitem__(self, key: str) -> np.ndarray:
        """
        Get array using dictionary syntax.

        Examples:
            >>> data = store["my_array"]
        """
        return self.get(key)

    def __setitem__(self, key: str, value: Union[np.ndarray, NDArray, list, str, int, float]):
        """
        Store array using dictionary syntax.

        Stores arrays in separate storage (supports slicing).
        Raw Python scalars and strings are not allowed - use arrays or ds.meta instead.

        Examples:
            >>> store["data"] = [1, 2, 3]           # ✓ List → separate storage
            >>> store["data"] = np.array([1, 2])    # ✓ NumPy array → separate storage
            >>> store["scalar"] = np.array(5)       # ✓ 0-d array → separate storage
            >>> store["test"] = 5                   # ✗ Error: use ds.meta["test"] = 5
            >>> store["label"] = "hello"            # ✗ Error: use ds.meta["label"] = "hello"

        For metadata storage (accepts scalars):
            >>> store.meta["test"] = 5              # ✓ Scalar → metadata
            >>> store.meta["label"] = "hello"       # ✓ String → metadata
            >>> store.meta["tags"] = ["a", "b"]     # ✓ String list → metadata
        """
        # Reject raw Python scalars and strings
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

        # All other types (arrays, lists) go to separate storage
        self.put(key, value)

    def get_metadata_keys(self) -> List[str]:
        """Get keys stored in metadata block"""
        return list(self._store.get_metadata_keys())

    def get_metadata_count(self) -> int:
        """Get count of metadata entries"""
        return self._store.get_metadata_count()

    def get_all_metadata(self) -> Dict[str, Any]:
        """
        Get all metadata entries at once as a dictionary.

        Loads the entire metadata block from disk on first call,
        subsequent calls use cached data. Returns native Python dict
        with numpy arrays as values.

        Returns:
            Dict[str, np.ndarray]: Dictionary mapping keys to numpy arrays

        Example:
            >>> store = StarDataset.open('file.star')
            >>> all_meta = store.get_all_metadata()
            >>> print(all_meta.keys())
            dict_keys(['key1', 'key2', 'key3'])
            >>> print(all_meta['key1'])
            array([1, 2, 3])
        """
        # Get all metadata keys
        keys = self.get_metadata_keys()

        # Get each value and convert to numpy array
        # Note: This is efficient because metadata block is already loaded by first get()
        result = {}
        for key in keys:
            meta = self._store.meta.get(key)
            if meta is not None:
                result[key] = MetadataValue(meta).to_numpy()

        return result

    def is_metadata_loaded(self) -> bool:
        """Check if metadata block is loaded in memory"""
        return self._store.is_metadata_loaded()

    def is_read_only(self) -> bool:
        """Check if dataset is in read-only mode"""
        return self._store.isReadOnly()

    def save_to(self, target_path: str):
        """
        Save dataset to a different file.

        Useful for migrating from read-only to writable,
        or copying between local and S3 storage.

        Args:
            target_path: Target file path (local or /vsis3/)
        """
        self._store.saveTo(target_path)

    def get_file_header(self):
        """
        Get file header information.

        Returns:
            FileHeader object with format_version, header_size, entry_count
        """
        return self._store.getFileHeader()

    def remove_metadata(self, key: str):
        """Remove metadata entry"""
        self._store.meta.remove(key)

    def clear_metadata(self):
        """Clear all metadata entries"""
        self._store.meta.clear()

    # Layer Management API
    def get_layer(self, layer_name: str):
        """
        Get existing layer view.

        Args:
            layer_name: Name of layer (e.g., "band_0", "wavelength_1")

        Returns:
            LayerView instance with same API as StarDataset

        Raises:
            RuntimeError: If layer doesn't exist

        Example:
            >>> ds = StarDataset.open("file.star")
            >>> layer = ds.get_layer("band_0")
            >>> layer.meta["wavelength"]  # Access layer data
        """
        cpp_layer = _pystar.StarDataset_get_layer(self._store, layer_name)
        return LayerView(cpp_layer, self)

    def create_layer(self, layer_name: str):
        """
        Create new layer and return view.

        Args:
            layer_name: Name of new layer

        Returns:
            LayerView instance with same API as StarDataset

        Raises:
            RuntimeError: If layer already exists

        Example:
            >>> ds = StarDataset.create("file.star")
            >>> ds.meta["instrument"] = "AVIRIS"
            >>>
            >>> layer = ds.create_layer("band_0")
            >>> layer.meta["wavelength"] = 450.5  # Layer-specific
            >>> layer.meta["instrument"]  # "AVIRIS" (inherited from base)
        """
        cpp_layer = _pystar.StarDataset_create_layer(self._store, layer_name)
        return LayerView(cpp_layer, self)

    def has_layer(self, layer_name: str) -> bool:
        """Check if layer exists"""
        return _pystar.StarDataset_has_layer(self._store, layer_name)

    def list_layers(self) -> List[str]:
        """Get names of all layers in dataset"""
        return list(_pystar.StarDataset_list_layers(self._store))

    def current_layer(self) -> str:
        """Get current default layer name"""
        return _pystar.StarDataset_current_layer(self._store)

    def close(self):
        """
        Flush all pending writes to disk.

        This is automatically called on context manager exit or object destruction.
        You can call this manually to ensure data is persisted without closing the dataset.

        Example:
            ds = StarDataset.create("data.star")
            ds["array"] = np.array([1, 2, 3])
            ds.close()  # Flushes to disk
            ds["array2"] = np.array([4, 5])  # Can still use dataset

        Note:
            Using the context manager is still preferred:
            with StarDataset.create("data.star") as ds:
                ds["array"] = np.array([1, 2, 3])
            # Automatically flushes when exiting context
        """
        if self._store is not None:
            self._store.close()


__all__ = ['StarDataset', 'LayerView']
