#include "../Star/include/star.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <map>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;
using namespace star;

// Forward declarations
void print_usage(const char* program_name);
void star_to_json(const std::string& input_file, const std::string& output_file);
void json_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size);
json NDArray_to_json(const ValueVariant& variant);
ValueVariant json_to_NDArray(const json& j);

#ifdef ENABLE_MSGPACK
#include <msgpack.hpp>
void star_to_msgpack(const std::string& input_file, const std::string& output_file);
void msgpack_to_star(const std::string& input_file, const std::string& output_file,
                    CompressionAlgorithm compression, size_t block_size);
#endif

// CSV support
void csv_to_star(const std::string& input_file, const std::string& output_file,
                CompressionAlgorithm compression, size_t block_size);
void star_to_csv(const std::string& input_file, const std::string& output_file);

// ISDS support (ISIS camera state files)
void isds_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size);
void star_to_isds(const std::string& input_file, const std::string& output_file);

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <input_file> <output_file>\n\n";
    std::cout << "Convert between STAR and other formats (JSON, MessagePack, ISDS)\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -f, --format <fmt>      Output format (json, msgpack, isds)\n";
    std::cout << "                          Auto-detected from output extension if not specified\n";
    std::cout << "  -c, --compression <alg> Compression algorithm for STAR output\n";
    std::cout << "                          Options: none, gzip, lz4, zstd\n";
    std::cout << "                          Default: lz4\n";
    std::cout << "  -b, --block-size <size> Block size for STAR compression (bytes)\n";
    std::cout << "                          Default: 1048576 (1MB)\n";
    std::cout << "  -t, --threshold <size>  Array size threshold for ISDS conversion (elements)\n";
    std::cout << "                          Arrays larger than this go to array storage,\n";
    std::cout << "                          smaller arrays go to metadata\n";
    std::cout << "                          Default: 100\n";
    std::cout << "\nSupported Formats:\n";
    std::cout << "  - STAR (Simple Tensors Arrays and Rasters) - .star extension\n";
    std::cout << "  - JSON - .json extension\n";
    std::cout << "  - CSV - .csv extension (2D arrays only)\n";
    std::cout << "  - ISDS (ISIS Dataset) - .star extension with ISDS flag\n";
#ifdef ENABLE_MSGPACK
    std::cout << "  - MessagePack - .msgpack or .mp extension\n";
#endif
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " data.star data.json           # STAR to JSON\n";
    std::cout << "  " << program_name << " data.json data.star           # JSON to STAR\n";
    std::cout << "  " << program_name << " -c gzip data.json data.star   # JSON to STAR with GZIP\n";
    std::cout << "  " << program_name << " data.csv data.star            # CSV to STAR\n";
    std::cout << "  " << program_name << " data.star data.csv            # STAR to CSV\n";
    std::cout << "  " << program_name << " -f isds input.star out.star   # ISDS to STAR conversion\n";
    std::cout << "  " << program_name << " -f isds -t 50 in.star out.star # ISDS with custom threshold\n";
#ifdef ENABLE_MSGPACK
    std::cout << "  " << program_name << " data.star data.msgpack        # STAR to MessagePack\n";
    std::cout << "  " << program_name << " data.msgpack data.star        # MessagePack to STAR\n";
#endif
}

std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return filename.substr(dot_pos + 1);
}

std::string detect_format(const std::string& filename, bool explicit_isds = false) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "star") {
        if (explicit_isds) return "isds";
        return "star";
    }
    if (ext == "json") return "json";
    if (ext == "csv") return "csv";
    if (ext == "msgpack" || ext == "mp") return "msgpack";

    return "";
}

json NDArray_to_json(const ValueVariant& variant) {
    json j;

    // Use std::visit to handle different NDArray types
    std::visit([&j](auto&& arr) {
        using T = std::decay_t<decltype(arr)>;
        using ValueType = typename T::value_type;

        j["dtype"] = datatype_to_string(TypeToDataType<ValueType>::value);
        j["shape"] = arr.shape();

        // Store data as flat array
        json data_array = json::array();
        for (const auto& val : arr) {
            if constexpr (std::is_same<ValueType, std::string>::value) {
                data_array.push_back(val);
            } else {
                data_array.push_back(val);
            }
        }
        j["data"] = data_array;
    }, variant);

    return j;
}

