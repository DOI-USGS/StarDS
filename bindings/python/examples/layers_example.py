#!/usr/bin/env python3
"""
Layers example for STARDS Python bindings.

This example demonstrates:
- Creating layers for multi-version data storage
- Layer isolation (each layer has independent data)
- Layer inheritance (layers can inherit from base)
- Use case: hyperspectral image processing pipeline
"""

import numpy as np
from pystar import StarDataset

def main():
    print("=== STARDS Layers Example ===\n")

    # Create a hyperspectral dataset
    print("Creating hyperspectral dataset with multiple processing versions...")
    ds = StarDataset.create("hyperspectral.star")

    # Base layer: raw data
    print("\n1. Storing raw (base) data...")
    raw_cube = np.random.rand(512, 512, 300).astype(np.float32)
    wavelengths = np.linspace(400, 2500, 300)

    ds["cube"] = raw_cube
    ds["wavelengths"] = wavelengths
    ds.meta["description"] = "Raw hyperspectral cube"
    ds.meta["instrument"] = "AVIRIS-NG"

    print(f"   Stored cube: shape={raw_cube.shape}")
    print(f"   Wavelength range: {wavelengths[0]:.0f}-{wavelengths[-1]:.0f} nm")

    # Layer 1: Atmospherically corrected
    print("\n2. Creating 'atm_corrected' layer...")
    atm_layer = ds.create_layer("atm_corrected")

    # Simulate atmospheric correction (just add offset for demo)
    corrected_cube = raw_cube + 0.05
    atm_layer["cube"] = corrected_cube
    # wavelengths inherited from base (not overridden)
    atm_layer.meta["description"] = "Atmospherically corrected"
    atm_layer.meta["correction_applied"] = "FLAASH"

    print(f"   Stored corrected cube: shape={corrected_cube.shape}")
    print("   Wavelengths inherited from base layer")

    # Layer 2: Spectral subset
    print("\n3. Creating 'vnir_only' layer...")
    vnir_layer = ds.create_layer("vnir_only")

    # Keep only VNIR bands (400-1000 nm)
    vnir_mask = wavelengths < 1000
    vnir_cube = raw_cube[:, :, vnir_mask]
    vnir_wavelengths = wavelengths[vnir_mask]

    vnir_layer["cube"] = vnir_cube
    vnir_layer["wavelengths"] = vnir_wavelengths  # Override wavelengths
    vnir_layer.meta["description"] = "VNIR subset"
    vnir_layer.meta["band_range"] = "400-1000 nm"

    print(f"   Stored VNIR cube: shape={vnir_cube.shape}")
    print(f"   Wavelength range: {vnir_wavelengths[0]:.0f}-{vnir_wavelengths[-1]:.0f} nm")

    # Save to disk
    print("\n4. Flushing to disk...")
    ds.flush()
    print(f"   File written: hyperspectral.star")

    # Clean up
    del ds, atm_layer, vnir_layer

    # Read back and verify
    print("\n=== Reading back from file ===\n")
    ds2 = StarDataset.open("hyperspectral.star")

    # Base layer
    print("Base layer:")
    base_cube = ds2["cube"]
    base_waves = ds2["wavelengths"]
    base_desc = ds2.meta["description"]
    print(f"  Cube: {base_cube.shape}, wavelengths: {len(base_waves)}")
    print(f"  Description: {base_desc}")
    print(f"  First pixel spectrum mean: {base_cube[0, 0, :].mean():.4f}")

    # Atmospherically corrected layer
    print("\nAtmospherically corrected layer:")
    atm_layer2 = ds2.get_layer("atm_corrected")
    atm_cube = atm_layer2["cube"]
    atm_waves = atm_layer2["wavelengths"]  # Inherited from base
    atm_desc = atm_layer2.meta["description"]
    correction = atm_layer2.meta["correction_applied"]
    print(f"  Cube: {atm_cube.shape}, wavelengths: {len(atm_waves)} (inherited)")
    print(f"  Description: {atm_desc}")
    print(f"  Correction: {correction}")
    print(f"  First pixel spectrum mean: {atm_cube[0, 0, :].mean():.4f}")

    # VNIR layer
    print("\nVNIR subset layer:")
    vnir_layer2 = ds2.get_layer("vnir_only")
    vnir_cube2 = vnir_layer2["cube"]
    vnir_waves2 = vnir_layer2["wavelengths"]  # Overridden
    vnir_desc = vnir_layer2.meta["description"]
    band_range = vnir_layer2.meta["band_range"]
    print(f"  Cube: {vnir_cube2.shape}, wavelengths: {len(vnir_waves2)} (overridden)")
    print(f"  Description: {vnir_desc}")
    print(f"  Band range: {band_range}")

    # Verify data integrity
    print("\n=== Verifying layer isolation ===")
    assert base_cube.shape == (512, 512, 300), "Base shape mismatch!"
    assert atm_cube.shape == (512, 512, 300), "Atm shape mismatch!"
    assert vnir_cube2.shape == (512, 512, vnir_mask.sum()), "VNIR shape mismatch!"

    assert len(base_waves) == 300, "Base wavelengths mismatch!"
    assert len(atm_waves) == 300, "Atm wavelengths should be inherited!"
    assert len(vnir_waves2) < 300, "VNIR wavelengths should be subset!"

    # Verify different values in each layer
    base_mean = base_cube[0, 0, :].mean()
    atm_mean = atm_cube[0, 0, :].mean()
    assert not np.isclose(base_mean, atm_mean), "Layers should have different data!"

    print("✓ All layers have independent, isolated data")
    print("✓ Inheritance works correctly")
    print("✓ Layer isolation verified!")

    print("\n=== Key Concept: Automatic Inheritance ===")
    print("When you access a key in a layer that wasn't explicitly set,")
    print("it AUTOMATICALLY falls back to the base layer.")
    print("\nExample from above:")
    print("- atm_layer['cube'] was SET → returns layer-specific data")
    print("- atm_layer['wavelengths'] was NOT SET → automatically returns base wavelengths")
    print("\nThis means you only store what's DIFFERENT in each layer!")
    print("Everything else is inherited automatically - no duplication needed.")

    print("\n=== Use Cases for Layers ===")
    print("- Hyperspectral processing pipelines (as shown)")
    print("- Multi-temporal datasets (different time slices)")
    print("- A/B testing (variant versions of data)")
    print("- Version control for datasets")
    print("- Feature engineering (derived features per variant)")

if __name__ == "__main__":
    main()
