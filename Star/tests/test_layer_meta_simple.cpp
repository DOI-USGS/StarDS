/**
 * @file test_layer_meta_simple.cpp
 * @brief Simple test to verify LayerMetadataAccessor works
 */

#include "star.h"
#include <iostream>

using namespace star;

int main() {
    std::string filename = "/tmp/test_layer_meta.star";

    // Create dataset
    auto store = StarDataset::create(filename);

    // Add base metadata
    store->meta.put("instrument", NDArray<std::string>({}, "AVIRIS"));
    store->meta.put("scene", NDArray<std::string>({}, "Yellowstone"));

    std::cout << "Base metadata:" << std::endl;
    std::cout << "  instrument: " << store->meta.get("instrument")->as<std::string>().flat(0) << std::endl;
    std::cout << "  scene: " << store->meta.get("scene")->as<std::string>().flat(0) << std::endl;

    // Create layer
    auto layer1 = store->create_layer("band_0");

    // Add layer-specific metadata
    layer1->meta.put("wavelength", NDArray<double>({}, 450.5));
    layer1->meta.put("calibration", NDArray<double>({}, 1.05));

    std::cout << "\nLayer metadata:" << std::endl;
    std::cout << "  wavelength: " << layer1->meta.get("wavelength")->as<double>().flat(0) << std::endl;
    std::cout << "  calibration: " << layer1->meta.get("calibration")->as<double>().flat(0) << std::endl;

    // Test inheritance
    std::cout << "\nInherited from base:" << std::endl;
    std::cout << "  instrument: " << layer1->meta.get("instrument")->as<std::string>().flat(0) << std::endl;
    std::cout << "  scene: " << layer1->meta.get("scene")->as<std::string>().flat(0) << std::endl;

    // Test contains
    std::cout << "\nContains checks:" << std::endl;
    std::cout << "  layer has wavelength: " << (layer1->meta.contains("wavelength") ? "yes" : "no") << std::endl;
    std::cout << "  layer has instrument: " << (layer1->meta.contains("instrument") ? "yes" : "no") << std::endl;
    std::cout << "  base has wavelength: " << (store->meta.contains("wavelength") ? "yes" : "no") << std::endl;

    std::cout << "\n✅ Layer metadata accessor works!" << std::endl;

    std::remove(filename.c_str());
    return 0;
}
