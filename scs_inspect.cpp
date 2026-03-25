#include "CameraStateFile/include/scs.h"
#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <file.scs>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -h, --help           Show this help message\n";
    std::cout << "  -k, --keys           List keys only (default)\n";
    std::cout << "  -d, --data <key>     Print data for specific key\n";
    std::cout << "  -a, --all            Print all data (WARNING: may be large)\n";
    std::cout << "  -v, --verbose        Verbose output with detailed metadata\n";
    std::cout << "\nExamples:\n";
    std::cout << "  " << program_name << " data.scs              # List all keys\n";
    std::cout << "  " << program_name << " -v data.scs           # Verbose listing\n";
    std::cout << "  " << program_name << " -d mykey data.scs     # Print specific key\n";
    std::cout << "  " << program_name << " -a data.scs           # Print all data\n";
}

template<typename T>
void print_ndarray_data(const NDArray<T>& arr, size_t max_elements = 100) {
    std::cout << "    Data (showing up to " << max_elements << " elements):\n    [";

    size_t count = 0;
    for (size_t i = 0; i < arr.data.size() && count < max_elements; ++i) {
        if (i > 0) std::cout << ", ";
        if (i > 0 && i % 10 == 0) std::cout << "\n     ";

        if constexpr (std::is_floating_point_v<T>) {
            std::cout << std::fixed << std::setprecision(3) << arr.data[i];
        } else {
            std::cout << arr.data[i];
        }
        count++;
    }

    if (arr.data.size() > max_elements) {
        std::cout << ", ... (" << (arr.data.size() - max_elements) << " more)";
    }

    std::cout << "]\n";
}

