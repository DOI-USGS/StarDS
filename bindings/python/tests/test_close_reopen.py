"""
Unit tests for explicit close/reopen paradigm in StarDataset.

Tests the new close() and reopen() functionality that provides explicit
file handle management with proper state tracking.
"""

import pytest
import numpy as np
from pystar import StarDataset
import tempfile
import os


@pytest.fixture
def temp_file():
    """Create a temporary file for testing"""
    with tempfile.NamedTemporaryFile(suffix='.star', delete=False) as f:
        filename = f.name
    yield filename
    # Cleanup
    if os.path.exists(filename):
        os.unlink(filename)


def test_explicit_close(temp_file):
    """Test that operations fail after explicit close"""
    ds = StarDataset.create(temp_file)
    ds["data"] = np.arange(100)
    ds.meta["version"] = 1
    ds.close()

    # All array operations should raise RuntimeError
    with pytest.raises(RuntimeError, match="closed"):
        _ = ds["data"]

    with pytest.raises(RuntimeError, match="closed"):
        ds["other"] = np.zeros(50)

    with pytest.raises(RuntimeError, match="closed"):
        ds.flush()

    with pytest.raises(RuntimeError, match="closed"):
        ds.keys()

    # Metadata operations should also fail
    with pytest.raises(RuntimeError, match="closed"):
        _ = ds.meta["version"]

    with pytest.raises(RuntimeError, match="closed"):
        ds.meta["other"] = 2


def test_idempotent_close(temp_file):
    """Test that multiple close() calls are safe"""
    ds = StarDataset.create(temp_file)
    ds["data"] = np.array([1, 2, 3])

    # Multiple closes should not raise
    ds.close()
    ds.close()
    ds.close()

    # Operations should still fail
    with pytest.raises(RuntimeError, match="closed"):
        _ = ds["data"]


def test_reopen_after_close(temp_file):
    """Test that reopen() restores functionality"""
    # Create and populate dataset
    ds = StarDataset.create(temp_file)
    original_data = np.arange(100)
    ds["data"] = original_data
    ds.meta["version"] = 42
    ds.flush()
    ds.close()

    # Reopen should work
    ds.reopen()
    reopened_data = ds["data"]
    np.testing.assert_array_equal(reopened_data, original_data)

    # Can add more data after reopen
    ds["more"] = np.ones(50)

    # Metadata should work
    version = ds.meta["version"]
    assert version == 42

    ds.close()


def test_context_manager_auto_close(temp_file):
    """Test that context manager closes automatically"""
    with StarDataset.create(temp_file) as ds:
        ds["data"] = np.array([1, 2, 3])

    # Outside context, should be closed
    with pytest.raises(RuntimeError, match="closed"):
        _ = ds["data"]


def test_metadata_after_close(temp_file):
    """Test that metadata access also fails after close"""
    ds = StarDataset.create(temp_file)
    ds.meta["version"] = 1
    ds.meta["tags"] = ["test", "example"]
    ds.close()

    # Metadata operations should fail
    with pytest.raises(RuntimeError, match="closed"):
        _ = ds.meta["version"]

    with pytest.raises(RuntimeError, match="closed"):
        ds.meta["other"] = 2

    with pytest.raises(RuntimeError, match="closed"):
        _ = list(ds.meta.keys())


def test_reopen_read_mode(temp_file):
    """Test reopening in read-only mode"""
    # Create and populate
    ds = StarDataset.create(temp_file)
    test_data = np.arange(100)
    ds["data"] = test_data
    ds.close()

    # Open in read mode
    ds_read = StarDataset.open(temp_file, mode="r")
    data = ds_read["data"]
    np.testing.assert_array_equal(data, test_data)
    ds_read.close()

    # Reopen read mode
    ds_read.reopen()
    data_after_reopen = ds_read["data"]
    np.testing.assert_array_equal(data_after_reopen, test_data)

    ds_read.close()


def test_multiple_close_reopen_cycles(temp_file):
    """Test that close/reopen cycle can be repeated"""
    ds = StarDataset.create(temp_file)
    initial_data = np.arange(10)
    ds["initial"] = initial_data
    ds.flush()

    # First cycle
    ds.close()
    with pytest.raises(RuntimeError):
        _ = ds["initial"]
    ds.reopen()
    np.testing.assert_array_equal(ds["initial"], initial_data)

    # Second cycle
    ds.close()
    with pytest.raises(RuntimeError):
        _ = ds["initial"]
    ds.reopen()
    np.testing.assert_array_equal(ds["initial"], initial_data)

    # Third cycle
    ds.close()
    with pytest.raises(RuntimeError):
        _ = ds["initial"]
    ds.reopen()

    # Verify data integrity after multiple cycles
    final_data = ds["initial"]
    np.testing.assert_array_equal(final_data, initial_data)


