#include "../CameraStateFile/include/scs.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <map>
#include <sstream>
#include <algorithm>

using json = nlohmann::json;

// Forward declarations
void print_usage(const char* program_name);
void scs_to_json(const std::string& input_file, const std::string& output_file);
void json_to_scs(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size);
json ndarray_to_json(const ValueVariant& variant);
ValueVariant json_to_ndarray(const json& j);

#ifdef ENABLE_MSGPACK
#include <msgpack.hpp>
void scs_to_msgpack(const std::string& input_file, const std::string& output_file);
void msgpack_to_scs(const std::string& input_file, const std::string& output_file,
                    CompressionAlgorithm compression, size_t block_size);
#endif

// CSV support
void csv_to_scs(const std::string& input_file, const std::string& output_file,
                CompressionAlgorithm compression, size_t block_size);
void scs_to_csv(const std::string& input_file, const std::string& output_file);

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <input_file> <output_file>\n\n";
    std::cout << "Convert between SCS and other formats (JSON, MessagePack)\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -f, --format <fmt>      Output format (json, msgpack)\n";
    std::cout << "                          Auto-detected from output extension if not specified\n";
    std::cout << "  -c, --compression <alg> Compression algorithm for SCS output (none, gzip)\n";
    std::cout << "                          Default: none\n";
    std::cout << "  -b, --block-size <size> Block size for SCS compression (bytes)\n";
    std::cout << "                          Default: 1048576 (1MB)\n";
    std::cout << "\nSupported Formats:\n";
    std::cout << "  - SCS (Simple Columnar Store) - .scs extension\n";
    std::cout << "  - JSON - .json extension\n";
    std::cout << "  - CSV - .csv extension (2D arrays only)\n";
#ifdef ENABLE_MSGPACK
    std::cout << "  - MessagePack - .msgpack or .mp extension\n";
#endif
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " data.scs data.json           # SCS to JSON\n";
    std::cout << "  " << program_name << " data.json data.scs           # JSON to SCS\n";
    std::cout << "  " << program_name << " -c gzip data.json data.scs   # JSON to SCS with GZIP\n";
    std::cout << "  " << program_name << " data.csv data.scs            # CSV to SCS\n";
    std::cout << "  " << program_name << " data.scs data.csv            # SCS to CSV\n";
#ifdef ENABLE_MSGPACK
    std::cout << "  " << program_name << " data.scs data.msgpack        # SCS to MessagePack\n";
    std::cout << "  " << program_name << " data.msgpack data.scs        # MessagePack to SCS\n";
#endif
}

std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return filename.substr(dot_pos + 1);
}

std::string detect_format(const std::string& filename) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "scs") return "scs";
    if (ext == "json") return "json";
    if (ext == "csv") return "csv";
    if (ext == "msgpack" || ext == "mp") return "msgpack";

    return "";
}

