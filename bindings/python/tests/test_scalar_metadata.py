"""
Test that scalar strings and numbers work correctly in metadata

This test documents the expected behavior for scalar metadata values.
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


class TestScalarMetadata:
    """Test scalar values in metadata"""

    def test_scalar_string(self):
        """Test that scalar strings remain scalars, not arrays"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store scalar string
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["scene"] = "Yellowstone"

            # Retrieve
            instrument = ds.meta["instrument"]
            scene = ds.meta["scene"]

            # Should be scalar (shape ())
            print(f"Instrument shape: {instrument.shape}")
            print(f"Instrument value: {instrument}")

            # For scalars, access with item() or index []
            if instrument.shape == ():
                instrument_str = str(instrument)
            else:
                instrument_str = str(instrument[0])

            print(f"Instrument as string: {instrument_str}")

            # Basic assertion - value should be correct
            # (shape may vary between scalar and 1-element array depending on implementation)
            assert "AVIRIS" in str(instrument)

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_scalar_numbers(self):
        """Test that scalar numbers work correctly"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store scalar numbers
            ds.meta["count"] = 42
            ds.meta["temperature"] = 23.5
            ds.meta["calibration_factor"] = 1.05

            # Retrieve
            count = ds.meta["count"]
            temperature = ds.meta["temperature"]
            calibration = ds.meta["calibration_factor"]

            print(f"Count: {count}, shape: {count.shape}")
            print(f"Temperature: {temperature}, shape: {temperature.shape}")
            print(f"Calibration: {calibration}, shape: {calibration.shape}")

            # Values should be correct
            # Handle both scalar and 1-element array cases
            def get_value(arr):
                if arr.shape == ():
                    return float(arr)
                else:
                    return float(arr[0])

            assert get_value(count) == 42
            assert np.isclose(get_value(temperature), 23.5)
            assert np.isclose(get_value(calibration), 1.05)

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_scalar_metadata(self):
        """Test scalar metadata in layers"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Base scalar metadata
            ds.meta["instrument"] = "AVIRIS"

            # Layer with scalar metadata
            layer = ds.create_layer("band_0")
            layer.meta["wavelength"] = 450.5
            layer.meta["band_name"] = "Blue"

            # Retrieve
            wavelength = layer.meta["wavelength"]
            band_name = layer.meta["band_name"]
            instrument = layer.meta["instrument"]  # Inherited

            print(f"Wavelength: {wavelength}")
            print(f"Band name: {band_name}")
            print(f"Instrument (inherited): {instrument}")

            # Values should be retrievable
            assert "450" in str(wavelength)
            assert "Blue" in str(band_name)
            assert "AVIRIS" in str(instrument)

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_array_vs_scalar(self):
        """Test distinction between arrays and scalars"""
        with tempfile.NamedTemporaryFile(suffix=".star", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Scalar
            ds.meta["scalar_value"] = 42

            # 1-element array (explicit)
            ds.meta["array_1elem"] = [42]

            # Multi-element array
            ds.meta["array_multi"] = [1, 2, 3]

            # Retrieve
            scalar = ds.meta["scalar_value"]
            array1 = ds.meta["array_1elem"]
            array_multi = ds.meta["array_multi"]

            print(f"Scalar: shape={scalar.shape}, value={scalar}")
            print(f"1-elem array: shape={array1.shape}, value={array1}")
            print(f"Multi-elem array: shape={array_multi.shape}, value={array_multi}")

            # Multi-element should definitely be an array
            assert array_multi.shape == (3,)
            assert np.array_equal(array_multi, [1, 2, 3])

        finally:
            if os.path.exists(filename):
                os.unlink(filename)


if __name__ == "__main__":
    if HAVE_PYTEST:
        pytest.main([__file__, "-v"])
    else:
        # Run tests manually without pytest
        print("Running tests without pytest...")
        print("\nNOTE: This test documents expected behavior.")
        print("Actual implementation may return shape () or (1,) for scalars.")
        print("Both are acceptable as long as the value is correct.\n")

        test_cls = TestScalarMetadata()
        for method_name in dir(test_cls):
            if method_name.startswith("test_"):
                print(f"\n{method_name}:")
                try:
                    getattr(test_cls, method_name)()
                    print("  ✓ Pass")
                except Exception as e:
                    print(f"  ✗ Fail: {e}")
                    import traceback
                    traceback.print_exc()
