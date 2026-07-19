#include "../StarDS/include/stards.h"
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
                 CompressionAlgorithm compression, size_t block_size, size_t array_threshold = 100);
json NDArray_to_json(const ValueVariant& variant);
ValueVariant json_to_NDArray(const json& j);

#ifdef ENABLE_MSGPACK
#include <msgpack.hpp>
void star_to_msgpack(const std::string& input_file, const std::string& output_file);
void msgpack_to_star(const std::string& input_file, const std::string& output_file,
                    CompressionAlgorithm compression, size_t block_size,
                    size_t array_threshold = 100);
#endif

// CSV support
void csv_to_star(const std::string& input_file, const std::string& output_file,
                CompressionAlgorithm compression, size_t block_size,
                size_t array_threshold = 100);
void star_to_csv(const std::string& input_file, const std::string& output_file);

// ISDS support (ISIS camera state files)
void isds_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size);

// STAR-to-STAR re-encode (change compression / block size, preserve structure)
void star_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size);

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <input_file> <output_file>\n\n";
    std::cout << "Convert between STAR and other formats (JSON, MessagePack, ISDS),\n";
    std::cout << "or re-encode a STAR file to change its compression / block size.\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help              Show this help message\n";
    std::cout << "  -f, --format <fmt>      Output format (json, msgpack, isds)\n";
    std::cout << "                          Auto-detected from output extension if not specified\n";
    std::cout << "  -c, --compression <alg> Compression algorithm for STAR output\n";
    std::cout << "                          Options: none, gzip, lz4, gzip-shuffle, lz4-shuffle\n";
    std::cout << "                          (*-shuffle adds a byte-shuffle prefilter that greatly\n";
    std::cout << "                           improves compression of numeric arrays, e.g. float64;\n";
    std::cout << "                           shuffled arrays are read whole, not sliceable)\n";
    std::cout << "                          Default: lz4\n";
    std::cout << "  -b, --block-size <size> Block size for STAR compression (bytes)\n";
    std::cout << "                          Default: 1048576 (1MB)\n";
    std::cout << "  -t, --threshold <size>  Array size threshold (elements) for JSON, CSV,\n";
    std::cout << "                          MessagePack, and ISDS conversion to STAR. Arrays with\n";
    std::cout << "                          MORE than this many elements go to (sliceable) array\n";
    std::cout << "                          storage; smaller values and scalars go to the metadata\n";
    std::cout << "                          block. Default: 100\n";
    std::cout << "\nSupported Formats:\n";
    std::cout << "  - STARDS (Simple Tensors Arrays and Rasters) - .stards extension\n";
    std::cout << "  - JSON - .json extension\n";
    std::cout << "  - CSV - .csv extension (2D arrays only)\n";
    std::cout << "  - ISDS (ISIS Dataset) - .stards extension with ISDS flag\n";
#ifdef ENABLE_MSGPACK
    std::cout << "  - MessagePack - .msgpack or .mp extension\n";
#endif
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " data.stards data.json           # STARDS to JSON\n";
    std::cout << "  " << program_name << " data.json data.stards           # JSON to STARDS\n";
    std::cout << "  " << program_name << " -c gzip data.json data.stards   # JSON to STARDS with GZIP\n";
    std::cout << "  " << program_name << " -c gzip in.stards out.stards    # Re-encode STARDS (change codec)\n";
    std::cout << "  " << program_name << " -c lz4-shuffle -b 262144 in.stards out.stards # Re-encode (codec + block size)\n";
    std::cout << "  " << program_name << " data.csv data.stards            # CSV to STARDS\n";
    std::cout << "  " << program_name << " data.stards data.csv            # STARDS to CSV\n";
    std::cout << "  " << program_name << " -f isds input.stards out.stards # ISDS to STARDS conversion\n";
    std::cout << "  " << program_name << " -f isds -t 50 in.stards out.stards # ISDS with custom threshold\n";