void print_key_data(SCStore& store, const std::string& key, bool verbose) {
    const auto& entry = store.m_index[key];

    std::cout << "\n=== Key: " << key << " ===\n";

    if (verbose) {
        std::cout << "  Type: " << datatype_to_string(entry.datatype);
        if (!entry.shape.empty()) {
            std::cout << "[";
            for (size_t i = 0; i < entry.shape.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << entry.shape[i];
            }
            std::cout << "]";
        } else {
            std::cout << " (scalar)";
        }
        std::cout << "\n";

        std::cout << "  Elements: " << entry.num_elements() << "\n";
        std::cout << "  Compression: ";
        switch(entry.compression) {
            case CompressionAlgorithm::NONE: std::cout << "None"; break;
            case CompressionAlgorithm::GZIP: std::cout << "GZIP"; break;
            case CompressionAlgorithm::ZSTD: std::cout << "ZSTD"; break;
            case CompressionAlgorithm::LZ4: std::cout << "LZ4"; break;
            default: std::cout << "Unknown";
        }
        std::cout << "\n";

        std::cout << "  Block size: " << entry.block_size << " bytes\n";
        std::cout << "  Num blocks: " << entry.blocks.size() << "\n";
        std::cout << "  Total bytes: " << entry.total_bytes << "\n";

        if (entry.blocks.size() > 1 || entry.compression != CompressionAlgorithm::NONE) {
            std::cout << "  Blocks:\n";
            for (size_t i = 0; i < entry.blocks.size(); ++i) {
                const auto& block = entry.blocks[i];
                double ratio = 100.0 * block.compressed_size / block.uncompressed_size;
                std::cout << "    [" << i << "] offset=" << block.offset
                          << ", compressed=" << block.compressed_size
                          << ", uncompressed=" << block.uncompressed_size
                          << " (" << std::fixed << std::setprecision(1) << ratio << "%)\n";
            }
        }
    }

    // Try to read and print the data
    std::cout << "\n  Reading data...\n";

    try {
        // Try each type
        switch(entry.datatype) {
            case DataType::INT8: {
                auto data = store.get<NDArray<int8_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::INT16: {
                auto data = store.get<NDArray<int16_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::INT32: {
                auto data = store.get<NDArray<int32_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::INT64: {
                auto data = store.get<NDArray<int64_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::UINT8: {
                auto data = store.get<NDArray<uint8_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::UINT16: {
                auto data = store.get<NDArray<uint16_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::UINT32: {
                auto data = store.get<NDArray<uint32_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::UINT64: {
                auto data = store.get<NDArray<uint64_t>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::FLOAT32: {
                auto data = store.get<NDArray<float>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::FLOAT64: {
                auto data = store.get<NDArray<double>>(key);
                if (data) print_ndarray_data(*data);
                break;
            }
            case DataType::STRING: {
                auto data = store.get<NDArray<std::string>>(key);
                if (data) print_ndarray_data(*data, 20); // Limit strings to 20
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
    std::string specific_key;
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
        SCStore store(filename);

        std::cout << "\n";
        std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║                      SCS File Contents                        ║\n";
        std::cout << "╚═══════════════════════════════════════════════════════════════╝\n\n";

        std::cout << "File: " << filename << "\n";
        std::cout << "Keys: " << store.m_index.size() << "\n";
        std::cout << "Header size: " << store.m_header_size << " bytes\n";

        if (store.m_index.empty()) {
            std::cout << "\nNo keys found in file.\n";
            return 0;
        }

        // Calculate total data size
        size_t total_bytes = 0;
        size_t total_elements = 0;
        for (const auto& [key, entry] : store.m_index) {
            total_bytes += entry.total_bytes;
            total_elements += entry.num_elements();
        }
        std::cout << "Total data: " << total_bytes << " bytes, " << total_elements << " elements\n";

        // If specific key requested
        if (!specific_key.empty()) {
            if (store.contains(specific_key)) {
                print_key_data(store, specific_key, verbose);
            } else {
                std::cerr << "\nError: Key '" << specific_key << "' not found in file\n";
                std::cout << "\nAvailable keys:\n";
                for (const auto& [key, entry] : store.m_index) {
                    std::cout << "  - " << key << "\n";
                }
                return 1;
            }
        }
        // If print all data
        else if (print_all_data) {
            for (const auto& [key, entry] : store.m_index) {
                print_key_data(store, key, verbose);
            }
        }
        // Otherwise just list keys
        else {
            std::cout << "\n";
            std::cout << "┌────────────────────────────────────────────────────────────┐\n";
            std::cout << "│ Keys                                                       │\n";
            std::cout << "├────────────────────────────────────────────────────────────┤\n";

            size_t idx = 0;
            for (const auto& [key, entry] : store.m_index) {
                std::cout << "│ [" << std::setw(2) << idx++ << "] " << std::left << std::setw(52) << key << "│\n";

                if (verbose) {
                    std::cout << "│     Type: " << std::left << std::setw(46) << datatype_to_string(entry.datatype);

                    if (!entry.shape.empty()) {
                        std::cout << "│\n│     Shape: [";
                        for (size_t i = 0; i < entry.shape.size(); ++i) {
                            if (i > 0) std::cout << ", ";
                            std::cout << entry.shape[i];
                        }
                        std::cout << "]";
                        // Padding
                        size_t shape_str_len = 12; // "     Shape: ["
                        for (size_t i = 0; i < entry.shape.size(); ++i) {
                            if (i > 0) shape_str_len += 2;
                            shape_str_len += std::to_string(entry.shape[i]).length();
                        }
                        shape_str_len += 1; // "]"
                        std::cout << std::string(54 - shape_str_len, ' ') << "│\n";
                    } else {
                        std::cout << "│\n│     Scalar" << std::string(44, ' ') << "│\n";
                    }

                    std::cout << "│     Elements: " << std::left << std::setw(41) << entry.num_elements() << "│\n";
                    std::cout << "│     Size: " << std::left << std::setw(45) << (std::to_string(entry.total_bytes) + " bytes") << "│\n";

                    std::string comp_str;
                    switch(entry.compression) {
                        case CompressionAlgorithm::NONE: comp_str = "None"; break;
                        case CompressionAlgorithm::GZIP: comp_str = "GZIP (" + std::to_string(entry.blocks.size()) + " blocks)"; break;
                        case CompressionAlgorithm::ZSTD: comp_str = "ZSTD (" + std::to_string(entry.blocks.size()) + " blocks)"; break;
                        case CompressionAlgorithm::LZ4: comp_str = "LZ4 (" + std::to_string(entry.blocks.size()) + " blocks)"; break;
                        default: comp_str = "Unknown";
                    }
                    std::cout << "│     Compression: " << std::left << std::setw(38) << comp_str << "│\n";
                    std::cout << "├────────────────────────────────────────────────────────────┤\n";
                } else {
                    std::string type_info = std::string(datatype_to_string(entry.datatype));
                    if (!entry.shape.empty()) {
                        type_info += "[";
                        for (size_t i = 0; i < entry.shape.size(); ++i) {
                            if (i > 0) type_info += ",";
                            type_info += std::to_string(entry.shape[i]);
                        }
                        type_info += "]";
                    }
                    type_info += ", " + std::to_string(entry.total_bytes) + " bytes";

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