ValueVariant json_to_NDArray(const json& j) {
    std::string dtype = j["dtype"].get<std::string>();
    std::vector<size_t> shape = j["shape"].get<std::vector<size_t>>();

    if (dtype == "int8") {
        NDArray<int8_t> arr(shape);
        auto data = j["data"].get<std::vector<int8_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "int16") {
        NDArray<int16_t> arr(shape);
        auto data = j["data"].get<std::vector<int16_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "int32") {
        NDArray<int32_t> arr(shape);
        auto data = j["data"].get<std::vector<int32_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "int64") {
        NDArray<int64_t> arr(shape);
        auto data = j["data"].get<std::vector<int64_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint8") {
        NDArray<uint8_t> arr(shape);
        auto data = j["data"].get<std::vector<uint8_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint16") {
        NDArray<uint16_t> arr(shape);
        auto data = j["data"].get<std::vector<uint16_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint32") {
        NDArray<uint32_t> arr(shape);
        auto data = j["data"].get<std::vector<uint32_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint64") {
        NDArray<uint64_t> arr(shape);
        auto data = j["data"].get<std::vector<uint64_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "float32") {
        NDArray<float> arr(shape);
        auto data = j["data"].get<std::vector<float>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "float64") {
        NDArray<double> arr(shape);
        auto data = j["data"].get<std::vector<double>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "string") {
        NDArray<std::string> arr(shape);
        auto data = j["data"].get<std::vector<std::string>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else {
        throw std::runtime_error("Unsupported data type: " + dtype);
    }
}

void star_to_json(const std::string& input_file, const std::string& output_file) {
    std::cout << "Converting STAR to JSON...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";

    // Open STAR file
    auto store = StarDataset::open(input_file, FileMode::READ_ONLY);

    json root;
    root["format"] = "star";
    root["version"] = "1.0";

    json arrays = json::object();

    // Get all keys (metadata block + separate arrays)
    std::vector<std::string> all_keys = store->get_all_keys();

    // Iterate through all keys
    for (const auto& key : all_keys) {
        std::cout << "  Converting key: " << key << "\n";

        ValueVariant variant;
        // Use new metadata API (no type guessing needed!)
        auto meta = store->meta.get(key);
        bool found = (meta != nullptr);

        if (found) {
            variant = meta->data;
        }

        if (!found) {
            std::cerr << "Warning: Could not read key: " << key << "\n";
            continue;
        }

        arrays[key] = NDArray_to_json(variant);
    }

    root["arrays"] = arrays;

    // Write JSON file
    std::ofstream ofs(output_file);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + output_file);
    }

    ofs << root.dump(2);
    ofs.close();

    std::cout << "Conversion complete! Converted " << arrays.size() << " arrays.\n";
}

// Helper: Detect type of JSON array elements
std::string detect_json_array_type(const json& arr) {
    if (arr.empty()) return "float64";

    bool all_int = true;
    bool all_number = true;

    for (const auto& elem : arr) {
        if (elem.is_string()) {
            return "string";
        }
        if (!elem.is_number()) {
            all_number = false;
            all_int = false;
            break;
        }
        if (!elem.is_number_integer()) {
            all_int = false;
        }
    }

    if (all_int) return "int64";
    if (all_number) return "float64";
    return "string";
}

// Forward declaration for recursive flattening
void flatten_json_object(const json& obj, const std::string& prefix, StarDataset& store, size_t& converted_count);

// Helper: Convert simple JSON value/array to NDArray
ValueVariant json_value_to_NDArray(const json& value, const std::string& key) {
    // Handle scalar values - convert to 1-element array
    if (value.is_number_integer()) {
        NDArray<int64_t> arr({1});
        arr(0) = value.get<int64_t>();
        return arr;
    } else if (value.is_number_float()) {
        NDArray<double> arr({1});
        arr(0) = value.get<double>();
        return arr;
    } else if (value.is_string()) {
        NDArray<std::string> arr({1});
        arr(0) = value.get<std::string>();
        return arr;
    } else if (value.is_boolean()) {
        NDArray<int64_t> arr({1});
        arr(0) = value.get<bool>() ? 1 : 0;
        return arr;
    }
    // Handle arrays
    else if (value.is_array()) {
        if (value.empty()) {
            // Empty array - default to float64
            NDArray<double> arr({0});
            return arr;
        }

        // Check if 2D array (array of arrays)
        bool is_2d = value[0].is_array();

        if (is_2d) {
            // 2D array handling
            size_t rows = value.size();
            size_t cols = 0;

            // Find max column size and validate all rows are arrays
            for (size_t i = 0; i < rows; ++i) {
                if (!value[i].is_array()) {
                    throw std::runtime_error("Inconsistent 2D array structure for key: " + key);
                }
                cols = std::max(cols, value[i].size());
            }

            if (cols == 0) {
                // Empty 2D array
                NDArray<double> arr({rows, 0});
                return arr;
            }

            // Detect type from first non-empty row
            std::string dtype = "float64";
            for (size_t i = 0; i < rows; ++i) {
                if (!value[i].empty()) {
                    dtype = detect_json_array_type(value[i]);
                    break;
                }
            }

            if (dtype == "int64") {
                NDArray<int64_t> arr({rows, cols});
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < value[i].size() && j < cols; ++j) {
                        arr(i, j) = value[i][j].get<int64_t>();
                    }
                }
                return arr;
            } else if (dtype == "float64") {
                NDArray<double> arr({rows, cols});
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < value[i].size() && j < cols; ++j) {
                        arr(i, j) = value[i][j].get<double>();
                    }
                }
                return arr;
            } else {
                NDArray<std::string> arr({rows, cols});
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < value[i].size() && j < cols; ++j) {
                        arr(i, j) = value[i][j].get<std::string>();
                    }
                }
                return arr;
            }
        } else {
            // 1D array handling
            std::string dtype = detect_json_array_type(value);
            size_t size = value.size();

            if (dtype == "int64") {
                NDArray<int64_t> arr({size});
                for (size_t i = 0; i < size; ++i) {
                    arr(i) = value[i].get<int64_t>();
                }
                return arr;
            } else if (dtype == "float64") {
                NDArray<double> arr({size});
                for (size_t i = 0; i < size; ++i) {
                    arr(i) = value[i].get<double>();
                }
                return arr;
            } else {
                NDArray<std::string> arr({size});
                for (size_t i = 0; i < size; ++i) {
                    arr(i) = value[i].get<std::string>();
                }
                return arr;
            }
        }
    }

    throw std::runtime_error("Unsupported JSON value type for key: " + key);
}

