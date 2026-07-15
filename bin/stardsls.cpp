#include "../StarDS/include/stards.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

using namespace star;

// Human-readable name for a compression algorithm.
const char* compression_name(CompressionAlgorithm c) {
    switch (c) {
        case CompressionAlgorithm::NONE: return "None";
        case CompressionAlgorithm::GZIP: return "GZIP";
        case CompressionAlgorithm::ZSTD: return "ZSTD";
        case CompressionAlgorithm::LZ4:  return "LZ4";
        case CompressionAlgorithm::GZIP_SHUFFLE: return "GZIP+shuffle";
        case CompressionAlgorithm::LZ4_SHUFFLE:  return "LZ4+shuffle";
        default: return "Unknown";
    }
}

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <file.stards>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -k, --keys           List keys only (default)\n";
    std::cout << "  -d, --data <key>     Print data for specific key\n";
    std::cout << "  -m, --metadata <key> Show metadata without loading data\n";
    std::cout << "  -a, --all            Print all data (WARNING: may be large)\n";
    std::cout << "  -v, --verbose        Verbose output with detailed metadata\n";
    std::cout << "  -f, --force          Force loading large arrays (bypass safety check)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " data.stards              # List all keys\n";
    std::cout << "  " << program_name << " -v data.stards           # Verbose listing\n";
    std::cout << "  " << program_name << " -m data data.stards      # Show metadata only\n";
    std::cout << "  " << program_name << " -d mykey data.stards     # Print specific key\n";
    std::cout << "  " << program_name << " -d mykey -f data.stards  # Force load large array\n";
    std::cout << "  " << program_name << " -a data.stards           # Print all data\n";
}

template<typename T>
void print_NDArray_data(const NDArray<T>& arr, size_t max_elements = 100) {
    std::cout << "    Data (showing up to " << max_elements << " elements):\n    [";

    size_t count = 0;
    for (const auto& val : arr) {
        if (count >= max_elements) break;
        if (count > 0) std::cout << ", ";
        if (count > 0 && count % 10 == 0) std::cout << "\n     ";

        if constexpr (std::is_floating_point<T>::value) {
            std::cout << std::fixed << std::setprecision(3) << val;
        } else {
            std::cout << val;
        }
        count++;
    }

    if (arr.size() > max_elements) {
        std::cout << ", ... (" << (arr.size() - max_elements) << " more)";
    }

    std::cout << "]\n";
}

void print_key_metadata(StarDataset* store, const std::string& key, bool verbose) {
    std::cout << "\n=== Key: " << key << " (metadata only) ===\n";

    // Find key in SoA
    auto it = store->m_key_to_index.find(key);
    if (it == store->m_key_to_index.end()) {
        std::cout << "  Key not found!\n";
        return;
    }

    size_t idx = it->second;

    // Print type and shape
    std::cout << "  Type: " << datatype_to_string(store->m_hot.dtypes[idx]);
    const auto& shape = store->m_cold.shapes[idx];
    if (!shape.empty()) {
        std::cout << "[";
        for (size_t i = 0; i < shape.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << shape[i];
        }
        std::cout << "]";
    } else {
        std::cout << " (scalar)";
    }
    std::cout << "\n";

    // Calculate num_elements
    size_t num_elements = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
    std::cout << "  Elements: " << num_elements << "\n";

    // Show sizes
    size_t compressed_size = store->m_cold.compressed_sizes[idx];
    size_t uncompressed_size = store->m_cold.uncompressed_sizes[idx];

    std::cout << "  Compressed size: "
              << std::fixed << std::setprecision(2)
              << (compressed_size / (1024.0 * 1024.0)) << " MB\n";
    std::cout << "  Uncompressed size: "
              << std::fixed << std::setprecision(2)
              << (uncompressed_size / (1024.0 * 1024.0 * 1024.0)) << " GB\n";

    // Compression info
    std::cout << "  Compression: " << compression_name(store->m_cold.compressions[idx]) << "\n";

    std::cout << "  Num blocks: " << store->m_cold.block_infos[idx].size() << "\n";
    std::cout << "  Stored in metadata: " << (store->m_cold.stored_in_metadata_flags[idx] ? "yes" : "no") << "\n";

    // Block details if verbose
    if (verbose && store->m_cold.block_infos[idx].size() > 0) {
        std::cout << "\n  Blocks:\n";
        size_t max_blocks = std::min<size_t>(10, store->m_cold.block_infos[idx].size());
        for (size_t i = 0; i < max_blocks; ++i) {
            const auto& block = store->m_cold.block_infos[idx][i];
            double ratio = 100.0 * block.compressed_size / block.uncompressed_size;
            std::cout << "    [" << i << "] offset=" << block.offset
                      << ", compressed=" << block.compressed_size
                      << ", uncompressed=" << block.uncompressed_size
                      << " (" << std::fixed << std::setprecision(1) << ratio << "%)\n";
        }
        if (store->m_cold.block_infos[idx].size() > max_blocks) {
            std::cout << "    ... (" << (store->m_cold.block_infos[idx].size() - max_blocks)
                      << " more blocks)\n";
        }
    }

    std::cout << "\n  Note: Data not loaded. Use -d to load data (if size permits).\n";
}

