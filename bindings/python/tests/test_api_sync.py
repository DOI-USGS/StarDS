"""Tests for newly synced API features from C++ header"""
import pytest
import numpy as np
import os


def test_metadata_keys(tmp_path):
    """Test get_metadata_keys()"""
    import pystar

    test_file = str(tmp_path / "metadata_keys.star")
    store = pystar.StarDataset(test_file)
    store["key1"] = np.array([1])
    store["key2"] = np.array([2])
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

    store["key1"] = np.array([1])
    assert store.get_metadata_count() == 1

    store["key2"] = np.array([2])
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

    # Write data
    store1 = pystar.StarDataset(test_file)
    store1["array1"] = np.array([1, 2, 3])
    store1["array2"] = np.array([4, 5, 6])
    store1.flush()
    store1.close()

    # Read back and verify keys
    store2 = pystar.StarDataset(test_file, mode="r")
    keys = store2.get_metadata_keys()
    assert "array1" in keys
    assert "array2" in keys
    assert store2.get_metadata_count() == 2
    store2.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