// Helper: Recursively flatten nested JSON objects
void flatten_json_object(const json& obj, const std::string& prefix, StarDataset& store, size_t& converted_count) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        std::string key = it.key();
        const json& value = it.value();

        // Build nested key with forward slash separator
        std::string full_key = prefix.empty() ? key : (prefix + "/" + key);

        // If this is another nested object, recurse
        if (value.is_object()) {
            std::cout << "  Flattening nested object: " << full_key << "\n";
            flatten_json_object(value, full_key, store, converted_count);
        } else {
            // Convert this value to NDArray
            try {
                std::cout << "  Converting key: " << full_key;

                ValueVariant variant = json_value_to_NDArray(value, full_key);

                // Show what was detected
                std::visit([](auto&& arr) {
                    using T = std::decay_t<decltype(arr)>;
                    using ValueType = typename T::value_type;
                    std::cout << " (" << datatype_to_string(TypeToDataType<ValueType>::value);
                    if (arr.size() == 1) {
                        std::cout << ", scalar → 1-element array";
                    } else {
                        std::cout << ", " << arr.size() << " elements";
                    }
                    std::cout << ")\n";
                }, variant);

                // Put into store using std::visit
                std::visit([&store, &full_key](auto&& arr) {
                    store.meta.put(full_key, arr);
                }, variant);

                converted_count++;
            } catch (const std::exception& e) {
                std::cerr << "  Warning: Skipping key '" << full_key << "': " << e.what() << "\n";
            }
        }
    }
}

void json_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Converting JSON to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: ";
    switch(compression) {
        case CompressionAlgorithm::NONE: std::cout << "None\n"; break;
        case CompressionAlgorithm::GZIP: std::cout << "GZIP\n"; break;
        default: std::cout << "Unknown\n";
    }
    std::cout << "  Block size: " << block_size << " bytes\n";

    // Read JSON file
    std::ifstream ifs(input_file);
    if (!ifs) {
        throw std::runtime_error("Failed to open input file: " + input_file);
    }

    json root;
    ifs >> root;
    ifs.close();

    // Create STAR store
    auto store = StarDataset::create(output_file);  // Compression/block size handled by metadata block config

    // Check if this is structured format or simple format
    bool is_structured_format = root.contains("format") && root.contains("arrays");

    if (is_structured_format) {
        // Structured format: {"format": "star", "version": "1.0", "arrays": {...}}
        std::cout << "  Detected: Structured JSON format\n";

        if (!root.contains("arrays")) {
            throw std::runtime_error("Invalid JSON format: missing 'arrays' field");
        }

        json arrays = root["arrays"];

        // Convert each array
        for (auto it = arrays.begin(); it != arrays.end(); ++it) {
            std::string key = it.key();
            json array_data = it.value();

            std::cout << "  Converting key: " << key << "\n";

            ValueVariant variant = json_to_NDArray(array_data);

            // Put into store using std::visit
            std::visit([&store, &key](auto&& arr) {
                store->meta.put(key, arr);
            }, variant);
        }

        std::cout << "Conversion complete! Converted " << arrays.size() << " arrays.\n";
    } else {
        // Simple format: direct key-value pairs
        std::cout << "  Detected: Simple JSON format\n";
        std::cout << "  Converting scalars to 1-element arrays\n";
        std::cout << "  Auto-detecting types (int64, float64, string)\n";
        std::cout << "  Flattening nested objects with slash notation (e.g., parent/child)\n";

        size_t converted_count = 0;

        for (auto it = root.begin(); it != root.end(); ++it) {
            std::string key = it.key();
            const json& value = it.value();

            // Handle nested objects by flattening
            if (value.is_object()) {
                std::cout << "  Flattening nested object: " << key << "\n";
                flatten_json_object(value, key, *store, converted_count);
            } else {
                // Convert this value to NDArray
                try {
                    std::cout << "  Converting key: " << key;

                    ValueVariant variant = json_value_to_NDArray(value, key);

                    // Show what was detected
                    std::visit([](auto&& arr) {
                        using T = std::decay_t<decltype(arr)>;
                        using ValueType = typename T::value_type;
                        std::cout << " (" << datatype_to_string(TypeToDataType<ValueType>::value);
                        if (arr.size() == 1) {
                            std::cout << ", scalar → 1-element array";
                        } else {
                            std::cout << ", " << arr.size() << " elements";
                        }
                        std::cout << ")\n";
                    }, variant);

                    // Put into store using std::visit
                    std::visit([&store, &key](auto&& arr) {
                        store->meta.put(key, arr);
                    }, variant);

                    converted_count++;
                } catch (const std::exception& e) {
                    std::cerr << "  Warning: Skipping key '" << key << "': " << e.what() << "\n";
                }
            }
        }

        std::cout << "Conversion complete! Converted " << converted_count << " arrays.\n";
    }
}