// Print a single metadata-namespace value in full (type, shape, all elements).
// Used by -d/-m for keys stored in the metadata block (not separate arrays).
void print_metadata_value(StarDataset* store, const std::string& key, bool verbose) {
    auto mv = store->meta.get(key);
    if (!mv) {
        std::cout << "  Key not found!\n";
        return;
    }

    std::cout << "\n=== Key: " << key << " (metadata) ===\n";
    std::cout << "  Type: " << datatype_to_string(mv->dtype);
    if (!mv->shape.empty()) {
        std::cout << "[";
        for (size_t i = 0; i < mv->shape.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << mv->shape[i];
        }
        std::cout << "]";
    } else {
        std::cout << " (scalar)";
    }
    std::cout << "\n  Elements: " << mv->size() << "\n";

    // Print all values, dispatching on dtype via as<T>().
    std::cout << "  Data: [";
    auto dump = [](auto&& arr) {
        for (size_t j = 0; j < arr.size(); ++j) {
            if (j > 0) std::cout << ", ";
            std::cout << arr.flat(j);
        }
    };
    switch (mv->dtype) {
        case DataType::INT8: { auto a = mv->as<int8_t>();
            for (size_t j = 0; j < a.size(); ++j) { if (j) std::cout << ", "; std::cout << (int)a.flat(j);} break; }
        case DataType::INT16:  dump(mv->as<int16_t>());  break;
        case DataType::INT32:  dump(mv->as<int32_t>());  break;
        case DataType::INT64:  dump(mv->as<int64_t>());  break;
        case DataType::UINT8: { auto a = mv->as<uint8_t>();
            for (size_t j = 0; j < a.size(); ++j) { if (j) std::cout << ", "; std::cout << (unsigned)a.flat(j);} break; }
        case DataType::UINT16: dump(mv->as<uint16_t>()); break;
        case DataType::UINT32: dump(mv->as<uint32_t>()); break;
        case DataType::UINT64: dump(mv->as<uint64_t>()); break;
        case DataType::FLOAT32: { auto a = mv->as<float>();
            std::cout << std::fixed << std::setprecision(6);
            for (size_t j = 0; j < a.size(); ++j) { if (j) std::cout << ", "; std::cout << a.flat(j);} break; }
        case DataType::FLOAT64: { auto a = mv->as<double>();
            std::cout << std::fixed << std::setprecision(6);
            for (size_t j = 0; j < a.size(); ++j) { if (j) std::cout << ", "; std::cout << a.flat(j);} break; }
        case DataType::STRING: { auto a = mv->as<std::string>();
            for (size_t j = 0; j < a.size(); ++j) { if (j) std::cout << ", "; std::cout << "\"" << a.flat(j) << "\"";} break; }
        default: std::cout << "<unsupported dtype>";
    }
    std::cout << "]\n";
    (void)verbose;
}

