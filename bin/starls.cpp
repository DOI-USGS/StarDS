#include "../Star/include/star.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

using namespace star;

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <file.star>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -k, --keys           List keys only (default)\n";
    std::cout << "  -d, --data <key>     Print data for specific key\n";
    std::cout << "  -m, --metadata <key> Show metadata without loading data\n";
    std::cout << "  -a, --all            Print all data (WARNING: may be large)\n";
    std::cout << "  -v, --verbose        Verbose output with detailed metadata\n";
    std::cout << "  -f, --force          Force loading large arrays (bypass safety check)\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " data.star              # List all keys\n";
    std::cout << "  " << program_name << " -v data.star           # Verbose listing\n";
    std::cout << "  " << program_name << " -m data data.star      # Show metadata only\n";
    std::cout << "  " << program_name << " -d mykey data.star     # Print specific key\n";
    std::cout << "  " << program_name << " -d mykey -f data.star  # Force load large array\n";
    std::cout << "  " << program_name << " -a data.star           # Print all data\n";
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
    std::cout << "  Compression: ";
    switch(store->m_cold.compressions[idx]) {
        case CompressionAlgorithm::NONE: std::cout << "None"; break;
        case CompressionAlgorithm::GZIP: std::cout << "GZIP"; break;
        case CompressionAlgorithm::ZSTD: std::cout << "ZSTD"; break;
        case CompressionAlgorithm::LZ4: std::cout << "LZ4"; break;
        default: std::cout << "Unknown";
    }
    std::cout << "\n";

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

