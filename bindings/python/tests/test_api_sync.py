"""Tests for newly synced API features from C++ header"""
import pytest
import numpy as np
import os


def test_metadata_keys(tmp_path):
    """Test get_metadata_keys()"""
    import pystar

    test_file = str(tmp_path / "metadata_keys.star")
    store = pystar.StarDataset(test_file)
    store.put_metadata("key1", np.array([1]))
    store.put_metadata("key2", np.array([2]))
    store.flush()

    meta_keys = store.get_metadata_keys()
    assert "key1" in meta_keys
    assert "key2" in meta_keys
    store.close()


def test_metadata_count(tmp_path):
    """Test get_metadata_count()"""
    import pystar

    test_file = str(tmp_path / "metadata_count.star")
    store = pystar.StarDataset(test_file)
    assert store.get_metadata_count() == 0

    store.put_metadata("key1", np.array([1]))
    assert store.get_metadata_count() == 1

    store.put_metadata("key2", np.array([2]))
    assert store.get_metadata_count() == 2
    store.close()


def test_remove_metadata(tmp_path):
    """Test remove_metadata()"""
    import pystar

    test_file = str(tmp_path / "remove_metadata.star")
    store = pystar.StarDataset(test_file)
    store["key1"] = np.array([1])
    assert "key1" in store

    store.remove_metadata("key1")
    assert "key1" not in store
    store.close()


def test_clear_metadata(tmp_path):
    """Test clear_metadata()"""
    import pystar

    test_file = str(tmp_path / "clear_metadata.star")
    store = pystar.StarDataset(test_file)
    store["key1"] = np.array([1])
    store["key2"] = np.array([2])

    store.clear_metadata()
    assert store.get_metadata_count() == 0
    store.close()


def test_is_read_only(tmp_path):
    """Test is_read_only()"""
    import pystar

    test_file = str(tmp_path / "read_only.star")
    store = pystar.StarDataset(test_file, mode="rw")
    assert not store.is_read_only()
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    store_ro = pystar.StarDataset(test_file, mode="r")
    assert store_ro.is_read_only()
    store_ro.close()


def test_save_to(tmp_path):
    """Test save_to()"""
    import pystar

    src = str(tmp_path / "source.star")
    dst = str(tmp_path / "dest.star")

    # Create source
    store = pystar.StarDataset(src)
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    # Open read-only and save to new location
    store_ro = pystar.StarDataset(src, mode="r")
    store_ro.save_to(dst)
    store_ro.close()

    # Verify destination has data
    store_dst = pystar.StarDataset(dst, mode="r")
    assert "data" in store_dst
    np.testing.assert_array_equal(store_dst["data"], [1, 2, 3])
    store_dst.close()


def test_get_file_header(tmp_path):
    """Test get_file_header()"""
    import pystar

    test_file = str(tmp_path / "file_header.star")
    store = pystar.StarDataset(test_file)
    store["data"] = np.array([1])
    store.flush()

    header = store.get_file_header()
    assert header.format_version == 2
    assert header.header_size > 0
    assert header.entry_count >= 1
    assert header.isValid()
    assert "Format v2" in header.getVersionString()
    store.close()


def test_library_version():
    """Test get_library_version()"""
    import pystar
    version = pystar.get_library_version()
    assert isinstance(version, str)
    assert len(version.split('.')) == 3  # X.Y.Z format


def test_create_factory_method(tmp_path):
    """Test StarDataset.create() factory method"""
    import pystar

    filepath = str(tmp_path / "created.star")

    # Create using factory method
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    # Verify file exists and can be opened
    assert os.path.exists(filepath)
    store2 = pystar.StarDataset.open(filepath, mode="r")
    assert "data" in store2
    np.testing.assert_array_equal(store2["data"], [1, 2, 3])
    store2.close()


def test_open_factory_method(tmp_path):
    """Test StarDataset.open() factory method"""
    import pystar

    test_file = str(tmp_path / "open_test.star")

    # Create file first
    store = pystar.StarDataset(test_file)
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    # Open using factory method
    store_opened = pystar.StarDataset.open(test_file, mode="r")
    assert store_opened.is_read_only()
    assert "data" in store_opened
    np.testing.assert_array_equal(store_opened["data"], [1, 2, 3])
    store_opened.close()

    # Verify read-write open
    store_rw = pystar.StarDataset.open(test_file, mode="rw")
    assert not store_rw.is_read_only()
    store_rw.close()


def test_is_metadata_loaded(tmp_path):
    """Test is_metadata_loaded()"""
    import pystar

    test_file = str(tmp_path / "metadata_loaded.star")
    store = pystar.StarDataset(test_file)

    # Initially no metadata should be loaded
    initial_state = store.is_metadata_loaded()

    # Add some metadata
    store["key1"] = np.array([1])
    store.flush()

    # Metadata should now be loaded
    assert isinstance(store.is_metadata_loaded(), bool)
    store.close()


def test_file_header_magic_string(tmp_path):
    """Test FileHeader magic_string property"""
    import pystar

    test_file = str(tmp_path / "magic_test.star")
    store = pystar.StarDataset(test_file)
    store["data"] = np.array([42])
    store.flush()

    header = store.get_file_header()
    magic = header.magic_string
    assert isinstance(magic, str)
    assert "STAR" in magic  # Should contain STAR magic bytes
    store.close()


