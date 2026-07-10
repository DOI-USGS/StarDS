"""
Test LayerView metadata accessor with inheritance

Tests that each layer has its own meta object and can inherit from base.
"""
try:
    import pytest
    HAVE_PYTEST = True
except ImportError:
    HAVE_PYTEST = False

import numpy as np
from pystards import StarDataset
import tempfile
import os


class TestLayerMetadata:
    """Test LayerView.meta with inheritance"""

    def test_layer_meta_put_get(self):
        """Test layer.meta['key'] = value and layer.meta['key']"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            layer = ds.create_layer("band_0")

            # Store layer-specific metadata
            layer.meta["wavelength"] = 450.5
            layer.meta["calibration"] = np.array([1.0, 0.99, 0.98])

            # Retrieve it
            wavelength = layer.meta["wavelength"]
            calibration = layer.meta["calibration"]

            # Scalar should be native float
            assert isinstance(wavelength, (float, np.floating))
            assert wavelength == 450.5

            # Array should be np.ndarray
            assert isinstance(calibration, np.ndarray)
            assert np.allclose(calibration, [1.0, 0.99, 0.98])

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_meta_inheritance(self):
        """Test that layer.meta inherits from base"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.set_layer_inheritance(True)  # inheritance is off by default

            # Store base metadata
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["scene"] = "Yellowstone"
            ds.meta["date"] = "2026-04-23"

            # Create layer
            layer = ds.create_layer("band_0")

            # Layer-specific metadata
            layer.meta["wavelength"] = 450.5

            # Layer should see both its own and inherited metadata
            assert "wavelength" in layer.meta
            assert "instrument" in layer.meta
            assert "scene" in layer.meta
            assert "date" in layer.meta

            # Retrieve inherited values
            instrument = layer.meta["instrument"]
            scene = layer.meta["scene"]

            # Should be native strings
            assert isinstance(instrument, str)
            assert isinstance(scene, str)
            assert instrument == "AVIRIS"
            assert scene == "Yellowstone"

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_meta_override(self):
        """Test that layer metadata can override base metadata"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Base calibration
            ds.meta["calibration"] = 1.0

            # Create layers with different calibrations
            layer0 = ds.create_layer("band_0")
            layer1 = ds.create_layer("band_1")

            layer0.meta["calibration"] = 1.05
            layer1.meta["calibration"] = 0.95

            # Each layer should have its own calibration
            assert layer0.meta["calibration"] == 1.05
            assert layer1.meta["calibration"] == 0.95

            # Base should still have original
            assert ds.meta["calibration"] == 1.0

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_meta_isolation(self):
        """Test that layer metadata is isolated between layers"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.set_layer_inheritance(True)  # inheritance is off by default
            ds.meta["instrument"] = "AVIRIS"

            # Create two layers with different metadata
            layer0 = ds.create_layer("band_0")
            layer1 = ds.create_layer("band_1")

            layer0.meta["wavelength"] = 450.5
            layer0.meta["quality"] = "good"

            layer1.meta["wavelength"] = 550.0
            layer1.meta["quality"] = "excellent"

            # Each layer should have independent values
            assert layer0.meta["wavelength"] == 450.5
            assert layer1.meta["wavelength"] == 550.0

            assert layer0.meta["quality"] == "good"
            assert layer1.meta["quality"] == "excellent"

            # Both inherit from base
            assert layer0.meta["instrument"] == "AVIRIS"
            assert layer1.meta["instrument"] == "AVIRIS"

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_meta_persistence(self):
        """Test that layer metadata persists across close/reopen"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            # Write
            ds = StarDataset.create(filename)
            ds.meta["instrument"] = "AVIRIS"

            layer = ds.create_layer("band_0")
            layer.meta["wavelength"] = 450.5
            layer.meta["calibration"] = 1.05

            ds.close()

            # Read (opt into inheritance, off by default)
            ds = StarDataset.open(filename)
            ds.set_layer_inheritance(True)
            layer = ds.get_layer("band_0")

            # Layer metadata should persist
            assert layer.meta["wavelength"] == 450.5
            assert layer.meta["calibration"] == 1.05

            # Inherited metadata should still work
            assert layer.meta["instrument"] == "AVIRIS"

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_meta_accessor_outlives_temporary_layer_view(self):
        """A metadata accessor obtained from a temporary LayerView must stay
        valid after that view is dropped (regression: use-after-free / segfault).
        """
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.set_layer_inheritance(True)  # inheritance is off by default
            ds.meta["instrument"] = "AVIRIS"
            layer = ds.create_layer("band_0")
            layer.meta["wavelength"] = 450.5

            # get_layer(...) returns a fresh LayerView that is discarded right
            # after we grab its accessor. Previously the underlying C++ view was
            # freed, leaving the accessor dangling.
            acc = ds.get_layer("band_0")._meta_accessor
            assert len(acc) == len(acc.keys())
            assert "wavelength" in acc.keys()
            assert "instrument" in acc.keys()

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_hyperspectral_use_case(self):
        """Test full hyperspectral workflow with per-band metadata"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Base metadata shared by all bands
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["scene"] = "Yellowstone"
            ds.meta["acquisition_date"] = "2026-04-23"

            # Base calibration data
            ds["dark_current"] = np.random.rand(512, 512)

            # Create 10 wavelength bands
            for i in range(10):
                layer = ds.create_layer(f"band_{i}")

                # Per-band metadata
                wavelength = 450.0 + i * 10.0  # nm
                layer.meta["wavelength"] = wavelength
                layer.meta["bandwidth"] = 10.0  # nm
                layer.meta["calibration_factor"] = 1.0 + i * 0.01

                # Per-band data
                layer["radiance"] = np.random.rand(512, 512)

            ds.close()

            # Reopen and verify (opt into inheritance, off by default)
            ds = StarDataset.open(filename)
            ds.set_layer_inheritance(True)

            # Check base metadata
            assert ds.meta["instrument"] == "AVIRIS"
            assert "dark_current" in ds

            # Check each band
            for i in range(10):
                layer = ds.get_layer(f"band_{i}")

                # Band-specific metadata
                wavelength = layer.meta["wavelength"]
                assert wavelength == 450.0 + i * 10.0

                # Inherited metadata
                assert layer.meta["instrument"] == "AVIRIS"
                assert layer.meta["scene"] == "Yellowstone"

                # Band can access base data
                assert "dark_current" in layer

        finally:
            if os.path.exists(filename):
                os.unlink(filename)


if __name__ == "__main__":
    if HAVE_PYTEST:
        pytest.main([__file__, "-v"])
    else:
        # Run tests manually without pytest
        print("Running tests without pytest...")
        test_cls = TestLayerMetadata()
        for method_name in dir(test_cls):
            if method_name.startswith("test_"):
                print(f"  {method_name}...", end=" ")
                try:
                    getattr(test_cls, method_name)()
                    print("✓")
                except Exception as e:
                    print(f"✗ {e}")
                    import traceback
                    traceback.print_exc()
