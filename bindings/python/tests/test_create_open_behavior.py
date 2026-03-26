"""Tests for create() and open() behavior with existing/missing files"""
import pytest
import numpy as np
import os


def test_create_on_non_existing_file(tmp_path):
    """Test create() on non-existing file - should succeed"""
    import pystar

    filepath = str(tmp_path / "new_file.star")

    # File doesn't exist
    assert not os.path.exists(filepath)

    # Create should succeed
    store = pystar.StarDataset.create(filepath)
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    # File should now exist
    assert os.path.exists(filepath)


def test_create_on_existing_file_overwrites(tmp_path):
    """Test create() on existing file - should overwrite"""
    import pystar

    filepath = str(tmp_path / "existing.star")

    # Create first file
    store1 = pystar.StarDataset.create(filepath)
    store1["old_data"] = np.array([1, 2, 3])
    store1.flush()
    store1.close()

    # Verify first file exists
    assert os.path.exists(filepath)
    store_check = pystar.StarDataset.open(filepath, mode="r")
    assert "old_data" in store_check
    store_check.close()

    # Create again - should overwrite
    store2 = pystar.StarDataset.create(filepath)
    store2["new_data"] = np.array([4, 5, 6])
    store2.flush()
    store2.close()

    # Verify file was overwritten
    store3 = pystar.StarDataset.open(filepath, mode="r")
    assert "new_data" in store3
    assert "old_data" not in store3
    np.testing.assert_array_equal(store3["new_data"], [4, 5, 6])
    store3.close()


def test_open_rw_on_non_existing_file_creates(tmp_path):
    """Test open() with mode='rw' on non-existing file - should create"""
    import pystar

    filepath = str(tmp_path / "new_via_open.star")

    # File doesn't exist
    assert not os.path.exists(filepath)

    # Open with rw mode should create it
    store = pystar.StarDataset.open(filepath, mode="rw")
    store["data"] = np.array([1, 2, 3])
    store.flush()
    store.close()

    # File should now exist
    assert os.path.exists(filepath)

    # Verify we can read it
    store2 = pystar.StarDataset.open(filepath, mode="r")
    assert "data" in store2
    np.testing.assert_array_equal(store2["data"], [1, 2, 3])
    store2.close()


def test_open_readonly_on_non_existing_file_fails(tmp_path):
    """Test open() with mode='r' on non-existing file - should fail"""
    import pystar

    filepath = str(tmp_path / "missing.star")

    # File doesn't exist
    assert not os.path.exists(filepath)

    # Open with r mode should fail
    with pytest.raises(RuntimeError, match="File does not exist.*Cannot open non-existent file in read-only mode"):
        store = pystar.StarDataset.open(filepath, mode="r")


def test_open_rw_on_existing_file(tmp_path):
    """Test open() with mode='rw' on existing file - should succeed"""
    import pystar

    filepath = str(tmp_path / "existing.star")

    # Create file first
    store1 = pystar.StarDataset.create(filepath)
    store1["data"] = np.array([1, 2, 3])
    store1.flush()
    store1.close()

    # Open with rw should succeed and be able to read data
    store2 = pystar.StarDataset.open(filepath, mode="rw")
    assert "data" in store2
    np.testing.assert_array_equal(store2["data"], [1, 2, 3])
    assert not store2.is_read_only()
    store2.close()


def test_open_readonly_on_existing_file(tmp_path):
    """Test open() with mode='r' on existing file - should succeed"""
    import pystar

    filepath = str(tmp_path / "existing.star")

    # Create file first
    store1 = pystar.StarDataset.create(filepath)
    store1["data"] = np.array([1, 2, 3])
    store1.flush()
    store1.close()

    # Open with r should succeed
    store2 = pystar.StarDataset.open(filepath, mode="r")
    assert "data" in store2
    np.testing.assert_array_equal(store2["data"], [1, 2, 3])
    assert store2.is_read_only()
    store2.close()


def test_open_on_corrupt_file_fails(tmp_path):
    """Test open() on corrupt file - should fail"""
    import pystar

    filepath = str(tmp_path / "corrupt.star")

    # Create a corrupt file (not a valid STAR file)
    with open(filepath, 'wb') as f:
        f.write(b"This is not a valid STAR file")

    # Open should fail with corruption error (magic string mismatch happens first)
    with pytest.raises(RuntimeError, match="Magic string mismatch|File may be corrupt"):
        store = pystar.StarDataset.open(filepath, mode="r")


def test_create_overwrite_workflow(tmp_path):
    """Test complete workflow: create -> read -> create (overwrite)"""
    import pystar

    filepath = str(tmp_path / "workflow.star")

    # Step 1: Create initial file
    store = pystar.StarDataset.create(filepath)
    store["v1_data"] = np.array([1, 2, 3])
    store["v1_meta"] = np.array([100])
    store.flush()
    store.close()

    # Step 2: Open and read data
    store = pystar.StarDataset.open(filepath, mode="r")
    assert "v1_data" in store
    assert "v1_meta" in store
    np.testing.assert_array_equal(store["v1_data"], [1, 2, 3])
    np.testing.assert_array_equal(store["v1_meta"], [100])
    store.close()

    # Step 3: Create again - overwrites everything
    store = pystar.StarDataset.create(filepath)
    store["fresh_data"] = np.array([10, 20, 30])
    store.flush()
    store.close()

    # Verify only new data exists
    store = pystar.StarDataset.open(filepath, mode="r")
    assert len(store) == 1
    assert "fresh_data" in store
    assert "v1_data" not in store
    assert "v1_meta" not in store
    np.testing.assert_array_equal(store["fresh_data"], [10, 20, 30])
    store.close()


def test_open_rw_vs_create_behavior(tmp_path):
    """Test that open(rw) and create() behave correctly"""
    import pystar

    filepath1 = str(tmp_path / "via_open_rw.star")
    filepath2 = str(tmp_path / "via_create.star")

    # Both files don't exist
    assert not os.path.exists(filepath1)
    assert not os.path.exists(filepath2)

    # Both should create new files
    store1 = pystar.StarDataset.open(filepath1, mode="rw")
    store1["data"] = np.array([1, 2, 3])
    store1.flush()
    store1.close()

    store2 = pystar.StarDataset.create(filepath2)
    store2["data"] = np.array([1, 2, 3])
    store2.flush()
    store2.close()

    # Both should have created valid files
    assert os.path.exists(filepath1)
    assert os.path.exists(filepath2)

    # Both should be readable
    store1 = pystar.StarDataset.open(filepath1, mode="r")
    store2 = pystar.StarDataset.open(filepath2, mode="r")

    np.testing.assert_array_equal(store1["data"], store2["data"])

    store1.close()
    store2.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