json ndarray_to_json(const ValueVariant& variant) {
    json j;

    // Use std::visit to handle different ndarray types
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

ValueVariant json_to_ndarray(const json& j) {
    std::string dtype = j["dtype"].get<std::string>();
    std::vector<size_t> shape = j["shape"].get<std::vector<size_t>>();

    if (dtype == "int8") {
        ndarray<int8_t> arr(shape);
        auto data = j["data"].get<std::vector<int8_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "int16") {
        ndarray<int16_t> arr(shape);
        auto data = j["data"].get<std::vector<int16_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "int32") {
        ndarray<int32_t> arr(shape);
        auto data = j["data"].get<std::vector<int32_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "int64") {
        ndarray<int64_t> arr(shape);
        auto data = j["data"].get<std::vector<int64_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint8") {
        ndarray<uint8_t> arr(shape);
        auto data = j["data"].get<std::vector<uint8_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint16") {
        ndarray<uint16_t> arr(shape);
        auto data = j["data"].get<std::vector<uint16_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint32") {
        ndarray<uint32_t> arr(shape);
        auto data = j["data"].get<std::vector<uint32_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "uint64") {
        ndarray<uint64_t> arr(shape);
        auto data = j["data"].get<std::vector<uint64_t>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "float32") {
        ndarray<float> arr(shape);
        auto data = j["data"].get<std::vector<float>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "float64") {
        ndarray<double> arr(shape);
        auto data = j["data"].get<std::vector<double>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else if (dtype == "string") {
        ndarray<std::string> arr(shape);
        auto data = j["data"].get<std::vector<std::string>>();
        std::copy(data.begin(), data.end(), arr.begin());
        return arr;
    } else {
        throw std::runtime_error("Unsupported data type: " + dtype);
    }
}

void scs_to_json(const std::string& input_file, const std::string& output_file) {
    std::cout << "Converting SCS to JSON...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";

    // Open SCS file
    SCStore store(input_file);

    json root;
    root["format"] = "scs";
    root["version"] = "1.0";

    json arrays = json::object();

    // Iterate through all keys in the store
    for (const auto& [key, entry] : store) {
        std::cout << "  Converting key: " << key << "\n";

        // Read the array based on its data type
        ValueVariant variant;

        switch(entry.datatype) {
            case DataType::INT8:
                variant = *store.get<ndarray<int8_t>>(key);
                break;
            case DataType::INT16:
                variant = *store.get<ndarray<int16_t>>(key);
                break;
            case DataType::INT32:
                variant = *store.get<ndarray<int32_t>>(key);
                break;
            case DataType::INT64:
                variant = *store.get<ndarray<int64_t>>(key);
                break;
            case DataType::UINT8:
                variant = *store.get<ndarray<uint8_t>>(key);
                break;
            case DataType::UINT16:
                variant = *store.get<ndarray<uint16_t>>(key);
                break;
            case DataType::UINT32:
                variant = *store.get<ndarray<uint32_t>>(key);
                break;
            case DataType::UINT64:
                variant = *store.get<ndarray<uint64_t>>(key);
                break;
            case DataType::FLOAT32:
                variant = *store.get<ndarray<float>>(key);
                break;
            case DataType::FLOAT64:
                variant = *store.get<ndarray<double>>(key);
                break;
            case DataType::STRING:
                variant = *store.get<ndarray<std::string>>(key);
                break;
            default:
                std::cerr << "Warning: Unsupported data type for key: " << key << "\n";
                continue;
        }

        arrays[key] = ndarray_to_json(variant);
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
void flatten_json_object(const json& obj, const std::string& prefix, SCStore& store, size_t& converted_count);

// Helper: Convert simple JSON value/array to ndarray
ValueVariant json_value_to_ndarray(const json& value, const std::string& key) {
    // Handle scalar values - convert to 1-element array
    if (value.is_number_integer()) {
        ndarray<int64_t> arr({1});
        arr(0) = value.get<int64_t>();
        return arr;
    } else if (value.is_number_float()) {
        ndarray<double> arr({1});
        arr(0) = value.get<double>();
        return arr;
    } else if (value.is_string()) {
        ndarray<std::string> arr({1});
        arr(0) = value.get<std::string>();
        return arr;
    } else if (value.is_boolean()) {
        ndarray<int64_t> arr({1});
        arr(0) = value.get<bool>() ? 1 : 0;
        return arr;
    }
    // Handle arrays
    else if (value.is_array()) {
        if (value.empty()) {
            // Empty array - default to float64
            ndarray<double> arr({0});
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
                ndarray<double> arr({rows, 0});
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
                ndarray<int64_t> arr({rows, cols});
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < value[i].size() && j < cols; ++j) {
                        arr(i, j) = value[i][j].get<int64_t>();
                    }
                }
                return arr;
            } else if (dtype == "float64") {
                ndarray<double> arr({rows, cols});
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < value[i].size() && j < cols; ++j) {
                        arr(i, j) = value[i][j].get<double>();
                    }
                }
                return arr;
            } else {
                ndarray<std::string> arr({rows, cols});
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
                ndarray<int64_t> arr({size});
                for (size_t i = 0; i < size; ++i) {
                    arr(i) = value[i].get<int64_t>();
                }
                return arr;
            } else if (dtype == "float64") {
                ndarray<double> arr({size});
                for (size_t i = 0; i < size; ++i) {
                    arr(i) = value[i].get<double>();
                }
                return arr;
            } else {
                ndarray<std::string> arr({size});
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
void flatten_json_object(const json& obj, const std::string& prefix, SCStore& store, size_t& converted_count) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        std::string key = it.key();
        const json& value = it.value();

        // Build nested key with colon separator
        std::string full_key = prefix.empty() ? key : (prefix + ":" + key);

        // If this is another nested object, recurse
        if (value.is_object()) {
            std::cout << "  Flattening nested object: " << full_key << "\n";
            flatten_json_object(value, full_key, store, converted_count);
        } else {
            // Convert this value to ndarray
            try {
                std::cout << "  Converting key: " << full_key;

                ValueVariant variant = json_value_to_ndarray(value, full_key);

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
                    store.put(full_key, arr);
                }, variant);

                converted_count++;
            } catch (const std::exception& e) {
                std::cerr << "  Warning: Skipping key '" << full_key << "': " << e.what() << "\n";
            }
        }
    }
}

