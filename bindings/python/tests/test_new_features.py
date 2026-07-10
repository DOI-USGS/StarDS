"""Tests for new API features: dict access, logger, and close()"""
import pytest
import numpy as np
import tempfile
import os


def test_dict_style_setitem_getitem(tmp_path):
    """Test dictionary-style array access with [] operator"""
    import pystards

    # Create test file
    test_file = tmp_path / "dict_test.stards"

    # Write using dict syntax
    store = pystards.StarDataset(str(test_file), "rw")

    # Test __setitem__
    store["vector"] = np.array([1, 2, 3, 4, 5], dtype=np.int64)
    store["matrix"] = np.random.rand(3, 3)
    store["tensor"] = np.arange(24).reshape(2, 3, 4).astype(np.float32)

    store.flush()
    store.close()

    # Read using dict syntax
    store = pystards.StarDataset(str(test_file), "r")

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
    import pystards

    test_file = tmp_path / "contains_test.stards"

    store = pystards.StarDataset(str(test_file), "rw")
    store["test_key"] = np.array([1, 2, 3])
    store.flush()

    # Test __contains__
    assert "test_key" in store
    assert "nonexistent_key" not in store

    store.close()


def test_dict_mixed_api(tmp_path):
    """Test mixing old put/get API with new dict syntax"""
    import pystards

    test_file = tmp_path / "mixed_test.stards"

    store = pystards.StarDataset(str(test_file), "rw")

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
    import pystards

    # Get default level
    default_level = pystards.get_log_level()
    assert isinstance(default_level, int)
    assert default_level == 4  # ERROR is default

    # Set to DEBUG
    pystards.set_log_level(pystards.LogLevel.DEBUG)
    assert pystards.get_log_level() == 1

    # Set to TRACE
    pystards.set_log_level(pystards.LogLevel.TRACE)
    assert pystards.get_log_level() == 0

    # Set to INFO
    pystards.set_log_level(pystards.LogLevel.INFO)
    assert pystards.get_log_level() == 2

    # Set to WARN
    pystards.set_log_level(pystards.LogLevel.WARN)
    assert pystards.get_log_level() == 3

    # Set to ERROR
    pystards.set_log_level(pystards.LogLevel.ERROR)
    assert pystards.get_log_level() == 4

    # Set using integer
    pystards.set_log_level(1)
    assert pystards.get_log_level() == 1

    # Restore default
    pystards.set_log_level(default_level)


def test_logger_string_names():
    """Test setting log level with string names"""
    import pystards

    original_level = pystards.get_log_level()

    # Test string names (case insensitive)
    pystards.set_log_level("DEBUG")
    assert pystards.get_log_level() == 1

    pystards.set_log_level("debug")
    assert pystards.get_log_level() == 1

    pystards.set_log_level("INFO")
    assert pystards.get_log_level() == 2

    pystards.set_log_level("WARN")
    assert pystards.get_log_level() == 3

    pystards.set_log_level("ERROR")
    assert pystards.get_log_level() == 4

    pystards.set_log_level("TRACE")
    assert pystards.get_log_level() == 0

    # Restore
    pystards.set_log_level(original_level)


def test_logger_invalid_input():
    """Test logger with invalid inputs"""
    import pystards

    with pytest.raises(ValueError):
        pystards.set_log_level("INVALID")

    with pytest.raises(ValueError):
        pystards.set_log_level(5)  # Out of range

    with pytest.raises(ValueError):
        pystards.set_log_level(-1)  # Out of range


def test_explicit_close(tmp_path):
    """Test explicit close() method"""
    import pystards

    test_file = tmp_path / "close_test.stards"

    # Without context manager
    store = pystards.StarDataset(str(test_file), "rw")
    store["data"] = np.array([1, 2, 3, 4, 5])

    # Explicit close
    store.close()

    # Verify data was written
    store2 = pystards.StarDataset(str(test_file), "r")
    data = store2["data"]
    np.testing.assert_array_equal(data, [1, 2, 3, 4, 5])
    store2.close()


def test_close_in_context_manager(tmp_path):
    """Test that context manager still auto-closes"""
    import pystards

    test_file = tmp_path / "context_test.stards"

    # Use context manager
    with pystards.StarDataset(str(test_file), "rw") as store:
        store["data"] = np.array([10, 20, 30])
        # No explicit close needed

    # Verify data was written by context manager
    with pystards.StarDataset(str(test_file), "r") as store:
        data = store["data"]
        np.testing.assert_array_equal(data, [10, 20, 30])


def test_close_read_only_mode(tmp_path):
    """Test close() in read-only mode (should not error)"""
    import pystards

    test_file = tmp_path / "readonly_test.stards"

    # Create file first
    with pystards.StarDataset(str(test_file), "rw") as store:
        store["data"] = np.array([1, 2, 3])

    # Open in read-only and close
    store = pystards.StarDataset(str(test_file), "r")
    data = store["data"]
    np.testing.assert_array_equal(data, [1, 2, 3])

    # Close should not error even in read-only mode
    store.close()


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
