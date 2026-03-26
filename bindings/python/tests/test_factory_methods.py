"""Comprehensive tests for StarDataset.create() and StarDataset.open() factory methods"""
import pytest
import numpy as np
import os


def test_create_basic(tmp_path):
    """Test basic StarDataset.create() usage"""
    import pystar

    filepath = str(tmp_path / "test_create.star")

    # Create a new dataset using factory method
    store = pystar.StarDataset.create(filepath)

    # Verify we can write data
    store["data"] = np.array([1, 2, 3, 4, 5])
    store.flush()

    # Verify mode is read-write
    assert not store.is_read_only()
    assert store.filename == filepath

    store.close()

    # Verify file was created
    assert os.path.exists(filepath)


def test_open_basic(tmp_path):
    """Test basic StarDataset.open() usage"""
    import pystar

    filepath = str(tmp_path / "test_open.star")

    # Create a file first using constructor
    store1 = pystar.StarDataset(filepath)
    store1["data"] = np.array([10, 20, 30])
    store1.flush()
    store1.close()

    # Open the file using factory method with read-write mode
    store2 = pystar.StarDataset.open(filepath, mode="rw")

    # Verify we can read the data
    assert "data" in store2
    np.testing.assert_array_equal(store2["data"], [10, 20, 30])

    # Verify we can write more data
    store2["more_data"] = np.array([40, 50])
    store2.flush()

    assert not store2.is_read_only()
    store2.close()


def test_open_read_only(tmp_path):
    """Test StarDataset.open() with read-only mode"""
    import pystar

    filepath = str(tmp_path / "test_readonly.star")

    # Create file
    store1 = pystar.StarDataset.create(filepath)
    store1["data"] = np.array([1, 2, 3])
    store1.flush()
    store1.close()

    # Open in read-only mode
    store2 = pystar.StarDataset.open(filepath, mode="r")

    # Verify mode
    assert store2.is_read_only()

    # Verify we can read data
    np.testing.assert_array_equal(store2["data"], [1, 2, 3])

    store2.close()


def test_create_then_open(tmp_path):
    """Test create() followed by open() workflow"""
    import pystar

    filepath = str(tmp_path / "workflow.star")

    # Step 1: Create and write initial data
    store = pystar.StarDataset.create(filepath)
    store["initial"] = np.array([1.0, 2.0, 3.0])
    store["metadata"] = np.array([42])
    store.flush()
    store.close()

    # Step 2: Open read-only and verify initial data
    store = pystar.StarDataset.open(filepath, mode="r")
    assert "initial" in store
    assert "metadata" in store

    np.testing.assert_array_equal(store["initial"], [1.0, 2.0, 3.0])
    np.testing.assert_array_equal(store["metadata"], [42])

    store.close()


def test_create_vs_constructor(tmp_path):
    """Verify create() behaves the same as constructor"""
    import pystar

    filepath1 = str(tmp_path / "via_create.star")
    filepath2 = str(tmp_path / "via_constructor.star")

    # Create via factory method
    store1 = pystar.StarDataset.create(filepath1)
    store1["data"] = np.array([1, 2, 3])
    store1.flush()
    store1.close()

    # Create via constructor
    store2 = pystar.StarDataset(filepath2)
    store2["data"] = np.array([1, 2, 3])
    store2.flush()
    store2.close()

    # Both should produce valid files
    assert os.path.exists(filepath1)
    assert os.path.exists(filepath2)

    # Both should be readable
    verify1 = pystar.StarDataset.open(filepath1, mode="r")
    verify2 = pystar.StarDataset.open(filepath2, mode="r")

    np.testing.assert_array_equal(verify1["data"], verify2["data"])

    verify1.close()
    verify2.close()


def test_open_vs_constructor(tmp_path):
    """Verify open() behaves the same as constructor"""
    import pystar

    filepath = str(tmp_path / "test.star")

    # Create file
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([10, 20, 30])
    store.flush()
    store.close()

    # Open via factory method (read-only to avoid modification issues)
    store1 = pystar.StarDataset.open(filepath, mode="r")
    data1 = store1["data"].copy()
    store1.close()

    # Open via constructor (read-only to avoid modification issues)
    store2 = pystar.StarDataset(filepath, mode="r")
    data2 = store2["data"].copy()
    store2.close()

    # Both should read the same data
    np.testing.assert_array_equal(data1, data2)
    np.testing.assert_array_equal(data1, [10, 20, 30])


def test_multiple_opens_read_only(tmp_path):
    """Test multiple concurrent read-only opens"""
    import pystar

    filepath = str(tmp_path / "multi_read.star")

    # Create file
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1, 2, 3, 4, 5])
    store.flush()
    store.close()

    # Open multiple times in read-only mode
    store1 = pystar.StarDataset.open(filepath, mode="r")
    store2 = pystar.StarDataset.open(filepath, mode="r")
    store3 = pystar.StarDataset.open(filepath, mode="r")

    # All should read the same data
    np.testing.assert_array_equal(store1["data"], [1, 2, 3, 4, 5])
    np.testing.assert_array_equal(store2["data"], [1, 2, 3, 4, 5])
    np.testing.assert_array_equal(store3["data"], [1, 2, 3, 4, 5])

    store1.close()
    store2.close()
    store3.close()