void print_metadata_block_summary(StarDataset* store, bool verbose) {
    // Metadata lives in the (lazily-loaded) per-layer metadata registry, not in
    // m_hot, so query it through the public API rather than iterating m_hot.
    std::vector<std::string> metadata_keys = store->get_metadata_keys();

    if (metadata_keys.empty()) {
        return;
    }

    std::cout << "\n╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║                      METADATA ENTRIES                         ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

    std::cout << "\n  Contains " << metadata_keys.size() << " keys:\n\n";

    // Print each metadata key with its value using new store->meta API
    for (size_t i = 0; i < metadata_keys.size(); ++i) {
        const auto& key = metadata_keys[i];
        std::cout << "  [" << std::setw(2) << i << "] " << std::left << std::setw(50) << key;

        // Use new API: single call with type introspection
        auto meta = store->meta.get(key);
        if (!meta) {
            std::cout << " = <not found>\n";
            continue;
        }

        // Use type introspection instead of guessing
        switch (meta->dtype) {
            case DataType::INT8: {
                auto arr = meta->as<int8_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << static_cast<int>(arr.flat(0));
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << static_cast<int>(arr.flat(j));
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }

            case DataType::INT16: {
                auto arr = meta->as<int16_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::INT32: {
                auto arr = meta->as<int32_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::INT64: {
                auto arr = meta->as<int64_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::UINT8: {
                auto arr = meta->as<uint8_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << static_cast<unsigned>(arr.flat(0));
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << static_cast<unsigned>(arr.flat(j));
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::UINT16: {
                auto arr = meta->as<uint16_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::UINT32: {
                auto arr = meta->as<uint32_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::UINT64: {
                auto arr = meta->as<uint64_t>();
                if (arr.size() == 1) {
                    std::cout << " = " << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::FLOAT32: {
                auto arr = meta->as<float>();
                if (arr.size() == 1) {
                    std::cout << " = " << std::fixed << std::setprecision(3) << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << std::fixed << std::setprecision(3) << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::FLOAT64: {
                auto arr = meta->as<double>();
                if (arr.size() == 1) {
                    std::cout << " = " << std::fixed << std::setprecision(3) << arr.flat(0);
                } else if (arr.size() <= 5) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::cout << std::fixed << std::setprecision(3) << arr.flat(j);
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " elements]";
                }
                break;
            }
            case DataType::STRING: {
                auto arr = meta->as<std::string>();
                if (arr.size() == 1) {
                    std::string val = arr.flat(0);
                    if (val.length() > 40) {
                        std::cout << " = \"" << val.substr(0, 37) << "...\"";
                    } else {
                        std::cout << " = \"" << val << "\"";
                    }
                } else if (arr.size() <= 3) {
                    std::cout << " = [";
                    for (size_t j = 0; j < arr.size(); ++j) {
                        if (j > 0) std::cout << ", ";
                        std::string val = arr.flat(j);
                        if (val.length() > 20) {
                            std::cout << "\"" << val.substr(0, 17) << "...\"";
                        } else {
                            std::cout << "\"" << val << "\"";
                        }
                    }
                    std::cout << "]";
                } else {
                    std::cout << " = [" << arr.size() << " strings]";
                }
                break;
            }
            default:
                std::cout << " = <unknown type>";
        }

        std::cout << "\n";
    }
}

void print_key_data(StarDataset* store, const std::string& key, bool verbose) {
    std::cout << "\n=== Key: " << key << " ===\n";

    // Find key in SoA
    auto it = store->m_key_to_index.find(key);
    if (it == store->m_key_to_index.end()) {
        std::cout << "  Key not found!\n";
        return;
    }

    size_t idx = it->second;

    if (verbose) {
        std::cout << "  Type: " << datatype_to_string(store->m_hot.dtypes[idx]);
        const auto& shape = store->m_cold.shapes[idx];
        if (!shape.empty()) {
            std::cout << "[";
            for (size_t i = 0; i < shape.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << shape[i];
            }
            std::cout << "]";
        } else {
            std::cout << " (scalar)";
        }
        std::cout << "\n";

        // Calculate num_elements
        size_t num_elements = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
        std::cout << "  Elements: " << num_elements << "\n";
        std::cout << "  Compression: " << compression_name(store->m_cold.compressions[idx]) << "\n";

        std::cout << "  Num blocks: " << store->m_cold.block_infos[idx].size() << "\n";
        std::cout << "  Total bytes: " << store->m_cold.compressed_sizes[idx] << "\n";
        std::cout << "  Stored in metadata: " << (store->m_cold.stored_in_metadata_flags[idx] ? "yes" : "no") << "\n";

        if (store->m_cold.block_infos[idx].size() > 1 || store->m_cold.compressions[idx] != CompressionAlgorithm::NONE) {
            std::cout << "  Blocks:\n";
            for (size_t i = 0; i < store->m_cold.block_infos[idx].size(); ++i) {
                const auto& block = store->m_cold.block_infos[idx][i];
                double ratio = 100.0 * block.compressed_size / block.uncompressed_size;
                std::cout << "    [" << i << "] offset=" << block.offset
                          << ", compressed=" << block.compressed_size
                          << ", uncompressed=" << block.uncompressed_size
                          << " (" << std::fixed << std::setprecision(1) << ratio << "%)\n";
            }
        }
    }

    std::cout << "\n  Reading data...\n";

    // A key can live in the ARRAY namespace (separate, block-compressed storage,
    // read via store->get<T>()) or the METADATA namespace (read via meta.get()).
    // Separately-stored arrays are NOT in the metadata block, so pick the right
    // reader based on where the key actually is.
    bool is_array = (store->m_cold.stored_in_metadata_flags[idx] == 0);
    DataType dtype = store->m_hot.dtypes[idx];

    try {
        // Dispatch on dtype; read from the array or metadata namespace as needed.
        // The lambda gives us one place per type that handles both cases.
        auto read_and_print = [&](auto tag) {
            using T = decltype(tag);
            if (is_array) {
                print_NDArray_data(store->get<T>(key));
            } else {
                auto meta = store->meta.get(key);
                if (!meta) { std::cout << "    Key not found\n"; return; }
                print_NDArray_data(meta->template as<T>());
            }
        };

        switch (dtype) {
            case DataType::INT8:    read_and_print(int8_t{});   break;
            case DataType::INT16:   read_and_print(int16_t{});  break;
            case DataType::INT32:   read_and_print(int32_t{});  break;
            case DataType::INT64:   read_and_print(int64_t{});  break;
            case DataType::UINT8:   read_and_print(uint8_t{});  break;
            case DataType::UINT16:  read_and_print(uint16_t{}); break;
            case DataType::UINT32:  read_and_print(uint32_t{}); break;
            case DataType::UINT64:  read_and_print(uint64_t{}); break;
            case DataType::FLOAT32: read_and_print(float{});    break;
            case DataType::FLOAT64: read_and_print(double{});   break;
            case DataType::STRING: {
                if (is_array) {
                    print_NDArray_data(store->get<std::string>(key), 20);
                } else {
                    auto meta = store->meta.get(key);
                    if (!meta) { std::cout << "    Key not found\n"; break; }
                    print_NDArray_data(meta->as<std::string>(), 20);
                }
                break;
            }
            default:
                std::cout << "    Unknown data type\n";
        }
    } catch (const std::exception& e) {
        std::cout << "    Error reading data: " << e.what() << "\n";
    }
}

int main(int argc, char* argv[]) {
    // Disable trace logging by default
    logger::set_log_level(logger::ERROR);

    bool verbose = false;
    bool print_all_data = false;
    bool force_load = false;
    std::string specific_key;
    std::string metadata_key;
    std::string filename;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--verbose") {
            verbose = true;
        } else if (arg == "-a" || arg == "--all") {
            print_all_data = true;
        } else if (arg == "-k" || arg == "--keys") {
            // Default behavior, do nothing
        } else if (arg == "-d" || arg == "--data") {
            if (i + 1 < argc) {
                specific_key = argv[++i];
            } else {
                std::cerr << "Error: -d/--data requires a key name\n";
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg == "-m" || arg == "--metadata") {
            if (i + 1 < argc) {
                metadata_key = argv[++i];
            } else {
                std::cerr << "Error: -m/--metadata requires a key name\n";
                print_usage(argv[0]);
                return 1;
            }
        } else if (arg == "-f" || arg == "--force") {
            force_load = true;
        } else if (arg[0] != '-') {
            filename = arg;
        } else {
            std::cerr << "Error: Unknown option " << arg << "\n";
            print_usage(argv[0]);
            return 1;
        }
    }

    if (filename.empty()) {
        std::cerr << "Error: No input file specified\n";
        print_usage(argv[0]);
        return 1;
    }

    try {
        // Open the store
        std::cout << "Opening STAR file: " << filename << "\n";
        auto store = StarDataset::open(filename, FileMode::READ_ONLY);

        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                      STAR File Contents                       ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

        std::cout << "File: " << filename << "\n";

        // Separately-stored array keys live in m_hot (stored_in_metadata_flags==0).
        // Metadata-block keys are tracked in the per-layer registry (lazy-loaded,
        // NOT in m_hot on open), so count them via the public metadata API.
        size_t user_key_count = 0;
        for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
            if (store->m_cold.stored_in_metadata_flags[i] == 0) {
                user_key_count++;
            }
        }
        size_t metadata_key_count = store->get_metadata_count();

        // Display file header information
        const auto& header = store->getFileHeader();
        std::cout << "Format Version: " << (int)header.format_version << "\n";
        std::cout << "Library Version: " << header.getVersionString() << "\n";
        std::cout << "Magic: " << std::string(header.magic, 6) << "\n";
        std::cout << "Header Size: " << header.header_size << " bytes\n";

        std::cout << "Keys: " << user_key_count;
        if (metadata_key_count > 0) {
            std::cout << " (" << metadata_key_count << " in metadata)";
        }
        std::cout << "\n";

        if (user_key_count == 0 && metadata_key_count == 0) {
            std::cout << "\nNo keys found in file.\n";
            return 0;
        }

        // Calculate total data size for separately stored arrays, and tally the
        // compression codec used across those arrays.
        size_t total_bytes = 0;
        size_t total_elements = 0;
        std::map<CompressionAlgorithm, size_t> comp_counts;
        // Per-array (uncompressed) block size, taken from the first block of each
        // array. Block size can differ between arrays, so tally distinct values.
        std::map<size_t, size_t> block_size_counts;
        for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
            if (store->m_cold.stored_in_metadata_flags[i] == 0) {
                total_bytes += store->m_cold.compressed_sizes[i];
                const auto& shape = store->m_cold.shapes[i];
                size_t num_elements = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
                total_elements += num_elements;
                comp_counts[store->m_cold.compressions[i]]++;
                const auto& blocks = store->m_cold.block_infos[i];
                if (!blocks.empty()) {
                    block_size_counts[blocks[0].uncompressed_size]++;
                }
            }
        }
        std::cout << "Total data: " << total_bytes << " bytes, " << total_elements << " elements\n";

        // Compression is per-array: report the single codec if uniform, else a
        // per-codec breakdown across the stored arrays.
        if (!comp_counts.empty()) {
            std::cout << "Compression: ";
            if (comp_counts.size() == 1) {
                std::cout << compression_name(comp_counts.begin()->first);
            } else {
                bool first = true;
                for (const auto& [algo, count] : comp_counts) {
                    if (!first) std::cout << ", ";
                    std::cout << compression_name(algo) << " (" << count << ")";
                    first = false;
                }
            }
            std::cout << "\n";
        }

        // Block size (uncompressed chunk size) for the separately-stored arrays.
        // Reported separately from the metadata block since the two can differ.
        if (!block_size_counts.empty()) {
            std::cout << "Array block size: ";
            if (block_size_counts.size() == 1) {
                std::cout << block_size_counts.begin()->first << " bytes";
            } else {
                bool first = true;
                for (const auto& [bs, count] : block_size_counts) {
                    if (!first) std::cout << ", ";
                    std::cout << bs << " bytes (" << count << ")";
                    first = false;
                }
            }
            std::cout << "\n";
        }

        // Metadata block: a single per-layer compressed unit. Report the base
        // layer's metadata block size (compressed bytes on disk); this is
        // independent of the array block size above. Use get_metadata_count()
        // rather than has_metadata_block() since the block is lazily loaded and
        // may not be reflected in m_cold on open.
        if (store->get_metadata_count() > 0) {
            size_t base_idx = store->m_layer_metadata_registry.get_layer_index("__base__");
            size_t meta_block_bytes = store->m_layer_metadata_registry.block_sizes[base_idx];
            std::cout << "Metadata block size: " << meta_block_bytes
                      << " bytes (compressed, single block)\n";
        }

        // If metadata-only requested
        if (!metadata_key.empty()) {
            auto it = store->m_key_to_index.find(metadata_key);
            if (it != store->m_key_to_index.end()) {
                // Separately-stored array: show block/compression details.
                print_key_metadata(store.get(), metadata_key, verbose);
            } else if (store->meta.contains(metadata_key)) {
                // Metadata-namespace value: describe it via the meta API.
                auto mv = store->meta.get(metadata_key);
                std::cout << "\n=== Key: " << metadata_key << " (metadata) ===\n";
                std::cout << "  Type: " << datatype_to_string(mv->dtype);
                if (!mv->shape.empty()) {
                    std::cout << "[";
                    for (size_t i = 0; i < mv->shape.size(); ++i) {
                        if (i > 0) std::cout << ", ";
                        std::cout << mv->shape[i];
                    }
                    std::cout << "]";
                } else {
                    std::cout << " (scalar)";
                }
                std::cout << "\n  Elements: " << mv->size() << "\n";
                std::cout << "  Stored in metadata: yes\n";
            } else {
                std::cerr << "\nError: Key '" << metadata_key << "' not found in file\n";
                std::cout << "\nAvailable keys:\n";
                for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
                    if (store->m_cold.stored_in_metadata_flags[i] == 0)
                        std::cout << "  - " << store->m_hot.keys[i] << "\n";
                }
                for (const auto& k : store->get_metadata_keys()) {
                    std::cout << "  - " << k << " (metadata)\n";
                }
                return 1;
            }
        }
        // If specific key requested with data
        else if (!specific_key.empty()) {
            auto it = store->m_key_to_index.find(specific_key);
            if (it != store->m_key_to_index.end()) {
                size_t idx = it->second;

                // Safety check (unless --force)
                if (!force_load) {
                    size_t uncompressed_size = store->m_cold.uncompressed_sizes[idx];
                    const size_t MAX_SAFE_SIZE = 2ULL * 1024 * 1024 * 1024;  // 2 GB

                    if (uncompressed_size > MAX_SAFE_SIZE) {
                        double size_gb = uncompressed_size / (1024.0 * 1024.0 * 1024.0);
                        std::cout << "\n⚠️  ERROR: Array too large to load safely\n";
                        std::cout << "   Uncompressed size: " << std::fixed << std::setprecision(2)
                                  << size_gb << " GB\n";
                        std::cout << "   Required RAM: ~" << std::fixed << std::setprecision(1)
                                  << (size_gb * 2) << " GB\n\n";
                        std::cout << "Options:\n";
                        std::cout << "  1. Use -m instead of -d to view metadata only\n";
                        std::cout << "  2. Use --force to bypass this check (may freeze system)\n";
                        return 1;
                    }
                }

                print_key_data(store.get(), specific_key, verbose);
            } else if (store->meta.contains(specific_key)) {
                // Key lives in the metadata namespace, not separate array storage.
                print_metadata_value(store.get(), specific_key, verbose);
            } else {
                std::cerr << "\nError: Key '" << specific_key << "' not found in file\n";
                std::cout << "\nAvailable keys:\n";
                for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
                    if (store->m_cold.stored_in_metadata_flags[i] == 0)
                        std::cout << "  - " << store->m_hot.keys[i] << "\n";
                }
                for (const auto& k : store->get_metadata_keys()) {
                    std::cout << "  - " << k << " (metadata)\n";
                }
                return 1;
            }
        }
        // If print all data
        else if (print_all_data) {
            // First, print metadata entries if they exist
            if (metadata_key_count > 0) {
                print_metadata_block_summary(store.get(), verbose);
            }

            // Then print all separately stored array keys
            for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
                if (store->m_cold.stored_in_metadata_flags[i] == 0) {
                    print_key_data(store.get(), store->m_hot.keys[i], verbose);
                }
            }
        }
        // Otherwise just list keys
        else {
            std::cout << "\n";
            std::cout << "┌────────────────────────────────────────────────────────────┐\n";
            std::cout << "│ Keys                                                       │\n";
            std::cout << "├────────────────────────────────────────────────────────────┤\n";

            for (size_t idx = 0; idx < store->m_hot.keys.size(); idx++) {
                const std::string& key = store->m_hot.keys[idx];
                std::cout << "│ [" << std::right << std::setw(2) << idx << "] " << std::left << std::setw(54) << key << "│\n";

                if (verbose) {
                    std::cout << "│     Type: " << std::left << std::setw(49) << datatype_to_string(store->m_hot.dtypes[idx]);

                    const auto& shape = store->m_cold.shapes[idx];
                    if (!shape.empty()) {
                        std::cout << "│\n│     Shape: [";
                        for (size_t i = 0; i < shape.size(); ++i) {
                            if (i > 0) std::cout << ", ";
                            std::cout << shape[i];
                        }
                        std::cout << "]";
                        // Padding
                        size_t shape_str_len = 13; // "     Shape: ["
                        for (size_t i = 0; i < shape.size(); ++i) {
                            if (i > 0) shape_str_len += 2;
                            shape_str_len += std::to_string(shape[i]).length();
                        }
                        shape_str_len += 1; // "]"
                        std::cout << std::string(60 - shape_str_len, ' ') << "│\n";
                    } else {
                        std::cout << "│\n│     Scalar" << std::string(49, ' ') << "│\n";
                    }

                    size_t num_elements = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
                    std::cout << "│     Elements: " << std::left << std::setw(45) << num_elements << "│\n";
                    std::cout << "│     Size: " << std::left << std::setw(49) << (std::to_string(store->m_cold.compressed_sizes[idx]) + " bytes") << "│\n";

                    CompressionAlgorithm algo = store->m_cold.compressions[idx];
                    std::string comp_str = compression_name(algo);
                    if (algo != CompressionAlgorithm::NONE) {
                        comp_str += " (" + std::to_string(store->m_cold.block_infos[idx].size()) + " blocks)";
                    }
                    std::cout << "│     Compression: " << std::left << std::setw(42) << comp_str << "│\n";
                    std::cout << "├────────────────────────────────────────────────────────────┤\n";
                } else if (!verbose) {
                    // For non-verbose, show type info
                    std::string type_info;
                    if (store->m_cold.stored_in_metadata_flags[idx] == 1) {
                        type_info = "(in metadata)";
                    } else {
                        type_info = std::string(datatype_to_string(store->m_hot.dtypes[idx]));
                        const auto& shape = store->m_cold.shapes[idx];
                        if (!shape.empty()) {
                            type_info += "[";
                            for (size_t i = 0; i < shape.size(); ++i) {
                                if (i > 0) type_info += ",";
                                type_info += std::to_string(shape[i]);
                            }
                            type_info += "]";
                        }
                        type_info += ", " + std::to_string(store->m_cold.compressed_sizes[idx]) + " bytes";
                    }

                    std::cout << "│     " << std::left << std::setw(55) << type_info << "│\n";
                }
            }

            std::cout << "└────────────────────────────────────────────────────────────┘\n";

            // Array keys (above) come from m_hot; metadata-block keys live in the
            // per-layer registry and won't appear there, so list them separately.
            if (metadata_key_count > 0) {
                print_metadata_block_summary(store.get(), verbose);
            }

            if (!verbose) {
                std::cout << "\nUse -v/--verbose for detailed metadata\n";
                std::cout << "Use -d/--data <key> to print specific key data\n";
                std::cout << "Use -a/--all to print all data (may be large)\n";
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