#ifdef ENABLE_MSGPACK
    std::cout << "  " << program_name << " data.stards data.msgpack        # STARDS to MessagePack\n";
    std::cout << "  " << program_name << " data.msgpack data.stards        # MessagePack to STARDS\n";
#endif
}

std::string get_file_extension(const std::string& filename) {
    size_t dot_pos = filename.find_last_of('.');
    if (dot_pos == std::string::npos) {
        return "";
    }
    return filename.substr(dot_pos + 1);
}

// Human-readable name for a compression algorithm (matches the -c option names).
std::string compression_name(CompressionAlgorithm c) {
    switch (c) {
        case CompressionAlgorithm::NONE:         return "none";
        case CompressionAlgorithm::GZIP:         return "gzip";
        case CompressionAlgorithm::LZ4:          return "lz4";
        case CompressionAlgorithm::GZIP_SHUFFLE: return "gzip-shuffle";
        case CompressionAlgorithm::LZ4_SHUFFLE:  return "lz4-shuffle";
        default:                                 return "unknown";
    }
}

std::string detect_format(const std::string& filename, bool explicit_isds = false) {
    std::string ext = get_file_extension(filename);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    // .stards is the canonical extension; .star is still accepted for back-compat.
    if (ext == "stards" || ext == "star") {
        if (explicit_isds) return "isds";
        return "star";
    }
    if (ext == "json") return "json";
    if (ext == "csv") return "csv";
    if (ext == "msgpack" || ext == "mp") return "msgpack";

    return "";
}

// Read a key from whichever namespace holds it (metadata block or array
// storage), returning a fully-populated MetadataValue (data + dtype + shape).
// Returns nullptr if the key is absent from both namespaces.
std::shared_ptr<MetadataValue> read_value_any_namespace(
        const std::shared_ptr<StarDataset>& store, const std::string& key) {
    auto meta = store->meta.get(key);
    if (meta) {
        return meta;
    }
    if (!store->contains(key)) {
        return nullptr;
    }
    DataType dtype = store->dtype_of(key);
    auto build = [&](auto sample) -> std::shared_ptr<MetadataValue> {
        using T = decltype(sample);
        NDArray<T> arr = store->get<T>(key);
        auto mv = std::make_shared<MetadataValue>();
        mv->dtype = dtype;
        mv->shape = arr.shape();
        mv->data = std::move(arr);
        return mv;
    };
    switch (dtype) {
        case DataType::INT8:    return build(int8_t{});
        case DataType::INT16:   return build(int16_t{});
        case DataType::INT32:   return build(int32_t{});
        case DataType::INT64:   return build(int64_t{});
        case DataType::UINT8:   return build(uint8_t{});
        case DataType::UINT16:  return build(uint16_t{});
        case DataType::UINT32:  return build(uint32_t{});
        case DataType::UINT64:  return build(uint64_t{});
        case DataType::FLOAT32: return build(float{});
        case DataType::FLOAT64: return build(double{});
        case DataType::STRING:  return build(std::string{});
        default:                return nullptr;
    }
}

// All keys across both namespaces (array storage + metadata block), deduplicated.
// get_all_keys() only returns array-namespace keys, so metadata-only keys must be
// added explicitly.
std::vector<std::string> all_keys_both_namespaces(const std::shared_ptr<StarDataset>& store) {
    std::vector<std::string> keys = store->get_all_keys();
    std::set<std::string> seen(keys.begin(), keys.end());
    for (const auto& k : store->get_metadata_keys()) {
        if (seen.insert(k).second) {
            keys.push_back(k);
        }
    }
    return keys;
}