def test_metadata_keys_persistence(tmp_path):
    """Test that metadata keys persist across file open/close"""
    import pystar

    test_file = str(tmp_path / "persist_keys.star")

    # Write data to metadata block explicitly
    store1 = pystar.StarDataset(test_file)
    store1.put_metadata("array1", np.array([1, 2, 3]))
    store1.put_metadata("array2", np.array([4, 5, 6]))
    store1.flush()
    store1.close()

    # Read back and verify keys
    store2 = pystar.StarDataset(test_file, mode="r")
    keys = store2.get_metadata_keys()
    assert "array1" in keys
    assert "array2" in keys
    assert store2.get_metadata_count() == 2
    store2.close()


def test_get_all_metadata(tmp_path):
    """Test get_all_metadata returns dict of all metadata"""
    import pystar

    file_path = str(tmp_path / "test_get_all.star")

    # Create store with metadata
    store = pystar.StarDataset.create(file_path)
    store.put_metadata("scalar_int", np.array(42, dtype=np.int32))
    store.put_metadata("scalar_float", np.array(3.14, dtype=np.float64))
    store.put_metadata("array", np.array([1, 2, 3], dtype=np.int64))
    store.flush()
    store.close()

    # Reopen and test get_all_metadata
    store = pystar.StarDataset.open(file_path)

    # Verify metadata not loaded yet
    assert not store.is_metadata_loaded()

    # Get all metadata
    all_meta = store.get_all_metadata()

    # Verify metadata now loaded
    assert store.is_metadata_loaded()

    # Verify returns native dict
    assert isinstance(all_meta, dict)
    assert len(all_meta) == 3
    assert "scalar_int" in all_meta
    assert "scalar_float" in all_meta
    assert "array" in all_meta

    # Verify values
    assert all_meta["scalar_int"].to_numpy() == 42
    assert np.isclose(all_meta["scalar_float"].to_numpy(), 3.14)
    assert np.array_equal(all_meta["array"].to_numpy(), [1, 2, 3])

    # Call again to verify cached (no redundant disk read)
    all_meta2 = store.get_all_metadata()
    assert len(all_meta2) == 3

    store.close()


def test_string_type_support(tmp_path):
    """Test storing strings and string arrays"""
    import pystar
    import numpy as np

    file_path = str(tmp_path / "test_strings.star")
    store = pystar.StarDataset.create(file_path)

    # Strings must use metadata (single string or lists)
    store.meta["label"] = "experiment_A"
    store.meta["tags"] = ["red", "blue", "green"]
    store.meta["names"] = np.array(["alice", "bob", "charlie"])

    # Python list of numbers works in separate storage
    store["data"] = [1, 2, 3, 4, 5]

    store.flush()
    store.close()

    # Verify
    store = pystar.StarDataset.open(file_path)

    assert store.meta["label"].tolist() == ["experiment_A"]
    assert store.meta["tags"].tolist() == ["red", "blue", "green"]
    assert store.meta["names"].tolist() == ["alice", "bob", "charlie"]
    assert store["data"].tolist() == [1, 2, 3, 4, 5]

    store.close()


def test_no_automatic_metadata_routing(tmp_path):
    """Test that __setitem__ throws errors for raw scalars/strings"""
    import pystar
    import numpy as np
    import pytest

    file_path = str(tmp_path / "test_no_auto.star")
    store = pystar.StarDataset.create(file_path)

    # Raw Python scalars should throw error
    with pytest.raises(TypeError, match="raw Python scalar"):
        store["test"] = 5

    # Raw Python strings should throw error
    with pytest.raises(TypeError, match="raw Python string"):
        store["label"] = "hello"

    # 0-d NumPy arrays are ALLOWED (not raw Python scalars)
    store["scalar_array"] = np.array(42)
    assert store["scalar_array"].tolist() == [42]  # Wrapped as 1-element array

    # Lists and arrays work
    store["test"] = [5]  # List
    store["matrix"] = np.array([[1, 2], [3, 4]])  # Multi-d array

    # Metadata accepts scalars directly
    store.meta["scalar"] = 99
    store.meta["label"] = "hello"

    # Verify
    assert store["test"].tolist() == [5]
    assert store["scalar_array"].item() == 42  # 0-d array
    assert store.meta["scalar"].item() == 99
    assert store.meta["label"].tolist() == ["hello"]

    store.close()


def test_scalar_string_errors(tmp_path):
    """Test that scalars and strings produce helpful error messages"""
    import pystar
    import pytest

    file_path = str(tmp_path / "test_errors.star")
    store = pystar.StarDataset.create(file_path)

    # Test scalar error message
    with pytest.raises(TypeError) as exc_info:
        store["count"] = 42

    error_msg = str(exc_info.value)
    assert "raw Python scalar" in error_msg
    assert "ds.meta" in error_msg

    # Test string error message
    with pytest.raises(TypeError) as exc_info:
        store["name"] = "test"

    error_msg = str(exc_info.value)
    assert "raw Python string" in error_msg
    assert "ds.meta" in error_msg

    store.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
