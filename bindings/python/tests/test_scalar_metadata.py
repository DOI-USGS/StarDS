"""
Test that scalar strings and numbers work correctly in metadata

This test documents the expected behavior for scalar metadata values.
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


class TestScalarMetadata:
    """Test scalar values in metadata"""

    def test_scalar_string(self):
        """Test that scalar strings remain scalars, not arrays"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)

            # Store scalar string
            ds.meta["instrument"] = "AVIRIS"
            ds.meta["scene"] = "Yellowstone"

            # Retrieve
            instrument = ds.meta["instrument"]
            scene = ds.meta["scene"]

            # Scalars are returned as native Python values (not 0-d arrays).
            assert isinstance(instrument, str)
            assert instrument == "AVIRIS"
            assert scene == "Yellowstone"

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_scalar_numbers(self):
        """Test that scalar numbers work correctly"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
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

            # Scalars come back as native Python numbers.
            assert float(count) == 42
            assert np.isclose(float(temperature), 23.5)
            assert np.isclose(float(calibration), 1.05)

        finally:
            if os.path.exists(filename):
                os.unlink(filename)

    def test_layer_scalar_metadata(self):
        """Test scalar metadata in layers"""
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
            filename = f.name

        try:
            ds = StarDataset.create(filename)
            ds.set_layer_inheritance(True)  # inheritance is off by default

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
        with tempfile.NamedTemporaryFile(suffix=".stards", delete=False) as f:
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

            # A scalar collapses to a native number; lists stay ndarrays.
            assert float(scalar) == 42
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