// Read a key from the ARRAY namespace only (separate block storage), returning a
// fully-populated MetadataValue. Unlike read_value_any_namespace() this never
// consults the metadata block, so it stays correct even when the same logical key
// exists in both namespaces. Returns nullptr if the key isn't an array. Works for
// layer-prefixed internal keys ("__layer_<name>__:<key>") too, since those live in
// the base array namespace.
std::shared_ptr<MetadataValue> read_array_only(
        const std::shared_ptr<StarDataset>& store, const std::string& key) {
    if (!store->contains(key)) {
        return nullptr;
    }
    DataType dtype;
    try {
        dtype = store->dtype_of(key);  // throws if not in array storage
    } catch (const std::exception&) {
        return nullptr;  // key is metadata-only, not an array
    }
    auto build = [&](auto sample) -> std::shared_ptr<MetadataValue> {
        using T = decltype(sample);
        NDArray<T> arr = store->get<T>(key);
        auto mv = std::make_shared<MetadataValue>();
        mv->dtype = dtype;
        mv->shape = arr.shape();
        mv->data = std::move(arr);
        return mv;
    };
    switch (dtype) {
        case DataType::INT8:    return build(int8_t{});
        case DataType::INT16:   return build(int16_t{});
        case DataType::INT32:   return build(int32_t{});
        case DataType::INT64:   return build(int64_t{});
        case DataType::UINT8:   return build(uint8_t{});
        case DataType::UINT16:  return build(uint16_t{});
        case DataType::UINT32:  return build(uint32_t{});
        case DataType::UINT64:  return build(uint64_t{});
        case DataType::FLOAT32: return build(float{});
        case DataType::FLOAT64: return build(double{});
        case DataType::STRING:  return build(std::string{});
        default:                return nullptr;
    }
}

