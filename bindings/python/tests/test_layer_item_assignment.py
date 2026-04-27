"""
Test LayerView item assignment (layer[key] = value)

Note: This test documents the expected API. The actual functionality
requires the Python bindings to be properly built and installed.
"""
import sys
sys.path.insert(0, 'build/bindings/python')

try:
    import pytest
    HAVE_PYTEST = True
except ImportError:
    HAVE_PYTEST = False

import numpy as np
from pystar import StarDataset
import tempfile
import os


class TestLayerItemAssignment:
    """Test LayerView supports item assignment like StarDataset"""

    def test_layer_setitem(self):
        """Test layer['key'] = value"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            layer = ds.create_layer("band_0")

            # Test __setitem__
            layer["wavelength_data"] = np.array([450.5, 451.0, 451.5])

            # Verify it was stored
            data = layer["wavelength_data"]
            assert np.array_equal(data, [450.5, 451.0, 451.5])
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_getitem(self):
        """Test value = layer['key']"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            layer = ds.create_layer("band_0")

            # Store via parent
            ds["test_array"] = np.array([1, 2, 3, 4, 5])

            # Retrieve via layer
            data = layer["test_array"]
            assert np.array_equal(data, [1, 2, 3, 4, 5])
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_get_put_methods(self):
        """Test layer.get() and layer.put() methods"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            layer = ds.create_layer("band_0")

            # Test put()
            layer.put("calibration", np.array([1.0, 0.99, 0.98]))

            # Test get()
            data = layer.get("calibration")
            assert np.array_equal(data, [1.0, 0.99, 0.98])

            # Verify __getitem__ also works
            data2 = layer["calibration"]
            assert np.array_equal(data2, data)
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_inheritance_via_getitem(self):
        """Test that layer[key] can access base layer data"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store in base
            ds["base_array"] = np.array([10, 20, 30])
            ds.meta["instrument"] = "AVIRIS"

            # Create layer
            layer = ds.create_layer("band_0")

            # Layer should access base data
            base_data = layer["base_array"]
            assert np.array_equal(base_data, [10, 20, 30])

            # Check contains works
            assert "base_array" in layer
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_setitem_with_different_types(self):
        """Test layer[key] = value with various data types"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            layer = ds.create_layer("band_0")

            # Test various types
            layer["int_array"] = np.array([1, 2, 3], dtype=np.int32)
            layer["float_array"] = np.array([1.1, 2.2, 3.3], dtype=np.float64)
            layer["list"] = [4, 5, 6]
            layer["scalar"] = 42

            # Verify all stored correctly
            assert np.array_equal(layer["int_array"], [1, 2, 3])
            assert np.allclose(layer["float_array"], [1.1, 2.2, 3.3])
            assert np.array_equal(layer["list"], [4, 5, 6])
            # Scalar stored as array
            assert layer["scalar"].flatten()[0] == 42
        finally:
            if os.path.exists(filename):
                os.unlink(filename)


if __name__ == "__main__":
    if HAVE_PYTEST:
        pytest.main([__file__, "-v"])
    else:
        # Run tests manually without pytest
        print("Running tests without pytest...")
        test_cls = TestLayerItemAssignment()
        for method_name in dir(test_cls):
            if method_name.startswith("test_"):
                print(f"  {method_name}...", end=" ")
                try:
                    getattr(test_cls, method_name)()
                    print("✓")
                except Exception as e:
                    print(f"✗ {e}")
