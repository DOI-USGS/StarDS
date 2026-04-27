"""
Tests for layer functionality
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


class TestLayerBasics:
    """Basic layer creation and access tests"""

    def test_create_layer(self):
        """Test creating a new layer"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Add base metadata
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["date"] = "2026-04-22"

            # Add base array
            ds["cube"] = np.random.rand(10, 10, 10)

            # Create layer
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

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
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            # Create and write
            ds = StarDataset.create(filename)
            ds.meta["instrument"] = "AVIRIS"
            ds.create_layer("band_0")
            ds.create_layer("band_1")
            ds.create_layer("band_2")
            ds.close()

            # Reopen and verify
            ds = StarDataset.open(filename)
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
