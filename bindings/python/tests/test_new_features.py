"""Tests for new API features: dict access, logger, and close()"""
import pytest
import numpy as np
import tempfile
import os


def test_dict_style_setitem_getitem(tmp_path):
    """Test dictionary-style array access with [] operator"""
    import pystar

    # Create test file
    test_file = tmp_path / "dict_test.star"

    # Write using dict syntax
    store = pystar.StarDataset(str(test_file), "rw")

    # Test __setitem__
    store["vector"] = np.array([1, 2, 3, 4, 5], dtype=np.int64)
    store["matrix"] = np.random.rand(3, 3)
    store["tensor"] = np.arange(24).reshape(2, 3, 4).astype(np.float32)

    store.flush()
    store.close()

    # Read using dict syntax
    store = pystar.StarDataset(str(test_file), "r")

    # Test __getitem__
    vector = store["vector"]
    assert isinstance(vector, np.ndarray)
    assert vector.shape == (5,)
    np.testing.assert_array_equal(vector, [1, 2, 3, 4, 5])

    matrix = store["matrix"]
    assert matrix.shape == (3, 3)

    tensor = store["tensor"]
    assert tensor.shape == (2, 3, 4)
    np.testing.assert_array_equal(tensor, np.arange(24).reshape(2, 3, 4))

    store.close()


def test_dict_style_contains(tmp_path):
    """Test 'in' operator with dictionary-style access"""
    import pystar

    test_file = tmp_path / "contains_test.star"

    store = pystar.StarDataset(str(test_file), "rw")
    store["test_key"] = np.array([1, 2, 3])
    store.flush()

    # Test __contains__
    assert "test_key" in store
    assert "nonexistent_key" not in store

    store.close()


def test_dict_mixed_api(tmp_path):
    """Test mixing old put/get API with new dict syntax"""
    import pystar

    test_file = tmp_path / "mixed_test.star"

    store = pystar.StarDataset(str(test_file), "rw")

    # Mix both styles
    store.put("old_style", np.array([1, 2, 3]))
    store["new_style"] = np.array([4, 5, 6])

    store.flush()

    # Read with mixed styles
    old_data = store.get("old_style")
    new_data = store["new_style"]

    np.testing.assert_array_equal(old_data, [1, 2, 3])
    np.testing.assert_array_equal(new_data, [4, 5, 6])

    # Read with opposite styles
    old_as_dict = store["old_style"]
    new_as_get = store.get("new_style")

    np.testing.assert_array_equal(old_as_dict, [1, 2, 3])
    np.testing.assert_array_equal(new_as_get, [4, 5, 6])

    store.close()


def test_logger_set_get():
    """Test logger level get/set"""
    import pystar

    # Get default level
    default_level = pystar.get_log_level()
    assert isinstance(default_level, int)
    assert default_level == 4  # ERROR is default

    # Set to DEBUG
    pystar.set_log_level(pystar.LogLevel.DEBUG)
    assert pystar.get_log_level() == 1

    # Set to TRACE
    pystar.set_log_level(pystar.LogLevel.TRACE)
    assert pystar.get_log_level() == 0

    # Set to INFO
    pystar.set_log_level(pystar.LogLevel.INFO)
    assert pystar.get_log_level() == 2

    # Set to WARN
    pystar.set_log_level(pystar.LogLevel.WARN)
    assert pystar.get_log_level() == 3

    # Set to ERROR
    pystar.set_log_level(pystar.LogLevel.ERROR)
    assert pystar.get_log_level() == 4

    # Set using integer
    pystar.set_log_level(1)
    assert pystar.get_log_level() == 1

    # Restore default
    pystar.set_log_level(default_level)


def test_logger_string_names():
    """Test setting log level with string names"""
    import pystar

    original_level = pystar.get_log_level()

    # Test string names (case insensitive)
    pystar.set_log_level("DEBUG")
    assert pystar.get_log_level() == 1

    pystar.set_log_level("debug")
    assert pystar.get_log_level() == 1

    pystar.set_log_level("INFO")
    assert pystar.get_log_level() == 2

    pystar.set_log_level("WARN")
    assert pystar.get_log_level() == 3

    pystar.set_log_level("ERROR")
    assert pystar.get_log_level() == 4

    pystar.set_log_level("TRACE")
    assert pystar.get_log_level() == 0

    # Restore
    pystar.set_log_level(original_level)


def test_logger_invalid_input():
    """Test logger with invalid inputs"""
    import pystar

    with pytest.raises(ValueError):
        pystar.set_log_level("INVALID")

    with pytest.raises(ValueError):
        pystar.set_log_level(5)  # Out of range

    with pytest.raises(ValueError):
        pystar.set_log_level(-1)  # Out of range


def test_explicit_close(tmp_path):
    """Test explicit close() method"""
    import pystar

    test_file = tmp_path / "close_test.star"

    # Without context manager
    store = pystar.StarDataset(str(test_file), "rw")
    store["data"] = np.array([1, 2, 3, 4, 5])

    # Explicit close
    store.close()

    # Verify data was written
    store2 = pystar.StarDataset(str(test_file), "r")
    data = store2["data"]
    np.testing.assert_array_equal(data, [1, 2, 3, 4, 5])
    store2.close()


def test_close_in_context_manager(tmp_path):
    """Test that context manager still auto-closes"""
    import pystar

    test_file = tmp_path / "context_test.star"

    # Use context manager
    with pystar.StarDataset(str(test_file), "rw") as store:
        store["data"] = np.array([10, 20, 30])
        # No explicit close needed

    # Verify data was written by context manager
    with pystar.StarDataset(str(test_file), "r") as store:
        data = store["data"]
        np.testing.assert_array_equal(data, [10, 20, 30])


def test_close_read_only_mode(tmp_path):
    """Test close() in read-only mode (should not error)"""
    import pystar

    test_file = tmp_path / "readonly_test.star"

    # Create file first
    with pystar.StarDataset(str(test_file), "rw") as store:
        store["data"] = np.array([1, 2, 3])

    # Open in read-only and close
    store = pystar.StarDataset(str(test_file), "r")
    data = store["data"]
    np.testing.assert_array_equal(data, [1, 2, 3])

    # Close should not error even in read-only mode
    store.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