#ifdef ENABLE_MSGPACK
void star_to_msgpack(const std::string& input_file, const std::string& output_file) {
    std::cout << "Converting STAR to MessagePack...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";

    // Open STAR file
    auto store = StarDataset::open(input_file, FileMode::READ_ONLY);

    // Get all keys
    std::vector<std::string> all_keys = store->get_all_keys();

    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(&buffer);

    // Pack as map
    packer.pack_map(2);

    // Format and version
    packer.pack(std::string("format"));
    packer.pack(std::string("star"));

    packer.pack(std::string("arrays"));
    packer.pack_map(all_keys.size());

    // Iterate through all keys
    for (const auto& key : all_keys) {
        std::cout << "  Converting key: " << key << "\n";

        packer.pack(key);
        packer.pack_map(3);

        // Use new metadata API for type introspection
        auto meta = store->meta.get(key);
        if (!meta) {
            std::cerr << "Warning: Could not read key: " << key << "\n";
            continue;
        }

        // Pack dtype
        packer.pack(std::string("dtype"));
        packer.pack(meta->type_name());

        // Pack shape
        packer.pack(std::string("shape"));
        packer.pack(meta->shape);

        // Pack data based on type
        packer.pack(std::string("data"));

        switch (meta->dtype) {
            case DataType::INT8: {
                auto arr = meta->as<int8_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::INT16: {
                auto arr = meta->as<int16_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::INT32: {
                auto arr = meta->as<int32_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::INT64: {
                auto arr = meta->as<int64_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::UINT8: {
                auto arr = meta->as<uint8_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::UINT16: {
                auto arr = meta->as<uint16_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::UINT32: {
                auto arr = meta->as<uint32_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::UINT64: {
                auto arr = meta->as<uint64_t>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::FLOAT32: {
                auto arr = meta->as<float>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::FLOAT64: {
                auto arr = meta->as<double>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            case DataType::STRING: {
                auto arr = meta->as<std::string>();
                packer.pack_array(arr.size());
                for (size_t i = 0; i < arr.size(); ++i) packer.pack(arr.flat(i));
                break;
            }
            default:
                std::cerr << "Warning: Unsupported data type for key: " << key << "\n";
                continue;
        }
    }

    // Write to file
    std::ofstream ofs(output_file, std::ios::binary);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + output_file);
    }

    ofs.write(buffer.data(), buffer.size());
    ofs.close();

    std::cout << "Conversion complete!\n";
}

void msgpack_to_star(const std::string& input_file, const std::string& output_file,
                    CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Converting MessagePack to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: ";
    switch(compression) {
        case CompressionAlgorithm::NONE: std::cout << "None\n"; break;
        case CompressionAlgorithm::GZIP: std::cout << "GZIP\n"; break;
        default: std::cout << "Unknown\n";
    }
    std::cout << "  Block size: " << block_size << " bytes\n";

    // Read MessagePack file
    std::ifstream ifs(input_file, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("Failed to open input file: " + input_file);
    }

    std::stringstream buffer;
    buffer << ifs.rdbuf();
    ifs.close();

    std::string str = buffer.str();

    msgpack::object_handle oh = msgpack::unpack(str.data(), str.size());
    msgpack::object obj = oh.get();

    // Validate format
    if (obj.type != msgpack::type::MAP) {
        throw std::runtime_error("Invalid MessagePack format: root must be a map");
    }

    msgpack::object_map map = obj.via.map;
    msgpack::object_kv* arrays_kv = nullptr;

    // Find arrays field
    for (uint32_t i = 0; i < map.size; ++i) {
        std::string key;
        map.ptr[i].key.convert(key);
        if (key == "arrays") {
            arrays_kv = &map.ptr[i];
            break;
        }
    }

    if (!arrays_kv || arrays_kv->val.type != msgpack::type::MAP) {
        throw std::runtime_error("Invalid MessagePack format: missing or invalid 'arrays' field");
    }

    // Create STAR store
    auto store = StarDataset::create(output_file);  // Compression/block size handled by metadata block config

    msgpack::object_map arrays_map = arrays_kv->val.via.map;

    // Process each array
    for (uint32_t i = 0; i < arrays_map.size; ++i) {
        std::string array_key;
        arrays_map.ptr[i].key.convert(array_key);

        std::cout << "  Converting key: " << array_key << "\n";

        msgpack::object_map array_obj = arrays_map.ptr[i].val.via.map;

        std::string dtype;
        std::vector<size_t> shape;
        msgpack::object* data_obj = nullptr;

        // Parse array metadata
        for (uint32_t j = 0; j < array_obj.size; ++j) {
            std::string field_name;
            array_obj.ptr[j].key.convert(field_name);

            if (field_name == "dtype") {
                array_obj.ptr[j].val.convert(dtype);
            } else if (field_name == "shape") {
                array_obj.ptr[j].val.convert(shape);
            } else if (field_name == "data") {
                data_obj = &array_obj.ptr[j].val;
            }
        }

        if (dtype.empty() || shape.empty() || !data_obj) {
            throw std::runtime_error("Invalid array format for key: " + array_key);
        }

        // Convert based on dtype
        if (dtype == "int8") {
            NDArray<int8_t> arr(shape);
            std::vector<int8_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "int16") {
            NDArray<int16_t> arr(shape);
            std::vector<int16_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "int32") {
            NDArray<int32_t> arr(shape);
            std::vector<int32_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "int64") {
            NDArray<int64_t> arr(shape);
            std::vector<int64_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "uint8") {
            NDArray<uint8_t> arr(shape);
            std::vector<uint8_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "uint16") {
            NDArray<uint16_t> arr(shape);
            std::vector<uint16_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "uint32") {
            NDArray<uint32_t> arr(shape);
            std::vector<uint32_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "uint64") {
            NDArray<uint64_t> arr(shape);
            std::vector<uint64_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "float32") {
            NDArray<float> arr(shape);
            std::vector<float> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "float64") {
            NDArray<double> arr(shape);
            std::vector<double> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else if (dtype == "string") {
            NDArray<std::string> arr(shape);
            std::vector<std::string> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store->meta.put(array_key, arr);
        } else {
            throw std::runtime_error("Unsupported data type: " + dtype);
        }
    }

    std::cout << "Conversion complete! Converted " << arrays_map.size << " arrays.\n";
}
#endif

// CSV helper: parse a CSV line
std::vector<std::string> parse_csv_line(const std::string& line, char delimiter = ',') {
    std::vector<std::string> result;
    std::string cell;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];

        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == delimiter && !in_quotes) {
            result.push_back(cell);
            cell.clear();
        } else {
            cell += c;
        }
    }
    result.push_back(cell);
    return result;
}

// CSV helper: detect data type from strings
std::string detect_csv_dtype(const std::vector<std::vector<std::string>>& data) {
    if (data.empty() || data[0].empty()) {
        return "string";
    }

    bool is_integer = true;
    bool is_float = true;

    // Sample first few rows to detect type
    size_t sample_size = std::min(data.size(), size_t(100));

    for (size_t i = 0; i < sample_size; ++i) {
        for (const auto& cell : data[i]) {
            if (cell.empty()) continue;

            // Try to parse as double
            try {
                size_t pos;
                double val = std::stod(cell, &pos);
                if (pos != cell.length()) {
                    is_integer = false;
                    is_float = false;
                    break;
                }
                // Check if it's an integer
                if (val != std::floor(val)) {
                    is_integer = false;
                }
            } catch (...) {
                is_integer = false;
                is_float = false;
                break;
            }
        }
        if (!is_float && !is_integer) break;
    }

    if (is_integer) return "int64";
    if (is_float) return "float64";
    return "string";
}

void csv_to_star(const std::string& input_file, const std::string& output_file,
                CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Converting CSV to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: ";
    switch(compression) {
        case CompressionAlgorithm::NONE: std::cout << "None\n"; break;
        case CompressionAlgorithm::GZIP: std::cout << "GZIP\n"; break;
        default: std::cout << "Unknown\n";
    }
    std::cout << "  Block size: " << block_size << " bytes\n";

    // Read CSV file
    std::ifstream ifs(input_file);
    if (!ifs) {
        throw std::runtime_error("Failed to open input file: " + input_file);
    }

    std::vector<std::vector<std::string>> data;
    std::string line;

    // Read all lines
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;
        data.push_back(parse_csv_line(line));
    }
    ifs.close();

    if (data.empty()) {
        throw std::runtime_error("CSV file is empty");
    }

    // Detect data type
    std::string dtype = detect_csv_dtype(data);
    size_t rows = data.size();
    size_t cols = data[0].size();

    std::cout << "  Detected: " << rows << " x " << cols << " " << dtype << " array\n";

    // Create STAR store
    auto store = StarDataset::create(output_file);  // Compression/block size handled by metadata block config

    // Convert based on detected type
    if (dtype == "int64") {
        NDArray<int64_t> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = std::stoll(data[i][j]);
            }
        }
        store->meta.put("data", arr);
    } else if (dtype == "float64") {
        NDArray<double> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = std::stod(data[i][j]);
            }
        }
        store->meta.put("data", arr);
    } else {
        NDArray<std::string> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = data[i][j];
            }
        }
        store->meta.put("data", arr);
    }

    std::cout << "Conversion complete! Created 'data' array.\n";
}

