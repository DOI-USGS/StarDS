"""
Tests for layer functionality
"""
try:
    import pytest
    HAVE_PYTEST = True
except ImportError:
    HAVE_PYTEST = False

import numpy as np
from pystards import StarDataset, OpenOptions
import tempfile
import os


class TestLayerBasics:
    """Basic layer creation and access tests"""

    def test_create_layer(self):
        """Test creating a new layer"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Create layer
            layer1 = ds.create_layer("layer1")
            assert layer1.name() == "layer1"

            # Verify layer exists
            assert ds.has_layer("layer1")
            assert "layer1" in ds.list_layers()
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_get_layer(self):
        """Test retrieving existing layer"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.create_layer("layer1")

            # Get layer
            layer1 = ds.get_layer("layer1")
            assert layer1.name() == "layer1"
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_get_nonexistent_layer_raises(self):
        """Test that getting non-existent layer raises error"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            if HAVE_PYTEST:
                with pytest.raises(RuntimeError, match="Layer not found"):
                    ds.get_layer("nonexistent")
            else:
                try:
                    ds.get_layer("nonexistent")
                    raise AssertionError("Expected RuntimeError")
                except RuntimeError as e:
                    assert "Layer not found" in str(e)
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_create_duplicate_layer_raises(self):
        """Test that creating duplicate layer raises error"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.create_layer("layer1")

            if HAVE_PYTEST:
                with pytest.raises(RuntimeError, match="Layer already exists"):
                    ds.create_layer("layer1")
            else:
                try:
                    ds.create_layer("layer1")
                    raise AssertionError("Expected RuntimeError")
                except RuntimeError as e:
                    assert "Layer already exists" in str(e)
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_list_layers(self):
        """Test listing all layers"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Initially empty
            assert ds.list_layers() == []

            # Create layers
            ds.create_layer("layer1")
            ds.create_layer("layer2")
            ds.create_layer("layer3")

            layers = ds.list_layers()
            assert len(layers) == 3
            assert "layer1" in layers
            assert "layer2" in layers
            assert "layer3" in layers
        finally:
            if os.path.exists(filename):
                os.unlink(filename)


class TestLayerInheritance:
    """Tests for layer inheritance from base"""

    def test_inheritance_keys(self):
        """Test that layer inherits keys from base"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Add base metadata
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["date"] = "2026-04-22"

            # Add base array
            ds["cube"] = np.random.rand(10, 10, 10)

            # Create layer
            ds.set_layer_inheritance(True)  # inheritance is off by default
            layer1 = ds.create_layer("layer1")

            # Layer should see base keys
            keys = layer1.keys()
            assert "instrument" in keys
            assert "date" in keys
            assert "cube" in keys
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_contains_inherited_keys(self):
        """Test that contains() works for inherited keys"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.set_layer_inheritance(True)  # inheritance is off by default
            ds.meta["base_key"] = "base_value"

            layer1 = ds.create_layer("layer1")

            # Should see base key
            assert "base_key" in layer1
        finally:
            if os.path.exists(filename):
                os.unlink(filename)


class TestHyperspectralUseCase:
    """Tests for hyperspectral imaging use case with many layers"""

    def test_create_many_layers(self):
        """Test creating 100 wavelength bands"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.set_layer_inheritance(True)  # inheritance is off by default

            # Base metadata
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["scene"] = "Yellowstone"

            # Create 100 bands
            for i in range(100):
                layer = ds.create_layer(f"band_{i}")
                # TODO: Add layer-specific metadata when LayerView.meta is implemented

            # Verify all layers exist
            layers = ds.list_layers()
            assert len(layers) == 100

            # Check inheritance works for all
            layer50 = ds.get_layer("band_50")
            assert "instrument" in layer50
            assert "scene" in layer50
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_persistence(self):
        """Test that layers persist across close/open"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            # Create and write
            ds = StarDataset.create(filename)
            ds.meta["instrument"] = "AVIRIS"
            ds.create_layer("band_0")
            ds.create_layer("band_1")
            ds.create_layer("band_2")
            ds.close()

            # Reopen and verify (opt into inheritance, off by default)
            opts = OpenOptions()
            opts.layer_inheritance = True
            ds = StarDataset.open(filename, mode="r", options=opts)
            layers = ds.list_layers()
            assert len(layers) == 3
            assert "band_0" in layers
            assert "band_1" in layers
            assert "band_2" in layers

            # Verify inheritance works after reload
            layer1 = ds.get_layer("band_1")
            assert "instrument" in layer1
        finally:
            if os.path.exists(filename):
                os.unlink(filename)


class TestLayerInheritanceToggle:
    """Layer inheritance is off by default; opt in via OpenOptions or setter."""

    def _make_file(self):
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name
        ds = StarDataset.create(filename)
        ds["A"] = np.full((8,), 1.0)      # base-only key
        layer = ds.create_layer("L")
        layer["B"] = np.full((8,), 2.0)   # layer-owned key
        ds.flush()
        ds.close()
        return filename

    def test_inheritance_off_by_default(self):
        """Base-only key is a miss through a layer when inheritance is off."""
        filename = self._make_file()
        try:
            ds = StarDataset.open(filename, mode="r")
            assert ds.layer_inheritance() is False
            layer = ds.get_layer("L")
            # Base-only key is not visible.
            assert "A" not in layer
            with pytest.raises(KeyError):
                _ = layer["A"]
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_inheritance_opt_in_via_open_options(self):
        """OpenOptions(layer_inheritance=True) restores base fallback."""
        filename = self._make_file()
        try:
            opts = OpenOptions()
            opts.layer_inheritance = True
            ds = StarDataset.open(filename, mode="r", options=opts)
            assert ds.layer_inheritance() is True
            layer = ds.get_layer("L")
            assert "A" in layer
            assert layer["A"][0] == 1.0
        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_inheritance_post_open_toggle(self):
        """set_layer_inheritance() toggles behavior live on an open dataset."""
        filename = self._make_file()
        try:
            ds = StarDataset.open(filename, mode="r")
            layer = ds.get_layer("L")
            assert "A" not in layer

            ds.set_layer_inheritance(True)
            assert ds.layer_inheritance() is True
            assert "A" in layer
            assert layer["A"][0] == 1.0

            ds.set_layer_inheritance(False)
            assert "A" not in layer
        finally:
            if os.path.exists(filename):
                os.unlink(filename)


if __name__ == "__main__":
    if HAVE_PYTEST:
        pytest.main([__file__, "-v"])
    else:
        # Run tests manually without pytest
        print("Running tests without pytest...")
        test_classes = [TestLayerBasics(), TestLayerInheritance(), TestHyperspectralUseCase()]
        for test_cls in test_classes:
            print(f"\n{test_cls.__class__.__name__}:")
            for method_name in dir(test_cls):
                if method_name.startswith("test_"):
                    print(f"  {method_name}...", end=" ")
                    try:
                        getattr(test_cls, method_name)()
                        print("✓")
                    except Exception as e:
                        print(f"✗ {e}")
