"""
Test that metadata returns native Python types for scalars

Scalars should return as native Python types, not NumPy arrays:
- String -> str
- Int -> int
- Float -> float
- Arrays -> np.ndarray
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


class TestNativeTypes:
    """Test that scalars return as native Python types"""

    def test_scalar_string_returns_str(self):
        """Test that scalar strings return as str, not np.ndarray"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store scalar strings
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["scene"] = "Yellowstone"
            ds.meta["date"] = "2026-04-23"

            # Retrieve - should be str
            instrument = ds.meta["instrument"]
            scene = ds.meta["scene"]
            date = ds.meta["date"]

            print(f"instrument: {instrument!r} (type: {type(instrument).__name__})")
            print(f"scene: {scene!r} (type: {type(scene).__name__})")
            print(f"date: {date!r} (type: {type(date).__name__})")

            # Should be Python str, not np.ndarray
            assert isinstance(instrument, str), f"Expected str, got {type(instrument)}"
            assert isinstance(scene, str), f"Expected str, got {type(scene)}"
            assert isinstance(date, str), f"Expected str, got {type(date)}"

            # Values should be correct
            assert instrument == "AVIRIS"
            assert scene == "Yellowstone"
            assert date == "2026-04-23"

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_scalar_numbers_return_native(self):
        """Test that scalar numbers return as int/float"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store scalar numbers
            ds.meta["count"] = 42
            ds.meta["temperature"] = 23.5

            # Retrieve - should be native types
            count = ds.meta["count"]
            temperature = ds.meta["temperature"]

            print(f"count: {count!r} (type: {type(count).__name__})")
            print(f"temperature: {temperature!r} (type: {type(temperature).__name__})")

            # Should be Python int/float or NumPy scalar (both work with == operator)
            assert isinstance(count, (int, np.integer)), f"Expected int, got {type(count)}"
            assert isinstance(temperature, (float, np.floating)), f"Expected float, got {type(temperature)}"

            # Values should be correct
            assert count == 42
            assert temperature == 23.5

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_arrays_return_ndarray(self):
        """Test that arrays still return as np.ndarray"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store arrays
            ds.meta["wavelengths"] = [450.0, 550.0, 650.0]
            ds.meta["calibration"] = np.array([1.0, 0.99, 0.98])

            # Retrieve - should be np.ndarray
            wavelengths = ds.meta["wavelengths"]
            calibration = ds.meta["calibration"]

            print(f"wavelengths: {wavelengths!r} (type: {type(wavelengths).__name__})")
            print(f"calibration: {calibration!r} (type: {type(calibration).__name__})")

            # Should be np.ndarray
            assert isinstance(wavelengths, np.ndarray)
            assert isinstance(calibration, np.ndarray)

            # Values should be correct
            assert np.allclose(wavelengths, [450.0, 550.0, 650.0])
            assert np.allclose(calibration, [1.0, 0.99, 0.98])

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_metadata_returns_native_types(self):
        """Test that layer metadata also returns native types"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Base metadata
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["base_temperature"] = 20.0

            # Layer metadata
            layer = ds.create_layer("band_0")
            layer.meta["wavelength"] = 450.5
            layer.meta["band_name"] = "Blue"

            # Retrieve from layer
            wavelength = layer.meta["wavelength"]
            band_name = layer.meta["band_name"]
            instrument = layer.meta["instrument"]  # Inherited

            print(f"wavelength: {wavelength!r} (type: {type(wavelength).__name__})")
            print(f"band_name: {band_name!r} (type: {type(band_name).__name__})")
            print(f"instrument: {instrument!r} (type: {type(instrument).__name__})")

            # Should be native types
            assert isinstance(band_name, str), f"Expected str, got {type(band_name)}"
            assert isinstance(instrument, str), f"Expected str, got {type(instrument)}"
            assert isinstance(wavelength, (float, np.floating)), f"Expected float, got {type(wavelength)}"

            # Values should be correct
            assert band_name == "Blue"
            assert instrument == "AVIRIS"
            assert wavelength == 450.5

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_persistence_with_native_types(self):
        """Test that native types persist across close/reopen"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            # Write
            ds = StarDataset.create(filename)
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["count"] = 100
            ds.meta["temperature"] = 25.5
            ds.meta["wavelengths"] = [400.0, 500.0, 600.0]
            ds.close()

            # Read
            ds = StarDataset.open(filename)

            instrument = ds.meta["instrument"]
            count = ds.meta["count"]
            temperature = ds.meta["temperature"]
            wavelengths = ds.meta["wavelengths"]

            # Scalars should still be native types
            assert isinstance(instrument, str)
            assert instrument == "AVIRIS"

            assert isinstance(count, (int, np.integer))
            assert count == 100

            assert isinstance(temperature, (float, np.floating))
            assert temperature == 25.5

            # Arrays should still be arrays
            assert isinstance(wavelengths, np.ndarray)
            assert np.allclose(wavelengths, [400.0, 500.0, 600.0])

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_direct_usage(self):
        """Test direct string usage without type checking"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store
            ds.meta["scene"] = "Yellowstone National Park"
            ds.meta["altitude"] = 2357.5

            # Retrieve and use directly
            scene = ds.meta["scene"]
            altitude = ds.meta["altitude"]

            # Should work like native types
            print(f"Scene: {scene}")
            print(f"Upper: {scene.upper()}")  # String method
            print(f"Length: {len(scene)}")    # String length
            print(f"Altitude: {altitude}")
            print(f"Rounded: {round(altitude, 1)}")  # Float method

            # String operations should work
            assert scene.upper() == "YELLOWSTONE NATIONAL PARK"
            assert len(scene) == 26
            assert round(altitude, 1) == 2357.5

        finally:
            if os.path.exists(filename):
                os.unlink(filename)


if __name__ == "__main__":
    if HAVE_PYTEST:
        pytest.main([__file__, "-v"])
    else:
        # Run tests manually without pytest
        print("Running tests without pytest...\n")

        test_cls = TestNativeTypes()
        passed = 0
        failed = 0

        for method_name in dir(test_cls):
            if method_name.startswith("test_"):
                print(f"{method_name}:")
                try:
                    getattr(test_cls, method_name)()
                    print("  ✓ PASS\n")
                    passed += 1
                except Exception as e:
                    print(f"  ✗ FAIL: {e}\n")
                    failed += 1
                    import traceback
                    traceback.print_exc()

        print(f"\n{'='*60}")
        print(f"Results: {passed} passed, {failed} failed")
        if failed == 0:
            print("✅ All tests passed!")
        print(f"{'='*60}")