def test_close_flushes_data(temp_file):
    """Test that close() properly flushes unflushed data"""
    # Create dataset and close without explicit flush
    ds1 = StarDataset.create(temp_file)
    test_data = np.full(100, 3.14)
    ds1["unflushed_data"] = test_data
    ds1.close()  # Should flush automatically

    # Reopen and verify data was persisted
    ds2 = StarDataset.open(temp_file)
    loaded_data = ds2["unflushed_data"]
    np.testing.assert_array_equal(loaded_data, test_data)


def test_reopen_on_open_dataset(temp_file):
    """Test that reopen() on an open dataset is a no-op"""
    ds = StarDataset.create(temp_file)
    ds["data"] = np.ones(100)

    # Reopen without closing - should be no-op
    ds.reopen()

    # Should still work normally
    data = ds["data"]
    assert len(data) == 100


def test_slice_after_reopen(temp_file):
    """Test that slicing works after reopen()"""
    # Create dataset with large array
    ds1 = StarDataset.create(temp_file)
    large_array = np.arange(1000)
    ds1["large_array"] = large_array
    ds1.close()

    # Open, close, reopen
    ds2 = StarDataset.open(temp_file)
    ds2.close()
    ds2.reopen()

    # Slicing should work
    subset = ds2.get_slice("large_array", [(100, 200)])
    expected = np.arange(100, 200)
    np.testing.assert_array_equal(subset, expected)


def test_iteration_after_reopen(temp_file):
    """Test that iteration works after reopen()"""
    # Create dataset with multiple arrays
    ds = StarDataset.create(temp_file)
    ds["array1"] = np.ones(10)
    ds["array2"] = np.zeros(20)
    ds["array3"] = np.arange(30)
    ds.close()

    # Reopen and iterate
    ds.reopen()
    keys = list(ds)
    assert "array1" in keys
    assert "array2" in keys
    assert "array3" in keys


def test_get_all_metadata_after_reopen(temp_file):
    """Test that get_all_metadata works after reopen()"""
    # Create dataset with metadata
    ds = StarDataset.create(temp_file)
    ds.meta["version"] = 100
    ds.meta["author"] = "TestUser"
    ds.meta["count"] = 42
    ds.close()

    # Reopen and get all metadata
    ds.reopen()
    all_meta = ds.get_all_metadata()
    assert "version" in all_meta
    assert "author" in all_meta
    assert "count" in all_meta


def test_error_message_clarity(temp_file):
    """Test that error messages are clear and helpful"""
    ds = StarDataset.create(temp_file)
    ds.close()

    try:
        _ = ds["nonexistent"]
        pytest.fail("Expected RuntimeError")
    except RuntimeError as e:
        error_msg = str(e)
        # Error should mention "closed"
        assert "closed" in error_msg.lower()


def test_destructor_closes_automatically(temp_file):
    """Test that destructor auto-closes (RAII pattern)"""
    # Create dataset without explicit close
    ds = StarDataset.create(temp_file)
    ds["data"] = np.full(50, 123)
    del ds  # Destructor should close

    # File should be properly closed and readable
    ds2 = StarDataset.open(temp_file)
    data = ds2["data"]
    assert len(data) == 50
    np.testing.assert_array_equal(data, np.full(50, 123))


def test_reopen_uninitialized_raises(temp_file):
    """Test that reopening an uninitialized dataset raises"""
    ds = StarDataset.__new__(StarDataset)
    ds._store = None

    with pytest.raises(RuntimeError, match="not properly initialized"):
        ds.reopen()


def test_close_and_reopen_with_metadata_iteration(temp_file):
    """Test metadata iteration after close/reopen"""
    ds = StarDataset.create(temp_file)
    ds.meta["key1"] = 1
    ds.meta["key2"] = 2
    ds.meta["key3"] = 3
    ds.close()

    ds.reopen()

    # Iterate over metadata
    meta_keys = list(ds.meta.keys())
    assert "key1" in meta_keys
    assert "key2" in meta_keys
    assert "key3" in meta_keys

    # Iterate and access
    for key in ds.meta:
        value = ds.meta[key]
        assert value is not None


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