def test_factory_methods_with_context_manager(tmp_path):
    """Test factory methods work with context managers"""
    import pystar

    filepath = str(tmp_path / "context.star")

    # Create with context manager
    with pystar.StarDataset.create(filepath) as store:
        store["data1"] = np.array([1, 2, 3])
        store["data2"] = np.array([4, 5, 6])

    # Open with context manager for reading
    with pystar.StarDataset.open(filepath, mode="r") as store:
        assert "data1" in store
        assert "data2" in store
        np.testing.assert_array_equal(store["data1"], [1, 2, 3])
        np.testing.assert_array_equal(store["data2"], [4, 5, 6])


def test_create_with_metadata(tmp_path):
    """Test create() with metadata operations"""
    import pystar

    filepath = str(tmp_path / "with_metadata.star")

    # Create and add metadata
    store = pystar.StarDataset.create(filepath)
    store["array1"] = np.array([1, 2, 3])
    store["array2"] = np.array([4, 5, 6])
    store.flush()

    # Verify metadata operations work
    assert store.get_metadata_count() == 2
    keys = store.get_metadata_keys()
    assert "array1" in keys
    assert "array2" in keys

    store.close()


def test_open_and_modify_metadata(tmp_path):
    """Test open() and reading metadata operations"""
    import pystar

    filepath = str(tmp_path / "modify_metadata.star")

    # Create initial file
    store = pystar.StarDataset.create(filepath)
    store["key1"] = np.array([1, 2, 3])
    store["key2"] = np.array([4, 5, 6])
    store.flush()
    store.close()

    # Open and verify metadata operations
    store = pystar.StarDataset.open(filepath, mode="r")
    assert store.get_metadata_count() == 2

    # Verify keys are accessible
    keys = store.get_metadata_keys()
    assert "key1" in keys
    assert "key2" in keys

    # Verify data can be read
    np.testing.assert_array_equal(store["key1"], [1, 2, 3])
    np.testing.assert_array_equal(store["key2"], [4, 5, 6])

    store.close()


def test_factory_methods_preserve_dtypes(tmp_path):
    """Test that factory methods preserve data types correctly"""
    import pystar

    filepath = str(tmp_path / "dtypes.star")

    # Create with various dtypes
    store = pystar.StarDataset.create(filepath)
    store["int8"] = np.array([1, 2, 3], dtype=np.int8)
    store["int64"] = np.array([100, 200, 300], dtype=np.int64)
    store["float32"] = np.array([1.5, 2.5, 3.5], dtype=np.float32)
    store["float64"] = np.array([10.5, 20.5, 30.5], dtype=np.float64)
    store.flush()
    store.close()

    # Open and verify dtypes
    store = pystar.StarDataset.open(filepath, mode="r")
    assert store["int8"].dtype == np.int8
    assert store["int64"].dtype == np.int64
    assert store["float32"].dtype == np.float32
    assert store["float64"].dtype == np.float64
    store.close()


def test_create_get_file_header(tmp_path):
    """Test that create() produces valid file header"""
    import pystar

    filepath = str(tmp_path / "header_test.star")

    # Create file
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1, 2, 3])
    store.flush()

    # Check header
    header = store.get_file_header()
    assert header.isValid()
    assert header.format_version == 2
    assert header.header_size > 0
    assert header.entry_count >= 1
    assert "STAR" in header.magic_string

    store.close()


def test_open_check_library_version(tmp_path):
    """Test library version is accessible with factory methods"""
    import pystar

    filepath = str(tmp_path / "version_test.star")

    # Get library version
    version = pystar.get_library_version()
    assert isinstance(version, str)
    assert len(version.split('.')) == 3  # X.Y.Z format

    # Create file and verify it works
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1])
    store.flush()
    store.close()

    # Open and verify header format version
    store = pystar.StarDataset.open(filepath, mode="r")
    header = store.get_file_header()
    assert header.format_version == 2
    store.close()


def test_factory_methods_save_to(tmp_path):
    """Test save_to() with factory methods"""
    import pystar

    src = str(tmp_path / "source.star")
    dst = str(tmp_path / "destination.star")

    # Create source
    store = pystar.StarDataset.create(src)
    store["original"] = np.array([1, 2, 3, 4, 5])
    store.flush()
    store.close()

    # Open read-only and save to new location
    store = pystar.StarDataset.open(src, mode="r")
    assert store.is_read_only()
    store.save_to(dst)
    store.close()

    # Open destination and verify
    store = pystar.StarDataset.open(dst, mode="r")
    assert "original" in store
    np.testing.assert_array_equal(store["original"], [1, 2, 3, 4, 5])
    store.close()


def test_open_default_mode(tmp_path):
    """Test that open() defaults to read-write mode"""
    import pystar

    filepath = str(tmp_path / "default_mode.star")

    # Create file
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    # Open without specifying mode (should default to "rw")
    store = pystar.StarDataset.open(filepath)

    # Should not be read-only (default is rw)
    assert not store.is_read_only()

    # Verify we can read existing data
    assert "data" in store
    np.testing.assert_array_equal(store["data"], [1, 2, 3])

    store.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