void star_to_csv(const std::string& input_file, const std::string& output_file) {
    std::cout << "Converting STAR to CSV...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";

    // Open STAR file
    auto store = StarDataset::open(input_file, FileMode::READ_ONLY);

    if (store->size() == 0) {
        throw std::runtime_error("STAR file is empty");
    }

    // For CSV, we'll export the first array only (or a specified one)
    // For simplicity, export the first 2D array we find
    std::ofstream ofs(output_file);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + output_file);
    }

    bool found = false;
    for (const auto& key : *store) {
        // Look up entry in SoA
        auto it = store->m_key_to_index.find(key);
        if (it == store->m_key_to_index.end()) continue;
        size_t idx = it->second;

        // Only export 2D arrays
        const auto& shape = store->m_cold.shapes[idx];
        if (shape.size() != 2) {
            std::cout << "  Skipping " << key << " (not 2D)\n";
            continue;
        }

        std::cout << "  Exporting key: " << key << "\n";

        size_t rows = shape[0];
        size_t cols = shape[1];

        // Export based on data type using new meta API
        auto meta = store->meta.get(key);
        if (!meta) {
            std::cerr << "Warning: Could not read key: " << key << "\n";
            continue;
        }

        switch(meta->dtype) {
            case DataType::INT8: {
                auto arr = meta->as<int8_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << static_cast<int>(arr(i, j));
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::INT16: {
                auto arr = meta->as<int16_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::INT32: {
                auto arr = meta->as<int32_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::INT64: {
                auto arr = meta->as<int64_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT8: {
                auto arr = meta->as<uint8_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << static_cast<unsigned int>(arr(i, j));
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT16: {
                auto arr = meta->as<uint16_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT32: {
                auto arr = meta->as<uint32_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT64: {
                auto arr = meta->as<uint64_t>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::FLOAT32: {
                auto arr = meta->as<float>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::FLOAT64: {
                auto arr = meta->as<double>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << arr(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::STRING: {
                auto arr = meta->as<std::string>();
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        const std::string& val = arr(i, j);
                        // Quote if contains comma or quotes
                        if (val.find(',') != std::string::npos || val.find('"') != std::string::npos) {
                            ofs << "\"" << val << "\"";
                        } else {
                            ofs << val;
                        }
                    }
                    ofs << "\n";
                }
                break;
            }
            default:
                std::cerr << "Warning: Unsupported data type for key: " << key << "\n";
                continue;
        }

        found = true;
        break; // Only export first 2D array
    }

    ofs.close();

    if (!found) {
        throw std::runtime_error("No 2D arrays found in STAR file");
    }

    std::cout << "Conversion complete!\n";
}

// ISDS conversion: reorganize STAR file by putting large arrays in array storage
// and small data in metadata storage
void isds_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size, size_t threshold = 100) {
    std::cout << "Converting ISDS to optimized STAR format...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Array size threshold: " << threshold << " elements\n";
    std::cout << "  Compression: ";
    switch(compression) {
        case CompressionAlgorithm::NONE: std::cout << "None\n"; break;
        case CompressionAlgorithm::GZIP: std::cout << "GZIP\n"; break;
        case CompressionAlgorithm::LZ4: std::cout << "LZ4\n"; break;
        case CompressionAlgorithm::ZSTD: std::cout << "ZSTD\n"; break;
        default: std::cout << "Unknown\n";
    }
    std::cout << "  Block size: " << block_size << " bytes\n\n";

    // Open input STAR file (read-only)
    auto input_store = StarDataset::open(input_file, FileMode::READ_ONLY);

    // Create output STAR file
    auto output_store = StarDataset::create(output_file);

    // Get all keys from input (both arrays and metadata)
    std::vector<std::string> all_keys;

    // Get array keys
    for (const auto& key : *input_store) {
        all_keys.push_back(key);
    }

    // Get metadata keys
    std::vector<std::string> metadata_keys = input_store->get_metadata_keys();
    for (const auto& key : metadata_keys) {
        all_keys.push_back(key);
    }

    size_t large_array_count = 0;
    size_t small_data_count = 0;

    std::cout << "Processing " << all_keys.size() << " keys...\n\n";

    // Process each key
    for (const auto& key : all_keys) {
        // Try to get metadata for this key to determine size
        // First try metadata storage, then try array storage
        auto meta = input_store->meta.get(key);
        if (!meta) {
            // Try reading as array from block storage
            try {
                // Try getting from array storage - we need to determine type first
                // Use index lookup to find dtype
                auto it = input_store->m_key_to_index.find(key);
                if (it == input_store->m_key_to_index.end()) {
                    std::cerr << "  Warning: Could not find key: " << key << "\n";
                    continue;
                }
                size_t idx = it->second;
                DataType dtype = input_store->m_hot.dtypes[idx];

                // Create metadata wrapper based on dtype
                switch(dtype) {
                    case DataType::INT8:
                        meta = std::make_shared<MetadataValue>(input_store->get<int8_t>(key));
                        break;
                    case DataType::INT16:
                        meta = std::make_shared<MetadataValue>(input_store->get<int16_t>(key));
                        break;
                    case DataType::INT32:
                        meta = std::make_shared<MetadataValue>(input_store->get<int32_t>(key));
                        break;
                    case DataType::INT64:
                        meta = std::make_shared<MetadataValue>(input_store->get<int64_t>(key));
                        break;
                    case DataType::UINT8:
                        meta = std::make_shared<MetadataValue>(input_store->get<uint8_t>(key));
                        break;
                    case DataType::UINT16:
                        meta = std::make_shared<MetadataValue>(input_store->get<uint16_t>(key));
                        break;
                    case DataType::UINT32:
                        meta = std::make_shared<MetadataValue>(input_store->get<uint32_t>(key));
                        break;
                    case DataType::UINT64:
                        meta = std::make_shared<MetadataValue>(input_store->get<uint64_t>(key));
                        break;
                    case DataType::FLOAT32:
                        meta = std::make_shared<MetadataValue>(input_store->get<float>(key));
                        break;
                    case DataType::FLOAT64:
                        meta = std::make_shared<MetadataValue>(input_store->get<double>(key));
                        break;
                    case DataType::STRING:
                        meta = std::make_shared<MetadataValue>(input_store->get<std::string>(key));
                        break;
                    default:
                        std::cerr << "  Warning: Unsupported dtype for key: " << key << "\n";
                        continue;
                }
            } catch (const std::exception& e) {
                std::cerr << "  Warning: Could not read key: " << key << " - " << e.what() << "\n";
                continue;
            }
        }

        // Calculate total number of elements
        size_t num_elements = 1;
        for (size_t dim : meta->shape) {
            num_elements *= dim;
        }

        // Decide where to store based on size
        if (num_elements > threshold) {
            // Large array: store in array storage
            std::cout << "  " << key << " (" << num_elements << " elements) -> array storage\n";

            // Copy to output array storage based on dtype
            switch(meta->dtype) {
                case DataType::INT8:
                    output_store->put(key, meta->as<int8_t>());
                    break;
                case DataType::INT16:
                    output_store->put(key, meta->as<int16_t>());
                    break;
                case DataType::INT32:
                    output_store->put(key, meta->as<int32_t>());
                    break;
                case DataType::INT64:
                    output_store->put(key, meta->as<int64_t>());
                    break;
                case DataType::UINT8:
                    output_store->put(key, meta->as<uint8_t>());
                    break;
                case DataType::UINT16:
                    output_store->put(key, meta->as<uint16_t>());
                    break;
                case DataType::UINT32:
                    output_store->put(key, meta->as<uint32_t>());
                    break;
                case DataType::UINT64:
                    output_store->put(key, meta->as<uint64_t>());
                    break;
                case DataType::FLOAT32:
                    output_store->put(key, meta->as<float>());
                    break;
                case DataType::FLOAT64:
                    output_store->put(key, meta->as<double>());
                    break;
                case DataType::STRING:
                    output_store->put(key, meta->as<std::string>());
                    break;
                default:
                    std::cerr << "  Warning: Unsupported dtype for key: " << key << "\n";
                    continue;
            }
            large_array_count++;
        } else {
            // Small data: store in metadata storage
            std::cout << "  " << key << " (" << num_elements << " elements) -> metadata storage\n";

            // Copy to output metadata storage based on dtype
            switch(meta->dtype) {
                case DataType::INT8:
                    output_store->meta.put(key, meta->as<int8_t>());
                    break;
                case DataType::INT16:
                    output_store->meta.put(key, meta->as<int16_t>());
                    break;
                case DataType::INT32:
                    output_store->meta.put(key, meta->as<int32_t>());
                    break;
                case DataType::INT64:
                    output_store->meta.put(key, meta->as<int64_t>());
                    break;
                case DataType::UINT8:
                    output_store->meta.put(key, meta->as<uint8_t>());
                    break;
                case DataType::UINT16:
                    output_store->meta.put(key, meta->as<uint16_t>());
                    break;
                case DataType::UINT32:
                    output_store->meta.put(key, meta->as<uint32_t>());
                    break;
                case DataType::UINT64:
                    output_store->meta.put(key, meta->as<uint64_t>());
                    break;
                case DataType::FLOAT32:
                    output_store->meta.put(key, meta->as<float>());
                    break;
                case DataType::FLOAT64:
                    output_store->meta.put(key, meta->as<double>());
                    break;
                case DataType::STRING:
                    output_store->meta.put(key, meta->as<std::string>());
                    break;
                default:
                    std::cerr << "  Warning: Unsupported dtype for key: " << key << "\n";
                    continue;
            }
            small_data_count++;
        }
    }

    std::cout << "\nConversion complete!\n";
    std::cout << "  Large arrays (array storage): " << large_array_count << "\n";
    std::cout << "  Small data (metadata): " << small_data_count << "\n";
    std::cout << "  Total keys: " << (large_array_count + small_data_count) << "\n";
}

void star_to_isds(const std::string& input_file, const std::string& output_file) {
    // This is just an alias - ISDS files are STAR files
    // The difference is organizational, not format
    std::cout << "Note: ISDS format is STAR format.\n";
    std::cout << "Use regular STAR tools to work with the file.\n";
    std::cout << "Input file: " << input_file << " is already in STAR format.\n";
}

int main(int argc, char* argv[]) {
    // Disable trace logging by default
    logger::set_log_level(logger::ERROR);

    std::string input_file;
    std::string output_file;
    std::string format;
    std::string compression_str = "lz4";
    size_t block_size = 1048576; // 1MB default
    size_t threshold = 100; // Default threshold for ISDS conversion

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < argc) {
                format = argv[++i];
            } else {
                std::cerr << "Error: -f/--format requires a format name\n";
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg == "-c" || arg == "--compression") {
            if (i + 1 < argc) {
                compression_str = argv[++i];
            } else {
                std::cerr << "Error: -c/--compression requires an algorithm name\n";
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg == "-b" || arg == "--block-size") {
            if (i + 1 < argc) {
                block_size = std::stoul(argv[++i]);
            } else {
                std::cerr << "Error: -b/--block-size requires a size\n";
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg == "-t" || arg == "--threshold") {
            if (i + 1 < argc) {
                threshold = std::stoul(argv[++i]);
            } else {
                std::cerr << "Error: -t/--threshold requires a size\n";
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg[0] != '-') {
            if (input_file.empty()) {
                input_file = arg;
            } else if (output_file.empty()) {
                output_file = arg;
            } else {
                std::cerr << "Error: Too many file arguments\n";
                print_usage(argv[0]);
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown option " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (input_file.empty() || output_file.empty()) {
        std::cerr << "Error: Input and output files required\n";
        print_usage(argv[0]);
        return 1;
    }

    // Parse compression algorithm
    CompressionAlgorithm compression = CompressionAlgorithm::NONE;
    std::transform(compression_str.begin(), compression_str.end(),
                   compression_str.begin(), ::tolower);

    if (compression_str == "none") {
        compression = CompressionAlgorithm::NONE;
    } else if (compression_str == "gzip") {
        compression = CompressionAlgorithm::GZIP;
    } else if (compression_str == "lz4") {
        compression = CompressionAlgorithm::LZ4;
    } else if (compression_str == "zstd") {
        compression = CompressionAlgorithm::ZSTD;
    } else {
        std::cerr << "Error: Unsupported compression algorithm: " << compression_str << std::endl;
        std::cerr << "Supported: none, gzip, lz4, zstd" << std::endl;
        return 1;
    }

    // Detect formats
    bool explicit_isds = (format == "isds");
    std::string input_format = detect_format(input_file, explicit_isds);
    std::string output_format;

    if (explicit_isds) {
        // When ISDS is specified, input is ISDS, output is optimized STAR
        output_format = "star";
    } else {
        output_format = format.empty() ? detect_format(output_file, false) : format;
    }

    if (input_format.empty()) {
        std::cerr << "Error: Cannot detect input file format\n";
        std::cerr << "Use -f/--format to specify explicitly\n";
        return 1;
    }

    if (output_format.empty()) {
        std::cerr << "Error: Cannot detect output file format\n";
        std::cerr << "Use -f/--format to specify explicitly\n";
        return 1;
    }

    std::cout << "File Format Conversion\n";
    std::cout << "======================\n";

    try {
        // Handle conversions
        if (input_format == "isds" && output_format == "star") {
            // ISDS to optimized STAR: reorganize by array size
            isds_to_star(input_file, output_file, compression, block_size, threshold);
        } else if (input_format == "star" && output_format == "isds") {
            // STAR to ISDS (just explain it's the same)
            star_to_isds(input_file, output_file);
        } else if (input_format == "star" && output_format == "json") {
            star_to_json(input_file, output_file);
        } else if (input_format == "json" && output_format == "star") {
            json_to_star(input_file, output_file, compression, block_size);
        } else if (input_format == "star" && output_format == "csv") {
            star_to_csv(input_file, output_file);
        } else if (input_format == "csv" && output_format == "star") {
            csv_to_star(input_file, output_file, compression, block_size);
        }
#ifdef ENABLE_MSGPACK
        else if (input_format == "star" && output_format == "msgpack") {
            star_to_msgpack(input_file, output_file);
        } else if (input_format == "msgpack" && output_format == "star") {
            msgpack_to_star(input_file, output_file, compression, block_size);
        }
#endif
        else {
            std::cerr << "Error: Unsupported conversion: " << input_format
                      << " -> " << output_format << "\n";
#ifndef ENABLE_MSGPACK
            if (input_format == "msgpack" || output_format == "msgpack") {
                std::cerr << "Note: MessagePack support not enabled in this build\n";
            }
#endif
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