void print_metadata_block_summary(StarDataset* store, bool verbose) {
    // Get all metadata keys (stored_in_metadata_flags = 1)
    std::vector<std::string> metadata_keys;
    for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
        if (store->m_cold.stored_in_metadata_flags[i] == 1) {
            metadata_keys.push_back(store->m_hot.keys[i]);
        }
    }

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
        std::cout << "  Compression: ";
        switch(store->m_cold.compressions[idx]) {
            case CompressionAlgorithm::NONE: std::cout << "None"; break;
            case CompressionAlgorithm::GZIP: std::cout << "GZIP"; break;
            case CompressionAlgorithm::ZSTD: std::cout << "ZSTD"; break;
            case CompressionAlgorithm::LZ4: std::cout << "LZ4"; break;
            default: std::cout << "Unknown";
        }
        std::cout << "\n";

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

    // Use new store->meta API (works for all metadata, single call with type introspection)
    std::cout << "\n  Reading data...\n";

    try {
        auto meta = store->meta.get(key);
        if (!meta) {
            std::cout << "    Key not found\n";
            return;
        }

        // Use type introspection to print the correct type
        switch (meta->dtype) {
            case DataType::INT8: {
                auto arr = meta->as<int8_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::INT16: {
                auto arr = meta->as<int16_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::INT32: {
                auto arr = meta->as<int32_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::INT64: {
                auto arr = meta->as<int64_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::UINT8: {
                auto arr = meta->as<uint8_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::UINT16: {
                auto arr = meta->as<uint16_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::UINT32: {
                auto arr = meta->as<uint32_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::UINT64: {
                auto arr = meta->as<uint64_t>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::FLOAT32: {
                auto arr = meta->as<float>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::FLOAT64: {
                auto arr = meta->as<double>();
                print_NDArray_data(arr);
                break;
            }
            case DataType::STRING: {
                auto arr = meta->as<std::string>();
                print_NDArray_data(arr, 20);
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
        std::cout << "Opening SCS file: " << filename << "\n";
        auto store = StarDataset::open(filename, FileMode::READ_ONLY);

        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                      SCS File Contents                        ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

        std::cout << "File: " << filename << "\n";

        // Count user keys (separately stored arrays with stored_in_metadata_flags = 0)
        size_t user_key_count = 0;
        size_t metadata_key_count = 0;
        for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
            if (store->m_cold.stored_in_metadata_flags[i] == 1) {
                metadata_key_count++;
            } else {
                user_key_count++;
            }
        }

        // Get all keys including those in metadata block
        auto all_keys = store->get_all_keys();
        size_t total_key_count = all_keys.size();

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

        // Calculate total data size for separately stored arrays
        size_t total_bytes = 0;
        size_t total_elements = 0;
        for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
            if (store->m_cold.stored_in_metadata_flags[i] == 0) {
                total_bytes += store->m_cold.compressed_sizes[i];
                const auto& shape = store->m_cold.shapes[i];
                size_t num_elements = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
                total_elements += num_elements;
            }
        }
        std::cout << "Total data: " << total_bytes << " bytes, " << total_elements << " elements\n";

        // If metadata-only requested
        if (!metadata_key.empty()) {
            auto it = store->m_key_to_index.find(metadata_key);
            if (it != store->m_key_to_index.end()) {
                print_key_metadata(store.get(), metadata_key, verbose);
            } else {
                std::cerr << "\nError: Key '" << metadata_key << "' not found in file\n";
                std::cout << "\nAvailable keys:\n";
                for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
                    std::cout << "  - " << store->m_hot.keys[i] << "\n";
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
            } else {
                std::cerr << "\nError: Key '" << specific_key << "' not found in file\n";
                std::cout << "\nAvailable keys:\n";
                for (size_t i = 0; i < store->m_hot.keys.size(); i++) {
                    std::cout << "  - " << store->m_hot.keys[i] << "\n";
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
                std::cout << "│ [" << std::setw(2) << idx << "] " << std::left << std::setw(52) << key << "│\n";

                if (verbose) {
                    std::cout << "│     Type: " << std::left << std::setw(46) << datatype_to_string(store->m_hot.dtypes[idx]);

                    const auto& shape = store->m_cold.shapes[idx];
                    if (!shape.empty()) {
                        std::cout << "│\n│     Shape: [";
                        for (size_t i = 0; i < shape.size(); ++i) {
                            if (i > 0) std::cout << ", ";
                            std::cout << shape[i];
                        }
                        std::cout << "]";
                        // Padding
                        size_t shape_str_len = 12; // "     Shape: ["
                        for (size_t i = 0; i < shape.size(); ++i) {
                            if (i > 0) shape_str_len += 2;
                            shape_str_len += std::to_string(shape[i]).length();
                        }
                        shape_str_len += 1; // "]"
                        std::cout << std::string(54 - shape_str_len, ' ') << "│\n";
                    } else {
                        std::cout << "│\n│     Scalar" << std::string(44, ' ') << "│\n";
                    }

                    size_t num_elements = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), 1ULL, std::multiplies<size_t>());
                    std::cout << "│     Elements: " << std::left << std::setw(41) << num_elements << "│\n";
                    std::cout << "│     Size: " << std::left << std::setw(45) << (std::to_string(store->m_cold.compressed_sizes[idx]) + " bytes") << "│\n";

                    std::string comp_str;
                    switch(store->m_cold.compressions[idx]) {
                        case CompressionAlgorithm::NONE: comp_str = "None"; break;
                        case CompressionAlgorithm::GZIP: comp_str = "GZIP (" + std::to_string(store->m_cold.block_infos[idx].size()) + " blocks)"; break;
                        case CompressionAlgorithm::ZSTD: comp_str = "ZSTD (" + std::to_string(store->m_cold.block_infos[idx].size()) + " blocks)"; break;
                        case CompressionAlgorithm::LZ4: comp_str = "LZ4 (" + std::to_string(store->m_cold.block_infos[idx].size()) + " blocks)"; break;
                        default: comp_str = "Unknown";
                    }
                    std::cout << "│     Compression: " << std::left << std::setw(38) << comp_str << "│\n";
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

                    std::cout << "│     " << std::left << std::setw(52) << type_info << "│\n";
                }
            }

            std::cout << "└────────────────────────────────────────────────────────────┘\n";

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