void json_to_scs(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Converting JSON to SCS...\n";
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

    // Create SCS store
    SCStore store(output_file, compression, block_size);

    // Check if this is structured format or simple format
    bool is_structured_format = root.contains("format") && root.contains("arrays");

    if (is_structured_format) {
        // Structured format: {"format": "scs", "version": "1.0", "arrays": {...}}
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

            ValueVariant variant = json_to_ndarray(array_data);

            // Put into store using std::visit
            std::visit([&store, &key](auto&& arr) {
                store.put(key, arr);
            }, variant);
        }

        std::cout << "Conversion complete! Converted " << arrays.size() << " arrays.\n";
    } else {
        // Simple format: direct key-value pairs
        std::cout << "  Detected: Simple JSON format\n";
        std::cout << "  Converting scalars to 1-element arrays\n";
        std::cout << "  Auto-detecting types (int64, float64, string)\n";
        std::cout << "  Flattening nested objects with colon notation (e.g., parent:child)\n";

        size_t converted_count = 0;

        for (auto it = root.begin(); it != root.end(); ++it) {
            std::string key = it.key();
            const json& value = it.value();

            // Handle nested objects by flattening
            if (value.is_object()) {
                std::cout << "  Flattening nested object: " << key << "\n";
                flatten_json_object(value, key, store, converted_count);
            } else {
                // Convert this value to ndarray
                try {
                    std::cout << "  Converting key: " << key;

                    ValueVariant variant = json_value_to_ndarray(value, key);

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
                        store.put(key, arr);
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
void scs_to_msgpack(const std::string& input_file, const std::string& output_file) {
    std::cout << "Converting SCS to MessagePack...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";

    // Open SCS file
    SCStore store(input_file);

    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(&buffer);

    // Pack as map
    packer.pack_map(2);

    // Format and version
    packer.pack(std::string("format"));
    packer.pack(std::string("scs"));

    packer.pack(std::string("arrays"));
    packer.pack_map(store.size());

    // Iterate through all keys in the store
    for (const auto& [key, entry] : store) {
        std::cout << "  Converting key: " << key << "\n";

        packer.pack(key);
        packer.pack_map(3);

        // Pack dtype
        packer.pack(std::string("dtype"));
        packer.pack(datatype_to_string(entry.datatype));

        // Pack shape
        packer.pack(std::string("shape"));
        packer.pack(entry.shape);

        // Pack data based on type
        packer.pack(std::string("data"));

        switch(entry.datatype) {
            case DataType::INT8: {
                auto arr = store.get<ndarray<int8_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::INT16: {
                auto arr = store.get<ndarray<int16_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::INT32: {
                auto arr = store.get<ndarray<int32_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::INT64: {
                auto arr = store.get<ndarray<int64_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::UINT8: {
                auto arr = store.get<ndarray<uint8_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::UINT16: {
                auto arr = store.get<ndarray<uint16_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::UINT32: {
                auto arr = store.get<ndarray<uint32_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::UINT64: {
                auto arr = store.get<ndarray<uint64_t>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::FLOAT32: {
                auto arr = store.get<ndarray<float>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::FLOAT64: {
                auto arr = store.get<ndarray<double>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
                break;
            }
            case DataType::STRING: {
                auto arr = store.get<ndarray<std::string>>(key);
                packer.pack_array(arr->size());
                for (const auto& val : *arr) packer.pack(val);
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

void msgpack_to_scs(const std::string& input_file, const std::string& output_file,
                    CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Converting MessagePack to SCS...\n";
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

    // Create SCS store
    SCStore store(output_file, compression, block_size);

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
            ndarray<int8_t> arr(shape);
            std::vector<int8_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "int16") {
            ndarray<int16_t> arr(shape);
            std::vector<int16_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "int32") {
            ndarray<int32_t> arr(shape);
            std::vector<int32_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "int64") {
            ndarray<int64_t> arr(shape);
            std::vector<int64_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "uint8") {
            ndarray<uint8_t> arr(shape);
            std::vector<uint8_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "uint16") {
            ndarray<uint16_t> arr(shape);
            std::vector<uint16_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "uint32") {
            ndarray<uint32_t> arr(shape);
            std::vector<uint32_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "uint64") {
            ndarray<uint64_t> arr(shape);
            std::vector<uint64_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "float32") {
            ndarray<float> arr(shape);
            std::vector<float> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "float64") {
            ndarray<double> arr(shape);
            std::vector<double> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
        } else if (dtype == "string") {
            ndarray<std::string> arr(shape);
            std::vector<std::string> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            store.put(array_key, arr);
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

void csv_to_scs(const std::string& input_file, const std::string& output_file,
                CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Converting CSV to SCS...\n";
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

    // Create SCS store
    SCStore store(output_file, compression, block_size);

    // Convert based on detected type
    if (dtype == "int64") {
        ndarray<int64_t> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = std::stoll(data[i][j]);
            }
        }
        store.put("data", arr);
    } else if (dtype == "float64") {
        ndarray<double> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = std::stod(data[i][j]);
            }
        }
        store.put("data", arr);
    } else {
        ndarray<std::string> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = data[i][j];
            }
        }
        store.put("data", arr);
    }

    std::cout << "Conversion complete! Created 'data' array.\n";
}

void scs_to_csv(const std::string& input_file, const std::string& output_file) {
    std::cout << "Converting SCS to CSV...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";

    // Open SCS file
    SCStore store(input_file);

    if (store.size() == 0) {
        throw std::runtime_error("SCS file is empty");
    }

    // For CSV, we'll export the first array only (or a specified one)
    // For simplicity, export the first 2D array we find
    std::ofstream ofs(output_file);
    if (!ofs) {
        throw std::runtime_error("Failed to open output file: " + output_file);
    }

    bool found = false;
    for (const auto& [key, entry] : store) {
        // Only export 2D arrays
        if (entry.shape.size() != 2) {
            std::cout << "  Skipping " << key << " (not 2D)\n";
            continue;
        }

        std::cout << "  Exporting key: " << key << "\n";

        size_t rows = entry.shape[0];
        size_t cols = entry.shape[1];

        // Export based on data type
        switch(entry.datatype) {
            case DataType::INT8: {
                auto arr = store.get<ndarray<int8_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << static_cast<int>((*arr)(i, j));
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::INT16: {
                auto arr = store.get<ndarray<int16_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::INT32: {
                auto arr = store.get<ndarray<int32_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::INT64: {
                auto arr = store.get<ndarray<int64_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT8: {
                auto arr = store.get<ndarray<uint8_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << static_cast<unsigned int>((*arr)(i, j));
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT16: {
                auto arr = store.get<ndarray<uint16_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT32: {
                auto arr = store.get<ndarray<uint32_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::UINT64: {
                auto arr = store.get<ndarray<uint64_t>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::FLOAT32: {
                auto arr = store.get<ndarray<float>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::FLOAT64: {
                auto arr = store.get<ndarray<double>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        ofs << (*arr)(i, j);
                    }
                    ofs << "\n";
                }
                break;
            }
            case DataType::STRING: {
                auto arr = store.get<ndarray<std::string>>(key);
                for (size_t i = 0; i < rows; ++i) {
                    for (size_t j = 0; j < cols; ++j) {
                        if (j > 0) ofs << ",";
                        const std::string& val = (*arr)(i, j);
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
        throw std::runtime_error("No 2D arrays found in SCS file");
    }

    std::cout << "Conversion complete!\n";
}

int main(int argc, char* argv[]) {
    // Disable trace logging by default
    logger::set_log_level(logger::ERROR);

    std::string input_file;
    std::string output_file;
    std::string format;
    std::string compression_str = "none";
    size_t block_size = 1048576; // 1MB default

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
    } else {
        std::cerr << "Error: Unknown compression algorithm: " << compression_str << "\n";
        std::cerr << "Supported: none, gzip\n";
        return 1;
    }

    // Detect formats
    std::string input_format = detect_format(input_file);
    std::string output_format = format.empty() ? detect_format(output_file) : format;

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
        if (input_format == "scs" && output_format == "json") {
            scs_to_json(input_file, output_file);
        } else if (input_format == "json" && output_format == "scs") {
            json_to_scs(input_file, output_file, compression, block_size);
        } else if (input_format == "scs" && output_format == "csv") {
            scs_to_csv(input_file, output_file);
        } else if (input_format == "csv" && output_format == "scs") {
            csv_to_scs(input_file, output_file, compression, block_size);
        }
#ifdef ENABLE_MSGPACK
        else if (input_format == "scs" && output_format == "msgpack") {
            scs_to_msgpack(input_file, output_file);
        } else if (input_format == "msgpack" && output_format == "scs") {
            msgpack_to_scs(input_file, output_file, compression, block_size);
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