// Dispatch a MetadataValue to a generic sink by its dtype, handing the sink a
// typed NDArray<T> (moved). Used to copy values without per-call-site type
// branching. Returns false if the dtype is unsupported.
template <typename Sink>
bool dispatch_value(const std::shared_ptr<MetadataValue>& mv, Sink&& sink) {
    switch (mv->dtype) {
        case DataType::INT8:    { auto a = mv->as<int8_t>();      sink(std::move(a)); return true; }
        case DataType::INT16:   { auto a = mv->as<int16_t>();     sink(std::move(a)); return true; }
        case DataType::INT32:   { auto a = mv->as<int32_t>();     sink(std::move(a)); return true; }
        case DataType::INT64:   { auto a = mv->as<int64_t>();     sink(std::move(a)); return true; }
        case DataType::UINT8:   { auto a = mv->as<uint8_t>();     sink(std::move(a)); return true; }
        case DataType::UINT16:  { auto a = mv->as<uint16_t>();    sink(std::move(a)); return true; }
        case DataType::UINT32:  { auto a = mv->as<uint32_t>();    sink(std::move(a)); return true; }
        case DataType::UINT64:  { auto a = mv->as<uint64_t>();    sink(std::move(a)); return true; }
        case DataType::FLOAT32: { auto a = mv->as<float>();       sink(std::move(a)); return true; }
        case DataType::FLOAT64: { auto a = mv->as<double>();      sink(std::move(a)); return true; }
        case DataType::STRING:  { auto a = mv->as<std::string>(); sink(std::move(a)); return true; }
        default:                return false;
    }
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
    std::vector<std::string> all_keys = all_keys_both_namespaces(store);

    // Iterate through all keys
    for (const auto& key : all_keys) {
        std::cout << "  Converting key: " << key << "\n";

        auto meta = read_value_any_namespace(store, key);
        if (!meta) {
            std::cerr << "Warning: Could not read key: " << key << "\n";
            continue;
        }

        arrays[key] = NDArray_to_json(meta->data);
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

// Route a converted value into the store by size: arrays with MORE than
// `array_threshold` elements go to separate (sliceable, block-compressed) array
// storage; everything else (scalars, short arrays, strings) goes to the metadata
// block. Prints where each value landed.
void store_value_by_size(StarDataset& store, const std::string& key,
                         ValueVariant& variant, size_t array_threshold) {
    std::visit([&store, &key, array_threshold](auto&& arr) {
        using T = std::decay_t<decltype(arr)>;
        using ValueType = typename T::value_type;
        std::cout << " (" << datatype_to_string(TypeToDataType<ValueType>::value);
        if (arr.size() == 1) {
            std::cout << ", scalar → 1-element array";
        } else {
            std::cout << ", " << arr.size() << " elements";
        }

        if (arr.size() > array_threshold) {
            std::cout << ") → array storage\n";
            store.put(key, std::move(arr));
        } else {
            std::cout << ") → metadata\n";
            store.meta.put(key, arr);
        }
    }, variant);
}

// Helper: Recursively flatten nested JSON objects
void flatten_json_object(const json& obj, const std::string& prefix, StarDataset& store,
                        size_t& converted_count, size_t array_threshold) {
    for (auto it = obj.begin(); it != obj.end(); ++it) {
        std::string key = it.key();
        const json& value = it.value();

        // Build nested key with forward slash separator
        std::string full_key = prefix.empty() ? key : (prefix + "/" + key);

        // If this is another nested object, recurse
        if (value.is_object()) {
            std::cout << "  Flattening nested object: " << full_key << "\n";
            flatten_json_object(value, full_key, store, converted_count, array_threshold);
        } else {
            // Convert this value to NDArray, then route it by size.
            try {
                std::cout << "  Converting key: " << full_key;
                ValueVariant variant = json_value_to_NDArray(value, full_key);
                store_value_by_size(store, full_key, variant, array_threshold);
                converted_count++;
            } catch (const std::exception& e) {
                std::cerr << "  Warning: Skipping key '" << full_key << "': " << e.what() << "\n";
            }
        }
    }
}

void json_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size, size_t array_threshold) {
    std::cout << "Converting JSON to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: " << compression_name(compression) << "\n";
    std::cout << "  Block size: " << block_size << " bytes\n";

    // Read the input file into a string so any non-JSON preamble can be
    // stripped before parsing.
    std::ifstream ifs(input_file);
    if (!ifs) {
        throw std::runtime_error("Failed to open input file: " + input_file);
    }
    std::stringstream file_buf;
    file_buf << ifs.rdbuf();
    ifs.close();
    std::string file_contents = file_buf.str();

    // USGSCSM model-state files (the output of getModelState() /
    // "usgscsm_cam_test --output-model-state") and GXP .sup files prepend a text
    // preamble -- e.g. "USGS_ASTRO_LINE_SCANNER_SENSOR_MODEL\n" -- before the
    // JSON state body. Strip anything before the first '{' (and after the
    // matching last '}') so these convert directly, mirroring how USGSCSM's own
    // stateAsJson() extracts the state. A plain JSON file (starts with '{') is
    // unaffected. The model name is preserved: it also lives in the body under
    // the "m_modelName" key, which is what USGSCSM reads back.
    size_t first_brace = file_contents.find_first_of('{');
    size_t last_brace = file_contents.find_last_of('}');
    if (first_brace != std::string::npos && last_brace != std::string::npos &&
        last_brace >= first_brace) {
        if (first_brace > 0) {
            std::cout << "  Detected model-state preamble; stripping "
                      << first_brace << " leading byte(s) before JSON body\n";
        }
        file_contents = file_contents.substr(first_brace,
                                             last_brace - first_brace + 1);
    }

    json root = json::parse(file_contents);

    // Create STAR store with the requested compression + block size. Apply it to
    // both the main (separately-stored array) data and the metadata block so the
    // -c/-b flags actually take effect on the output.
    StarConfig config;
    config.compression = compression;
    // The metadata block holds mixed-type/variable-width values and is read as a
    // unit (never unshuffled), so use the base codec there, not a shuffle variant.
    config.metadata_compression = base_compression(compression);
    config.block_size = block_size;
    auto store = StarDataset::create(output_file, config);

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

            std::cout << "  Converting key: " << key;

            ValueVariant variant = json_to_NDArray(array_data);

            // Route by size: large arrays → array storage, else → metadata.
            store_value_by_size(*store, key, variant, array_threshold);
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
                flatten_json_object(value, key, *store, converted_count, array_threshold);
            } else {
                // Convert this value to NDArray, then route it by size.
                try {
                    std::cout << "  Converting key: " << key;
                    ValueVariant variant = json_value_to_NDArray(value, key);
                    store_value_by_size(*store, key, variant, array_threshold);
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

    // Resolve every readable key up front so the packed map count matches what
    // we actually emit (packing a fixed count then skipping keys would produce a
    // malformed msgpack map).
    std::vector<std::pair<std::string, std::shared_ptr<MetadataValue>>> entries;
    for (const auto& key : all_keys_both_namespaces(store)) {
        auto meta = read_value_any_namespace(store, key);
        if (!meta) {
            std::cerr << "Warning: Could not read key: " << key << "\n";
            continue;
        }
        entries.emplace_back(key, meta);
    }

    msgpack::sbuffer buffer;
    msgpack::packer<msgpack::sbuffer> packer(&buffer);

    // Pack as map
    packer.pack_map(2);

    // Format and version
    packer.pack(std::string("format"));
    packer.pack(std::string("star"));

    packer.pack(std::string("arrays"));
    packer.pack_map(entries.size());

    // Iterate through all resolved entries
    for (const auto& [key, meta] : entries) {
        std::cout << "  Converting key: " << key << "\n";

        packer.pack(key);
        packer.pack_map(3);

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
                    CompressionAlgorithm compression, size_t block_size,
                    size_t array_threshold) {
    std::cout << "Converting MessagePack to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: " << compression_name(compression) << "\n";
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
    StarConfig config;
    config.compression = compression;
    // The metadata block holds mixed-type/variable-width values and is read as a
    // unit (never unshuffled), so use the base codec there, not a shuffle variant.
    config.metadata_compression = base_compression(compression);
    config.block_size = block_size;
    auto store = StarDataset::create(output_file, config);

    msgpack::object_map arrays_map = arrays_kv->val.via.map;

    // Process each array
    for (uint32_t i = 0; i < arrays_map.size; ++i) {
        std::string array_key;
        arrays_map.ptr[i].key.convert(array_key);

        std::cout << "  Converting key: " << array_key;

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
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "int16") {
            NDArray<int16_t> arr(shape);
            std::vector<int16_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "int32") {
            NDArray<int32_t> arr(shape);
            std::vector<int32_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "int64") {
            NDArray<int64_t> arr(shape);
            std::vector<int64_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "uint8") {
            NDArray<uint8_t> arr(shape);
            std::vector<uint8_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "uint16") {
            NDArray<uint16_t> arr(shape);
            std::vector<uint16_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "uint32") {
            NDArray<uint32_t> arr(shape);
            std::vector<uint32_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "uint64") {
            NDArray<uint64_t> arr(shape);
            std::vector<uint64_t> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "float32") {
            NDArray<float> arr(shape);
            std::vector<float> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "float64") {
            NDArray<double> arr(shape);
            std::vector<double> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
        } else if (dtype == "string") {
            NDArray<std::string> arr(shape);
            std::vector<std::string> data;
            data_obj->convert(data);
            std::copy(data.begin(), data.end(), arr.begin());
            { ValueVariant v = std::move(arr); store_value_by_size(*store, array_key, v, array_threshold); }
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
                std::stod(cell, &pos);
                if (pos != cell.length()) {
                    is_integer = false;
                    is_float = false;
                    break;
                }
                // Only integer form (no '.', 'e'/'E') qualifies as int64. A
                // floor-equal float like "1e3" is NOT integer form: stoll would
                // truncate it to 1, corrupting the value.
                if (cell.find_first_of(".eE") != std::string::npos) {
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
                CompressionAlgorithm compression, size_t block_size,
                size_t array_threshold) {
    std::cout << "Converting CSV to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: " << compression_name(compression) << "\n";
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
    StarConfig config;
    config.compression = compression;
    // The metadata block holds mixed-type/variable-width values and is read as a
    // unit (never unshuffled), so use the base codec there, not a shuffle variant.
    config.metadata_compression = base_compression(compression);
    config.block_size = block_size;
    auto store = StarDataset::create(output_file, config);

    // Convert based on detected type, then route by size (-t threshold): large
    // arrays go to sliceable array storage, smaller ones to the metadata block.
    std::cout << "  Converting key: data";
    if (dtype == "int64") {
        NDArray<int64_t> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = std::stoll(data[i][j]);
            }
        }
        ValueVariant variant = std::move(arr);
        store_value_by_size(*store, "data", variant, array_threshold);
    } else if (dtype == "float64") {
        NDArray<double> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = std::stod(data[i][j]);
            }
        }
        ValueVariant variant = std::move(arr);
        store_value_by_size(*store, "data", variant, array_threshold);
    } else {
        NDArray<std::string> arr({rows, cols});
        for (size_t i = 0; i < rows; ++i) {
            for (size_t j = 0; j < cols && j < data[i].size(); ++j) {
                arr(i, j) = data[i][j];
            }
        }
        ValueVariant variant = std::move(arr);
        store_value_by_size(*store, "data", variant, array_threshold);
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

        auto meta = read_value_any_namespace(store, key);
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

// STAR-to-STAR re-encode: copy every value into a fresh file that uses the
// requested compression / block size, faithfully preserving the ORIGINAL
// structure (which namespace each key lives in, and which layer owns it). This
// is a lossless re-pack — unlike the ISDS path, it does NOT move values between
// the array and metadata namespaces. Use it to change codec or block size on an
// existing dataset.
void star_to_star(const std::string& input_file, const std::string& output_file,
                 CompressionAlgorithm compression, size_t block_size) {
    std::cout << "Re-encoding STAR to STAR...\n";
    std::cout << "  Input:  " << input_file << "\n";
    std::cout << "  Output: " << output_file << "\n";
    std::cout << "  Compression: " << compression_name(compression) << "\n";
    std::cout << "  Block size: " << block_size << " bytes\n";

    if (input_file == output_file) {
        throw std::runtime_error(
            "Refusing to re-encode a file onto itself; choose a different output path.");
    }

    auto in = StarDataset::open(input_file, FileMode::READ_ONLY);
    // Layer keys inherit from base by default-off; enable inheritance so we can
    // read base-only values through a layer if ever needed. Copies below only
    // touch layer-OWNED keys, so this is belt-and-suspenders.
    in->set_layer_inheritance(true);

    StarConfig config;
    config.compression = compression;
    // The metadata block holds mixed-type/variable-width values and is read as a
    // unit (never unshuffled), so use the base codec there, not a shuffle variant.
    config.metadata_compression = base_compression(compression);
    config.block_size = block_size;
    auto out = StarDataset::create(output_file, config);

    size_t array_count = 0, meta_count = 0, layer_array_count = 0, layer_meta_count = 0;

    // --- Base level -----------------------------------------------------------
    // Array namespace: get_all_keys() returns array-storage keys, but that
    // includes layer-prefixed internal keys ("__layer_<name>__:<key>"), which we
    // handle separately per layer below. Skip them here.
    for (const auto& key : in->get_all_keys()) {
        if (key.rfind("__layer_", 0) == 0) continue;  // layer-owned, handled later
        auto mv = read_array_only(in, key);
        if (!mv) continue;  // not actually an array (shouldn't happen for these keys)
        bool ok = dispatch_value(mv, [&](auto&& arr) {
            out->put(key, std::move(arr));
        });
        if (ok) { std::cout << "  [array]  " << key << "\n"; array_count++; }
        else std::cerr << "  Warning: unsupported dtype for array key: " << key << "\n";
    }

    // Metadata namespace (base layer): keys tracked in the base metadata registry.
    for (const auto& key : in->get_metadata_keys()) {
        auto mv = in->meta.get(key);
        if (!mv) continue;
        bool ok = dispatch_value(mv, [&](auto&& arr) {
            out->meta.put(key, arr);
        });
        if (ok) { std::cout << "  [meta]   " << key << "\n"; meta_count++; }
        else std::cerr << "  Warning: unsupported dtype for metadata key: " << key << "\n";
    }

    // --- Layers ---------------------------------------------------------------
    // Recreate each layer and copy only the keys it OWNS (not inherited base
    // keys), preserving the array vs. metadata split within the layer.
    for (const auto& layer_name : in->list_layers()) {
        std::cout << "  Layer: " << layer_name << "\n";
        auto in_layer = in->get_layer(layer_name);
        auto out_layer = out->create_layer(layer_name);

        // Layer-owned array keys: stored under "__layer_<name>__:<logical_key>".
        const std::string prefix = "__layer_" + layer_name + "__:";
        for (const auto& storage_key : in->get_all_keys()) {
            if (storage_key.rfind(prefix, 0) != 0) continue;
            std::string logical_key = storage_key.substr(prefix.size());
            auto mv = read_array_only(in, storage_key);
            if (!mv) continue;
            bool ok = dispatch_value(mv, [&](auto&& arr) {
                out_layer->put(logical_key, std::move(arr));
            });
            if (ok) { std::cout << "    [array]  " << logical_key << "\n"; layer_array_count++; }
            else std::cerr << "    Warning: unsupported dtype for layer array key: " << logical_key << "\n";
        }

        // Layer-owned metadata keys: everything the layer's own metadata registry
        // holds. With inheritance on, keys() also lists inherited base keys, so
        // filter to keys the layer actually owns via key_in_layer().
        for (const auto& key : in_layer->meta.keys()) {
            if (!in->key_in_layer(key, layer_name)) continue;  // inherited, skip
            auto mv = in_layer->meta.get(key);
            if (!mv) continue;
            bool ok = dispatch_value(mv, [&](auto&& arr) {
                out_layer->meta.put(key, arr);
            });
            if (ok) { std::cout << "    [meta]   " << key << "\n"; layer_meta_count++; }
            else std::cerr << "    Warning: unsupported dtype for layer metadata key: " << key << "\n";
        }
    }

    out->flush();

    std::cout << "\nRe-encode complete!\n";
    std::cout << "  Base arrays:    " << array_count << "\n";
    std::cout << "  Base metadata:  " << meta_count << "\n";
    if (!in->list_layers().empty()) {
        std::cout << "  Layers:         " << in->list_layers().size() << "\n";
        std::cout << "  Layer arrays:   " << layer_array_count << "\n";
        std::cout << "  Layer metadata: " << layer_meta_count << "\n";
    }
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
        // Read from whichever namespace holds the key (populates dtype/shape).
        auto meta = read_value_any_namespace(input_store, key);
        if (!meta) {
            std::cerr << "  Warning: Could not read key: " << key << "\n";
            continue;
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
    } else if (compression_str == "gzip-shuffle") {
        compression = CompressionAlgorithm::GZIP_SHUFFLE;
    } else if (compression_str == "lz4-shuffle") {
        compression = CompressionAlgorithm::LZ4_SHUFFLE;
    } else {
        std::cerr << "Error: Unsupported compression algorithm: " << compression_str << std::endl;
        std::cerr << "Supported: none, gzip, lz4, gzip-shuffle, lz4-shuffle" << std::endl;
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
        } else if (input_format == "star" && output_format == "star") {
            // STAR to STAR: lossless re-encode with new compression / block size
            star_to_star(input_file, output_file, compression, block_size);
        } else if (input_format == "star" && output_format == "json") {
            star_to_json(input_file, output_file);
        } else if (input_format == "json" && output_format == "star") {
            json_to_star(input_file, output_file, compression, block_size, threshold);
        } else if (input_format == "star" && output_format == "csv") {
            star_to_csv(input_file, output_file);
        } else if (input_format == "csv" && output_format == "star") {
            csv_to_star(input_file, output_file, compression, block_size, threshold);
        }
#ifdef ENABLE_MSGPACK
        else if (input_format == "star" && output_format == "msgpack") {
            star_to_msgpack(input_file, output_file);
        } else if (input_format == "msgpack" && output_format == "star") {
            msgpack_to_star(input_file, output_file, compression, block_size, threshold);
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
