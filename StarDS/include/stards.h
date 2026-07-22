#pragma once 

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include <chrono>
#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <future>
#include <optional>
#include <ctime>
#include <tuple>
#ifndef _WIN32
// POSIX-only headers, needed solely for the guarded fsync() durability block in
// flush() (see the #ifndef _WIN32 block later in this file). MSVC has no
// <unistd.h>, so keep these out of the Windows translation unit entirely.
#include <fcntl.h>
#include <unistd.h>
#endif


#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

#ifdef ENABLE_LZ4
#include <lz4.h>
#endif

// System headers for the optional CURL (HTTP) and S3 (OpenSSL) features. These
// MUST be included at file scope — NOT inside `namespace star` further down —
// because on Windows <curl/curl.h> transitively pulls in <winsock2.h> -> <windows.h>,
// which drags in the SSE intrinsic headers (<xmmintrin.h> etc.). If that happens
// inside a namespace, types like __m128d become star::__m128d and later break
// libstdc++'s <random> SSE path ("__m128i does not name a type"). Hoisting them
// here keeps every third-party declaration at global scope.
//
// On Windows <windef.h> also #defines function-like min/max macros that clash with
// the many std::min/std::max calls in this header; NOMINMAX suppresses them and
// WIN32_LEAN_AND_MEAN trims the rest of the rarely-used surface. (The wingdi.h
// `ERROR` macro is avoided separately by prefixing the logger enum — STARDS_ERROR.)
#ifdef ENABLE_CURL
#ifdef _WIN32
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#endif
#include <curl/curl.h>
#endif

#ifdef ENABLE_S3
#include <openssl/sha.h>
#include <openssl/hmac.h>
#ifndef _WIN32
#include <dirent.h>
#endif
#endif

// Library version constants.
//
// star.h is header-only and must compile standalone even when copy-pasted
// elsewhere, so the canonical version is hardcoded HERE. The repo-root VERSION
// file mirrors these numbers (CMake reads VERSION to stamp the build and the
// Python package); keep the three values below in sync with /VERSION.
#ifndef STAR_VERSION_MAJOR
#define STAR_VERSION_MAJOR 1
#endif
#ifndef STAR_VERSION_MINOR
#define STAR_VERSION_MINOR 0
#endif
#ifndef STAR_VERSION_PATCH
#define STAR_VERSION_PATCH 0
#endif

// Stringify helpers so the version string is derived from the macros above
// (single source of truth within this header — no separate literal to drift).
#define STAR_STRINGIFY_IMPL(x) #x
#define STAR_STRINGIFY(x) STAR_STRINGIFY_IMPL(x)
#define STAR_VERSION_STRING \
    STAR_STRINGIFY(STAR_VERSION_MAJOR) "." \
    STAR_STRINGIFY(STAR_VERSION_MINOR) "." \
    STAR_STRINGIFY(STAR_VERSION_PATCH)

//==============================================================================
// STAR Namespace
//==============================================================================

namespace star {

inline const std::string PROJECT_NAME = "STARDS " STAR_VERSION_STRING;
inline const char* MAGIC_STRING = "STARDS";
inline const size_t MAGIC_STRING_LENGTH = 6;

//==============================================================================
// Fixed-width serialization helpers
//==============================================================================
//
// The on-disk format uses fixed-width little-endian integers so a file written
// on one platform reads identically on another. In-memory sizes/offsets are
// C++ `size_t` (width varies: 8 bytes on LP64, 4 bytes on ILP32/Win32), so
// serializing a raw `size_t` would emit a platform-dependent number of bytes and
// break cross-platform portability. These helpers pin every persisted count,
// offset, and dimension to a fixed 64-bit (or 32-bit) wire type regardless of
// the host `size_t`.
//
// The format is defined as little-endian (see the format spec), so on a
// big-endian host the bytes must be swapped. STARDS_IS_BIG_ENDIAN detects this;
// on the overwhelmingly common little-endian targets these helpers are a plain
// fixed-width copy with no byte swapping.

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && \
    __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define STARDS_IS_BIG_ENDIAN 1
#else
#define STARDS_IS_BIG_ENDIAN 0
#endif

// Write `value` as `N` little-endian bytes to the stream.
template <typename UInt>
inline void write_le(std::ostream& os, UInt value) {
    static_assert(std::is_unsigned<UInt>::value, "write_le requires an unsigned type");
    unsigned char buf[sizeof(UInt)];
    for (size_t i = 0; i < sizeof(UInt); ++i) {
        buf[i] = static_cast<unsigned char>((value >> (8 * i)) & 0xFF);
    }
    os.write(reinterpret_cast<const char*>(buf), sizeof(UInt));
}

// Read `N` little-endian bytes from the stream into an unsigned integer.
template <typename UInt>
inline UInt read_le(std::istream& is) {
    static_assert(std::is_unsigned<UInt>::value, "read_le requires an unsigned type");
    unsigned char buf[sizeof(UInt)];
    is.read(reinterpret_cast<char*>(buf), sizeof(UInt));
    UInt value = 0;
    for (size_t i = 0; i < sizeof(UInt); ++i) {
        value |= static_cast<UInt>(buf[i]) << (8 * i);
    }
    return value;
}

// Convenience wrappers for the two persisted integer widths. Callers pass any
// integer (e.g. a size_t count) and it is narrowed to the fixed wire width.
inline void write_u64(std::ostream& os, uint64_t v) { write_le<uint64_t>(os, v); }
inline void write_u32(std::ostream& os, uint32_t v) { write_le<uint32_t>(os, v); }
inline void write_u16(std::ostream& os, uint16_t v) { write_le<uint16_t>(os, v); }
inline void write_u8(std::ostream& os, uint8_t v)   { os.write(reinterpret_cast<const char*>(&v), 1); }

inline uint64_t read_u64(std::istream& is) { return read_le<uint64_t>(is); }
inline uint32_t read_u32(std::istream& is) { return read_le<uint32_t>(is); }
inline uint16_t read_u16(std::istream& is) { return read_le<uint16_t>(is); }
inline uint8_t  read_u8(std::istream& is)  { unsigned char b = 0; is.read(reinterpret_cast<char*>(&b), 1); return b; }

//==============================================================================
// Threading Configuration (Global, Runtime)
//==============================================================================

inline size_t g_num_threads = 0;  // 0 = auto-detect
inline size_t g_min_blocks_for_threading = 4;
inline size_t g_min_bytes_for_threading = 256 * 1024;  // 256KB

/**
 * @brief Set number of threads for parallel operations (all datasets)
 * @param num_threads Number of threads (0 = auto-detect, 1 = single-threaded)
 */
inline void setNumThreads(size_t num_threads) {
    g_num_threads = num_threads;
}

/**
 * @brief Set minimum blocks threshold for using threading
 * @param min_blocks Minimum number of blocks (default: 4)
 */
inline void setMinBlocksForThreading(size_t min_blocks) {
    g_min_blocks_for_threading = min_blocks;
}

/**
 * @brief Set minimum data size threshold for using threading
 * @param min_bytes Minimum data size in bytes (default: 256KB)
 */
inline void setMinBytesForThreading(size_t min_bytes) {
    g_min_bytes_for_threading = min_bytes;
}

/**
 * @brief Get current thread count setting
 */
inline size_t getNumThreads() {
    return g_num_threads;
}

//==============================================================================
// Network Request Counter (Global, Runtime) — observability for remote reads
//==============================================================================

// Incremented on every network round trip (each curl_easy_perform: HEAD, GET,
// PUT, ...). Local-file reads never touch it. Lets tests and callers verify that
// opening/reading a remote .stards issues the expected (small) number of
// requests, and catch regressions that reintroduce per-read connections/HEADs.
inline std::atomic<uint64_t> g_network_request_count{0};

// Total network requests issued this process (across all datasets/threads).
inline uint64_t getNetworkRequestCount() {
    return g_network_request_count.load(std::memory_order_relaxed);
}

// Reset the counter (e.g. at the start of a test or a timed section).
inline void resetNetworkRequestCount() {
    g_network_request_count.store(0, std::memory_order_relaxed);
}
// NOTE: the curl-performing wrapper star_curl_perform() is defined just after
// <curl/curl.h> is included (further down), since it needs the CURL type.

//==============================================================================
// File Format Structures
//==============================================================================

/**
 * @brief File header structure (31 bytes fixed size)
 *
 * Layout:
 * - magic[6]: "STARDS" magic string (6 bytes)
 * - format_version: File format version (1 byte, currently 1)
 * - header_size: Total size of header + index section (8 bytes)
 * - entry_count: Number of entries in the index (8 bytes)
 * - layer_count: Number of named layers, excluding __base__ (4 bytes)
 * - key_registry_count: Number of keys in the global key registry (4 bytes)
 *
 * Note: Software/library version is NOT stored in the file, only the format version.
 */
struct FileHeader {
    char magic[MAGIC_STRING_LENGTH] = {'S','T','A','R','D','S'};
    uint8_t format_version = 1;  // Format v1 with per-layer metadata blocks
    uint64_t header_size = 0;
    uint64_t entry_count = 0;
    uint32_t layer_count = 0;
    uint32_t key_registry_count = 0;  // Number of keys in global registry

    /**
     * @brief Check if magic string is valid
     */
    bool isValid() const {
        return std::memcmp(magic, MAGIC_STRING, MAGIC_STRING_LENGTH) == 0;
    }

    /**
     * @brief Get format version string
     */
    std::string getVersionString() const {
        return "Format v" + std::to_string(format_version);
    }

    /**
     * @brief Get fixed size of FileHeader struct
     */
    static constexpr size_t size() {
        return MAGIC_STRING_LENGTH + sizeof(uint8_t) + sizeof(uint64_t) * 2 + sizeof(uint32_t) * 2;  // 31 bytes (v1)
    }

    /**
     * @brief Write header to stream
     */
    void write(std::ostream& os) const {
        os.write(magic, MAGIC_STRING_LENGTH);
        write_u8(os, format_version);
        write_u64(os, header_size);
        write_u64(os, entry_count);
        write_u32(os, layer_count);
        write_u32(os, key_registry_count);
    }

    /**
     * @brief Read header from stream
     */
    void read(std::istream& is) {
        is.read(magic, MAGIC_STRING_LENGTH);
        format_version = read_u8(is);
        header_size = read_u64(is);
        entry_count = read_u64(is);
        layer_count = read_u32(is);
        key_registry_count = read_u32(is);
    }
};

/**
 * @brief Get library version string
 * @return Version string (e.g., "1.0.0")
 */
inline std::string getLibraryVersion() {
    return std::to_string(STAR_VERSION_MAJOR) + "." +
           std::to_string(STAR_VERSION_MINOR) + "." +
           std::to_string(STAR_VERSION_PATCH);
}

//==============================================================================
// Logging System
//==============================================================================

namespace logger {
    // Enumerators are STARDS_-prefixed to avoid collisions with preprocessor macros
    // from platform headers — most importantly Windows <wingdi.h>, which does
    // `#define ERROR 0` (pulled in transitively via <curl/curl.h> -> <windows.h>),
    // and `DEBUG`. The numeric values (0..4) are unchanged, so the Python LogLevel
    // shim (which passes raw ints) and any persisted level are unaffected.
    enum LogLevel { STARDS_TRACE=0, STARDS_DEBUG, STARDS_INFO, STARDS_WARN, STARDS_ERROR };
    static inline LogLevel current_log_level = STARDS_ERROR;
    
    static constexpr const char* LOG_LEVEL_STRINGS[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR"};
    
    /**
     * @brief Base implementation for logging a single value
     * @param os Output stream to write to
     * @param first Value to log
     */
    template<typename T>
    void log_impl(std::ostream& os, const T& first) {
        os << first;
    }
    
    /**
     * @brief Recursive implementation for logging multiple values
     * @param os Output stream to write to
     * @param first First value to log
     * @param args Remaining values to log
     */
    template<typename T, typename... Args>
    void log_impl(std::ostream& os, const T& first, const Args&... args) {
        os << first;
        log_impl(os, args...);
    }
    
    /**
     * @brief Internal logging function that formats and outputs log messages
     * @param level Log level of the message
     * @param line Source code line number
     * @param func Function name
     * @param args Values to log
     */
    template<typename... Args>
    void log_internal(LogLevel level, int line, const char* func, const Args&... args) {
        if (level >= current_log_level) {
            auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            
            std::tm tm_buf{};
            #ifdef _WIN32
            localtime_s(&tm_buf, &time);
            #else
            localtime_r(&time, &tm_buf);
            #endif
            std::stringstream ss;
            ss << "[" << PROJECT_NAME << "][" << LOG_LEVEL_STRINGS[level] << "]"
               << "[" << std::put_time(&tm_buf, "%Y-%m-%d %H:%M:%S") << "]"
               << "[" << func << ":" << line << "] ";
            log_impl(ss, args...);
            ss << std::endl;
            
            std::cerr << ss.str();
        }
    }
    
    /**
     * @brief Sets the current log level
     * @param level New log level to use
     */
    inline void set_log_level(LogLevel level) {
        current_log_level = level;
    }

    /**
     * @brief Gets the current log level
     * @return Current log level
     */
    inline LogLevel get_log_level() {
        return current_log_level;
    }
  }
  
  // Logging macros for different severity levels that avoid code generation if level is too low
  #define LOG_TRACE(...) \
      do { if (logger::STARDS_TRACE >= logger::current_log_level) logger::log_internal(logger::STARDS_TRACE, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_DEBUG(...) \
      do { if (logger::STARDS_DEBUG >= logger::current_log_level) logger::log_internal(logger::STARDS_DEBUG, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_INFO(...) \
      do { if (logger::STARDS_INFO >= logger::current_log_level) logger::log_internal(logger::STARDS_INFO, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_WARN(...) \
      do { if (logger::STARDS_WARN >= logger::current_log_level) logger::log_internal(logger::STARDS_WARN, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_ERROR(...) \
      do { if (logger::STARDS_ERROR >= logger::current_log_level) logger::log_internal(logger::STARDS_ERROR, __LINE__, __func__, __VA_ARGS__); } while(0)



//==============================================================================
// Architecture-Independent Data Types
//==============================================================================

enum class DataType : uint8_t {
    // Integer types (explicit bit width)
    INT8 = 0,
    INT16 = 1,
    INT32 = 2,
    INT64 = 3,
    UINT8 = 4,
    UINT16 = 5,
    UINT32 = 6,
    UINT64 = 7,

    // Floating point (IEEE 754)
    FLOAT32 = 8,
    FLOAT64 = 9,

    // String (UTF-8)
    STRING = 10,
};

enum class CompressionAlgorithm : uint8_t {
    NONE = 0,
    GZIP = 1,
    ZSTD = 2,
    LZ4 = 3,
    // Byte-shuffle prefilter + a base codec. Shuffle regroups an array of
    // fixed-width elements so all byte-0s are contiguous, then all byte-1s, etc.
    // This clusters the slowly-varying high-order bytes of numeric data (e.g.
    // float64 coordinates), which GZIP/LZ4 then compress far better. Applied only
    // to fixed-width numeric arrays; strings fall back to the base codec.
    GZIP_SHUFFLE = 4,
    LZ4_SHUFFLE = 5,
};

/**
 * @brief The underlying block codec for a (possibly shuffle-prefiltered) algorithm.
 *
 * The block (de)compressor only understands the base codecs; the shuffle variants
 * differ only in a byte-reordering prefilter applied around it.
 */
inline CompressionAlgorithm base_compression(CompressionAlgorithm c) {
    switch (c) {
        case CompressionAlgorithm::GZIP_SHUFFLE: return CompressionAlgorithm::GZIP;
        case CompressionAlgorithm::LZ4_SHUFFLE:  return CompressionAlgorithm::LZ4;
        default: return c;
    }
}

/**
 * @brief Whether a compression algorithm uses the byte-shuffle prefilter.
 */
inline bool uses_shuffle(CompressionAlgorithm c) {
    return c == CompressionAlgorithm::GZIP_SHUFFLE || c == CompressionAlgorithm::LZ4_SHUFFLE;
}

/**
 * @brief Byte-shuffle: reorder `count` elements of `elem_size` bytes so that byte
 *        plane 0 of every element comes first, then plane 1, etc.
 *
 * Splits an array-of-structs byte layout into a struct-of-byte-planes layout.
 * A no-op for elem_size <= 1. `out` must hold `count * elem_size` bytes.
 */
inline void byte_shuffle(const char* in, char* out, size_t count, size_t elem_size) {
    if (elem_size <= 1) { std::memcpy(out, in, count * elem_size); return; }
    for (size_t b = 0; b < elem_size; ++b) {
        char* dst = out + b * count;
        for (size_t i = 0; i < count; ++i) {
            dst[i] = in[i * elem_size + b];
        }
    }
}

/**
 * @brief Inverse of byte_shuffle(): reassemble byte planes back into elements.
 */
inline void byte_unshuffle(const char* in, char* out, size_t count, size_t elem_size) {
    if (elem_size <= 1) { std::memcpy(out, in, count * elem_size); return; }
    for (size_t b = 0; b < elem_size; ++b) {
        const char* src = in + b * count;
        for (size_t i = 0; i < count; ++i) {
            out[i * elem_size + b] = src[i];
        }
    }
}

/**
 * @brief Hash function for keys in global key registry
 */
inline uint64_t hash_key(const std::string& key) {
    std::hash<std::string> hasher;
    return hasher(key);
}

/**
 * @brief Global key registry using data-oriented design (Structure of Arrays)
 *
 * Stores all unique keys once with precomputed hashes for O(1) lookups.
 * Keys are referenced by uint16 indices throughout the system.
 */
struct KeyRegistry {
    std::vector<std::string> names;                          // All key names
    std::vector<uint64_t> hashes;                            // Precomputed hashes
    std::unordered_map<uint64_t, uint16_t> hash_to_index;   // O(1) hash → index
    std::unordered_map<std::string, uint16_t> name_to_index; // O(1) name → index

    /**
     * @brief Get or create a key index
     * @return uint16 index into the registry
     */
    uint16_t get_or_create(const std::string& key) {
        uint64_t hash = hash_key(key);
        // Look up by full name: name_to_index is collision-free, whereas
        // hash_to_index can alias two distinct keys that share a 64-bit hash.
        auto it = name_to_index.find(key);
        if (it != name_to_index.end()) {
            return it->second;  // Name match
        }
        // Indices are stored as uint16_t on disk; refuse to wrap past the max
        // rather than silently aliasing distinct keys to the same index.
        if (names.size() > std::numeric_limits<uint16_t>::max()) {
            throw std::runtime_error("Key registry full: cannot exceed 65536 unique keys");
        }
        uint16_t idx = static_cast<uint16_t>(names.size());
        names.push_back(key);
        hashes.push_back(hash);
        hash_to_index[hash] = idx;
        name_to_index[key] = idx;
        return idx;
    }

    /**
     * @brief Get existing key index (throws if not found)
     * @return uint16 index into the registry
     */
    uint16_t get_index(const std::string& key) const {
        auto it = name_to_index.find(key);
        if (it != name_to_index.end()) {
            return it->second;
        }
        throw std::runtime_error("Key not found in registry: " + key);
    }

    /**
     * @brief Check if key exists in registry
     */
    bool contains(const std::string& key) const {
        return name_to_index.find(key) != name_to_index.end();
    }
};

/**
 * @brief Per-layer metadata registry using data-oriented design (Structure of Arrays)
 *
 * Stores metadata about each layer's metadata block for lazy loading.
 * Each layer has its own independent STARMeta block in the file.
 */
struct LayerMetadataRegistry {
    std::vector<std::string> layer_names;                         // Layer names
    std::vector<uint64_t> block_positions;                        // File positions of STARMeta blocks
    std::vector<uint32_t> block_sizes;                            // Compressed sizes
    std::vector<CompressionAlgorithm> compressions;               // Compression algorithms
    std::vector<std::unordered_set<uint16_t>> key_indices;        // Keys in each layer (O(1) lookup)
    std::unordered_map<std::string, size_t> name_to_layer_index;  // O(1) layer name → index

    /**
     * @brief Get layer index by name
     * @return size_t index into the registry
     */
    size_t get_layer_index(const std::string& layer_name) const {
        auto it = name_to_layer_index.find(layer_name);
        if (it == name_to_layer_index.end()) {
            throw std::runtime_error("Layer not found: " + layer_name);
        }
        return it->second;
    }

    /**
     * @brief Check if layer exists
     */
    bool contains_layer(const std::string& layer_name) const {
        return name_to_layer_index.find(layer_name) != name_to_layer_index.end();
    }

    /**
     * @brief Add a new layer to the registry
     */
    size_t add_layer(const std::string& layer_name) {
        if (contains_layer(layer_name)) {
            return get_layer_index(layer_name);
        }
        size_t idx = layer_names.size();
        layer_names.push_back(layer_name);
        block_positions.push_back(0);
        block_sizes.push_back(0);
        compressions.push_back(CompressionAlgorithm::NONE);
        key_indices.push_back(std::unordered_set<uint16_t>());
        name_to_layer_index[layer_name] = idx;
        return idx;
    }
};

/**
 * @brief Get string representation of DataType
 * @param dtype DataType to convert
 * @return String representation
 */
inline const char* datatype_to_string(DataType dtype) {
    switch(dtype) {
        case DataType::INT8: return "int8";
        case DataType::INT16: return "int16";
        case DataType::INT32: return "int32";
        case DataType::INT64: return "int64";
        case DataType::UINT8: return "uint8";
        case DataType::UINT16: return "uint16";
        case DataType::UINT32: return "uint32";
        case DataType::UINT64: return "uint64";
        case DataType::FLOAT32: return "float32";
        case DataType::FLOAT64: return "float64";
        case DataType::STRING: return "string";
        default: return "unknown";
    }
}

/**
 * @brief Get element size in bytes for a DataType
 * @param dtype DataType to query
 * @return Size in bytes (0 for variable-length types like STRING)
 */
inline size_t datatype_size(DataType dtype) {
    switch(dtype) {
        case DataType::INT8:
        case DataType::UINT8: return 1;
        case DataType::INT16:
        case DataType::UINT16: return 2;
        case DataType::INT32:
        case DataType::UINT32:
        case DataType::FLOAT32: return 4;
        case DataType::INT64:
        case DataType::UINT64:
        case DataType::FLOAT64: return 8;
        case DataType::STRING: return 0; // Variable length
        default: return 0;
    }
}

//==============================================================================
// Type Mapping Between C++ and Portable Types
//==============================================================================

template<typename T>
struct TypeToDataType {
    // Default - will cause compile error for unsupported types
};

// Specializations for explicit-width types
template<> struct TypeToDataType<int8_t> {
    static constexpr DataType value = DataType::INT8;
};
template<> struct TypeToDataType<int16_t> {
    static constexpr DataType value = DataType::INT16;
};
template<> struct TypeToDataType<int32_t> {
    static constexpr DataType value = DataType::INT32;
};
template<> struct TypeToDataType<int64_t> {
    static constexpr DataType value = DataType::INT64;
};
template<> struct TypeToDataType<uint8_t> {
    static constexpr DataType value = DataType::UINT8;
};
template<> struct TypeToDataType<uint16_t> {
    static constexpr DataType value = DataType::UINT16;
};
template<> struct TypeToDataType<uint32_t> {
    static constexpr DataType value = DataType::UINT32;
};
template<> struct TypeToDataType<uint64_t> {
    static constexpr DataType value = DataType::UINT64;
};
template<> struct TypeToDataType<float> {
    static constexpr DataType value = DataType::FLOAT32;
};
template<> struct TypeToDataType<double> {
    static constexpr DataType value = DataType::FLOAT64;
};
template<> struct TypeToDataType<std::string> {
    static constexpr DataType value = DataType::STRING;
};

//==============================================================================
// Block Compression Metadata
//==============================================================================

/**
 * @brief Metadata for a single compressed block
 */
struct BlockInfo {
    uint64_t offset;              // Offset within this key's data section
    uint64_t compressed_size;     // Compressed size (may equal uncompressed if not compressed)
    uint64_t uncompressed_size;   // Original block size

    // Fixed 64-bit fields so the in-memory width matches the on-disk width on
    // every platform (including 32-bit wasm32, where size_t is 4 bytes and would
    // otherwise truncate these values).
    void write(std::ostream& os) const {
        write_u64(os, offset);
        write_u64(os, compressed_size);
        write_u64(os, uncompressed_size);
    }

    void read(std::istream& is) {
        offset = read_u64(is);
        compressed_size = read_u64(is);
        uncompressed_size = read_u64(is);
    }
};

// =============================================================================
// N-Dimensional Array Slicing 
// =============================================================================

/**
 * @brief Describes a slice along one dimension (Python-style slicing)
 * Plain struct - no methods except helpers, just data
 *
 * Usage:
 *   Slice{100, 200}        // start=100, stop=200, step=1 (default)
 *   Slice{100, 200, 2}     // start=100, stop=200, step=2 (every other)
 */
struct Slice {
    size_t start;      // Starting index (inclusive)
    size_t stop;       // Ending index (exclusive, Python-style)
    size_t step = 1;   // Step size (defaults to 1 for contiguous)

    // Convenience helper for calculating slice length
    size_t length() const {
        if (step == 0 || stop <= start) return 0;
        return (stop - start + step - 1) / step;
    }
};

inline Slice slice_all(size_t dim_size) {
    return Slice{0, dim_size, 1};
}

inline Slice slice_range(size_t start, size_t stop) {
    return Slice{start, stop, 1};
}

/**
 * @brief Complete slice specification for n-dimensional array
 */
struct SliceSpec {
    std::vector<Slice> slices;           // One per dimension
    std::vector<size_t> output_shape;    // Resulting array shape
    size_t total_elements;               // Total elements in result
    size_t element_size;                 // Bytes per element

    // Validation flags (computed once, reused)
    bool is_contiguous;                  // All slices contiguous?
    bool is_full_rows;                   // Taking complete rows?
};

/**
 * @brief Maps logical elements to physical blocks
 */
struct BlockMap {
    std::vector<size_t> block_indices;   // Which blocks to read
    std::vector<size_t> block_offsets;   // File offset for each block
    std::vector<size_t> block_sizes;     // Compressed size for each
    size_t total_compressed_bytes;       // Sum of block_sizes
    bool blocks_contiguous;              // Sequential in file?
    size_t contiguous_start_offset;      // If contiguous: start
};

/**
 * @brief Describes how to extract elements from blocks
 */
struct ExtractionPlan {
    struct ElementRange {
        size_t block_idx;                // Which block (index into BlockMap)
        size_t element_start;            // First element in block
        size_t element_count;            // Number of elements
        size_t output_offset;            // Where to copy in output
    };

    std::vector<ElementRange> ranges;    // Linear extraction plan
    size_t total_elements;
};

/**
 * @brief Index entry with block compression support and shape information
 *
 * @note On-disk portability: this descriptor and its BlockInfo entries serialize
 * their size_t fields as fixed-width little-endian uint64 values via write_u64 /
 * read_u64 (which use the shift-based write_le/read_le, independent of host width
 * and byte order). So .stards files interchange freely across platforms of any
 * pointer width or endianness, including 32-bit wasm32 (size_t == 4 bytes) — the
 * fields are narrowed to/from size_t with explicit static_casts on read. This
 * matches the documented format-v1 layout.
 */
struct IndexEntry {
    uint64_t position;                  // Position in file where data starts
    uint64_t total_bytes;               // Total bytes (all blocks + metadata)
    DataType datatype;                  // Base element type
    std::vector<size_t> shape;          // Array dimensions (empty = scalar); size_t
                                        // to interoperate with NDArray::shape() and
                                        // the in-memory slicing machinery. Each dim
                                        // is serialized as a fixed-width u64.
    CompressionAlgorithm compression;   // Compression algorithm
    uint64_t block_size;                // Uncompressed block size (0 = no blocking)
    std::vector<BlockInfo> blocks;      // Per-block metadata
    bool dirty;                         // In-memory changes not yet flushed
    // NOTE: is_metadata_block is not serialized and is never set true in the v1
    // format, so it is effectively always false; the !is_metadata_block guard in
    // is_scalar() is therefore a no-op today. Kept for source/ABI stability.
    bool is_metadata_block = false;     // (unused in v1) special metadata block container marker
    bool stored_in_metadata = false;    // True if this item is stored in the metadata block (not as separate entry)

    // Helper to check if scalar
    bool is_scalar() const {
        return shape.empty() && !is_metadata_block;
    }

    // Helper to get total number of elements
    size_t num_elements() const {
        if (shape.empty()) return 1; // scalar
        size_t total = 1;
        for (size_t dim : shape) {
            total *= dim;
        }
        return total;
    }

    // On-disk all counts/offsets/dims are fixed-width little-endian integers
    // (u64 for sizes/offsets/dims, u8 for the type + compression + flag enums) so
    // the layout is identical on 32- and 64-bit platforms. serialized_size()
    // below must mirror this byte-for-byte (calculateHeaderSize() relies on it).
    void write(std::ostream& os) const {
        write_u64(os, position);
        write_u64(os, total_bytes);
        write_u8(os, static_cast<uint8_t>(datatype));

        // Write shape (ndim = 0 means scalar)
        write_u64(os, shape.size());
        for (size_t dim : shape) {
            write_u64(os, dim);
        }

        write_u8(os, static_cast<uint8_t>(compression));
        write_u64(os, block_size);

        // Write number of blocks, then each block info
        write_u64(os, blocks.size());
        for (const auto& block : blocks) {
            block.write(os);
        }

        // Write stored_in_metadata flag
        write_u8(os, stored_in_metadata ? 1 : 0);
    }

    void read(std::istream& is) {
        position = read_u64(is);
        total_bytes = read_u64(is);
        datatype = static_cast<DataType>(read_u8(is));
        LOG_TRACE("IndexEntry::read - position=", position, ", total_bytes=", total_bytes, ", datatype=", (int)datatype);

        // Read shape (ndim = 0 means scalar)
        uint64_t ndim = read_u64(is);
        LOG_TRACE("IndexEntry::read - ndim=", ndim);
        if (ndim > 0) {
            if (ndim > 100) {
                LOG_ERROR("SUSPICIOUS: ndim=", ndim, " is too large!");
                throw std::runtime_error("Invalid ndim value: " + std::to_string(ndim));
            }
            shape.resize(static_cast<size_t>(ndim));
            for (auto& dim : shape) {
                dim = read_u64(is);
            }
        } else {
            shape.clear(); // Explicitly make empty for scalar
        }

        compression = static_cast<CompressionAlgorithm>(read_u8(is));
        block_size = read_u64(is);
        LOG_TRACE("IndexEntry::read - compression=", (int)compression, ", block_size=", block_size);

        // Read number of blocks
        uint64_t num_blocks = read_u64(is);
        LOG_TRACE("IndexEntry::read - num_blocks=", num_blocks);

        if (num_blocks > 10000) {
            LOG_ERROR("SUSPICIOUS: num_blocks=", num_blocks, " is too large!");
            throw std::runtime_error("Invalid num_blocks value: " + std::to_string(num_blocks));
        }

        // Read each block info
        blocks.resize(static_cast<size_t>(num_blocks));
        for (auto& block : blocks) {
            block.read(is);
        }

        // Read stored_in_metadata flag
        stored_in_metadata = (read_u8(is) != 0);
        LOG_TRACE("IndexEntry::read - complete, stored_in_metadata=", stored_in_metadata);
    }

    size_t serialized_size() const {
        // Must match write() exactly: position(u64) + total_bytes(u64) +
        // datatype(u8) + ndim(u64) + shape(u64*ndim) + compression(u8) +
        // block_size(u64) + num_blocks(u64) + blocks(u64*3 each) + stored_flag(u8).
        return sizeof(uint64_t) + sizeof(uint64_t) + sizeof(uint8_t) +
               sizeof(uint64_t) + (shape.size() * sizeof(uint64_t)) +
               sizeof(uint8_t) + sizeof(uint64_t) + sizeof(uint64_t) +
               (blocks.size() * (sizeof(uint64_t) * 3)) +
               sizeof(uint8_t);  // stored_in_metadata flag
    }

    // Helper to print metadata
    void print() const {
        std::cout << "  Type: " << datatype_to_string(datatype);
        if (shape.empty()) {
            std::cout << " (scalar)";
        } else {
            std::cout << "[";
            for (size_t i = 0; i < shape.size(); ++i) {
                if (i > 0) std::cout << ", ";
                std::cout << shape[i];
            }
            std::cout << "]";
        }
        std::cout << std::endl;
        std::cout << "  Elements: " << num_elements() << std::endl;
        std::cout << "  Compression: " << (int)compression << std::endl;
        std::cout << "  Block size: " << block_size << std::endl;
        std::cout << "  Num blocks: " << blocks.size() << std::endl;
        std::cout << "  Total bytes: " << total_bytes << std::endl;
    }
};


//==============================================================================
// HTTP Stream Implementation
//==============================================================================


#ifdef ENABLE_CURL
// NOTE: <curl/curl.h> is included at FILE SCOPE near the top of this header, not
// here — including it inside `namespace star` namespaces the SSE intrinsic types
// it transitively pulls in on Windows. These std headers are already included at
// the top too; kept here only for standalone clarity (include guards make them
// no-ops).
#include <string>
#include <vector>
#include <streambuf>
#include <iostream>
#include <memory>

// Perform a curl request and count it (see g_network_request_count above). Every
// network round trip in this header MUST go through this wrapper so the counter
// stays authoritative for observability/tests.
inline CURLcode star_curl_perform(CURL* handle) {
    g_network_request_count.fetch_add(1, std::memory_order_relaxed);
    return curl_easy_perform(handle);
}

/**
 * @brief Custom streambuf implementation for HTTP requests with range support
 */
class HttpStreamBuf : public std::streambuf {
private:
    CURL* m_curl;
    std::string m_url;
    std::vector<char> m_buffer;
    size_t m_position;
    size_t m_content_length;
    
    static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        std::vector<char>* buffer = static_cast<std::vector<char>*>(userdata);
        size_t bytes = size * nmemb;
        buffer->insert(buffer->end(), ptr, ptr + bytes);
        return bytes;
    }
    
    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t bytes = size * nitems;
        std::string header(buffer, bytes);
        HttpStreamBuf* stream = static_cast<HttpStreamBuf*>(userdata);
        
        // Extract Content-Length if present
        if (header.find("Content-Length:") == 0 || header.find("content-length:") == 0) {
            std::string length = header.substr(header.find(":") + 1);
            // std::stoul throws on malformed/overflowing input; a C++ exception
            // must never propagate out of this libcurl C callback (UB). Parse
            // defensively and leave m_content_length at 0 on error.
            try {
                stream->m_content_length = std::stoul(length);
            } catch (const std::exception&) {
                // malformed Content-Length header; leave m_content_length as-is
            }
        }
        return bytes;
    }
    
    bool fetchRange(size_t start, size_t end) {
        // Discard any bytes from a previous range. WriteCallback appends to
        // m_buffer, so without this a fetch after a seek would serve the old
        // range's data (stale reads / corruption).
        m_buffer.clear();

        // Set range header
        std::string range = "Range: bytes=" + std::to_string(start) + "-" + std::to_string(end);
        LOG_TRACE("Fetching range: ", range);
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, range.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        
        // Perform request
        CURLcode res = star_curl_perform(m_curl);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            return false;
        }
        
        // Set buffer pointers
        if (!m_buffer.empty()) {
            setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + m_buffer.size());
            return true;
        }
        return false;
    }
    
protected:
    virtual int_type underflow() override {
        LOG_TRACE("Calling Underflow");
        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }
        
        // Calculate next range to fetch
        size_t start = m_position;
        size_t end = m_content_length - 1;
        
        if (start >= m_content_length) {
            return traits_type::eof();
        }
        
        if (!fetchRange(start, end)) {
            return traits_type::eof();
        }
        
        m_position = end + 1;
        return traits_type::to_int_type(*gptr());
    }
    
    // Override seekpos to support seeking to absolute positions
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) override {
        if (which & std::ios_base::in) {
            // Check if position is valid (allow seeking to end)
            if (pos >= 0 && pos <= static_cast<pos_type>(m_content_length)) {
                // Clear current buffer
                setg(nullptr, nullptr, nullptr);
                m_position = static_cast<size_t>(pos);
                LOG_TRACE("HTTP seek to position ", pos, " (content_length=", m_content_length, ")");
                return pos;
            } else {
                LOG_ERROR("HTTP seek failed: pos=", pos, " content_length=", m_content_length);
            }
        }
        return pos_type(off_type(-1));
    }
    
    // Override seekoff to support seeking relative to current position or start/end
    virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, 
                            std::ios_base::openmode which = std::ios_base::in) override {
        if (which & std::ios_base::in) {
            pos_type new_pos;
            
            // Calculate new position based on direction
            switch (dir) {
                case std::ios_base::beg:
                    new_pos = off;
                    break;
                case std::ios_base::cur:
                    // If we have a buffer, adjust for current get position
                    if (gptr() && egptr()) {
                        new_pos = m_position - (egptr() - gptr()) + off;
                    } else {
                        new_pos = m_position + off;
                    }
                    break;
                case std::ios_base::end:
                    new_pos = m_content_length + off;
                    break;
                default:
                    return pos_type(off_type(-1));
            }
            
            // Check if new position is valid (allow seeking to end)
            if (new_pos >= 0 && new_pos <= static_cast<pos_type>(m_content_length)) {
                // Clear current buffer
                setg(nullptr, nullptr, nullptr);
                m_position = static_cast<size_t>(new_pos);
                LOG_TRACE("HTTP seekoff to position ", new_pos, " (content_length=", m_content_length, ")");
                return new_pos;
            } else {
                LOG_ERROR("HTTP seekoff failed: new_pos=", new_pos, " content_length=", m_content_length);
            }
        }
        return pos_type(off_type(-1));
    }
    
public:
    HttpStreamBuf(const std::string& url) 
        : m_url(url), m_position(0), m_content_length(0) {
        m_buffer.reserve(8192);
        
        // Initialize curl
        m_curl = curl_easy_init();
        if (!m_curl) {
            throw std::runtime_error("Failed to initialize curl");
        }
        
        // Set basic options
        curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_buffer);
        curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
        curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
        
        // Make HEAD request to get content length
        curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);
        CURLcode res = star_curl_perform(m_curl);
        curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);

        if (res != CURLE_OK) {
            LOG_ERROR("Failed to perform HEAD request: ", curl_easy_strerror(res));
        } else {
            LOG_TRACE("HTTP stream initialized, content_length=", m_content_length);
        }

        if (m_content_length == 0) {
            LOG_WARN("Content length is 0 for URL: ", m_url);
        }
    }
    
    // Get the total size of the remote file
    size_t size() const {
        return m_content_length;
    }
    
    ~HttpStreamBuf() {
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
    }
};

/**
 * @brief Input stream for HTTP resources with range request support
 */
class HttpStream : public std::istream {
private:
    HttpStreamBuf m_streambuf;
    
public:
    HttpStream(const std::string& url) 
        : std::istream(nullptr), m_streambuf(url) {
        rdbuf(&m_streambuf);
    }
    
    // Get the total size of the remote file
    size_t size() const {
        return m_streambuf.size();
    }
};
#endif // ENABLE_CURL


//==============================================================================
// File Modes and Path Information
//==============================================================================


enum class FileMode {
    READ_WRITE,  // Default: Allow reads and writes
    READ_ONLY    // Read-only: Prevent flush() modifications to source
};

// Parse string mode to FileMode enum
inline FileMode parseModeString(const std::string& mode_str) {
    if (mode_str == "r" || mode_str == "read") {
        return FileMode::READ_ONLY;
    } else if (mode_str == "w" || mode_str == "rw" || mode_str == "a" || mode_str == "write") {
        return FileMode::READ_WRITE;
    } else {
        throw std::runtime_error("Invalid mode string: " + mode_str + ". Use 'r', 'w', 'rw', or 'a'.");
    }
}

// File path information after parsing
struct FilePathInfo {
    // MEMORY is an in-process source/sink (no path): the whole .stards image
    // lives in a byte buffer. Used by StarDataset::open_bytes()/write_bytes().
    enum Type { LOCAL, HTTP, S3, MEMORY };
    Type type;
    std::string path;      // For LOCAL/HTTP
    std::string bucket;    // For S3
    std::string key;       // For S3
    std::string region;    // For S3
};

/**
 * @brief S3 endpoint resolution (default AWS, or an override for S3-compatible
 *        services such as MinIO and for local testing).
 *
 * Controlled by environment variables (GDAL-compatible names):
 *   - AWS_S3_ENDPOINT      host[:port] to use instead of s3.<region>.amazonaws.com
 *   - AWS_VIRTUAL_HOSTING  "FALSE"/"NO" -> path-style (endpoint/bucket/key);
 *                          otherwise virtual-hosted (bucket.endpoint/key)
 *   - AWS_HTTPS            "NO"/"FALSE" -> http:// scheme (default https://)
 *
 * With none set, behavior is byte-identical to the historical
 * "https://<bucket>.s3.<region>.amazonaws.com/<key>" (virtual-hosted, https).
 *
 * IMPORTANT: the SigV4 `host` header must equal the actual connection host
 * (including any :port) AND the signed canonical URI must equal the URL path, so
 * host(), url(), and canonical_uri() are all derived here to stay consistent.
 */
struct S3EndpointConfig {
    std::string endpoint;      // empty => AWS default (s3.<region>.amazonaws.com)
    bool path_style = false;   // true => endpoint/bucket/key
    bool use_https = true;

    static bool env_is_false(const char* name) {
        const char* v = std::getenv(name);
        if (!v) return false;
        std::string s(v);
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s == "false" || s == "no" || s == "0" || s == "off";
    }

    static S3EndpointConfig resolve() {
        S3EndpointConfig cfg;
        const char* ep = std::getenv("AWS_S3_ENDPOINT");
        if (ep) cfg.endpoint = ep;
        // A custom endpoint defaults to path-style (simplest to route); the
        // AWS_VIRTUAL_HOSTING flag can force either mode explicitly.
        cfg.path_style = env_is_false("AWS_VIRTUAL_HOSTING") || !cfg.endpoint.empty();
        if (const char* vh = std::getenv("AWS_VIRTUAL_HOSTING")) {
            std::string s(vh);
            std::transform(s.begin(), s.end(), s.begin(), ::tolower);
            if (s == "true" || s == "yes" || s == "1" || s == "on") cfg.path_style = false;
        }
        cfg.use_https = !env_is_false("AWS_HTTPS");
        return cfg;
    }

    // Host header value (no scheme), including :port if the endpoint has one.
    std::string host(const std::string& bucket, const std::string& region) const {
        std::string base = endpoint.empty()
            ? ("s3." + region + ".amazonaws.com")
            : endpoint;
        return path_style ? base : (bucket + "." + base);
    }

    // Full request URL. `encoded_key` must already be urlEncode()'d.
    std::string url(const std::string& bucket, const std::string& region,
                    const std::string& encoded_key) const {
        std::string scheme = use_https ? "https://" : "http://";
        std::string h = host(bucket, region);
        if (path_style) {
            return scheme + h + "/" + bucket + "/" + encoded_key;
        }
        return scheme + h + "/" + encoded_key;
    }

    // The SigV4 canonical URI (path portion of the URL). For path-style the
    // bucket is part of the path and MUST be signed; for virtual-hosted it is in
    // the host, so only the key is in the path.
    std::string canonical_uri(const std::string& bucket,
                              const std::string& encoded_key) const {
        return path_style ? ("/" + bucket + "/" + encoded_key)
                          : ("/" + encoded_key);
    }
};

//==============================================================================
// S3 Support - AWS Authentication and Streaming
//==============================================================================

#ifdef ENABLE_S3
// NOTE: the third-party <openssl/*> and POSIX <dirent.h> headers are included at
// FILE SCOPE near the top of this header, not here — see the hoisting note there
// (same Windows SSE-in-namespace hazard as curl). The std headers below are also
// already included at the top; kept for standalone clarity (include-guarded no-ops).
#include <optional>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <ctime>
#include <tuple>

/**
 * @brief AWS Signature Version 4 signer
 *
 * Implements AWS SigV4 authentication for S3 requests using OpenSSL.
 * This provides authentication without requiring the full AWS SDK.
 */
class AWSV4Signer {
private:
    std::string m_access_key;
    std::string m_secret_key;
    std::string m_session_token;
    std::string m_region;

    friend class S3Writer;  // Allow S3Writer to access m_session_token

    // SHA256 hash using OpenSSL
    static std::string sha256(const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), hash);
        return hexEncode(hash, SHA256_DIGEST_LENGTH);
    }

    // HMAC-SHA256 using OpenSSL
    static std::string hmacSha256(const std::string& key, const std::string& data) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        unsigned int len = SHA256_DIGEST_LENGTH;

        HMAC(EVP_sha256(),
             key.c_str(), static_cast<int>(key.length()),
             reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
             hash, &len);

        return std::string(reinterpret_cast<char*>(hash), len);
    }

    // Create canonical headers string
    std::string createCanonicalHeaders(const std::map<std::string, std::string>& headers) const {
        std::ostringstream canonical;
        for (const auto& [key, value] : headers) {
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            canonical << lowerKey << ":" << value << "\n";
        }
        return canonical.str();
    }

    // Create signed headers string
    std::string createSignedHeaders(const std::map<std::string, std::string>& headers) const {
        std::ostringstream signed_hdrs;
        bool first = true;
        for (const auto& [key, _] : headers) {
            if (!first) signed_hdrs << ";";
            std::string lowerKey = key;
            std::transform(lowerKey.begin(), lowerKey.end(), lowerKey.begin(), ::tolower);
            signed_hdrs << lowerKey;
            first = false;
        }
        return signed_hdrs.str();
    }

public:
    AWSV4Signer(const std::string& access_key,
                const std::string& secret_key,
                const std::string& region,
                const std::string& session_token = "")
        : m_access_key(access_key)
        , m_secret_key(secret_key)
        , m_session_token(session_token)
        , m_region(region) {}

    /**
     * @brief Convert binary data to hex string (public utility)
     */
    static std::string hexEncode(const unsigned char* data, size_t len) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (size_t i = 0; i < len; ++i) {
            oss << std::setw(2) << static_cast<int>(data[i]);
        }
        return oss.str();
    }

    // URL encode an S3 object key (slashes preserved). Must match the encoding
    // used to build the canonical URI in signRequest(), or the signature won't
    // match the request URL.
    static std::string urlEncode(const std::string& value) {
        std::ostringstream escaped;
        escaped << std::hex << std::setfill('0');

        for (char c : value) {
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                escaped << c;
            } else if (c == '/') {
                escaped << c;  // Don't encode slashes in S3 keys
            } else {
                escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            }
        }

        return escaped.str();
    }

    /**
     * @brief Sign an S3 request using AWS Signature Version 4
     *
     * @param method HTTP method (GET, PUT, etc.)
     * @param bucket S3 bucket name
     * @param key S3 object key
     * @param content_hash SHA256 hash of request body (hex-encoded)
     * @param headers Request headers (must include host and x-amz-date)
     * @return Authorization header value
     */
    std::string signRequest(const std::string& method,
                           const std::string& bucket,
                           const std::string& key,
                           const std::string& content_hash,
                           const std::map<std::string, std::string>& headers) const {
        // Create canonical request. The canonical URI must match the URL path
        // curl actually requests: path-style includes the bucket, virtual-hosted
        // does not. Derive both from the same S3EndpointConfig.
        std::string canonical_uri =
            S3EndpointConfig::resolve().canonical_uri(bucket, urlEncode(key));
        std::string canonical_query = "";
        std::string canonical_headers = createCanonicalHeaders(headers);
        std::string signed_headers = createSignedHeaders(headers);

        std::ostringstream canonical_request;
        canonical_request << method << "\n"
                         << canonical_uri << "\n"
                         << canonical_query << "\n"
                         << canonical_headers << "\n"
                         << signed_headers << "\n"
                         << content_hash;

        std::string canonical_request_str = canonical_request.str();
        std::string canonical_request_hash = sha256(canonical_request_str);

        // Create string to sign
        std::string amz_date = headers.at("x-amz-date");
        std::string date_stamp = amz_date.substr(0, 8);  // YYYYMMDD
        std::string credential_scope = date_stamp + "/" + m_region + "/s3/aws4_request";

        std::ostringstream string_to_sign;
        string_to_sign << "AWS4-HMAC-SHA256\n"
                       << amz_date << "\n"
                       << credential_scope << "\n"
                       << canonical_request_hash;

        // Calculate signature
        std::string k_date = hmacSha256("AWS4" + m_secret_key, date_stamp);
        std::string k_region = hmacSha256(k_date, m_region);
        std::string k_service = hmacSha256(k_region, "s3");
        std::string k_signing = hmacSha256(k_service, "aws4_request");
        std::string signature = hmacSha256(k_signing, string_to_sign.str());
        std::string signature_hex = hexEncode(
            reinterpret_cast<const unsigned char*>(signature.c_str()),
            signature.length()
        );

        // Create authorization header
        std::ostringstream authorization;
        authorization << "AWS4-HMAC-SHA256 Credential=" << m_access_key << "/" << credential_scope
                     << ", SignedHeaders=" << signed_headers
                     << ", Signature=" << signature_hex;

        return authorization.str();
    }

    const std::string& getSessionToken() const { return m_session_token; }
};

/**
 * @brief AWS Configuration file parser
 *
 * Parses INI-style configuration files like ~/.aws/config and ~/.aws/credentials
 */
class AWSConfigParser {
private:
    std::map<std::string, std::map<std::string, std::string>> m_profiles;

    static std::string getHomeDir() {
        const char* home = std::getenv("HOME");
        if (!home) {
            home = std::getenv("USERPROFILE");  // Windows
        }
        return home ? std::string(home) : "";
    }

    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\r\n");
        return str.substr(start, end - start + 1);
    }

public:
    AWSConfigParser(const std::string& config_file) {
        std::ifstream file(config_file);
        if (!file.is_open()) {
            return;  // File doesn't exist or can't be read
        }

        std::string current_profile;
        std::string line;

        while (std::getline(file, line)) {
            line = trim(line);

            // Skip empty lines and comments
            if (line.empty() || line[0] == '#' || line[0] == ';') {
                continue;
            }

            // Profile section
            if (line[0] == '[' && line.back() == ']') {
                std::string profile = line.substr(1, line.length() - 2);
                profile = trim(profile);

                // Handle [profile name] vs [name] syntax
                if (profile.substr(0, 8) == "profile ") {
                    current_profile = profile.substr(8);
                } else {
                    current_profile = profile;
                }
                current_profile = trim(current_profile);
                continue;
            }

            // Key-value pair
            size_t eq_pos = line.find('=');
            if (eq_pos != std::string::npos && !current_profile.empty()) {
                std::string key = trim(line.substr(0, eq_pos));
                std::string value = trim(line.substr(eq_pos + 1));
                m_profiles[current_profile][key] = value;
            }
        }
    }

    bool hasProfile(const std::string& profile) const {
        return m_profiles.find(profile) != m_profiles.end();
    }

    std::string getValue(const std::string& profile, const std::string& key,
                        const std::string& default_value = "") const {
        auto profile_it = m_profiles.find(profile);
        if (profile_it == m_profiles.end()) {
            return default_value;
        }

        auto key_it = profile_it->second.find(key);
        if (key_it == profile_it->second.end()) {
            return default_value;
        }

        return key_it->second;
    }

    static std::string getConfigPath() {
        std::string home = getHomeDir();
        if (home.empty()) return "";
        return home + "/.aws/config";
    }

    static std::string getCredentialsPath() {
        std::string home = getHomeDir();
        if (home.empty()) return "";
        return home + "/.aws/credentials";
    }
};

/**
 * @brief AWS SSO Token Cache reader
 *
 * Reads and manages SSO tokens cached by AWS CLI
 */
class AWSTokenCache {
private:
    struct SSOToken {
        std::string access_token;
        std::string expires_at;  // ISO 8601 timestamp
        std::string region;
        std::string start_url;

        bool isExpired() const {
            // Simple expiration check - compare timestamps
            // Format: 2024-01-15T12:00:00UTC
            auto now = std::chrono::system_clock::now();
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);

            // Parse expires_at (simplified parsing)
            struct tm tm = {};
            std::istringstream ss(expires_at);
            ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
            // expires_at is UTC; timegm interprets tm as UTC (mktime would use local
            // time). MSVC has no timegm — its UTC equivalent is _mkgmtime.
            #ifdef _WIN32
            std::time_t expires_time = _mkgmtime(&tm);
            #else
            std::time_t expires_time = timegm(&tm);
            #endif

            return now_time >= expires_time;
        }
    };

    static std::string getHomeDir() {
        const char* home = std::getenv("HOME");
        if (!home) {
            home = std::getenv("USERPROFILE");  // Windows
        }
        return home ? std::string(home) : "";
    }

    static std::string getSSOCacheDir() {
        std::string home = getHomeDir();
        if (home.empty()) return "";
        return home + "/.aws/sso/cache";
    }

    // Lowercase-hex SHA1 of `input`, matching AWS CLI / GDAL cache-key hashing.
    // The AWS CLI names each SSO token cache file sha1hex(key).json, where key is
    // the sso-session name (if the profile uses an sso-session block) else the
    // sso_start_url. Reused here so we can open the exact file directly (works on
    // every platform, no directory scan needed). See GDAL port/cpl_aws.cpp.
    static std::string sha1LowerHex(const std::string& input) {
        unsigned char digest[SHA_DIGEST_LENGTH];
        SHA1(reinterpret_cast<const unsigned char*>(input.data()), input.size(), digest);
        return AWSV4Signer::hexEncode(digest, SHA_DIGEST_LENGTH);
    }

    // Parse a cached token JSON blob and return it if it matches `start_url` and is
    // unexpired. Shared by both the direct-filename and directory-scan paths.
    static std::optional<SSOToken> parseTokenJson(const std::string& json_content,
                                                  const std::string& start_url) {
        std::string cached_start_url = extractJsonValue(json_content, "startUrl");
        if (cached_start_url != start_url) {
            return std::nullopt;
        }
        SSOToken token;
        token.access_token = extractJsonValue(json_content, "accessToken");
        token.expires_at = extractJsonValue(json_content, "expiresAt");
        token.region = extractJsonValue(json_content, "region");
        token.start_url = cached_start_url;
        if (!token.access_token.empty() && !token.isExpired()) {
            return token;
        }
        return std::nullopt;  // Found but expired/invalid
    }

    // Simple JSON value extractor (for known structure)
    static std::string extractJsonValue(const std::string& json, const std::string& key) {
        std::string search = "\"" + key + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";

        pos = json.find(":", pos);
        if (pos == std::string::npos) return "";

        pos = json.find("\"", pos);
        if (pos == std::string::npos) return "";

        size_t start = pos + 1;
        size_t end = json.find("\"", start);
        if (end == std::string::npos) return "";

        return json.substr(start, end - start);
    }

public:
    /**
     * @brief Find and read SSO token for a given start URL
     *
     * @param start_url   The SSO start URL from AWS config
     * @param sso_session The sso-session name (if the profile uses one). When
     *                    non-empty it is the cache-key the AWS CLI hashes; otherwise
     *                    the start_url is used. Defaults to "" for compatibility.
     * @return SSOToken if found and valid, empty optional otherwise
     */
    static std::optional<SSOToken> readToken(const std::string& start_url,
                                             const std::string& sso_session = "") {
        std::string cache_dir = getSSOCacheDir();
        if (cache_dir.empty()) {
            return std::nullopt;
        }

        // Primary path (all platforms, incl. Windows): the AWS CLI names each token
        // file sha1hex(key).json, where key is the sso-session name if present else
        // the start_url (matches GDAL). Open that exact file directly — no directory
        // iteration, so this is fully portable and is what enables SSO on Windows.
        {
            std::string key = sso_session.empty() ? start_url : sso_session;
            std::string filepath = cache_dir + "/" + sha1LowerHex(key) + ".json";
            std::ifstream file(filepath);
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                auto token = parseTokenJson(buffer.str(), start_url);
                if (token) {
                    return token;
                }
                // File exists but didn't match/was expired: fall through to the scan
                // below (Unix) as a safety net for non-standard cache layouts.
            }
        }

        // Fallback path (Unix only, UNCHANGED behavior): scan every *.json in the
        // cache dir and match by startUrl. Kept as a safety net so no token the
        // legacy scan would have found is lost. On Windows there is no <dirent.h>,
        // so this compiles away and the primary path above is authoritative.
        #ifndef _WIN32
        DIR* dir = opendir(cache_dir.c_str());
        if (!dir) {
            return std::nullopt;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.length() < 5 || filename.substr(filename.length() - 5) != ".json") {
                continue;
            }

            std::string filepath = cache_dir + "/" + filename;
            std::ifstream file(filepath);
            if (!file.is_open()) continue;

            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string json_content = buffer.str();

            // Check if this token matches the start URL
            std::string cached_start_url = extractJsonValue(json_content, "startUrl");
            if (cached_start_url != start_url) {
                continue;
            }

            // Parse token
            SSOToken token;
            token.access_token = extractJsonValue(json_content, "accessToken");
            token.expires_at = extractJsonValue(json_content, "expiresAt");
            token.region = extractJsonValue(json_content, "region");
            token.start_url = cached_start_url;

            closedir(dir);

            if (!token.access_token.empty() && !token.isExpired()) {
                return token;
            }

            return std::nullopt;  // Found but expired
        }

        closedir(dir);
        #endif

        return std::nullopt;
    }

    /**
     * @brief Get temporary credentials using SSO token
     *
     * Makes API call to AWS SSO service to exchange token for credentials
     *
     * @param access_token SSO access token
     * @param account_id AWS account ID
     * @param role_name IAM role name
     * @param region AWS region
     * @return tuple of (access_key, secret_key, session_token)
     */
    static std::optional<std::tuple<std::string, std::string, std::string>>
    getCredentialsFromToken(const std::string& access_token,
                           const std::string& account_id,
                           const std::string& role_name,
                           const std::string& region) {
        #ifdef ENABLE_CURL
        // Construct SSO endpoint URL
        std::string url = "https://portal.sso." + region + ".amazonaws.com/federation/credentials"
                        + "?account_id=" + account_id
                        + "&role_name=" + role_name;

        // Make HTTP GET request with bearer token
        CURL* curl = curl_easy_init();
        if (!curl) {
            return std::nullopt;
        }

        std::string response_data;
        auto write_callback = [](char* ptr, size_t size, size_t nmemb, void* userdata) -> size_t {
            std::string* str = static_cast<std::string*>(userdata);
            str->append(ptr, size * nmemb);
            return size * nmemb;
        };

        std::string auth_header = "Authorization: Bearer " + access_token;
        std::string sso_header = "x-amz-sso_bearer_token: " + access_token;
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, auth_header.c_str());
        headers = curl_slist_append(headers, sso_header.c_str());

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, +write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

        CURLcode res = star_curl_perform(curl);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            return std::nullopt;
        }

        // Parse JSON response (simplified)
        std::string access_key = extractJsonValue(response_data, "accessKeyId");
        std::string secret_key = extractJsonValue(response_data, "secretAccessKey");
        std::string session_token = extractJsonValue(response_data, "sessionToken");

        if (access_key.empty() || secret_key.empty() || session_token.empty()) {
            return std::nullopt;
        }

        return std::make_tuple(access_key, secret_key, session_token);
        #else
        return std::nullopt;  // CURL not available
        #endif
    }
};

/**
 * @brief AWS Credentials with resolution chain
 *
 * Resolves credentials in standard AWS order:
 * 1. Environment variables
 * 2. AWS SSO session
 * 3. Credentials file
 */
struct S3Credentials {
    std::string access_key;
    std::string secret_key;
    std::string session_token;

    /**
     * @brief Resolve credentials using standard AWS chain
     *
     * @param profile Profile name (from AWS_PROFILE env var or "default")
     * @return S3Credentials
     * @throws std::runtime_error if no credentials found
     */
    static S3Credentials resolve(const std::string& profile = "") {
        std::string profile_name = profile;
        if (profile_name.empty()) {
            const char* env_profile = std::getenv("AWS_PROFILE");
            profile_name = env_profile ? std::string(env_profile) : "default";
        }

        // Priority 1: Environment variables
        if (auto creds = fromEnvironment()) {
            return *creds;
        }

        // Priority 2: AWS SSO session
        if (auto creds = fromSSO(profile_name)) {
            return *creds;
        }

        // Priority 3: Credentials file
        if (auto creds = fromCredentialsFile(profile_name)) {
            return *creds;
        }

        throw std::runtime_error(
            "No AWS credentials found. Set AWS_ACCESS_KEY_ID/AWS_SECRET_ACCESS_KEY, "
            "configure AWS SSO, or add credentials to ~/.aws/credentials");
    }

private:
    // Priority 1: Environment variables
    static std::optional<S3Credentials> fromEnvironment() {
        const char* access_key = std::getenv("AWS_ACCESS_KEY_ID");
        const char* secret_key = std::getenv("AWS_SECRET_ACCESS_KEY");
        const char* session_token = std::getenv("AWS_SESSION_TOKEN");

        if (access_key && secret_key) {
            S3Credentials creds;
            creds.access_key = access_key;
            creds.secret_key = secret_key;
            creds.session_token = session_token ? std::string(session_token) : "";
            return creds;
        }

        return std::nullopt;
    }

    // Priority 2: AWS SSO session
    static std::optional<S3Credentials> fromSSO(const std::string& profile) {
        std::string config_path = AWSConfigParser::getConfigPath();
        if (config_path.empty()) {
            return std::nullopt;
        }

        AWSConfigParser config(config_path);
        if (!config.hasProfile(profile)) {
            return std::nullopt;
        }

        // Check if this profile uses SSO
        std::string sso_session = config.getValue(profile, "sso_session");
        if (sso_session.empty()) {
            return std::nullopt;  // Not an SSO profile
        }

        // Get SSO configuration
        std::string sso_account_id = config.getValue(profile, "sso_account_id");
        std::string sso_role_name = config.getValue(profile, "sso_role_name");
        std::string sso_start_url = config.getValue("sso-session " + sso_session, "sso_start_url");
        std::string sso_region = config.getValue("sso-session " + sso_session, "sso_region");

        if (sso_account_id.empty() || sso_role_name.empty() ||
            sso_start_url.empty() || sso_region.empty()) {
            return std::nullopt;  // Incomplete SSO configuration
        }

        // Read cached SSO token. Pass the sso-session name too: the AWS CLI hashes
        // it (not the start_url) to name the cache file when an sso-session is used.
        auto token = AWSTokenCache::readToken(sso_start_url, sso_session);
        if (!token) {
            return std::nullopt;  // No valid token found
        }

        // Get temporary credentials from SSO
        auto creds_tuple = AWSTokenCache::getCredentialsFromToken(
            token->access_token, sso_account_id, sso_role_name, sso_region
        );

        if (!creds_tuple) {
            return std::nullopt;
        }

        S3Credentials creds;
        std::tie(creds.access_key, creds.secret_key, creds.session_token) = *creds_tuple;
        return creds;
    }

    // Priority 3: Credentials file
    static std::optional<S3Credentials> fromCredentialsFile(const std::string& profile) {
        std::string creds_path = AWSConfigParser::getCredentialsPath();
        if (creds_path.empty()) {
            return std::nullopt;
        }

        AWSConfigParser creds_file(creds_path);
        if (!creds_file.hasProfile(profile)) {
            return std::nullopt;
        }

        std::string access_key = creds_file.getValue(profile, "aws_access_key_id");
        std::string secret_key = creds_file.getValue(profile, "aws_secret_access_key");
        std::string session_token = creds_file.getValue(profile, "aws_session_token");

        if (access_key.empty() || secret_key.empty()) {
            return std::nullopt;
        }

        S3Credentials creds;
        creds.access_key = access_key;
        creds.secret_key = secret_key;
        creds.session_token = session_token;
        return creds;
    }
};

// Helper functions for S3 support

/**
 * @brief Get S3 region from environment or default
 */
inline std::string getS3Region() {
    const char* region = std::getenv("AWS_DEFAULT_REGION");
    return region ? std::string(region) : "us-east-1";
}

/**
 * @brief Custom streambuf for S3 reads with range request support
 *
 * Mirrors HttpStreamBuf but uses AWS Signature V4 authentication
 */
class S3StreamBuf : public std::streambuf {
private:
    CURL* m_curl;
    std::string m_bucket;
    std::string m_key;
    std::string m_region;
    std::unique_ptr<AWSV4Signer> m_signer;
    std::vector<char> m_buffer;  
    size_t m_position;
    size_t m_content_length;
    bool m_valid;  // Track if stream is valid (HEAD succeeded)

    static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        std::vector<char>* buffer = static_cast<std::vector<char>*>(userdata);
        size_t bytes = size * nmemb;
        buffer->insert(buffer->end(), ptr, ptr + bytes);
        return bytes;
    }

    struct HeaderData {
        size_t content_length = 0;
        std::string bucket_region;
    };

    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t bytes = size * nitems;
        std::string header(buffer, bytes);
        HeaderData* data = static_cast<HeaderData*>(userdata);

        // Extract Content-Length
        if (header.find("Content-Length:") == 0 || header.find("content-length:") == 0) {
            std::string length = header.substr(header.find(":") + 1);
            // Trim whitespace
            length.erase(0, length.find_first_not_of(" \t\r\n"));
            length.erase(length.find_last_not_of(" \t\r\n") + 1);
            if (!length.empty()) {
                // Never let std::stoul throw out of this libcurl C callback (UB);
                // leave content_length unchanged on malformed input.
                try {
                    data->content_length = std::stoul(length);
                } catch (const std::exception&) {
                }
            }
        }
        // Extract bucket region from redirect
        else if (header.find("x-amz-bucket-region:") == 0 || header.find("X-Amz-Bucket-Region:") == 0) {
            std::string region = header.substr(header.find(":") + 1);
            // Trim whitespace
            region.erase(0, region.find_first_not_of(" \t\r\n"));
            region.erase(region.find_last_not_of(" \t\r\n") + 1);
            data->bucket_region = region;
        }
        return bytes;
    }

    std::string getS3Url() const {
        // Encode the key the same way signRequest() encodes the canonical URI,
        // or curl fetches a different path than what was signed. Honors the
        // AWS_S3_ENDPOINT/AWS_VIRTUAL_HOSTING/AWS_HTTPS override.
        return S3EndpointConfig::resolve().url(m_bucket, m_region,
                                               AWSV4Signer::urlEncode(m_key));
    }

    std::string getS3Host() const {
        return S3EndpointConfig::resolve().host(m_bucket, m_region);
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
        #ifdef _WIN32
        gmtime_s(&tm, &now_time);
        #else
        gmtime_r(&now_time, &tm);
        #endif

        char buffer[17];
        std::strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", &tm);
        return std::string(buffer);
    }

    bool fetchRange(size_t start, size_t end) {
        m_buffer.clear();

        std::string url = getS3Url();
        std::string timestamp = getCurrentTimestamp();
        std::string host = getS3Host();

        // Prepare headers for signing
        std::map<std::string, std::string> sign_headers;
        sign_headers["host"] = host;
        sign_headers["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";  // Empty body hash
        sign_headers["x-amz-date"] = timestamp;

        if (!m_signer->getSessionToken().empty()) {
            sign_headers["x-amz-security-token"] = m_signer->getSessionToken();
        }

        // Sign the request
        std::string authorization = m_signer->signRequest(
            "GET", m_bucket, m_key,
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            sign_headers
        );

        // Set up CURL headers
        struct curl_slist* headers = nullptr;
        std::string range_header = "Range: bytes=" + std::to_string(start) + "-" + std::to_string(end);
        headers = curl_slist_append(headers, range_header.c_str());
        headers = curl_slist_append(headers, ("Host: " + host).c_str());
        headers = curl_slist_append(headers, ("x-amz-date: " + timestamp).c_str());
        headers = curl_slist_append(headers, "x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        headers = curl_slist_append(headers, ("Authorization: " + authorization).c_str());

        if (!m_signer->getSessionToken().empty()) {
            headers = curl_slist_append(headers, ("x-amz-security-token: " + m_signer->getSessionToken()).c_str());
        }

        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        // The constructor pointed CURLOPT_HEADERDATA at a stack-local HeaderData
        // that is now out of scope; clear the header callback so curl does not
        // write response headers into that dangling pointer (stack corruption).
        curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, nullptr);
        curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, nullptr);

        CURLcode res = star_curl_perform(m_curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK) {
            LOG_ERROR("S3 range request failed: ", curl_easy_strerror(res));
            return false;
        }

        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code >= 400) {
            LOG_ERROR("S3 returned error code: ", response_code);
            return false;
        }

        if (!m_buffer.empty()) {
            setg(m_buffer.data(), m_buffer.data(), m_buffer.data() + m_buffer.size());
            return true;
        }
        return false;
    }

protected:
    virtual int_type underflow() override {
        // If stream is invalid, return EOF immediately
        if (!m_valid) {
            return traits_type::eof();
        }

        if (gptr() < egptr()) {
            return traits_type::to_int_type(*gptr());
        }

        // Fetch next range
        size_t start = m_position;
        size_t end = std::min(m_position + 65536, m_content_length - 1);  // 64KB chunks

        if (start >= m_content_length) {
            return traits_type::eof();
        }

        if (!fetchRange(start, end)) {
            return traits_type::eof();
        }

        m_position = end + 1;
        return traits_type::to_int_type(*gptr());
    }

    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode which = std::ios_base::in) override {
        if (which & std::ios_base::in) {
            if (pos >= 0 && pos <= static_cast<pos_type>(m_content_length)) {
                setg(nullptr, nullptr, nullptr);
                m_position = static_cast<size_t>(pos);
                LOG_TRACE("S3 seek to position ", pos);
                return pos;
            }
        }
        return pos_type(off_type(-1));
    }

    virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                            std::ios_base::openmode which = std::ios_base::in) override {
        if (which & std::ios_base::in) {
            pos_type new_pos;

            switch (dir) {
                case std::ios_base::beg:
                    new_pos = off;
                    break;
                case std::ios_base::cur:
                    if (gptr() && egptr()) {
                        new_pos = m_position - (egptr() - gptr()) + off;
                    } else {
                        new_pos = m_position + off;
                    }
                    break;
                case std::ios_base::end:
                    new_pos = m_content_length + off;
                    break;
                default:
                    return pos_type(off_type(-1));
            }

            if (new_pos >= 0 && new_pos <= static_cast<pos_type>(m_content_length)) {
                setg(nullptr, nullptr, nullptr);
                m_position = static_cast<size_t>(new_pos);
                LOG_TRACE("S3 seekoff to position ", new_pos);
                return new_pos;
            }
        }
        return pos_type(off_type(-1));
    }

public:
    S3StreamBuf(const std::string& bucket, const std::string& key,
                const std::string& region, const S3Credentials& creds)
        : m_bucket(bucket), m_key(key), m_region(region), m_position(0), m_content_length(0), m_valid(false) {

        m_buffer.reserve(8192);  

        // Initialize CURL
        m_curl = curl_easy_init();
        if (!m_curl) {
            throw std::runtime_error("Failed to initialize CURL for S3");
        }

        // Set basic CURL options
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_buffer);
        curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 0L);  // Don't follow redirects (breaks signature)

        // Make HEAD request to detect bucket region (may get 301 redirect)
        HeaderData header_data;
        curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &header_data);

        // Create initial signer with provided region
        m_signer = std::make_unique<AWSV4Signer>(
            creds.access_key, creds.secret_key, m_region, creds.session_token
        );

        std::string url = getS3Url();
        std::string timestamp = getCurrentTimestamp();
        std::string host = getS3Host();

        std::map<std::string, std::string> sign_headers;
        sign_headers["host"] = host;
        sign_headers["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
        sign_headers["x-amz-date"] = timestamp;

        if (!creds.session_token.empty()) {
            sign_headers["x-amz-security-token"] = creds.session_token;
        }

        std::string authorization = m_signer->signRequest(
            "HEAD", m_bucket, m_key,
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            sign_headers
        );

        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, ("Host: " + host).c_str());
        headers = curl_slist_append(headers, ("x-amz-date: " + timestamp).c_str());
        headers = curl_slist_append(headers, "x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        headers = curl_slist_append(headers, ("Authorization: " + authorization).c_str());

        if (!creds.session_token.empty()) {
            headers = curl_slist_append(headers, ("x-amz-security-token: " + creds.session_token).c_str());
        }

        curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);  // HEAD request

        CURLcode res = star_curl_perform(m_curl);
        curl_slist_free_all(headers);
        curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);

        if (res != CURLE_OK) {
            LOG_ERROR("S3 HEAD request failed: ", curl_easy_strerror(res));
            throw std::runtime_error("S3 HEAD request failed: " + std::string(curl_easy_strerror(res)));
        }

        // Check HTTP response code
        long response_code;
        curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);

        // If we got a 301 redirect, the bucket is in a different region
        if (response_code == 301 && !header_data.bucket_region.empty()) {
            LOG_TRACE("Bucket is in region ", header_data.bucket_region, ", retrying with correct region");
            m_region = header_data.bucket_region;

            // Re-create signer with correct region
            m_signer = std::make_unique<AWSV4Signer>(
                creds.access_key, creds.secret_key, m_region, creds.session_token
            );

            // Retry HEAD request with correct region
            header_data.content_length = 0;
            header_data.bucket_region.clear();

            url = getS3Url();
            timestamp = getCurrentTimestamp();
            host = getS3Host();

            sign_headers.clear();
            sign_headers["host"] = host;
            sign_headers["x-amz-content-sha256"] = "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
            sign_headers["x-amz-date"] = timestamp;

            if (!creds.session_token.empty()) {
                sign_headers["x-amz-security-token"] = creds.session_token;
            }

            authorization = m_signer->signRequest(
                "HEAD", m_bucket, m_key,
                "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
                sign_headers
            );

            headers = nullptr;
            headers = curl_slist_append(headers, ("Host: " + host).c_str());
            headers = curl_slist_append(headers, ("x-amz-date: " + timestamp).c_str());
            headers = curl_slist_append(headers, "x-amz-content-sha256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
            headers = curl_slist_append(headers, ("Authorization: " + authorization).c_str());

            if (!creds.session_token.empty()) {
                headers = curl_slist_append(headers, ("x-amz-security-token: " + creds.session_token).c_str());
            }

            curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
            curl_easy_setopt(m_curl, CURLOPT_NOBODY, 1L);

            res = star_curl_perform(m_curl);
            curl_slist_free_all(headers);
            curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);

            if (res != CURLE_OK) {
                LOG_ERROR("S3 HEAD request (retry) failed: ", curl_easy_strerror(res));
                throw std::runtime_error("S3 HEAD request failed: " + std::string(curl_easy_strerror(res)));
            }

            curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
        }

        // A 3xx redirect we could not resolve (e.g. a 301 with no
        // x-amz-bucket-region header, or a 301 that recurs on retry) is not a
        // valid object stream and must not be read as if it were the object.
        if (response_code >= 300 && response_code < 400) {
            LOG_ERROR("S3 HEAD returned unresolved redirect code: ", response_code);
            throw std::runtime_error("S3 HEAD request failed with redirect code " + std::to_string(response_code));
        }

        if (response_code >= 400) {
            // For 404 (not found), don't throw - allow graceful handling for new files
            if (response_code == 404) {
                LOG_TRACE("S3 object not found (404) - stream will be invalid");
                m_valid = false;
                return;
            }
            // For other errors, throw
            LOG_ERROR("S3 HEAD returned error code: ", response_code);
            throw std::runtime_error("S3 HEAD request failed with code " + std::to_string(response_code));
        }

        m_content_length = header_data.content_length;
        m_valid = true;
        LOG_TRACE("S3 stream initialized for s3://", m_bucket, "/", m_key,
                  ", content_length=", m_content_length);
    }

    ~S3StreamBuf() {
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
    }

    size_t size() const {
        return m_content_length;
    }

    bool isValid() const {
        return m_valid;
    }
};

/**
 * @brief Input stream for S3 objects with range request support
 */
class S3Stream : public std::istream {
private:
    S3StreamBuf m_streambuf;

public:
    S3Stream(const std::string& bucket, const std::string& key,
             const std::string& region, const S3Credentials& creds)
        : std::istream(nullptr), m_streambuf(bucket, key, region, creds) {
        rdbuf(&m_streambuf);
        // If streambuf is invalid (e.g., 404), set stream state to fail
        if (!m_streambuf.isValid()) {
            setstate(std::ios::failbit);
        }
    }

    size_t size() const {
        return m_streambuf.size();
    }
};

/**
 * @brief S3 writer for uploading objects
 *
 * Handles uploading data to S3 using PUT requests with AWS Signature V4 authentication.
 * Supports single-shot uploads (entire object in one PUT request).
 */
class S3Writer {
private:
    CURL* m_curl;
    std::string m_bucket;
    std::string m_key;
    std::string m_region;
    std::unique_ptr<AWSV4Signer> m_signer;

    std::string getS3Url() const {
        // Encode the key the same way signRequest() encodes the canonical URI,
        // or curl fetches a different path than what was signed. Honors the
        // AWS_S3_ENDPOINT/AWS_VIRTUAL_HOSTING/AWS_HTTPS override.
        return S3EndpointConfig::resolve().url(m_bucket, m_region,
                                               AWSV4Signer::urlEncode(m_key));
    }

    std::string getS3Host() const {
        return S3EndpointConfig::resolve().host(m_bucket, m_region);
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm now_tm{};
        #ifdef _WIN32
        gmtime_s(&now_tm, &now_c);
        #else
        gmtime_r(&now_c, &now_tm);
        #endif

        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", &now_tm);
        return std::string(buffer);
    }

    static size_t ReadCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        struct UploadData {
            const char* data;
            size_t size;
            size_t offset;
        };

        UploadData* upload = static_cast<UploadData*>(userdata);
        size_t bytes_to_copy = std::min(size * nmemb, upload->size - upload->offset);

        if (bytes_to_copy > 0) {
            std::memcpy(ptr, upload->data + upload->offset, bytes_to_copy);
            upload->offset += bytes_to_copy;
        }

        return bytes_to_copy;
    }

public:
    S3Writer(const std::string& bucket, const std::string& key,
             const std::string& region, const S3Credentials& creds)
        : m_bucket(bucket), m_key(key), m_region(region) {

        m_curl = curl_easy_init();
        if (!m_curl) {
            throw std::runtime_error("Failed to initialize CURL for S3 writer");
        }

        m_signer = std::make_unique<AWSV4Signer>(
            creds.access_key, creds.secret_key, region, creds.session_token
        );
    }

    ~S3Writer() {
        if (m_curl) {
            curl_easy_cleanup(m_curl);
        }
    }

    struct HeaderData {
        std::string bucket_region;
    };

    static size_t HeaderCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
        size_t bytes = size * nitems;
        std::string header(buffer, bytes);
        HeaderData* data = static_cast<HeaderData*>(userdata);

        // Extract bucket region from redirect
        if (header.find("x-amz-bucket-region:") == 0 || header.find("X-Amz-Bucket-Region:") == 0) {
            std::string region = header.substr(header.find(":") + 1);
            // Trim whitespace
            region.erase(0, region.find_first_not_of(" \t\r\n"));
            region.erase(region.find_last_not_of(" \t\r\n") + 1);
            data->bucket_region = region;
        }
        return bytes;
    }

    static size_t WriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
        std::string* response = static_cast<std::string*>(userdata);
        size_t bytes = size * nmemb;
        response->append(ptr, bytes);
        return bytes;
    }

    /**
     * @brief Upload object to S3
     * @param data Pointer to data to upload
     * @param size Size of data in bytes
     */
    void putObject(const char* data, size_t size) {
        HeaderData header_data;
        std::string response_body;

        curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
        curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, &header_data);
        curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response_body);

        // Try upload (may need to retry with correct region)
        for (int attempt = 0; attempt < 2; attempt++) {
            std::string url = getS3Url();
            std::string timestamp = getCurrentTimestamp();

            // Calculate SHA256 of content
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256(reinterpret_cast<const unsigned char*>(data), size, hash);
            std::string content_hash = AWSV4Signer::hexEncode(hash, SHA256_DIGEST_LENGTH);

            // Build headers
            std::map<std::string, std::string> headers;
            headers["host"] = getS3Host();
            headers["x-amz-date"] = timestamp;
            headers["x-amz-content-sha256"] = content_hash;

            if (!m_signer->m_session_token.empty()) {
                headers["x-amz-security-token"] = m_signer->m_session_token;
            }

            // Sign request
            std::string auth_header = m_signer->signRequest("PUT", m_bucket, m_key, content_hash, headers);

            // Set up CURL
            curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
            curl_easy_setopt(m_curl, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(size));

            // Set up headers
            struct curl_slist* header_list = nullptr;
            header_list = curl_slist_append(header_list, ("Host: " + headers["host"]).c_str());
            header_list = curl_slist_append(header_list, ("x-amz-date: " + timestamp).c_str());
            header_list = curl_slist_append(header_list, ("x-amz-content-sha256: " + content_hash).c_str());
            header_list = curl_slist_append(header_list, ("Authorization: " + auth_header).c_str());

            if (!m_signer->m_session_token.empty()) {
                header_list = curl_slist_append(header_list,
                    ("x-amz-security-token: " + m_signer->m_session_token).c_str());
            }

            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, header_list);

            // Set up upload data
            struct UploadData {
                const char* data;
                size_t size;
                size_t offset;
            } upload_data = {data, size, 0};

            curl_easy_setopt(m_curl, CURLOPT_READFUNCTION, ReadCallback);
            curl_easy_setopt(m_curl, CURLOPT_READDATA, &upload_data);

            // Perform upload
            CURLcode res = star_curl_perform(m_curl);

            // Clean up
            curl_slist_free_all(header_list);

            if (res != CURLE_OK) {
                throw std::runtime_error("S3 PUT failed: " + std::string(curl_easy_strerror(res)));
            }

            // Check HTTP response code
            long response_code = 0;
            curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);

            // If we got 301 and this is first attempt, retry with correct region
            if (response_code == 301 && !header_data.bucket_region.empty() && attempt == 0) {
                LOG_TRACE("S3 PUT got 301, retrying with region ", header_data.bucket_region);
                // Get credentials before recreating signer
                std::string access_key = m_signer->m_access_key;
                std::string secret_key = m_signer->m_secret_key;
                std::string session_token = m_signer->m_session_token;

                m_region = header_data.bucket_region;
                // Re-create signer with correct region
                m_signer = std::make_unique<AWSV4Signer>(
                    access_key, secret_key, m_region, session_token
                );
                header_data.bucket_region.clear();
                response_body.clear();
                continue;  // Retry
            }

            // Success or non-retryable error
            if (response_code >= 200 && response_code < 300) {
                return;  // Success
            }

            throw std::runtime_error("S3 PUT failed with HTTP " + std::to_string(response_code));
        }

        throw std::runtime_error("S3 PUT failed after retries");
    }
};

#endif // ENABLE_S3


/**
 * @brief Parse a file path and determine its type (local, HTTP, or S3)
 *
 * Recognizes both the GDAL virtual-filesystem prefixes and the equivalent plain
 * URL/URI forms, so either works interchangeably:
 *   - S3:   "/vsis3/bucket/key"   OR  "s3://bucket/key"
 *   - HTTP: "/vsicurl/https://host/path"  OR  "https://host/path" / "http://..."
 *   - anything else -> a local filesystem path.
 * The `/vsi*` prefixes stay supported for GDAL compatibility; the plain forms are
 * the natural way to name a remote object and map to the same handling.
 *
 * Defined unconditionally: StarDataset::open()/ctor call it for every path, and
 * the LOCAL branch (the common case) has no S3/curl dependency. Only the S3
 * region-resolution needs AWSConfigParser, so that part is gated on ENABLE_S3;
 * without S3 support an S3 path is rejected with a clear error.
 */
inline FilePathInfo parseFilePath(const std::string& filename) {
    FilePathInfo info;

    auto starts_with = [&filename](const char* prefix) {
        const size_t n = std::strlen(prefix);
        return filename.size() >= n && filename.compare(0, n, prefix) == 0;
    };

    // --- S3: GDAL "/vsis3/" prefix OR an "s3://" URI (both -> bucket + key) ---
    const bool is_vsis3 = starts_with("/vsis3/");
    const bool is_s3_uri = starts_with("s3://");
    if (is_vsis3 || is_s3_uri) {
        info.type = FilePathInfo::S3;
        // Strip the recognized prefix ("/vsis3/" = 7 chars, "s3://" = 5), leaving
        // "bucket/key".
        std::string remainder = filename.substr(is_vsis3 ? 7 : 5);
        size_t slash = remainder.find('/');
        if (slash == std::string::npos) {
            throw std::runtime_error(
                "Invalid S3 path format. Expected: /vsis3/bucket/key or s3://bucket/key");
        }
        info.bucket = remainder.substr(0, slash);
        info.key = remainder.substr(slash + 1);

#ifdef ENABLE_S3
        // Get region (priority: AWS_DEFAULT_REGION env var > AWS config file > default)
        const char* env_region = std::getenv("AWS_DEFAULT_REGION");
        if (env_region) {
            info.region = env_region;
        } else {
            // Try to read region from AWS config file
            std::string config_path = AWSConfigParser::getConfigPath();
            if (!config_path.empty()) {
                AWSConfigParser config(config_path);
                const char* env_profile = std::getenv("AWS_PROFILE");
                std::string profile = env_profile ? std::string(env_profile) : "default";

                if (config.hasProfile(profile)) {
                    std::string config_region = config.getValue(profile, "region");
                    if (!config_region.empty()) {
                        info.region = config_region;
                    } else {
                        info.region = "us-east-1";  // Default fallback
                    }
                } else {
                    info.region = "us-east-1";  // Default fallback
                }
            } else {
                info.region = "us-east-1";  // Default fallback
            }
        }

        return info;
#else
        throw std::runtime_error(
            "S3 path '" + filename + "' requires S3 support, but this build was "
            "compiled without ENABLE_S3.");
#endif
    } else if (starts_with("/vsicurl/")) {
        // HTTP behind the GDAL "/vsicurl/" prefix: the remainder is the full URL.
        info.type = FilePathInfo::HTTP;
        info.path = filename.substr(9);
        return info;
    } else if (starts_with("http://") || starts_with("https://")) {
        // Plain URL: treat as HTTP; the whole string (scheme included) is the URL.
        info.type = FilePathInfo::HTTP;
        info.path = filename;
        return info;
    } else {
        // Local file path
        info.type = FilePathInfo::LOCAL;
        info.path = filename;
        return info;
    }
}


//==============================================================================
// RangeReader - a persistent, connection-reusing byte-range reader
//==============================================================================
//
// StarDS reads a file as a sequence of explicit byte ranges (the header, then
// per-array/per-layer blocks whose exact offsets+sizes are already in the
// header). RangeReader collapses those reads onto ONE reused connection and
// issues only ranged GETs — no HEAD, no new TLS handshake per read. A single
// RangeReader is held by StarDataset for its lifetime.
//
// Phases this implements:
//   1. connection reuse   — one handle/connection for the whole dataset
//   2. no HEAD requests    — reads are range-explicit; size() is derived lazily
//   3. coalesced reads     — read_at(off, len) fetches the exact span in 1 GET
//   4. whole-file cache    — small files are fetched once and served from memory

// Small buffer of raw bytes at a known file offset.
class RangeReader {
public:
    virtual ~RangeReader() = default;

    // Read exactly `len` bytes starting at absolute file offset `offset` into
    // `out` (resized to the bytes actually available). Returns the number of
    // bytes read. Implementations fetch the requested span in as few requests
    // as possible (one GET for remote readers).
    virtual size_t read_at(size_t offset, size_t len, std::vector<char>& out) = 0;

    // Total size of the underlying object, if known cheaply. Returns SIZE_MAX
    // when unknown (e.g. a remote object whose size we have not needed yet).
    virtual size_t size_or_unknown() = 0;

    // True if the object is known to exist / be readable.
    virtual bool good() const = 0;

    // Phase 4 small-file fast path: if the whole object is no larger than
    // `max_bytes`, pull it into memory in a SINGLE request and serve all
    // subsequent read_at() calls from that buffer. Returns true if the object is
    // now fully cached. Implementations must do this without a prior HEAD — a
    // remote reader issues one ranged GET of [0, max_bytes] and uses the
    // Content-Range total to decide whether the whole object fit.
    virtual bool ensure_whole_cached(size_t max_bytes) { (void)max_bytes; return false; }
};

// LOCAL: a plain file. seek+read; no network. Optionally slurps the whole file.
class LocalRangeReader : public RangeReader {
public:
    explicit LocalRangeReader(const std::string& path) : m_path(path) {}

    size_t read_at(size_t offset, size_t len, std::vector<char>& out) override {
        if (!m_whole.empty() || m_cached) {
            return serve_from_cache(offset, len, out);
        }
        std::ifstream in(m_path, std::ios::binary);
        if (!in.good()) { out.clear(); return 0; }
        in.seekg(static_cast<std::streamoff>(offset));
        out.resize(len);
        in.read(out.data(), static_cast<std::streamsize>(len));
        out.resize(static_cast<size_t>(in.gcount()));
        return out.size();
    }

    size_t size_or_unknown() override {
        if (m_cached) return m_whole.size();
        std::ifstream in(m_path, std::ios::binary | std::ios::ate);
        if (!in.good()) return SIZE_MAX;
        std::streamoff n = in.tellg();
        return n < 0 ? SIZE_MAX : static_cast<size_t>(n);
    }

    bool good() const override {
        std::ifstream in(m_path, std::ios::binary);
        return in.good();
    }

    bool ensure_whole_cached(size_t max_bytes) override {
        if (m_cached) return true;
        std::ifstream in(m_path, std::ios::binary | std::ios::ate);
        if (!in.good()) return false;
        std::streamoff n = in.tellg();
        if (n < 0) return false;
        if (static_cast<size_t>(n) > max_bytes) return false;  // too big to cache
        in.seekg(0);
        m_whole.resize(static_cast<size_t>(n));
        in.read(m_whole.data(), n);
        m_whole.resize(static_cast<size_t>(in.gcount()));
        m_cached = true;
        return true;
    }

private:
    size_t serve_from_cache(size_t offset, size_t len, std::vector<char>& out) {
        if (offset >= m_whole.size()) { out.clear(); return 0; }
        size_t avail = std::min(len, m_whole.size() - offset);
        out.assign(m_whole.begin() + offset, m_whole.begin() + offset + avail);
        return avail;
    }
    std::string m_path;
    std::vector<char> m_whole;
    bool m_cached = false;
};

// MEMORY: the entire .stards image is held in a byte buffer (no file, no
// network). Serves every read from that buffer — this backs open_bytes().
class MemoryRangeReader : public RangeReader {
public:
    explicit MemoryRangeReader(std::vector<char> bytes) : m_bytes(std::move(bytes)) {}

    size_t read_at(size_t offset, size_t len, std::vector<char>& out) override {
        if (offset >= m_bytes.size()) { out.clear(); return 0; }
        size_t avail = std::min(len, m_bytes.size() - offset);
        out.assign(m_bytes.begin() + offset, m_bytes.begin() + offset + avail);
        return avail;
    }

    size_t size_or_unknown() override { return m_bytes.size(); }
    bool good() const override { return true; }
    // Already fully in memory; nothing to fetch.
    bool ensure_whole_cached(size_t /*max_bytes*/) override { return true; }

private:
    std::vector<char> m_bytes;
};

#ifdef ENABLE_CURL
// HTTP: one persistent CURL handle, reused across ranged GETs. libcurl keeps the
// TCP+TLS connection alive between star_curl_perform() calls on the same handle,
// so N array reads cost ~N GETs on ONE connection (vs N HEAD+GET+handshakes).
class HttpRangeReader : public RangeReader {
public:
    explicit HttpRangeReader(const std::string& url) : m_url(url) {
        m_curl = curl_easy_init();
        if (m_curl) {
            curl_easy_setopt(m_curl, CURLOPT_URL, m_url.c_str());
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &HttpRangeReader::write_cb);
            curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, &HttpRangeReader::header_cb);
            curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
            curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 1L);
            // Reuse the connection across requests (default, but be explicit).
            curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 0L);
        }
    }
    ~HttpRangeReader() override { if (m_curl) curl_easy_cleanup(m_curl); }

    size_t read_at(size_t offset, size_t len, std::vector<char>& out) override {
        if (m_cached) return serve_from_cache(offset, len, out);
        out.clear();
        if (!m_curl || len == 0) return 0;
        m_sink = &out;
        std::string range = "Range: bytes=" + std::to_string(offset) + "-" +
                            std::to_string(offset + len - 1);
        struct curl_slist* headers = curl_slist_append(nullptr, range.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        CURLcode res = star_curl_perform(m_curl);  // single ranged GET
        curl_slist_free_all(headers);
        m_sink = nullptr;
        if (res != CURLE_OK) { out.clear(); return 0; }
        return out.size();
    }

    // Content-Length is captured opportunistically from the Content-Range of any
    // GET we already made; we never issue a HEAD just to learn the size.
    size_t size_or_unknown() override { return m_total_size; }

    bool good() const override { return m_curl != nullptr; }

    bool ensure_whole_cached(size_t max_bytes) override {
        if (m_cached) return true;
        if (!m_curl || max_bytes == 0) return false;
        // ONE ranged GET of [0, max_bytes]. The server reports the object's true
        // size in Content-Range ("bytes 0-N/TOTAL"); if TOTAL <= max_bytes we got
        // the whole object in this single request and can cache it. If it's
        // larger, we bail (the partial bytes are discarded; a real ranged read
        // path is used instead) — but no HEAD was ever issued.
        std::vector<char> buf;
        m_sink = &buf;
        std::string range = "Range: bytes=0-" + std::to_string(max_bytes - 1);
        struct curl_slist* headers = curl_slist_append(nullptr, range.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        CURLcode res = star_curl_perform(m_curl);
        curl_slist_free_all(headers);
        m_sink = nullptr;
        if (res != CURLE_OK) return false;
        // If we learned the total size and it fits, or the server returned fewer
        // bytes than we asked for (=> whole object), treat as fully cached.
        bool whole = (m_total_size != SIZE_MAX && m_total_size <= max_bytes) ||
                     (buf.size() < max_bytes);
        if (whole) {
            m_whole = std::move(buf);
            m_cached = true;
            if (m_total_size == SIZE_MAX) m_total_size = m_whole.size();
            return true;
        }
        return false;
    }

private:
    static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* self = static_cast<HttpRangeReader*>(userdata);
        size_t bytes = size * nmemb;
        if (self->m_sink) self->m_sink->insert(self->m_sink->end(), ptr, ptr + bytes);
        return bytes;
    }
    static size_t header_cb(char* buffer, size_t size, size_t nitems, void* userdata) {
        auto* self = static_cast<HttpRangeReader*>(userdata);
        size_t bytes = size * nitems;
        std::string h(buffer, bytes);
        // Prefer Content-Range total ("bytes a-b/TOTAL"); fall back to
        // Content-Length only for a non-ranged (whole-object) response.
        auto lower = h; std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.rfind("content-range:", 0) == 0) {
            auto slash = h.find('/');
            if (slash != std::string::npos) {
                try { self->m_total_size = std::stoull(h.substr(slash + 1)); } catch (...) {}
            }
        }
        return bytes;
    }
    size_t serve_from_cache(size_t offset, size_t len, std::vector<char>& out) {
        if (offset >= m_whole.size()) { out.clear(); return 0; }
        size_t avail = std::min(len, m_whole.size() - offset);
        out.assign(m_whole.begin() + offset, m_whole.begin() + offset + avail);
        return avail;
    }

    CURL* m_curl = nullptr;
    std::string m_url;
    std::vector<char>* m_sink = nullptr;
    std::vector<char> m_whole;
    bool m_cached = false;
    size_t m_total_size = SIZE_MAX;
};
#endif // ENABLE_CURL

#ifdef ENABLE_S3
// S3: one persistent CURL handle + one cached AWS SigV4 signer, reused across
// signed ranged GETs. Bucket-region (301) resolution happens lazily on the first
// request and is then cached — no per-read HEAD, no per-read handshake.
class S3RangeReader : public RangeReader {
public:
    S3RangeReader(std::string bucket, std::string key, std::string region,
                  S3Credentials creds)
        : m_bucket(std::move(bucket)), m_key(std::move(key)),
          m_region(std::move(region)), m_creds(std::move(creds)) {
        m_curl = curl_easy_init();
        if (m_curl) {
            curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, &S3RangeReader::write_cb);
            curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, this);
            curl_easy_setopt(m_curl, CURLOPT_HEADERFUNCTION, &S3RangeReader::header_cb);
            curl_easy_setopt(m_curl, CURLOPT_HEADERDATA, this);
            curl_easy_setopt(m_curl, CURLOPT_FOLLOWLOCATION, 0L);  // signed; no redirects
            curl_easy_setopt(m_curl, CURLOPT_FORBID_REUSE, 0L);
        }
        m_signer = std::make_unique<AWSV4Signer>(
            m_creds.access_key, m_creds.secret_key, m_region, m_creds.session_token);
    }
    ~S3RangeReader() override { if (m_curl) curl_easy_cleanup(m_curl); }

    size_t read_at(size_t offset, size_t len, std::vector<char>& out) override {
        if (m_cached) return serve_from_cache(offset, len, out);
        out.clear();
        if (!m_curl || len == 0) return 0;
        // One signed ranged GET, with a one-time region retry on 301.
        if (!signed_get(offset, offset + len - 1, out)) { out.clear(); return 0; }
        return out.size();
    }

    size_t size_or_unknown() override { return m_total_size; }
    bool good() const override { return m_curl != nullptr; }

    bool ensure_whole_cached(size_t max_bytes) override {
        if (m_cached) return true;
        if (!m_curl || max_bytes == 0) return false;
        // ONE signed ranged GET of [0, max_bytes]. Use the Content-Range total to
        // decide whether the whole object fit; no HEAD is issued.
        std::vector<char> buf;
        if (!signed_get(0, max_bytes - 1, buf)) return false;
        bool whole = (m_total_size != SIZE_MAX && m_total_size <= max_bytes) ||
                     (buf.size() < max_bytes);
        if (whole) {
            m_whole = std::move(buf);
            m_cached = true;
            if (m_total_size == SIZE_MAX) m_total_size = m_whole.size();
            return true;
        }
        return false;
    }

private:
    static size_t write_cb(char* ptr, size_t size, size_t nmemb, void* userdata) {
        auto* self = static_cast<S3RangeReader*>(userdata);
        size_t bytes = size * nmemb;
        if (self->m_sink) self->m_sink->insert(self->m_sink->end(), ptr, ptr + bytes);
        return bytes;
    }
    static size_t header_cb(char* buffer, size_t size, size_t nitems, void* userdata) {
        auto* self = static_cast<S3RangeReader*>(userdata);
        size_t bytes = size * nitems;
        std::string h(buffer, bytes);
        std::string lower = h;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower.rfind("content-range:", 0) == 0) {
            auto slash = h.find('/');
            if (slash != std::string::npos) {
                try { self->m_total_size = std::stoull(h.substr(slash + 1)); } catch (...) {}
            }
        } else if (lower.rfind("x-amz-bucket-region:", 0) == 0) {
            auto colon = h.find(':');
            if (colon != std::string::npos) {
                std::string v = h.substr(colon + 1);
                // trim
                v.erase(0, v.find_first_not_of(" \t\r\n"));
                v.erase(v.find_last_not_of(" \t\r\n") + 1);
                self->m_redirect_region = v;
            }
        }
        return bytes;
    }

    std::string getUrl() const {
        return S3EndpointConfig::resolve().url(m_bucket, m_region,
                                               AWSV4Signer::urlEncode(m_key));
    }
    std::string getHost() const {
        return S3EndpointConfig::resolve().host(m_bucket, m_region);
    }
    static std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};
#ifdef _WIN32
        gmtime_s(&tm, &t);
#else
        gmtime_r(&t, &tm);
#endif
        char buf[17];
        std::strftime(buf, sizeof(buf), "%Y%m%dT%H%M%SZ", &tm);
        return std::string(buf);
    }

    // Perform one signed GET for [start, end] (end==SIZE_MAX => open-ended to EOF).
    // Retries once against the bucket's real region if the first attempt 301s.
    bool signed_get(size_t start, size_t end, std::vector<char>& out) {
        for (int attempt = 0; attempt < 2; ++attempt) {
            out.clear();
            m_sink = &out;
            m_redirect_region.clear();

            static const char* kEmptyHash =
                "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
            std::string url = getUrl();
            std::string host = getHost();
            std::string ts = timestamp();

            std::map<std::string, std::string> sign_headers;
            sign_headers["host"] = host;
            sign_headers["x-amz-content-sha256"] = kEmptyHash;
            sign_headers["x-amz-date"] = ts;
            if (!m_creds.session_token.empty()) {
                sign_headers["x-amz-security-token"] = m_creds.session_token;
            }
            std::string authorization = m_signer->signRequest(
                "GET", m_bucket, m_key, kEmptyHash, sign_headers);

            struct curl_slist* headers = nullptr;
            std::string range = (end == SIZE_MAX)
                ? ("Range: bytes=" + std::to_string(start) + "-")
                : ("Range: bytes=" + std::to_string(start) + "-" + std::to_string(end));
            headers = curl_slist_append(headers, range.c_str());
            headers = curl_slist_append(headers, ("Host: " + host).c_str());
            headers = curl_slist_append(headers, ("x-amz-date: " + ts).c_str());
            headers = curl_slist_append(headers,
                (std::string("x-amz-content-sha256: ") + kEmptyHash).c_str());
            headers = curl_slist_append(headers, ("Authorization: " + authorization).c_str());
            if (!m_creds.session_token.empty()) {
                headers = curl_slist_append(headers,
                    ("x-amz-security-token: " + m_creds.session_token).c_str());
            }

            curl_easy_setopt(m_curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
            CURLcode res = star_curl_perform(m_curl);
            curl_slist_free_all(headers);
            m_sink = nullptr;

            if (res != CURLE_OK) return false;
            long code = 0;
            curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &code);

            if (code == 301 && !m_redirect_region.empty() && attempt == 0) {
                // Re-sign for the correct region and retry once.
                m_region = m_redirect_region;
                m_signer = std::make_unique<AWSV4Signer>(
                    m_creds.access_key, m_creds.secret_key, m_region, m_creds.session_token);
                continue;
            }
            return code >= 200 && code < 300;
        }
        return false;
    }

    size_t serve_from_cache(size_t offset, size_t len, std::vector<char>& out) {
        if (offset >= m_whole.size()) { out.clear(); return 0; }
        size_t avail = std::min(len, m_whole.size() - offset);
        out.assign(m_whole.begin() + offset, m_whole.begin() + offset + avail);
        return avail;
    }

    CURL* m_curl = nullptr;
    std::string m_bucket, m_key, m_region;
    S3Credentials m_creds;
    std::unique_ptr<AWSV4Signer> m_signer;
    std::vector<char>* m_sink = nullptr;
    std::string m_redirect_region;
    std::vector<char> m_whole;
    bool m_cached = false;
    size_t m_total_size = SIZE_MAX;
};
#endif // ENABLE_S3


//==============================================================================
// NDArray Class - Modern n-dimensional array implementation
//==============================================================================

/**
 * @brief Modern n-dimensional array class with xtensor-style API
 *
 * This class implements an n-dimensional array stored as a flat 1D vector
 * with operator() indexing, iterators, and static factory methods.
 */
template<typename T>
class NDArray {
public:
    // Type aliases
    using value_type = T;
    using iterator = typename std::vector<T>::iterator;
    using const_iterator = typename std::vector<T>::const_iterator;
    using reverse_iterator = typename std::vector<T>::reverse_iterator;
    using const_reverse_iterator = typename std::vector<T>::const_reverse_iterator;

private:
    // Private members
    std::vector<T> m_data;           // Flat 1D storage for array elements
    std::vector<size_t> m_shape;     // Dimensions of the array
    std::vector<size_t> m_strides;   // Number of elements to step in each dimension

public:
    /**
     * @brief Default constructor
     */
    NDArray() = default;

    /**
     * @brief Constructor with shape
     * @param shape_in Dimensions of the array
     */
    explicit NDArray(const std::vector<size_t>& shape_in) : m_shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        m_data.resize(total_size);
    }

    /**
     * @brief Constructor with shape and initial value
     * @param shape_in Dimensions of the array
     * @param initial_value Value to initialize all elements with
     */
    NDArray(const std::vector<size_t>& shape_in, const T& initial_value) : m_shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        m_data.resize(total_size, initial_value);
    }

    /**
     * @brief Constructor with data and shape
     * @param data_in Flat data to use
     * @param shape_in Dimensions of the array
     */
    NDArray(const std::vector<T>& data_in, const std::vector<size_t>& shape_in) : m_data(data_in), m_shape(shape_in) {
        calculateStrides();
        if (m_data.size() != computeTotalSize()) {
            throw std::runtime_error("Data size does not match the specified shape");
        }
    }

    /**
     * @brief Move constructor with data and shape (zero-copy for Python bindings)
     * @param data_in Flat data to move from
     * @param shape_in Dimensions of the array
     */
    NDArray(std::vector<T>&& data_in, const std::vector<size_t>& shape_in) : m_data(std::move(data_in)), m_shape(shape_in) {
        calculateStrides();
        if (m_data.size() != computeTotalSize()) {
            throw std::runtime_error("Data size does not match the specified shape");
        }
    }

    /**
     * @brief Factory method to create NDArray by moving vector (for Python bindings)
     * SWIG doesn't handle rvalue references well, so we use this factory method
     * @param data_in Vector to move from
     * @param shape_in Dimensions of the array
     */
    static NDArray from_vector_move(std::vector<T>& data_in, const std::vector<size_t>& shape_in) {
        return NDArray(std::move(data_in), shape_in);
    }

    /**
     * @brief Copy constructor
     * @param other NDArray to copy from
     */
    NDArray(const NDArray& other) : m_data(other.m_data), m_shape(other.m_shape), m_strides(other.m_strides) {
    }

    // ========== Accessors ==========

    /**
     * @brief Get reference to internal data vector
     */
    std::vector<T>& data() { return m_data; }
    const std::vector<T>& data() const { return m_data; }

    /**
     * @brief Get raw pointer to data
     */
    T* data_ptr() { return m_data.data(); }
    const T* data_ptr() const { return m_data.data(); }

    /**
     * @brief Get shape vector
     */
    const std::vector<size_t>& shape() const { return m_shape; }

    /**
     * @brief Get specific dimension size
     */
    size_t shape(size_t dim) const {
        if (dim >= m_shape.size()) {
            throw std::runtime_error("Dimension index out of bounds");
        }
        return m_shape[dim];
    }

    /**
     * @brief Get strides vector
     */
    const std::vector<size_t>& strides() const { return m_strides; }

    /**
     * @brief Get number of dimensions
     */
    size_t dimension() const { return m_shape.size(); }

    /**
     * @brief Get total number of elements
     */
    size_t size() const {
        if (m_shape.empty()) return 1;  // Scalar has 1 element
        size_t total = 1;
        for (auto dim : m_shape) {
            total *= dim;
        }
        return total;
    }

    // ========== Indexing ==========

    /**
     * @brief Variadic operator() for multi-dimensional indexing
     */
    template<typename... Indices>
    T& operator()(Indices... indices) {
        return m_data[flattenIndex(std::forward<Indices>(indices)...)];
    }

    template<typename... Indices>
    const T& operator()(Indices... indices) const {
        return m_data[flattenIndex(std::forward<Indices>(indices)...)];
    }

    /**
     * @brief Flat indexing for 1D access
     */
    T& flat(size_t index) {
        if (index >= m_data.size()) {
            throw std::runtime_error("Flat index out of bounds");
        }
        return m_data[index];
    }

    const T& flat(size_t index) const {
        if (index >= m_data.size()) {
            throw std::runtime_error("Flat index out of bounds");
        }
        return m_data[index];
    }

    /**
     * @brief Get Array element
     * @deprecated Use operator() instead
     */
    T& at(const std::vector<size_t>& indices) {
        return m_data[flattenIndex(indices)];
    }

    const T& at(const std::vector<size_t>& indices) const {
        return m_data[flattenIndex(indices)];
    }

    // ========== Iterators ==========

    iterator begin() { return m_data.begin(); }
    const_iterator begin() const { return m_data.begin(); }
    const_iterator cbegin() const { return m_data.cbegin(); }

    iterator end() { return m_data.end(); }
    const_iterator end() const { return m_data.end(); }
    const_iterator cend() const { return m_data.cend(); }

    reverse_iterator rbegin() { return m_data.rbegin(); }
    const_reverse_iterator rbegin() const { return m_data.rbegin(); }
    reverse_iterator rend() { return m_data.rend(); }
    const_reverse_iterator rend() const { return m_data.rend(); }

    // ========== Static Factories ==========

    /**
     * @brief Create zero-initialized array
     */
    static NDArray<T> zeros(const std::vector<size_t>& shape) {
        return NDArray<T>(shape, T{0});
    }

    /**
     * @brief Create one-initialized array
     */
    static NDArray<T> ones(const std::vector<size_t>& shape) {
        return NDArray<T>(shape, T{1});
    }

    /**
     * @brief Create array filled with specific value
     */
    static NDArray<T> full(const std::vector<size_t>& shape, const T& value) {
        return NDArray<T>(shape, value);
    }

    /**
     * @brief Create uninitialized array
     */
    static NDArray<T> empty(const std::vector<size_t>& shape) {
        return NDArray<T>(shape);
    }

    /**
     * @brief Create range array (numeric types only)
     */
#ifndef SWIG // Dont let swig wrap this 
    template<typename U = T>
    static typename std::enable_if<std::is_arithmetic<U>::value, NDArray<T>>::type
    arange(T start, T stop, T step = T{1}) {
        if (step == T{0}) {
            throw std::runtime_error("Step cannot be zero");
        }

        size_t count = static_cast<size_t>(std::ceil((stop - start) / step));
        NDArray<T> result({count});

        T value = start;
        for (size_t i = 0; i < count; ++i) {
            result.m_data[i] = value;
            value += step;
        }

        return result;
    }
#endif

    // ========== Shape Operations ==========

    /**
     * @brief Reshape array (no reallocation)
     */
    void reshape(const std::vector<size_t>& new_shape) {
        size_t new_size = 1;
        for (auto dim : new_shape) {
            new_size *= dim;
        }

        if (new_size != m_data.size()) {
            throw std::runtime_error("Cannot reshape: size mismatch");
        }

        m_shape = new_shape;
        calculateStrides();
    }

    /**
     * @brief Resize array (with reallocation)
     */
#ifndef SWIG // dont let swig wrap this
    void resize(const std::vector<size_t>& new_shape, const T& fill_value = T{}) {
        size_t new_size = 1;
        for (auto dim : new_shape) {
            new_size *= dim;
        }

        m_data.resize(new_size, fill_value);
        m_shape = new_shape;
        calculateStrides();
    }
#endif

    /**
     * @brief Write array to output stream
     * @param os Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const NDArray<T>& arr) {
        LOG_TRACE("Writing NDArray<", typeid(T).name(), "> to output stream at position ", os.tellp());
        // All counts/dims/lengths are written as fixed 64-bit little-endian
        // integers (not raw size_t, whose width varies by platform) so the
        // stream is portable across 32- and 64-bit builds.

        // Write number of dimensions
        uint64_t num_dims = arr.m_shape.size();
        write_u64(os, num_dims);
        LOG_TRACE("Wrote number of dimensions: ", num_dims);

        // Write shape
        for (size_t dim : arr.m_shape) {
            write_u64(os, dim);
        }

        // Write strides
        for (size_t stride : arr.m_strides) {
            write_u64(os, stride);
        }

        if constexpr (std::is_same<T, std::string>::value) {
            // Write data (strings need special handling): u64 length + bytes.
            for (const auto& str : arr.m_data) {
                write_u64(os, str.size());
                os.write(str.data(), static_cast<std::streamsize>(str.size()));
            }
        } else {
            // Write data in chunks (raw element bytes; sizeof(T) is fixed).
            constexpr size_t CHUNK_SIZE = 1024; // 1KB chunks
            size_t size_to_write = sizeof(T) * arr.m_data.size();
            const char* data_ptr = reinterpret_cast<const char*>(arr.m_data.data());

            size_t bytes_written = 0;
            while (bytes_written < size_to_write) {
                size_t bytes_to_write = std::min(CHUNK_SIZE, size_to_write - bytes_written);
                os.write(data_ptr + bytes_written, bytes_to_write);
                bytes_written += bytes_to_write;
            }
            LOG_TRACE("Completed writing data of size ", size_to_write);
        }
        return os;
    }

    /**
     * @brief Read array from input stream
     * @param is Input stream
     */
    friend std::istream& operator>>(std::istream& is, NDArray<T>& arr) {
        LOG_TRACE("Reading NDArray<", typeid(T).name(), "> from input stream from byte position ", is.tellg());

        // Read number of dimensions (fixed u64, matching operator<<).
        size_t num_dims = static_cast<size_t>(read_u64(is));
        LOG_TRACE("Read number of dimensions: ", num_dims);

        // Read shape
        arr.m_shape.resize(num_dims);
        for (auto& dim : arr.m_shape) {
            dim = static_cast<size_t>(read_u64(is));
        }

        // Read strides
        arr.m_strides.resize(num_dims);
        for (auto& stride : arr.m_strides) {
            stride = static_cast<size_t>(read_u64(is));
        }

        // Calculate total size and read data
        size_t total_size = arr.computeTotalSize();
        if constexpr (std::is_same<T, std::string>::value) {
            arr.m_data.resize(total_size);
            for (size_t i = 0; i < total_size; ++i) {
                size_t str_len = static_cast<size_t>(read_u64(is));
                std::string str(str_len, '\0');
                is.read(&str[0], static_cast<std::streamsize>(str_len));
                arr.m_data[i] = std::move(str);
            }
            LOG_TRACE("Finished reading NDArray<std::string>");
        } else {
            arr.m_data.resize(total_size);
            is.read(reinterpret_cast<char*>(arr.m_data.data()), sizeof(T) * total_size);
            LOG_TRACE("Read data array of size ", sizeof(T) * total_size);
        }
        return is;
    }

private:
    /**
     * @brief Calculate strides based on shape
     */
    void calculateStrides() {
        m_strides.resize(m_shape.size());
        if (m_shape.empty()) return;

        m_strides[m_shape.size() - 1] = 1;
        for (int i = static_cast<int>(m_shape.size()) - 2; i >= 0; --i) {
            m_strides[i] = m_strides[i + 1] * m_shape[i + 1];
        }
    }

    /**
     * @brief Compute total size of the array
     * @return Total number of elements
     */
    size_t computeTotalSize() const {
        if (m_shape.empty()) return 1;  // Scalar has 1 element
        size_t size = 1;
        for (auto dim : m_shape) {
            size *= dim;
        }
        return size;
    }

    /**
     * @brief Convert multi-dimensional index to flat index
     * @param indices Indices for each dimension
     * @return Flat index
     */
    size_t flattenIndex(const std::vector<size_t>& indices) const {
        if (indices.size() != m_shape.size()) {
            throw std::runtime_error("Index dimensions do not match array dimensions");
        }

        size_t flat_index = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] >= m_shape[i]) {
                throw std::runtime_error("Index out of bounds");
            }
            flat_index += indices[i] * m_strides[i];
        }

        return flat_index;
    }

    /**
     * @brief Variadic flattenIndex for parameter pack
     */
    template<typename... Indices>
    size_t flattenIndex(Indices... indices) const {
        std::array<size_t, sizeof...(Indices)> idx_array = {
            static_cast<size_t>(indices)...
        };

        // Special case: scalar (empty shape) accessed with single index [0]
        if (m_shape.empty() && idx_array.size() == 1 && idx_array[0] == 0) {
            return 0;
        }

        if (idx_array.size() != m_shape.size()) {
            throw std::runtime_error("Index dimensions do not match array dimensions");
        }

        size_t flat_index = 0;
        for (size_t i = 0; i < idx_array.size(); ++i) {
            if (idx_array[i] >= m_shape[i]) {
                throw std::runtime_error("Index out of bounds");
            }
            flat_index += idx_array[i] * m_strides[i];
        }

        return flat_index;
    }
};


//==============================================================================
// Type Definitions
//==============================================================================

using ValueVariant = std::variant<
    NDArray<int8_t>, NDArray<int16_t>, NDArray<int32_t>, NDArray<int64_t>,
    NDArray<uint8_t>, NDArray<uint16_t>, NDArray<uint32_t>, NDArray<uint64_t>,
    NDArray<float>, NDArray<double>,
    NDArray<std::string>
>;

//==============================================================================
// Thread Pool for Parallel Block Operations
//==============================================================================

/**
 * @brief Simple thread pool for parallel block operations
 *
 * Provides work-stealing task execution for parallel compression/decompression.
 * Thread-safe and exception-safe.
 */
class ThreadPool {
public:
    /**
     * @brief Construct thread pool
     * @param num_threads Number of worker threads (0 = auto-detect)
     */
    explicit ThreadPool(size_t num_threads = 0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0) num_threads = 1;
        }

        for (size_t i = 0; i < num_threads; ++i) {
            workers.emplace_back([this] { worker_thread(); });
        }
    }

    /**
     * @brief Destructor - stops all worker threads
     */
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (auto& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    // Delete copy/move to prevent resource issues
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    /**
     * @brief Execute function in parallel for range [start, end)
     * @param start Start index (inclusive)
     * @param end End index (exclusive)
     * @param func Function to execute for each index
     */
    template<typename Func>
    void parallel_for(size_t start, size_t end, Func&& func) {
        if (workers.empty()) {
            // Single-threaded fallback
            for (size_t i = start; i < end; ++i) {
                func(i);
            }
            return;
        }

        std::vector<std::future<void>> futures;
        futures.reserve(end - start);

        for (size_t i = start; i < end; ++i) {
            futures.push_back(enqueue([i, &func] { func(i); }));
        }

        // Wait for all tasks to complete
        for (auto& future : futures) {
            future.get();  // Propagates exceptions
        }
    }

    /**
     * @brief Enqueue a task for execution
     * @param f Function to execute
     * @return Future for result
     */
    template<typename Func>
    std::future<void> enqueue(Func&& f) {
        auto task = std::make_shared<std::packaged_task<void()>>(std::forward<Func>(f));
        std::future<void> result = task->get_future();

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("ThreadPool stopped");
            }
            tasks.emplace([task] { (*task)(); });
        }

        condition.notify_one();
        return result;
    }

    /**
     * @brief Get number of worker threads
     */
    size_t size() const { return workers.size(); }

private:
    /**
     * @brief Worker thread main loop
     */
    void worker_thread() {
        while (true) {
            std::function<void()> task;

            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                condition.wait(lock, [this] { return stop || !tasks.empty(); });

                if (stop && tasks.empty()) {
                    return;
                }

                task = std::move(tasks.front());
                tasks.pop();
            }

            task();  // Execute outside lock
        }
    }

    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop = false;
};

//==============================================================================
// Block Compression Functions
//==============================================================================

/**
 * @brief Estimate compressed size using deflateBound without actually compressing
 *
 * This provides an upper bound on the compressed size, which is useful for
 * pre-calculating file positions before actual compression. The estimate is
 * typically 10-30% larger than actual compressed size.
 *
 * @param data Raw uncompressed data
 * @param data_size Size of raw data
 * @param algorithm Compression algorithm to use
 * @param block_size Size of each uncompressed block
 * @return Estimated upper bound on compressed size
 */
inline size_t estimateCompressedSize(const char* data, size_t data_size,
                                     CompressionAlgorithm algorithm, size_t block_size) {
    if (algorithm == CompressionAlgorithm::NONE) {
        return data_size;
    }

    #ifdef ENABLE_ZLIB
    if (algorithm == CompressionAlgorithm::GZIP) {
        size_t num_blocks = (data_size + block_size - 1) / block_size;
        size_t total_bound = 0;

        for (size_t i = 0; i < num_blocks; i++) {
            size_t offset = i * block_size;
            size_t block_size_actual = std::min(block_size, data_size - offset);

            // deflateBound gives upper bound on compressed size
            total_bound += deflateBound(nullptr, block_size_actual);
        }

        return total_bound;
    }
    #endif

    // Conservative fallback: assume no compression
    return data_size;
}

/**
 * @brief Compresses data in blocks using pre-allocated buffer 
 * @param data Raw uncompressed data
 * @param data_size Size of raw data
 * @param algorithm Compression algorithm to use
 * @param block_size Size of each uncompressed block
 * @param compressed_output Pre-allocated output buffer (will be cleared and reused)
 * @param temp_buffer Pre-allocated temporary buffer for compression (will be resized as needed)
 * @param thread_pool Optional thread pool for parallel compression (nullptr = single-threaded)
 * @return Block metadata
 */
inline std::vector<BlockInfo>
compressBlocksBuffered(const char* data, size_t data_size,
                       CompressionAlgorithm algorithm, size_t block_size,
                       std::vector<char>& compressed_output,
                       std::vector<char>& temp_buffer,
                       ThreadPool* thread_pool = nullptr) {

    // The shuffle prefilter (if any) is applied by the caller before this point;
    // here we only run the underlying block codec.
    algorithm = base_compression(algorithm);

    compressed_output.clear();
    std::vector<BlockInfo> block_infos;

    size_t num_blocks = (data_size + block_size - 1) / block_size;
    LOG_TRACE("Compressing ", data_size, " bytes into ", num_blocks, " blocks (buffered)");

    // Check if we should use threading (heuristic fallback for small workloads)
    bool use_threading = thread_pool &&
                        num_blocks >= g_min_blocks_for_threading &&
                        data_size >= g_min_bytes_for_threading;

    if (use_threading) {
        // Estimate compressed size bounds for each block
        std::vector<size_t> block_bounds(num_blocks);
        block_infos.resize(num_blocks);

        for (size_t i = 0; i < num_blocks; ++i) {
            size_t offset_in = i * block_size;
            size_t block_uncompressed_size = std::min(block_size, data_size - offset_in);

            block_infos[i].uncompressed_size = block_uncompressed_size;

            if (algorithm == CompressionAlgorithm::GZIP) {
                #ifdef ENABLE_ZLIB
                block_bounds[i] = compressBound(block_uncompressed_size);
                #else
                block_bounds[i] = block_uncompressed_size;
                #endif
            } else if (algorithm == CompressionAlgorithm::LZ4) {
                #ifdef ENABLE_LZ4
                block_bounds[i] = LZ4_compressBound(block_uncompressed_size);
                #else
                block_bounds[i] = block_uncompressed_size;
                #endif
            } else {
                block_bounds[i] = block_uncompressed_size;  // No compression
            }
        }

        LOG_TRACE("Phase 1: Estimated ", num_blocks, " block bounds");

        // Calculate block positions and pre-allocate output buffer
        std::vector<size_t> block_offsets(num_blocks);
        size_t total_bound = 0;

        for (size_t i = 0; i < num_blocks; ++i) {
            block_offsets[i] = total_bound;
            total_bound += block_bounds[i];
        }

        compressed_output.resize(total_bound);  // Pre-allocate with upper bound

        LOG_TRACE("Phase 2: Pre-allocated ", total_bound, " bytes for ", num_blocks, " blocks");

        // Compress blocks in parallel, write directly to final positions
        thread_pool->parallel_for(0, num_blocks, [&](size_t i) {
            size_t offset_in = i * block_size;
            size_t block_uncompressed_size = block_infos[i].uncompressed_size;

            if (algorithm == CompressionAlgorithm::GZIP) {
                #ifdef ENABLE_ZLIB
                uLongf actual_compressed_size = block_bounds[i];

                // Compress DIRECTLY into final position in output buffer
                int result = compress2(
                    reinterpret_cast<Bytef*>(compressed_output.data() + block_offsets[i]),
                    &actual_compressed_size,
                    reinterpret_cast<const Bytef*>(data + offset_in),
                    block_uncompressed_size,
                    Z_BEST_COMPRESSION
                );

                if (result != Z_OK) {
                    throw std::runtime_error("Block compression failed");
                }

                block_infos[i].compressed_size = actual_compressed_size;

                LOG_TRACE("Block ", i, ": ", block_uncompressed_size, " -> ",
                         actual_compressed_size, " bytes (temp offset ", block_offsets[i], ")",
                         " (", (100.0 * actual_compressed_size / block_uncompressed_size), "%)");
                #else
                throw std::runtime_error("zlib not enabled");
                #endif
            } else if (algorithm == CompressionAlgorithm::LZ4) {
                #ifdef ENABLE_LZ4
                int compressed_size = LZ4_compress_default(
                    reinterpret_cast<const char*>(data + offset_in),
                    reinterpret_cast<char*>(compressed_output.data() + block_offsets[i]),
                    block_uncompressed_size,
                    block_bounds[i]
                );

                if (compressed_size <= 0) {
                    throw std::runtime_error("LZ4 block compression failed");
                }

                block_infos[i].compressed_size = compressed_size;

                LOG_TRACE("Block ", i, ": ", block_uncompressed_size, " -> ",
                         compressed_size, " bytes (temp offset ", block_offsets[i], ")",
                         " (", (100.0 * compressed_size / block_uncompressed_size), "%)");
                #else
                throw std::runtime_error("LZ4 not enabled");
                #endif
            } else if (algorithm == CompressionAlgorithm::NONE) {
                // No compression - direct memcpy to final position
                std::memcpy(compressed_output.data() + block_offsets[i],
                           data + offset_in,
                           block_uncompressed_size);
                block_infos[i].compressed_size = block_uncompressed_size;
            } else {
                throw std::runtime_error("Unsupported compression algorithm");
            }
        });

        LOG_TRACE("Phase 3: Compressed ", num_blocks, " blocks in parallel");

        // Calculate actual contiguous offsets
        size_t actual_offset = 0;
        for (size_t i = 0; i < num_blocks; ++i) {
            size_t old_offset = block_offsets[i];
            size_t compressed_size = block_infos[i].compressed_size;

            // Move block if not already at correct position
            if (old_offset != actual_offset) {
                std::memmove(compressed_output.data() + actual_offset,
                            compressed_output.data() + old_offset,
                            compressed_size);
            }

            block_infos[i].offset = actual_offset;
            actual_offset += compressed_size;
        }

        // Trim output to actual contiguous size
        compressed_output.resize(actual_offset);

        LOG_TRACE("Phase 4: Compacted blocks from ", total_bound, " to ", actual_offset, " bytes");
    } else {
        // SINGLE-THREADED PATH: Original sequential implementation
        compressed_output.reserve(data_size);

        for (size_t i = 0; i < num_blocks; ++i) {
        size_t offset_in = i * block_size;
        size_t block_uncompressed_size = std::min(block_size, data_size - offset_in);

        BlockInfo info;
        info.offset = compressed_output.size();
        info.uncompressed_size = block_uncompressed_size;

        if (algorithm == CompressionAlgorithm::GZIP) {
            #ifdef ENABLE_ZLIB
            uLongf compressed_bound = compressBound(block_uncompressed_size);

            // Reuse temp_buffer instead of allocating
            if (temp_buffer.size() < compressed_bound) {
                temp_buffer.resize(compressed_bound);
            }

            uLongf actual_compressed_size = compressed_bound;
            int result = compress2(
                reinterpret_cast<Bytef*>(temp_buffer.data()),
                &actual_compressed_size,
                reinterpret_cast<const Bytef*>(data + offset_in),
                block_uncompressed_size,
                Z_BEST_COMPRESSION
            );

            if (result != Z_OK) {
                throw std::runtime_error("Block compression failed");
            }

            info.compressed_size = actual_compressed_size;

            // Append compressed block to output
            compressed_output.insert(
                compressed_output.end(),
                temp_buffer.begin(),
                temp_buffer.begin() + actual_compressed_size
            );
            #else
            throw std::runtime_error("zlib not enabled");
            #endif
        } else if (algorithm == CompressionAlgorithm::LZ4) {
            #ifdef ENABLE_LZ4
            int compressed_bound = LZ4_compressBound(block_uncompressed_size);

            // Reuse temp_buffer instead of allocating
            if (temp_buffer.size() < static_cast<size_t>(compressed_bound)) {
                temp_buffer.resize(compressed_bound);
            }

            int compressed_size = LZ4_compress_default(
                reinterpret_cast<const char*>(data + offset_in),
                reinterpret_cast<char*>(temp_buffer.data()),
                block_uncompressed_size,
                compressed_bound
            );

            if (compressed_size <= 0) {
                throw std::runtime_error("LZ4 block compression failed");
            }

            info.compressed_size = compressed_size;

            // Append compressed block to output
            compressed_output.insert(
                compressed_output.end(),
                temp_buffer.begin(),
                temp_buffer.begin() + compressed_size
            );
            #else
            throw std::runtime_error("LZ4 not enabled");
            #endif
        } else if (algorithm == CompressionAlgorithm::NONE) {
            // No compression - just copy
            info.compressed_size = block_uncompressed_size;
            compressed_output.insert(
                compressed_output.end(),
                data + offset_in,
                data + offset_in + block_uncompressed_size
            );
        } else {
            throw std::runtime_error("Unsupported compression algorithm");
        }

            block_infos.push_back(info);
            LOG_TRACE("Block ", i, ": ", info.uncompressed_size, " -> ", info.compressed_size,
                      " bytes (", (100.0 * info.compressed_size / info.uncompressed_size), "%)");
        }
    }

    return block_infos;
}

/**
 * @brief Compresses data in blocks and returns block metadata (legacy version)
 * @param data Raw uncompressed data
 * @param data_size Size of raw data
 * @param algorithm Compression algorithm to use
 * @param block_size Size of each uncompressed block
 * @return Pair of compressed data and block metadata
 */
inline std::pair<std::vector<char>, std::vector<BlockInfo>>
compressBlocks(const char* data, size_t data_size,
               CompressionAlgorithm algorithm, size_t block_size) {

    std::vector<char> compressed_output;
    std::vector<char> temp_buffer;
    auto block_infos = compressBlocksBuffered(data, data_size, algorithm, block_size,
                                               compressed_output, temp_buffer);
    return {std::move(compressed_output), std::move(block_infos)};
}

/**
 * @brief Decompresses specific blocks from compressed data
 * @param compressed_data Full compressed data
 * @param blocks Block metadata
 * @param algorithm Compression algorithm used
 * @param block_indices Which blocks to decompress (empty = all)
 * @param thread_pool Optional thread pool for parallel decompression (nullptr = single-threaded)
 * @return Decompressed data
 */
inline std::vector<char> decompressBlocks(
    const std::vector<char>& compressed_data,
    const std::vector<BlockInfo>& blocks,
    CompressionAlgorithm algorithm,
    const std::vector<size_t>& block_indices = {},
    ThreadPool* thread_pool = nullptr) {

    // Undo any shuffle-variant labeling: the blocks were written with the base
    // codec (the caller unshuffles the reconstructed bytes afterward).
    algorithm = base_compression(algorithm);

    std::vector<char> decompressed_output;

    // If no specific blocks requested, decompress all
    std::vector<size_t> indices_to_decompress = block_indices;
    if (indices_to_decompress.empty()) {
        for (size_t i = 0; i < blocks.size(); ++i) {
            indices_to_decompress.push_back(i);
        }
    }

    // Calculate total output size and per-block offsets
    std::vector<size_t> output_offsets(indices_to_decompress.size());
    size_t total_size = 0;
    for (size_t i = 0; i < indices_to_decompress.size(); ++i) {
        output_offsets[i] = total_size;
        total_size += blocks[indices_to_decompress[i]].uncompressed_size;
    }

    // Check if we should use threading
    bool use_threading = thread_pool &&
                        indices_to_decompress.size() >= g_min_blocks_for_threading &&
                        total_size >= g_min_bytes_for_threading;

    if (use_threading) {
        // PARALLEL PATH: Direct-write decompression
        // Pre-allocate full output buffer
        try {
            decompressed_output.resize(total_size);
        } catch (const std::bad_alloc& e) {
            throw std::runtime_error("Failed to allocate " +
                std::to_string(total_size / 1024.0 / 1024.0 / 1024.0) +
                " GB for decompression buffer: " + e.what());
        }

        // Decompress blocks in parallel, each writing to its designated region
        thread_pool->parallel_for(0, indices_to_decompress.size(), [&](size_t i) {
            size_t idx = indices_to_decompress[i];
            const BlockInfo& block = blocks[idx];
            size_t output_offset = output_offsets[i];

            if (algorithm == CompressionAlgorithm::GZIP) {
                #ifdef ENABLE_ZLIB
                uLongf dest_len = block.uncompressed_size;
                int result = uncompress(
                    reinterpret_cast<Bytef*>(decompressed_output.data() + output_offset),
                    &dest_len,
                    reinterpret_cast<const Bytef*>(compressed_data.data() + block.offset),
                    block.compressed_size
                );

                if (result != Z_OK) {
                    throw std::runtime_error("Block decompression failed: block " + std::to_string(idx));
                }
                #else
                throw std::runtime_error("zlib not enabled");
                #endif
            } else if (algorithm == CompressionAlgorithm::LZ4) {
                #ifdef ENABLE_LZ4
                // Ensure sizes fit in int (LZ4 API requirement)
                if (block.compressed_size > INT_MAX || block.uncompressed_size > INT_MAX) {
                    throw std::runtime_error("LZ4 block size exceeds INT_MAX limit");
                }

                int decompressed_size = LZ4_decompress_safe(
                    reinterpret_cast<const char*>(compressed_data.data() + block.offset),
                    reinterpret_cast<char*>(decompressed_output.data() + output_offset),
                    static_cast<int>(block.compressed_size),
                    static_cast<int>(block.uncompressed_size)
                );

                if (decompressed_size != static_cast<int>(block.uncompressed_size)) {
                    throw std::runtime_error("LZ4 block decompression failed: block " + std::to_string(idx));
                }
                #else
                throw std::runtime_error("LZ4 not enabled");
                #endif
            } else if (algorithm == CompressionAlgorithm::NONE) {
                // No compression - just copy directly to output position
                std::memcpy(
                    decompressed_output.data() + output_offset,
                    compressed_data.data() + block.offset,
                    block.compressed_size
                );
            } else {
                throw std::runtime_error("Unsupported compression algorithm");
            }
        });
    } else {
        // SINGLE-THREADED PATH: Original sequential implementation
        decompressed_output.reserve(total_size);

        for (size_t idx : indices_to_decompress) {
            const BlockInfo& block = blocks[idx];

        if (algorithm == CompressionAlgorithm::GZIP) {
            #ifdef ENABLE_ZLIB
            std::vector<char> block_output(block.uncompressed_size);
            uLongf dest_len = block.uncompressed_size;

            int result = uncompress(
                reinterpret_cast<Bytef*>(block_output.data()),
                &dest_len,
                reinterpret_cast<const Bytef*>(compressed_data.data() + block.offset),
                block.compressed_size
            );

            if (result != Z_OK) {
                throw std::runtime_error("Block decompression failed: block " + std::to_string(idx));
            }

            decompressed_output.insert(decompressed_output.end(),
                                      block_output.begin(), block_output.end());
            #else
            throw std::runtime_error("zlib not enabled");
            #endif
            } else if (algorithm == CompressionAlgorithm::LZ4) {
                #ifdef ENABLE_LZ4
                // Ensure sizes fit in int (LZ4 API requirement)
                if (block.compressed_size > INT_MAX || block.uncompressed_size > INT_MAX) {
                    throw std::runtime_error("LZ4 block size exceeds INT_MAX limit");
                }

                std::vector<char> block_output(block.uncompressed_size);

                int decompressed_size = LZ4_decompress_safe(
                    reinterpret_cast<const char*>(compressed_data.data() + block.offset),
                    block_output.data(),
                    static_cast<int>(block.compressed_size),
                    static_cast<int>(block.uncompressed_size)
                );

                if (decompressed_size != static_cast<int>(block.uncompressed_size)) {
                    throw std::runtime_error("LZ4 block decompression failed: block " + std::to_string(idx));
                }

                decompressed_output.insert(decompressed_output.end(),
                                          block_output.begin(), block_output.end());
                #else
                throw std::runtime_error("LZ4 not enabled");
                #endif
            } else if (algorithm == CompressionAlgorithm::NONE) {
                // No compression - just copy
                decompressed_output.insert(
                    decompressed_output.end(),
                    compressed_data.begin() + block.offset,
                    compressed_data.begin() + block.offset + block.compressed_size
                );
            } else {
                throw std::runtime_error("Unsupported compression algorithm");
            }
        }
    }

    return decompressed_output;
}

//==============================================================================
// Metadata Block Configuration
//==============================================================================

/**
 * @brief Configuration for metadata block optimization
 *
 * The metadata block system groups small scalar/array values into a single
 * compressed block to reduce overhead and cloud access requests.
 */
struct StarConfig {
    // Main data compression settings.
    //
    // Default: LZ4_SHUFFLE (fast, strong on numeric arrays) when LZ4 is compiled
    // in, else NONE. The default is chosen at compile time from the enabled
    // codecs so a build never defaults to a codec it cannot actually run — e.g. a
    // browser/WASM build without LZ4 falls back to NONE rather than producing
    // files it can't read back. NOTE: shuffle codecs are not sliceable
    // (get_slice()/is_sliceable() require a non-shuffle codec) — pass an explicit
    // NONE/GZIP/LZ4 StarConfig when you need partial reads.
    CompressionAlgorithm compression =
#ifdef ENABLE_LZ4
        CompressionAlgorithm::LZ4_SHUFFLE;
#else
        CompressionAlgorithm::NONE;
#endif
    size_t block_size = 1024 * 1024;  // 1MB default

    // Metadata block settings. The metadata block holds mixed-type, variably
    // sized values read as a unit, so the byte-shuffle prefilter does not apply —
    // use the base LZ4 codec (else NONE when LZ4 is unavailable).
    bool metadata_block_enabled = true;
    size_t metadata_max_block_size = 64 * 1024;    // 64KB max total metadata block
    CompressionAlgorithm metadata_compression =
#ifdef ENABLE_LZ4
        CompressionAlgorithm::LZ4;
#else
        CompressionAlgorithm::NONE;
#endif
    std::set<std::string> metadata_force_separate_keys;  // Keys to never store in metadata

    // Buffer management (memory optimization)
    size_t buffer_shrink_threshold = 1024 * 1024;  // 1MB - shrink buffers above this size

    // Arena allocation
    size_t arena_chunk_size = 1 * 1024 * 1024;  // 1MB chunks
};

/**
 * @brief Read-time options for StarDataset::open().
 *
 * Distinct from StarConfig, which configures how a NEW dataset is *written* by
 * create() (compression, block sizes). OpenOptions only affects how an existing
 * dataset is *read*; nothing here is persisted to the .stards file, so it is a
 * per-open setting (and can also be changed after open — see
 * StarDataset::set_layer_inheritance / set_open_options).
 */
struct OpenOptions {
    // When false (the default), LayerView lookups do NOT fall back to the base
    // layer: a key absent from the layer is reported as missing rather than
    // inheriting the base value. Set to true to opt into base-layer inheritance
    // (a key missing from a layer resolves to the base layer's value).
    bool layer_inheritance = false;

    // Whole-file prefetch threshold (bytes) for REMOTE reads (HTTP/S3). If the
    // object's size is known to be at or below this, StarDS fetches it once into
    // memory on open and serves every subsequent read from that buffer — one GET
    // instead of many ranged requests. Camera-model .stards files are small, so
    // the default (8 MiB) covers them. Set to 0 to disable the whole-file cache
    // and always use ranged reads. Local files are unaffected.
    size_t prefetch_whole_below_bytes = 8u * 1024u * 1024u;
};

/**
 * @brief Storage classification for values (DEPRECATED - use StorageLocation)
 */
enum class StorageClass {
    METADATA_BLOCK,    // Store in shared metadata block
    SEPARATE_ARRAY,    // Store as separate compressed array
    FORCE_SEPARATE     // User override to force separate storage
};

/**
 * @brief Storage location state for unified storage model
 */
enum class StorageLocation {
    PENDING,      // In-memory, not yet flushed to disk
    PERSISTED,    // Written to disk, not yet loaded in memory
    CACHED        // Loaded from disk into memory
};


/**
 * @brief Hot storage - frequently accessed data (cache-friendly)
 *
 * These fields are accessed during every get/put/contains operation.
 * Packed together for better cache locality.
 */
struct HotStorage {
    std::vector<std::string> keys;              // Key names
    std::vector<DataType> dtypes;               // Data types
    std::vector<StorageLocation> locations;     // PENDING/PERSISTED/CACHED
    std::vector<bool> dirty_flags;              // Needs flush
    std::vector<bool> loaded_flags;             // Track if loaded in memory (for efficient save_to)
    std::vector<size_t> data_indices;           // Index into data_storage (SIZE_MAX if not loaded)
};

/**
 * @brief Cold storage - infrequently accessed data
 *
 * These fields are only accessed during flush/serialization.
 * Separated from hot data to avoid cache pollution.
 */
struct ColdStorage {
    std::vector<std::vector<size_t>> shapes;            // Array dimensions
    std::vector<uint64_t> file_positions;               // Position in file
    std::vector<uint32_t> compressed_sizes;             // Compressed size
    std::vector<uint32_t> uncompressed_sizes;           // Uncompressed size
    std::vector<CompressionAlgorithm> compressions;     // Compression algorithm
    std::vector<std::vector<BlockInfo>> block_infos;    // Block metadata
    std::vector<uint8_t> stored_in_metadata_flags;      // 0=separate, 1=metadata block
};

//==============================================================================
// Metadata API - Type-Erased Value Wrapper
//==============================================================================

/**
 * @brief Type-erased wrapper for metadata values
 *
 * Provides type introspection and safe casting for metadata retrieved without
 * knowing the type ahead of time. Eliminates the need for type guessing loops.
 */
struct MetadataValue {
    ValueVariant data;              // Underlying variant storage
    DataType dtype;                 // Type information
    std::vector<size_t> shape;      // Array dimensions

    // Helper: check if scalar (shape is empty or single element)
    bool is_scalar() const {
        return shape.empty() || (shape.size() == 1 && shape[0] == 1);
    }

    // Helper: check if array (has dimensions > 0)
    bool is_array() const {
        return !shape.empty() && !(shape.size() == 1 && shape[0] == 1);
    }

    // Helper: get number of dimensions
    size_t ndim() const {
        return shape.size();
    }

    // Helper: get total number of elements
    size_t size() const {
        if (shape.empty()) return 1;
        size_t total = 1;
        for (size_t dim : shape) {
            total *= dim;
        }
        return total;
    }

    // Helper: cast to specific type (throws on mismatch)
    template<typename T>
    NDArray<T> as() const {
        auto ptr = std::get_if<NDArray<T>>(&data);
        if (!ptr) {
            throw std::runtime_error("Type mismatch in MetadataValue::as()");
        }
        return *ptr;
    }

    // Helper: try cast to type (returns nullptr on mismatch)
    template<typename T>
    std::shared_ptr<NDArray<T>> try_as() const {
        auto ptr = std::get_if<NDArray<T>>(&data);
        if (!ptr) {
            return nullptr;
        }
        return std::make_shared<NDArray<T>>(*ptr);
    }

    // Get type name (e.g. "int64", "float32", "string")
    std::string type_name() const {
        return datatype_to_string(dtype);
    }
};

// Forward declarations
class StarDataset;
class LayerView;

/**
 * @brief Accessor for metadata operations
 *
 * Provides explicit metadata API via store.meta.put() and store.meta.get()
 * for clear intent when working with metadata block items.
 */
class MetadataAccessor {
private:
    StarDataset* m_store;  // Reference to parent store
    friend class StarDataset;

public:
    explicit MetadataAccessor(StarDataset* store) : m_store(store) {}

    // Put metadata (always goes to metadata block)
    template<typename V>
    void put(const std::string& key, const V& value);

    // Get metadata without knowing type (solves type discovery!)
    std::shared_ptr<MetadataValue> get(const std::string& key);

    // Batch operations
    template<typename V>
    void put_batch(const std::map<std::string, V>& values);

    std::map<std::string, MetadataValue> get_batch(
        const std::vector<std::string>& keys);

    std::map<std::string, MetadataValue> get_all();

    // Management operations
    void remove(const std::string& key);
    void clear();
    bool contains(const std::string& key) const;
};

//==============================================================================
// LayerMetadataAccessor Class
//==============================================================================

/**
 * @brief Metadata accessor for a specific layer with inheritance
 *
 * Provides layer-specific metadata operations with automatic fallback to base layer.
 * When getting metadata, checks layer first, then falls back to base if not found.
 * When putting metadata, stores in layer-specific namespace.
 */
class LayerMetadataAccessor {
private:
    StarDataset* m_store;      // Reference to parent store
    std::string m_layer_name;  // Layer name for namespacing

    // Helper to create layer-prefixed key
    std::string make_layer_key(const std::string& key) const {
        return "__layer_" + m_layer_name + "__" + key;
    }

public:
    LayerMetadataAccessor(StarDataset* store, const std::string& layer_name)
        : m_store(store), m_layer_name(layer_name) {}

    // Method declarations (implementations after StarDataset is complete)
    template<typename V>
    void put(const std::string& key, const V& value);

    std::shared_ptr<MetadataValue> get(const std::string& key);
    bool contains(const std::string& key) const;
    void remove(const std::string& key);
    std::vector<std::string> keys() const;
};

//==============================================================================
// LayerView Class
//==============================================================================

/**
 * @brief Lightweight view into a specific layer with inheritance from base
 *
 * LayerView provides the same API as StarDataset but operates on a specific layer.
 * Keys not found in the layer automatically fall back to the base layer.
 *
 * Example:
 *   auto ds = StarDataset::open("file.stards");
 *   auto layer1 = ds->create_layer("band_0");
 *   layer1->meta.put("wavelength", NDArray<double>({}, 450.5));  // Layer-specific
 *   auto inst = layer1->meta.get("instrument");  // Falls back to base
 */
class LayerView {
private:
    std::shared_ptr<StarDataset> m_base;  // Keep dataset alive via shared_ptr
    std::string m_layer_name;              // This layer's name

public:
    // Layer-specific metadata accessor with inheritance
    LayerMetadataAccessor meta;

    LayerView(std::shared_ptr<StarDataset> base, const std::string& layer_name)
        : m_base(base), m_layer_name(layer_name), meta(base.get(), layer_name) {}

    /**
     * @brief Check if key exists in this layer or base (with inheritance)
     * @param key Key to check
     * @return true if key exists in layer or base
     */
    bool contains(const std::string& key) const;

    /**
     * @brief Get all keys in this layer (local + inherited)
     * @return Vector of key names
     */
    std::vector<std::string> keys() const;

    /**
     * @brief Get layer name
     * @return Layer name
     */
    std::string name() const { return m_layer_name; }

    /**
     * @brief Get parent dataset
     * @return Shared pointer to base StarDataset
     */
    std::shared_ptr<StarDataset> base() const { return m_base; }

    /**
     * @brief Get array from this layer with inheritance
     * @tparam T Element type
     * @param key Key to retrieve
     * @return NDArray with data from layer or base
     */
    template<typename T>
    NDArray<T> get(const std::string& key);

    /**
     * @brief Store array in this layer
     * @tparam T Element type
     * @param key Key to store
     * @param value Data to store
     */
    template<typename T>
    void put(const std::string& key, NDArray<T>&& value);
};

//==============================================================================
// StarDataset Class
//==============================================================================

/**
 * @brief A cloud-optimized binary key-value store for serializable data types
 * 
 * This class implements a binary key-value store that can persist data to disk in a 
 * cloud-optimized format. It uses a single file with an index section followed by data
 * for efficient cloud storage and retrieval. Large arrays are chunked 
 * for better performance over networks.
 */
class StarDataset : public std::enable_shared_from_this<StarDataset> {
    // Friend classes that need access to internal methods
    friend class LayerMetadataAccessor;

public:
    // Delete copy constructor and copy assignment (class contains std::atomic which is non-copyable)

    // Type aliases for iterators (now iterate over keys in m_hot)
    using iterator = std::vector<std::string>::iterator;
    using const_iterator = std::vector<std::string>::const_iterator;

    std::string m_filename;
    FileHeader m_file_header;                                       // File header with version info
    size_t m_header_size = 0; // Size of the header section in bytes
    bool m_header_dirty = false; // Flag to indicate if header size needs recalculation
    mutable std::shared_mutex m_mutex; // Mutex for thread safety

    HotStorage m_hot;                                           // Frequently accessed data
    ColdStorage m_cold;                                         // Infrequently accessed data
    mutable std::vector<ValueVariant> m_data_storage;           // Actual data when loaded (mutable for lazy loading)
    std::unordered_map<std::string, size_t> m_key_to_index;  // Fast key -> index lookup
    mutable bool m_metadata_loaded = false;                     // Metadata block loaded flag (mutable for lazy loading)

    // Layer support (v1)
    KeyRegistry m_key_registry;                                   // Global key registry (SoA)
    LayerMetadataRegistry m_layer_metadata_registry;              // Per-layer metadata info (SoA)
    mutable std::vector<bool> m_layer_metadata_loaded;            // Track which layers are loaded (mutable for lazy loading)
    mutable std::vector<std::unordered_map<uint16_t, size_t>> m_layer_metadata_indices;  // layer_idx → (key_idx → storage_idx) (mutable for lazy loading)
    std::map<std::string, std::vector<uint64_t>> m_layer_presence; // layer_name → bitmap (for data arrays)
    mutable std::unordered_map<size_t, DataType> m_metadata_dtypes;        // storage_idx → dtype for metadata entries (mutable for lazy loading)
    mutable std::unordered_map<size_t, std::vector<size_t>> m_metadata_shapes;  // storage_idx → shape for metadata entries (mutable for lazy loading)

    // Pre-allocated buffers (reduces allocations during flush)
    std::vector<char> m_serialize_buffer;                       // Reusable buffer for serialization
    std::vector<char> m_compress_buffer;                        // Reusable buffer for compression

    // Metadata API accessor
    MetadataAccessor meta;                                      // Explicit metadata operations
    FileMode m_file_mode;                                       // Read-only or read-write mode
    FilePathInfo m_path_info;                                   // Cached parsed path info

#ifdef ENABLE_S3
    // S3 support - cached for performance (avoid repeated parsing/resolution)
    std::unique_ptr<S3Credentials> m_s3_credentials;           // Cached S3 credentials
#endif

    // Configuration and state
    StarConfig m_config;                                        // Current configuration for this dataset
    OpenOptions m_open_options;                                 // Read-time options (e.g. layer inheritance)
    bool m_flushed = false;                                     // Track if already flushed (prevents redundant flushes)
    std::unique_ptr<ThreadPool> m_thread_pool;                  // Thread pool for parallel operations

    // Persistent byte-range reader (one reused connection for the whole dataset).
    // Lazily created on first read; all read paths (index, layer metadata, array
    // blocks) go through this so remote reads reuse the connection and never HEAD.
    std::unique_ptr<RangeReader> m_reader;

    // In-memory source for open_bytes(): the whole .stards image as bytes. When
    // set (MEMORY path type), reader() serves from a MemoryRangeReader over this.
    std::vector<char> m_memory_source;

    // When non-null, flush_internal() writes the assembled file image here
    // instead of to disk/S3 — this backs write_bytes(). Cleared after capture.
    std::vector<char>* m_capture_image = nullptr;

    /**
     * @brief Check if threading should be used based on workload
     * @param num_blocks Number of blocks to process
     * @param data_size Total data size in bytes
     * @return true if threading should be used, false otherwise
     */
    bool useThreading(size_t num_blocks, size_t data_size) const {
        return m_thread_pool &&
               num_blocks >= g_min_blocks_for_threading &&
               data_size >= g_min_bytes_for_threading;
    }


    /**
     * @brief Return the dataset's persistent byte-range reader, creating it on
     *        first use. One reader (= one reused connection for remote files) is
     *        shared by every read path — the index load, layer-metadata loads,
     *        and array-block reads — so remote reads never open a new connection
     *        or issue a HEAD per read.
     *
     * For small REMOTE files, this also triggers the whole-file prefetch
     * (OpenOptions::prefetch_whole_below_bytes): the object is fetched once and
     * all subsequent reads are served from memory.
     */
    RangeReader& reader() {
        if (m_reader) return *m_reader;

        switch (m_path_info.type) {
            case FilePathInfo::MEMORY:
                // Serve reads from the in-memory image supplied to open_bytes().
                m_reader = std::make_unique<MemoryRangeReader>(m_memory_source);
                break;
            case FilePathInfo::LOCAL:
                m_reader = std::make_unique<LocalRangeReader>(m_path_info.path);
                break;
            case FilePathInfo::HTTP:
#ifdef ENABLE_CURL
                m_reader = std::make_unique<HttpRangeReader>(m_path_info.path);
#else
                throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
#endif
                break;
            case FilePathInfo::S3:
#ifdef ENABLE_S3
                if (!m_s3_credentials) {
                    throw std::runtime_error("S3 credentials not available");
                }
                m_reader = std::make_unique<S3RangeReader>(
                    m_path_info.bucket, m_path_info.key, m_path_info.region,
                    *m_s3_credentials);
#else
                throw std::runtime_error("S3 support not enabled, cannot open S3 URL");
#endif
                break;
        }

        // Phase 4: whole-file fast path for small REMOTE objects. One ranged GET
        // of [0, threshold] pulls the whole object if it fits (the reader learns
        // the true size from Content-Range) and serves every later read from
        // memory — turning "header GET + N block GETs" into a single request.
        // Local files are cheap to seek, so they are not prefetched here.
        if (m_path_info.type != FilePathInfo::LOCAL &&
            m_open_options.prefetch_whole_below_bytes > 0) {
            m_reader->ensure_whole_cached(m_open_options.prefetch_whole_below_bytes);
        }
        return *m_reader;
    }

    /**
     * @brief Read exactly [position, position+len) from the backing file via the
     *        persistent reader. Returns the bytes actually read.
     */
    std::vector<char> read_range(size_t position, size_t len) {
        std::vector<char> buf;
        reader().read_at(position, len, buf);
        return buf;
    }

    /**
     * @brief Load entry from disk into memory
     * @param idx Index in SoA arrays
     */
    void load_entry(size_t idx) {
        // Check if already loaded
        if (m_hot.data_indices[idx] != SIZE_MAX) {
            return;
        }

        // If this is a metadata entry, load the entire metadata block
        if (m_cold.stored_in_metadata_flags[idx] == 1) {
            load_metadata_block();
            return;
        }

        // For array entries, load individual entry from file
        LOG_TRACE("Loading array entry at index ", idx);

        size_t position = m_cold.file_positions[idx];
        size_t compressed_size = m_cold.compressed_sizes[idx];

        // Read the entry's compressed bytes in a single ranged request via the
        // dataset's persistent reader (one reused connection, no HEAD).
        std::vector<char> compressed_data = read_range(position, compressed_size);
        if (compressed_data.size() != compressed_size) {
            throw std::runtime_error("Short read loading entry (expected " +
                std::to_string(compressed_size) + ", got " +
                std::to_string(compressed_data.size()) + ")");
        }

        // Decompress data (with parallel decompression if thread pool available)
        std::vector<char> decompressed_data = decompressBlocks(
            compressed_data,
            m_cold.block_infos[idx],
            m_cold.compressions[idx],
            {},  // decompress all blocks
            m_thread_pool.get()
        );

        // Deserialize based on type
        DataType dtype = m_hot.dtypes[idx];
        const auto& shape = m_cold.shapes[idx];

        // If a byte-shuffle prefilter was applied on write, reverse it now (the
        // decompressed bytes are byte-planes; unshuffle back to element order).
        // Only fixed-width numeric arrays are shuffled; strings never are.
        if (uses_shuffle(m_cold.compressions[idx]) && dtype != DataType::STRING) {
            size_t elem_size = datatype_size(dtype);
            if (elem_size > 1 && decompressed_data.size() % elem_size == 0) {
                size_t count = decompressed_data.size() / elem_size;
                std::vector<char> unshuffled(decompressed_data.size());
                byte_unshuffle(decompressed_data.data(), unshuffled.data(), count, elem_size);
                decompressed_data = std::move(unshuffled);
            }
        }

        std::stringstream data_stream;
        data_stream.write(decompressed_data.data(), decompressed_data.size());
        data_stream.seekg(0);

        ValueVariant value = deserialize_typed_value(data_stream, dtype, shape, decompressed_data.size());

        // Store in m_data_storage
        m_data_storage.push_back(std::move(value));
        m_hot.data_indices[idx] = m_data_storage.size() - 1;
        m_hot.loaded_flags[idx] = true;
        m_hot.locations[idx] = StorageLocation::CACHED;

        LOG_TRACE("Loaded array entry at index ", idx);
    }

    /**
     * @brief Deserializes a key from the input stream
     * @param is Input stream
     * @return Deserialized key
     */
    std::string deserializeKey(std::istream& is) {
        LOG_TRACE("Deserializing key");
        size_t len = 0;
        is.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string key(len, '\0');
        is.read(&key[0], len);
        LOG_TRACE("Deserialized key of length: ", len, " and value: ", key);
        return key;
    }
    
    /**
     * @brief Calculates the size of the header based on current index
     * @return Size of the header in bytes
     */
    size_t calculateHeaderSize() {
        LOG_TRACE("Calculating header size");

        size_t size = FileHeader::size();  // Fixed header size (31 bytes in v1)

        // Add key registry size
        for (const auto& key_name : m_key_registry.names) {
            size += sizeof(uint16_t);  // key length
            size += key_name.size();   // key string
            size += sizeof(uint64_t);  // hash
        }

        // Add layer metadata registry size
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            size += sizeof(uint16_t);  // layer name length
            size += m_layer_metadata_registry.layer_names[i].size();  // layer name
            size += sizeof(uint64_t);  // metadata_block_position
            size += sizeof(uint32_t);  // metadata_block_size
            size += sizeof(uint8_t);   // compression
            size += sizeof(uint16_t);  // metadata_key_count
            size += m_layer_metadata_registry.key_indices[i].size() * sizeof(uint16_t);  // key indices
        }

        // Count data array entries first (skip metadata entries)
        size_t data_entry_count = 0;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 0) {
                data_entry_count++;
            }
        }

        // Add layer presence bitmaps size (for data arrays only)
        size_t bitmap_words = (data_entry_count + 63) / 64;
        size += m_layer_metadata_registry.layer_names.size() * bitmap_words * sizeof(uint64_t);
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1) {
                continue;  // Skip metadata items (stored in STARMeta blocks)
            }

            // v1 format: key index (uint16_t) instead of full string
            size += sizeof(uint16_t);  // key index

            // Use IndexEntry::serialized_size() to get exact size
            IndexEntry temp_entry;
            temp_entry.position = m_cold.file_positions[i];
            temp_entry.total_bytes = m_cold.compressed_sizes[i];
            temp_entry.datatype = m_hot.dtypes[i];
            temp_entry.shape = m_cold.shapes[i];
            temp_entry.compression = m_cold.compressions[i];
            temp_entry.block_size = m_config.block_size;
            temp_entry.blocks = m_cold.block_infos[i];
            temp_entry.stored_in_metadata = false;  // Always false in v1

            size += temp_entry.serialized_size();
        }

        LOG_TRACE("Header size calculated to ", size);
        return size;
    }

    /**
     * @brief Loads the index from the file
     */
    void loadIndex() {
        LOG_TRACE("Loading index for file ", m_filename);

        // Use the dataset's persistent reader (one reused connection; no HEAD).
        if (!reader().good()) {
            return;
        }

        // First read the initial INITIAL_READ_SIZE bytes in one ranged request.
        constexpr size_t INITIAL_READ_SIZE = 2056;
        std::vector<char> initial = read_range(0, INITIAL_READ_SIZE);
        size_t bytes_read = initial.size();
        const char* initial_buffer = initial.data();

        LOG_TRACE("Read initial ", bytes_read, " bytes");

        // Too few bytes for even a header prefix: the object is absent or empty
        // (e.g. a remote path opened READ_WRITE that doesn't exist yet). Leave
        // m_header_size == 0 so open() treats it as a new file, rather than
        // throwing a spurious "magic mismatch".
        // The on-disk header_size field is a fixed uint64_t (see FileHeader::write /
        // write_u64), NOT sizeof(size_t) — those differ on 32-bit builds (wasm32),
        // so use the fixed width here and below.
        if (bytes_read < MAGIC_STRING_LENGTH + sizeof(uint8_t) + sizeof(uint64_t)) {
            LOG_TRACE("Initial read too short (", bytes_read, " bytes); treating as empty/new file");
            return;
        }

        // Create a stream from the initial buffer
        std::stringstream initial_stream;
        initial_stream.rdbuf()->sputn(initial_buffer, bytes_read);
        initial_stream.seekg(0);
        initial_stream.clear();

        char magic[MAGIC_STRING_LENGTH];
        initial_stream.read(magic, MAGIC_STRING_LENGTH);
        std::string magic_string(magic, MAGIC_STRING_LENGTH);
        LOG_TRACE("Read magic string: ", magic_string);
        if(magic_string != MAGIC_STRING) {
            LOG_ERROR("Magic string mismatch, expected ", MAGIC_STRING, " but got ", magic_string);
            throw std::runtime_error("Magic string mismatch");
        }

        // Read format
        uint8_t format_version = 1;
        initial_stream.read(reinterpret_cast<char*>(&format_version), sizeof(format_version));
        LOG_TRACE("Found format version: ", (int)format_version);
        m_file_header.format_version = format_version;

        // header_size is a fixed uint64_t on disk; read it as such (sizeof(size_t)
        // would read only 4 bytes on wasm32 and desync the stream).
        m_header_size = static_cast<size_t>(read_u64(initial_stream));
        if (m_header_size == 0) {
            LOG_ERROR("Header size is 0, file is empty");
            throw std::runtime_error("Header size is 0, file is empty");
        }
        LOG_TRACE("Found header size: ", m_header_size);
        m_file_header.header_size = m_header_size;

        std::stringstream header_stream;

        // If header is larger than our initial read, fetch the full header in one
        // more ranged request; otherwise reuse the bytes we already have.
        if (m_header_size > INITIAL_READ_SIZE) {
            LOG_TRACE("Header size (", m_header_size, ") exceeds initial read (", INITIAL_READ_SIZE, "), reading full header");
            std::vector<char> header_buffer = read_range(0, m_header_size);
            header_stream.write(header_buffer.data(), header_buffer.size());
        } else {
            // Use the data we already read
            LOG_TRACE("Using initial read for header, size: ", m_header_size);
            header_stream.write(initial_buffer, m_header_size);
        }

        // Reset stream position to after magic + format_version + header_size. The
        // on-disk header_size is a fixed uint64_t, so advance by sizeof(uint64_t)
        // (not sizeof(size_t), which is 4 on wasm32), then read entry_count as the
        // fixed uint64_t the writer emitted.
        header_stream.seekg(MAGIC_STRING_LENGTH + sizeof(format_version) + sizeof(uint64_t), std::ios::beg);
        size_t count = static_cast<size_t>(read_u64(header_stream));

        LOG_TRACE("Found ", count, " entries in index");
        m_file_header.entry_count = count;

        // Read layer_count
        uint32_t layer_count = 0;
        header_stream.read(reinterpret_cast<char*>(&layer_count), sizeof(layer_count));
        LOG_TRACE("Found ", layer_count, " layers");
        m_file_header.layer_count = layer_count;

        // Read key_registry_count
        uint32_t key_registry_count = 0;
        header_stream.read(reinterpret_cast<char*>(&key_registry_count), sizeof(key_registry_count));
        LOG_TRACE("Found ", key_registry_count, " keys in registry");
        m_file_header.key_registry_count = key_registry_count;

        // Read global key registry
        m_key_registry.names.reserve(key_registry_count);
        m_key_registry.hashes.reserve(key_registry_count);

        for (uint32_t i = 0; i < key_registry_count; ++i) {
            // Read key length
            uint16_t key_len;
            header_stream.read(reinterpret_cast<char*>(&key_len), sizeof(uint16_t));

            // Read key string
            std::string key(key_len, '\0');
            header_stream.read(&key[0], key_len);

            // Read hash
            uint64_t hash;
            header_stream.read(reinterpret_cast<char*>(&hash), sizeof(uint64_t));

            // Store in registry
            m_key_registry.names.push_back(key);
            m_key_registry.hashes.push_back(hash);
            m_key_registry.hash_to_index[hash] = i;
            m_key_registry.name_to_index[key] = i;

            LOG_TRACE("Loaded key[", i, "]: ", key);
        }

        // Read layer metadata registry (layer_count + 1 to include __base__)
        uint32_t total_layers = layer_count + 1;  // +1 for __base__ layer
        m_layer_metadata_registry.layer_names.reserve(total_layers);
        m_layer_metadata_registry.block_positions.reserve(total_layers);
        m_layer_metadata_registry.block_sizes.reserve(total_layers);
        m_layer_metadata_registry.compressions.reserve(total_layers);
        m_layer_metadata_registry.key_indices.reserve(total_layers);

        for (uint32_t i = 0; i < total_layers; ++i) {
            // Read layer name
            uint16_t layer_name_len;
            header_stream.read(reinterpret_cast<char*>(&layer_name_len), sizeof(uint16_t));
            std::string layer_name(layer_name_len, '\0');
            header_stream.read(&layer_name[0], layer_name_len);

            // Read metadata block info
            uint64_t block_position;
            header_stream.read(reinterpret_cast<char*>(&block_position), sizeof(uint64_t));

            uint32_t block_size;
            header_stream.read(reinterpret_cast<char*>(&block_size), sizeof(uint32_t));

            uint8_t compression_byte;
            header_stream.read(reinterpret_cast<char*>(&compression_byte), sizeof(uint8_t));
            CompressionAlgorithm compression = static_cast<CompressionAlgorithm>(compression_byte);

            // Read metadata key count and indices
            uint16_t metadata_key_count;
            header_stream.read(reinterpret_cast<char*>(&metadata_key_count), sizeof(uint16_t));

            std::unordered_set<uint16_t> key_indices_set;
            for (uint16_t j = 0; j < metadata_key_count; ++j) {
                uint16_t key_idx;
                header_stream.read(reinterpret_cast<char*>(&key_idx), sizeof(uint16_t));
                key_indices_set.insert(key_idx);
            }

            // Store in registry
            m_layer_metadata_registry.layer_names.push_back(layer_name);
            m_layer_metadata_registry.block_positions.push_back(block_position);
            m_layer_metadata_registry.block_sizes.push_back(block_size);
            m_layer_metadata_registry.compressions.push_back(compression);
            m_layer_metadata_registry.key_indices.push_back(std::move(key_indices_set));
            m_layer_metadata_registry.name_to_layer_index[layer_name] = i;

            LOG_TRACE("Loaded layer[", i, "]: ", layer_name, " with ", metadata_key_count, " metadata keys");
        }

        // Initialize layer metadata loaded flags
        m_layer_metadata_loaded.resize(total_layers, false);
        m_layer_metadata_indices.resize(total_layers);

        // Read layer presence bitmaps (for data arrays)
        size_t bitmap_words = (count + 63) / 64;
        for (uint32_t i = 0; i < total_layers; ++i) {
            const std::string& layer_name = m_layer_metadata_registry.layer_names[i];
            std::vector<uint64_t> bitmap(bitmap_words);
            header_stream.read(reinterpret_cast<char*>(bitmap.data()), bitmap_words * sizeof(uint64_t));
            uint64_t first_word = bitmap.empty() ? 0 : bitmap[0];
            m_layer_presence[layer_name] = std::move(bitmap);
            LOG_DEBUG("Loaded presence bitmap for layer '", layer_name, "': ", bitmap_words, " words, first word = ",
                     first_word);
        }

        // Reserve capacity to prevent reallocation (critical for string_view validity)
        m_hot.keys.reserve(count);
        m_hot.dtypes.reserve(count);
        m_hot.locations.reserve(count);
        m_hot.dirty_flags.reserve(count);
        m_hot.loaded_flags.reserve(count);
        m_hot.data_indices.reserve(count);
        m_cold.shapes.reserve(count);
        m_cold.file_positions.reserve(count);
        m_cold.compressed_sizes.reserve(count);
        m_cold.uncompressed_sizes.reserve(count);
        m_cold.compressions.reserve(count);
        m_cold.block_infos.reserve(count);
        m_cold.stored_in_metadata_flags.reserve(count);

        for (size_t i = 0; i < count; i++) {
            LOG_DEBUG("Reading index entry ", i, " of ", count, ", stream position: ", header_stream.tellg());

            // v1 format: Read key index instead of full string
            uint16_t key_idx;
            header_stream.read(reinterpret_cast<char*>(&key_idx), sizeof(uint16_t));
            LOG_DEBUG("Read key_idx=", key_idx);

            // Look up key name from registry
            if (key_idx >= m_key_registry.names.size()) {
                throw std::runtime_error("Invalid key index in file");
            }
            std::string key = m_key_registry.names[key_idx];
            LOG_TRACE("Key name: ", key);

            // Read IndexEntry
            IndexEntry entry;
            entry.read(header_stream);

            // Skip metadata-only entries - they exist only in layer metadata system
            if (entry.stored_in_metadata) {
                LOG_TRACE("Skipping metadata-only entry: ", key);
                continue;
            }

            // Populate SoA arrays from IndexEntry (only for data arrays, not metadata)
            size_t idx = m_hot.keys.size();

            m_hot.keys.push_back(key);
            m_hot.dtypes.push_back(entry.datatype);
            m_hot.locations.push_back(StorageLocation::PERSISTED);
            m_hot.dirty_flags.push_back(false);
            m_hot.loaded_flags.push_back(false);
            m_hot.data_indices.push_back(SIZE_MAX);  // Not loaded yet

            // Calculate uncompressed size from blocks
            size_t uncompressed_size = 0;
            for (const auto& block : entry.blocks) {
                uncompressed_size += block.uncompressed_size;
            }

            m_cold.shapes.push_back(entry.shape);
            m_cold.file_positions.push_back(entry.position);
            m_cold.compressed_sizes.push_back(entry.total_bytes);
            m_cold.uncompressed_sizes.push_back(uncompressed_size);
            m_cold.compressions.push_back(entry.compression);
            m_cold.block_infos.push_back(entry.blocks);
            m_cold.stored_in_metadata_flags.push_back(entry.stored_in_metadata ? 1 : 0);

            m_key_to_index[m_hot.keys[idx]] = idx;

            LOG_TRACE("Loaded key ", key, " at index ", idx, " with position ", entry.position, " and bytes ", entry.total_bytes);
        }

        m_header_dirty = false;
        m_file_header.entry_count = count;
        LOG_TRACE("Index loaded successfully with ", count, " entries");

        // v1 format: Load base layer metadata eagerly for compatibility
        // Other layers are lazy-loaded via ensure_layer_metadata_loaded()
        if (total_layers > 0) {
            load_layer_metadata("__base__");
            m_metadata_loaded = true;  // Mark as loaded to prevent redundant loads in save_to()
        }
    }

    /**
     * @brief Helper to serialize metadata value (string specialization)
     */
#ifndef SWIG
    template<typename T>
    typename std::enable_if<std::is_same<T, std::string>::value, void>::type
    serialize_metadata_value(std::ostream& os, const std::vector<T>& data) const {
        // Strings: write total data length first, then each string with length prefix
        // Calculate total length: sum of (4 bytes for length + string bytes) for each string
        uint32_t total_len = 0;
        for (const auto& str : data) {
            total_len += 4 + static_cast<uint32_t>(str.size());
        }
        os.write(reinterpret_cast<const char*>(&total_len), 4);

        // Now write each string with its length prefix
        for (const auto& str : data) {
            uint32_t str_len = static_cast<uint32_t>(str.size());
            os.write(reinterpret_cast<const char*>(&str_len), 4);
            os.write(str.data(), str_len);
        }
    }

    /**
     * @brief Helper to serialize metadata value (numeric types)
     */
    template<typename T>
    typename std::enable_if<!std::is_same<T, std::string>::value, void>::type
    serialize_metadata_value(std::ostream& os, const std::vector<T>& data) const {
        // Numeric types: write raw bytes only (length is calculated from shape on deserialize)
        uint32_t data_len = static_cast<uint32_t>(data.size() * sizeof(T));
        os.write(reinterpret_cast<const char*>(data.data()), data_len);
    }

    /**
     * @brief Serialize an NDArray's element data into a byte buffer for block
     *        storage, in the SAME wire format deserialize_typed_value() reads.
     *
     * Numeric types are raw contiguous bytes; std::string arrays are
     * length-prefixed (uint32 total length, then per-element uint32 length +
     * bytes). Using this everywhere fixes the bug where string arrays were
     * memcpy'd as raw std::string OBJECTS (pointers/SSO) and came back empty
     * after reload. For numeric types the bytes are identical to the old memcpy.
     */
    template<typename T>
    std::vector<char> serialize_array_data(const std::vector<T>& data) const {
        std::string buf;
        {
            std::ostringstream oss(std::ios::binary);
            serialize_metadata_value<T>(oss, data);
            buf = oss.str();
        }
        return std::vector<char>(buf.begin(), buf.end());
    }

    /**
     * @brief Serialize array data, applying the byte-shuffle prefilter if the
     *        codec is a *_SHUFFLE variant.
     *
     * Shuffle is only valid for fixed-width numeric elements laid out as raw
     * contiguous bytes; std::string arrays are length-prefixed and variable
     * width, so they are never shuffled (they store fine under the base codec).
     */
    template<typename T>
    std::vector<char> serialize_array_data(const std::vector<T>& data,
                                           CompressionAlgorithm codec) const {
        std::vector<char> buf = serialize_array_data<T>(data);
        if constexpr (!std::is_same<T, std::string>::value) {
            if (uses_shuffle(codec) && sizeof(T) > 1 && !data.empty()) {
                std::vector<char> shuffled(buf.size());
                byte_shuffle(buf.data(), shuffled.data(), data.size(), sizeof(T));
                return shuffled;
            }
        }
        return buf;
    }
#endif

    /**
     * @brief Serializes a single layer's metadata in STARMeta format (v1)
     * @param os Output stream
     * @param layer_name Layer name
     */
    void serialize_layer_metadata_block(std::ostream& os, const std::string& layer_name) {
        LOG_DEBUG("serialize_layer_metadata_block: layer_name=", layer_name);

        // Ensure this layer's metadata is loaded before serializing
        size_t layer_idx = m_layer_metadata_registry.get_layer_index(layer_name);
        if (!m_layer_metadata_loaded[layer_idx]) {
            // Only try to load if there's actually a block to load
            uint32_t block_size = m_layer_metadata_registry.block_sizes[layer_idx];
            if (block_size > 0) {
                // Load it now (this happens for layers that weren't accessed during this session)
                load_layer_metadata(layer_name);
            } else {
                // New layer with no persisted data - just mark as loaded
                m_layer_metadata_loaded[layer_idx] = true;
            }
        }

        // Magic header: "STARMETA" (8 bytes, not null-terminated)
        const char magic[8] = {'S','T','A','R','M','E','T','A'};
        os.write(magic, 8);

        // Version
        uint8_t version = 1;
        os.write(reinterpret_cast<const char*>(&version), sizeof(uint8_t));

        // Layer name
        uint16_t layer_name_len = static_cast<uint16_t>(layer_name.size());
        os.write(reinterpret_cast<const char*>(&layer_name_len), sizeof(uint16_t));
        os.write(layer_name.c_str(), layer_name_len);

        // Count entries for this layer
        uint16_t entry_count = static_cast<uint16_t>(m_layer_metadata_registry.key_indices[layer_idx].size());
        LOG_DEBUG("  entry_count=", entry_count);
        os.write(reinterpret_cast<const char*>(&entry_count), sizeof(uint16_t));

        // Serialize each entry
        for (uint16_t key_idx : m_layer_metadata_registry.key_indices[layer_idx]) {
            LOG_DEBUG("  Serializing key_idx=", key_idx);

            // Write key index
            os.write(reinterpret_cast<const char*>(&key_idx), sizeof(uint16_t));

            // Find this key in layer metadata indices to get its storage index
            if (key_idx >= m_key_registry.names.size()) {
                throw std::runtime_error("Invalid key_idx during serialization: " + std::to_string(key_idx));
            }
            const std::string& key = m_key_registry.names[key_idx];
            LOG_DEBUG("    key=", key);

            auto it = m_layer_metadata_indices[layer_idx].find(key_idx);
            if (it == m_layer_metadata_indices[layer_idx].end()) {
                throw std::runtime_error("Key not found in layer metadata indices during serialization: " + key);
            }
            size_t storage_idx = it->second;
            LOG_DEBUG("    storage_idx=", storage_idx);

            // Get data from storage
            if (storage_idx >= m_data_storage.size()) {
                throw std::runtime_error("Invalid storage index for metadata: " + key);
            }
            const ValueVariant& variant = m_data_storage[storage_idx];

            // Serialize dtype, shape, and data
            std::visit([&](auto&& arr) {
                using T = typename std::decay_t<decltype(arr)>::value_type;
                DataType dtype = TypeToDataType<T>::value;
                write_u8(os, static_cast<uint8_t>(dtype));

                // Shape: 1-byte ndim, then each dimension as a fixed u64 (a raw
                // size_t would be 4 bytes on 32-bit hosts, breaking portability).
                const auto& arr_shape = arr.shape();
                write_u8(os, static_cast<uint8_t>(arr_shape.size()));
                for (size_t dim : arr_shape) {
                    write_u64(os, dim);
                }

                // Data
                serialize_metadata_value<T>(os, arr.data());
            }, variant);
        }
    }

    /**
     * @brief Serializes metadata block to stream (OLD FORMAT - deprecated)
     * @param os Output stream
     */
    void serialize_metadata_block(std::ostream& os) const {
        // Magic header
        const char magic[8] = {'S','T','A','R','M','E','T','A'};
        os.write(magic, 8);

        // Format version
        write_u8(os, 1);

        // Count entries stored in metadata block (using SoA)
        uint32_t count = 0;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1 && m_hot.data_indices[i] != SIZE_MAX) {
                count++;
            }
        }
        write_u32(os, count);

        // Serialize all metadata block entries from SoA
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            // Only serialize items in metadata block with loaded data
            if (m_cold.stored_in_metadata_flags[i] != 1 || m_hot.data_indices[i] == SIZE_MAX) {
                continue;
            }

            const std::string& key = m_hot.keys[i];
            const auto& variant = m_data_storage[m_hot.data_indices[i]];

            // Key
            write_u16(os, static_cast<uint16_t>(key.size()));
            os.write(key.data(), static_cast<std::streamsize>(key.size()));

            // Type and value (visit variant)
            std::visit([&](auto&& arr) {
                using T = typename std::decay_t<decltype(arr)>::value_type;
                DataType dtype = TypeToDataType<T>::value;
                write_u8(os, static_cast<uint8_t>(dtype));

                // Shape: 1-byte ndim, then each dimension as a fixed u64.
                const auto& arr_shape = arr.shape();
                write_u8(os, static_cast<uint8_t>(arr_shape.size()));
                for (size_t dim : arr_shape) {
                    write_u64(os, dim);
                }

                // Data bytes - different handling for strings vs numeric types
                serialize_metadata_value<T>(os, arr.data());
            }, variant);
        }
    }

    /**
     * @brief Deserializes typed value from stream
     * @param is Input stream
     * @param dtype Data type
     * @param shape Array shape
     * @param data_len Data length in bytes
     * @return ValueVariant containing the deserialized value
     */
    ValueVariant deserialize_typed_value(std::istream& is, DataType dtype,
                                        const std::vector<size_t>& shape,
                                        size_t data_len) {
        // Guard against corrupt files where the declared byte length is larger
        // than the buffer the shape allocates. Reading data_len bytes into an
        // NDArray sized from `shape` would otherwise overflow the heap buffer.
        // Strings are excluded: they carry their own per-element length prefixes
        // and do not use data_len for a bulk read.
        if (dtype != DataType::STRING) {
            size_t elem_count = 1;
            for (size_t dim : shape) {
                elem_count *= dim;
            }
            size_t capacity_bytes = elem_count * datatype_size(dtype);
            if (data_len > capacity_bytes) {
                throw std::runtime_error(
                    "Corrupt entry: declared data length (" + std::to_string(data_len) +
                    " bytes) exceeds capacity implied by shape (" +
                    std::to_string(capacity_bytes) + " bytes)");
            }
        }

        switch (dtype) {
            case DataType::INT8: {
                NDArray<int8_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::INT16: {
                NDArray<int16_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::INT32: {
                NDArray<int32_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::INT64: {
                NDArray<int64_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::UINT8: {
                NDArray<uint8_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::UINT16: {
                NDArray<uint16_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::UINT32: {
                NDArray<uint32_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::UINT64: {
                NDArray<uint64_t> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::FLOAT32: {
                NDArray<float> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::FLOAT64: {
                NDArray<double> arr(shape);
                is.read(reinterpret_cast<char*>(arr.data().data()), data_len);
                return arr;
            }
            case DataType::STRING: {
                // Skip total length field (written by serialize_metadata_value)
                uint32_t total_len;
                is.read(reinterpret_cast<char*>(&total_len), 4);

                NDArray<std::string> arr(shape);
                auto& arr_data = arr.data();
                for (auto& str : arr_data) {
                    uint32_t str_len;
                    is.read(reinterpret_cast<char*>(&str_len), 4);
                    str.resize(str_len);
                    is.read(&str[0], str_len);
                }
                return arr;
            }
            default:
                throw std::runtime_error("Unsupported metadata block type: " +
                                       std::to_string(static_cast<int>(dtype)));
        }
    }

    /**
     * @brief Loads metadata block from file if not already loaded (supports HTTP/remote URLs)
     */
    /**
     * @brief Ensure a specific layer's metadata is loaded (v3 format)
     * @param layer_idx Index of the layer to load
     */
    void ensure_layer_metadata_loaded(size_t layer_idx) {
        if (m_layer_metadata_loaded[layer_idx]) {
            return;  // Already loaded
        }
        load_layer_metadata(m_layer_metadata_registry.layer_names[layer_idx]);
    }

    /**
     * @brief Load a specific layer's metadata block (v3 format)
     * @param layer_name Name of the layer to load
     */
    void load_layer_metadata(const std::string& layer_name) {
        size_t layer_idx = m_layer_metadata_registry.get_layer_index(layer_name);

        if (m_layer_metadata_loaded[layer_idx]) {
            return;  // Already loaded
        }

        LOG_DEBUG("Loading metadata for layer: ", layer_name);

        // Get layer metadata info
        uint64_t position = m_layer_metadata_registry.block_positions[layer_idx];
        uint32_t size = m_layer_metadata_registry.block_sizes[layer_idx];
        CompressionAlgorithm compression = m_layer_metadata_registry.compressions[layer_idx];

        if (size == 0) {
            LOG_DEBUG("Layer has no metadata block: ", layer_name);
            m_layer_metadata_loaded[layer_idx] = true;
            return;
        }

        // Read the compressed metadata block in one ranged request via the
        // dataset's persistent reader (one reused connection, no HEAD).
        std::vector<char> compressed = read_range(position, size);
        if (compressed.size() != size) {
            throw std::runtime_error("Short read loading layer metadata for '" +
                layer_name + "' (expected " + std::to_string(size) + ", got " +
                std::to_string(compressed.size()) + ")");
        }

        // Decompress
        std::vector<char> decompressed = decompress_single_block(compressed, compression);

        // Parse STARMeta block
        parse_layer_metadata_block(layer_idx, decompressed);

        m_layer_metadata_loaded[layer_idx] = true;
        LOG_DEBUG("Loaded metadata for layer: ", layer_name);
    }

    /**
     * @brief Load all layer metadata blocks at once (v3 format)
     */
    void load_all_metadata() {
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            if (!m_layer_metadata_loaded[i]) {
                load_layer_metadata(m_layer_metadata_registry.layer_names[i]);
            }
        }
    }

    /**
     * @brief Parse a layer's metadata block (STARMeta format)
     * @param layer_idx Layer index
     * @param decompressed Decompressed block data
     */
    void parse_layer_metadata_block(size_t layer_idx, const std::vector<char>& decompressed) {
        std::stringstream stream(std::string(decompressed.begin(), decompressed.end()));

        // Read magic
        char magic[8];
        stream.read(magic, 8);
        if (std::memcmp(magic, "STARMETA", 8) != 0) {
            throw std::runtime_error("Invalid STARMETA magic");
        }

        // Read version
        uint8_t version;
        stream.read(reinterpret_cast<char*>(&version), sizeof(uint8_t));

        // Read layer name
        uint16_t layer_name_len = read_u16(stream);
        std::string layer_name(layer_name_len, '\0');
        stream.read(&layer_name[0], layer_name_len);

        // Read entry count
        uint16_t entry_count = read_u16(stream);

        LOG_TRACE("Parsing STARMeta block for layer ", layer_name, " with ", entry_count, " entries");

        // Parse each entry
        for (uint16_t i = 0; i < entry_count; ++i) {
            // Read key index
            uint16_t key_index = read_u16(stream);
            LOG_TRACE("  Entry ", i, ": key_index=", key_index);

            // Read data type
            uint8_t dtype_byte = read_u8(stream);
            LOG_TRACE("  Entry ", i, ": dtype_byte=", static_cast<int>(dtype_byte));
            DataType dtype = static_cast<DataType>(dtype_byte);

            // Read shape: 1-byte ndim, then each dimension as a fixed u64
            // (matches serialize_layer_metadata_block()).
            uint8_t ndim = read_u8(stream);
            std::vector<size_t> shape(ndim);
            for (auto& dim : shape) {
                dim = static_cast<size_t>(read_u64(stream));
            }

            // Calculate data size based on dtype and shape
            size_t num_elements = 1;
            for (size_t dim : shape) {
                num_elements *= dim;
            }

            size_t data_len = 0;
            if (dtype == DataType::STRING) {
                // For strings, we'll read each string's length prefix in deserialize_typed_value
                // Pass a dummy value - the function will read lengths dynamically
                data_len = num_elements;
            } else {
                // For numeric types, calculate exact byte count
                size_t element_size = 0;
                switch (dtype) {
                    case DataType::INT8: case DataType::UINT8: element_size = 1; break;
                    case DataType::INT16: case DataType::UINT16: element_size = 2; break;
                    case DataType::INT32: case DataType::UINT32: case DataType::FLOAT32: element_size = 4; break;
                    case DataType::INT64: case DataType::UINT64: case DataType::FLOAT64: element_size = 8; break;
                    default: throw std::runtime_error("Unknown data type in metadata deserialization");
                }
                data_len = num_elements * element_size;
            }

            // Deserialize value
            auto value = deserialize_typed_value(stream, dtype, shape, data_len);

            // Store in m_data_storage
            size_t storage_idx = m_data_storage.size();
            m_data_storage.push_back(std::move(value));

            // Store dtype and shape for metadata (use map since storage_idx might have gaps)
            m_metadata_dtypes[storage_idx] = dtype;
            m_metadata_shapes[storage_idx] = shape;

            // Track in layer metadata indices (separate namespace from arrays)
            m_layer_metadata_indices[layer_idx][key_index] = storage_idx;
        }
    }

    /**
     * @brief Compress a single block (helper for layer metadata)
     * @param uncompressed Uncompressed data
     * @param compression Compression algorithm
     * @return Compressed data
     */
    std::vector<char> compress_single_block(const std::vector<char>& uncompressed, CompressionAlgorithm compression) {
        if (compression == CompressionAlgorithm::NONE) {
            return uncompressed;
        }

#ifdef ENABLE_ZLIB
        if (compression == CompressionAlgorithm::GZIP) {
            uLongf dest_len = compressBound(uncompressed.size());
            std::vector<char> compressed(dest_len);

            int result = compress2(reinterpret_cast<Bytef*>(compressed.data()), &dest_len,
                                  reinterpret_cast<const Bytef*>(uncompressed.data()), uncompressed.size(),
                                  Z_DEFAULT_COMPRESSION);
            if (result != Z_OK) {
                throw std::runtime_error("GZIP compression failed");
            }

            compressed.resize(dest_len);
            return compressed;
        }
#endif

#ifdef ENABLE_LZ4
        if (compression == CompressionAlgorithm::LZ4) {
            int max_compressed = LZ4_compressBound(uncompressed.size());
            std::vector<char> compressed(sizeof(uint64_t) + max_compressed);

            // Store original size in first 8 bytes
            uint64_t original_size = uncompressed.size();
            std::memcpy(compressed.data(), &original_size, sizeof(uint64_t));

            int compressed_size = LZ4_compress_default(uncompressed.data(),
                                                      compressed.data() + sizeof(uint64_t),
                                                      uncompressed.size(),
                                                      max_compressed);
            if (compressed_size <= 0) {
                throw std::runtime_error("LZ4 compression failed");
            }

            compressed.resize(sizeof(uint64_t) + compressed_size);
            return compressed;
        }
#endif

        throw std::runtime_error("Unsupported compression algorithm");
    }

    /**
     * @brief Decompress a single block (helper for layer metadata)
     * @param compressed Compressed data
     * @param compression Compression algorithm
     * @return Decompressed data
     */
    std::vector<char> decompress_single_block(const std::vector<char>& compressed, CompressionAlgorithm compression) {
        LOG_DEBUG("decompress_single_block called, compressed size: ", compressed.size(), " compression: ", static_cast<int>(compression));

        if (compression == CompressionAlgorithm::NONE) {
            return compressed;
        }

#ifdef ENABLE_ZLIB
        if (compression == CompressionAlgorithm::GZIP) {
            std::vector<char> decompressed;

            // Try progressively larger buffers if needed. The ceiling lets the
            // multiplier reach 1280x, which exceeds DEFLATE's ~1032:1 maximum
            // compression ratio, so valid high-ratio blocks are not rejected
            // while the buffer stays bounded.
            for (size_t multiplier = 10; multiplier <= 2048; multiplier *= 2) {
                uLongf dest_len = compressed.size() * multiplier;
                decompressed.resize(dest_len);

                int result = uncompress(reinterpret_cast<Bytef*>(decompressed.data()), &dest_len,
                                       reinterpret_cast<const Bytef*>(compressed.data()), compressed.size());

                if (result == Z_OK) {
                    decompressed.resize(dest_len);
                    return decompressed;
                } else if (result != Z_BUF_ERROR) {
                    // Other error (not buffer size)
                    std::cerr << "[ERROR] GZIP decompression failed: code=" << result
                              << " compressed_size=" << compressed.size()
                              << " dest_len=" << dest_len << std::endl;
                    // Print first few bytes of compressed data for debugging
                    std::cerr << "[ERROR] First 16 bytes: ";
                    for (size_t i = 0; i < std::min(size_t(16), compressed.size()); ++i) {
                        std::cerr << std::hex << (int)(unsigned char)compressed[i] << " ";
                    }
                    std::cerr << std::dec << std::endl;
                    throw std::runtime_error("GZIP decompression failed with error code: " + std::to_string(result));
                }
                // Z_BUF_ERROR: try larger buffer
            }

            throw std::runtime_error("GZIP decompression failed: buffer too small even after trying large buffers");
        }
#endif

#ifdef ENABLE_LZ4
        if (compression == CompressionAlgorithm::LZ4) {
            // Guard against truncated blocks before reading the 8-byte size
            // header: prevents an out-of-bounds read here and the size_t
            // underflow of compressed.size() - sizeof(uint64_t) below.
            if (compressed.size() < sizeof(uint64_t)) {
                throw std::runtime_error("LZ4 block too small to contain size header");
            }

            // Read original size from first 8 bytes
            uint64_t original_size;
            std::memcpy(&original_size, compressed.data(), sizeof(uint64_t));

            std::vector<char> decompressed(original_size);
            int result = LZ4_decompress_safe(compressed.data() + sizeof(uint64_t),
                                            decompressed.data(),
                                            compressed.size() - sizeof(uint64_t),
                                            original_size);
            if (result < 0) {
                throw std::runtime_error("LZ4 decompression failed");
            }

            return decompressed;
        }
#endif

        throw std::runtime_error("Unsupported compression algorithm");
    }

    // v1 load_metadata_block - loads base layer metadata
    void load_metadata_block() {
        if (m_metadata_loaded) {
            return;
        }

        // In v1 format, just load the base layer
        try {
            load_layer_metadata("__base__");
            m_metadata_loaded = true;
        } catch (const std::exception& e) {
            // If base layer doesn't exist or has no metadata, that's okay
            LOG_TRACE("Could not load base layer metadata: ", e.what());
            m_metadata_loaded = true;
        }
    }

    /**
     * @brief Checks if file has metadata block
     * @return True if metadata block exists
     */
    bool has_metadata_block() const {
        // Check if any entry is stored in metadata block
        for (size_t i = 0; i < m_cold.stored_in_metadata_flags.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1) {
                return true;
            }
        }
        return false;
    }

    void print_header() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        std::cout << "==== STAR File Header ====" << std::endl;
        std::cout << "Filename: " << m_filename << std::endl;
        std::cout << "Header size: " << m_header_size << std::endl;
        std::cout << "Entry count: " << m_hot.keys.size() << std::endl;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            std::cout << "  [" << i << "] Key: \"" << m_hot.keys[i] << "\"" << std::endl;
            std::cout << "      Type: " << static_cast<int>(m_hot.dtypes[i]) << std::endl;
            std::cout << "      In metadata: " << (m_cold.stored_in_metadata_flags[i] ? "yes" : "no") << std::endl;
        }
        std::cout << "=========================" << std::endl;
    }


    /**
     * @brief Writes all data to the file (including metadata block)
     */
    void flush() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        // An explicit flush() on a read-only dataset is a caller error: the
        // request is to persist changes, which cannot be done to a read-only
        // source. (Automatic cleanup flushes from close()/the destructor use the
        // quiet path and do NOT throw — see flush_quiet().)
        if (m_file_mode == FileMode::READ_ONLY) {
            throw std::runtime_error(
                "Cannot flush a dataset opened in read-only mode. "
                "Use save_to() with a different path to persist changes.");
        }
        flush_internal();
    }

    /**
     * @brief Flush without throwing on read-only (silently skips instead).
     *
     * Used by close() and the destructor, where flushing is best-effort cleanup
     * rather than an explicit persist request — a read-only dataset must be
     * destructible without raising.
     */
    void flush_quiet() {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        flush_internal();  // flush_internal() already no-ops in read-only mode
    }

private:
    /**
     * @brief Internal flush implementation (no closed state check, no locking)
     * Must be called with m_mutex already held
     * Used by close() which already holds the lock
     */
    void flush_internal() {
        // Caller must hold m_mutex
        LOG_DEBUG("flush_internal() called");

        // Early return if already flushed
        if (m_flushed) {
            LOG_TRACE("flush() - already flushed, skipping");
            return;
        }
        LOG_DEBUG("Proceeding with flush");

        // Check read-only mode
        if (m_file_mode == FileMode::READ_ONLY) {
            LOG_DEBUG("File opened in read-only mode, skipping flush");
            return;  // Don't throw, just skip flush
        }

        LOG_DEBUG("flush() starting for ", m_filename);

        // Collect dirty entries
        std::vector<size_t> dirty_indices;
        std::vector<size_t> metadata_indices;

        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (!m_hot.dirty_flags[i]) continue;

            if (m_cold.stored_in_metadata_flags[i]) {
                metadata_indices.push_back(i);
            } else {
                dirty_indices.push_back(i);
            }
        }

        LOG_DEBUG("flush() - found ", dirty_indices.size(), " dirty entries, ",
                  metadata_indices.size(), " metadata entries");


        // Simple data structure for metadata
        struct ArrayMeta {
            std::vector<BlockInfo> block_infos;
            size_t compressed_size = 0;
            std::vector<char> serialized_data;  // Cache serialized data (only for small arrays)
        };

        std::vector<ArrayMeta> array_metadata(m_hot.keys.size());

        // Only cache arrays smaller than this threshold (10MB) to avoid memory pressure
        const size_t CACHE_THRESHOLD = 10 * 1024 * 1024;

        // Parallel estimation for dirty arrays
        if (m_thread_pool && !dirty_indices.empty()) {
            m_thread_pool->parallel_for(0, dirty_indices.size(), [&](size_t i) {
                size_t idx = dirty_indices[i];

                if (m_hot.data_indices[idx] == SIZE_MAX) {
                    LOG_WARN("Skipping dirty entry '", m_hot.keys[idx], "' - data not loaded");
                    return;
                }

                std::vector<char> serialize_buffer;
                const ValueVariant& var = m_data_storage[m_hot.data_indices[idx]];
                std::visit([this, &serialize_buffer](auto&& arr) {
                    using T = typename std::decay_t<decltype(arr)>::value_type;
                    // Length-prefixes strings; raw bytes for numerics; byte-shuffle
                    // prefilter applied for *_SHUFFLE codecs (numeric only).
                    serialize_buffer = serialize_array_data<T>(arr.data(), m_config.compression);
                }, var);

                // Estimate compressed size (NO COMPRESSION!)
                size_t estimated_size = estimateCompressedSize(
                    serialize_buffer.data(),
                    serialize_buffer.size(),
                    m_config.compression,
                    m_config.block_size
                );

                // Create estimated block infos
                size_t num_blocks = (serialize_buffer.size() + m_config.block_size - 1) / m_config.block_size;
                std::vector<BlockInfo> estimated_blocks(num_blocks);

                size_t offset_in = 0;
                size_t offset_out = 0;
                for (size_t b = 0; b < num_blocks; b++) {
                    size_t block_uncompressed = std::min(m_config.block_size, serialize_buffer.size() - offset_in);

                    #ifdef ENABLE_ZLIB
                    size_t block_bound = deflateBound(nullptr, block_uncompressed);
                    #else
                    size_t block_bound = block_uncompressed;
                    #endif

                    estimated_blocks[b].offset = offset_out;
                    estimated_blocks[b].uncompressed_size = block_uncompressed;
                    estimated_blocks[b].compressed_size = block_bound;

                    offset_in += block_uncompressed;
                    offset_out += block_bound;
                }

                // Store ESTIMATED metadata and conditionally cache serialized data
                array_metadata[idx].block_infos = std::move(estimated_blocks);
                array_metadata[idx].compressed_size = estimated_size;

                // Only cache small arrays to avoid memory pressure
                if (serialize_buffer.size() <= CACHE_THRESHOLD) {
                    array_metadata[idx].serialized_data = std::move(serialize_buffer);
                }
            });
        } else {
            // Serial fallback
            for (size_t idx : dirty_indices) {
                if (m_hot.data_indices[idx] == SIZE_MAX) {
                    LOG_WARN("Skipping dirty entry '", m_hot.keys[idx], "' - data not loaded");
                    continue;
                }

                // Same logic as parallel, but using member buffers
                m_serialize_buffer.clear();
                const ValueVariant& var = m_data_storage[m_hot.data_indices[idx]];
                std::visit([this](auto&& arr) {
                    using T = typename std::decay_t<decltype(arr)>::value_type;
                    // Length-prefixes strings; raw bytes for numerics; byte-shuffle
                    // prefilter applied for *_SHUFFLE codecs (numeric only).
                    m_serialize_buffer = serialize_array_data<T>(arr.data(), m_config.compression);
                }, var);

                size_t estimated_size = estimateCompressedSize(
                    m_serialize_buffer.data(),
                    m_serialize_buffer.size(),
                    m_config.compression,
                    m_config.block_size
                );

                size_t num_blocks = (m_serialize_buffer.size() + m_config.block_size - 1) / m_config.block_size;
                std::vector<BlockInfo> estimated_blocks(num_blocks);

                size_t offset_in = 0;
                size_t offset_out = 0;
                for (size_t b = 0; b < num_blocks; b++) {
                    size_t block_uncompressed = std::min(m_config.block_size, m_serialize_buffer.size() - offset_in);

                    #ifdef ENABLE_ZLIB
                    size_t block_bound = deflateBound(nullptr, block_uncompressed);
                    #else
                    size_t block_bound = block_uncompressed;
                    #endif

                    estimated_blocks[b].offset = offset_out;
                    estimated_blocks[b].uncompressed_size = block_uncompressed;
                    estimated_blocks[b].compressed_size = block_bound;

                    offset_in += block_uncompressed;
                    offset_out += block_bound;
                }

                array_metadata[idx].block_infos = std::move(estimated_blocks);
                array_metadata[idx].compressed_size = estimated_size;

                // Only cache small arrays
                if (m_serialize_buffer.size() <= CACHE_THRESHOLD) {
                    array_metadata[idx].serialized_data = m_serialize_buffer;
                }
            }
        }

        // For non-dirty arrays: copy metadata from existing index
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (!m_hot.dirty_flags[i] && !m_cold.stored_in_metadata_flags[i]) {
                array_metadata[i].block_infos = m_cold.block_infos[i];
                array_metadata[i].compressed_size = m_cold.compressed_sizes[i];
            }
        }

        // Estimate metadata block size and create estimated BlockInfo structures
        std::string metadata_str;
        size_t metadata_estimated_size = 0;
        std::vector<BlockInfo> metadata_estimated_blocks;
        if (!metadata_indices.empty()) {
            std::ostringstream metadata_stream;
            serialize_metadata_block(metadata_stream);
            metadata_str = metadata_stream.str();
            metadata_estimated_size = estimateCompressedSize(
                metadata_str.data(),
                metadata_str.size(),
                m_config.metadata_compression,
                m_config.metadata_max_block_size
            );

            // Create estimated BlockInfo structures for metadata block
            size_t num_metadata_blocks = (metadata_str.size() + m_config.metadata_max_block_size - 1) / m_config.metadata_max_block_size;
            metadata_estimated_blocks.resize(num_metadata_blocks);

            size_t offset_in = 0;
            size_t offset_out = 0;
            for (size_t b = 0; b < num_metadata_blocks; b++) {
                size_t block_uncompressed = std::min(m_config.metadata_max_block_size, metadata_str.size() - offset_in);

                #ifdef ENABLE_ZLIB
                size_t block_bound = deflateBound(nullptr, block_uncompressed);
                #else
                size_t block_bound = block_uncompressed;
                #endif

                metadata_estimated_blocks[b].offset = offset_out;
                metadata_estimated_blocks[b].uncompressed_size = block_uncompressed;
                metadata_estimated_blocks[b].compressed_size = block_bound;

                offset_in += block_uncompressed;
                offset_out += block_bound;
            }
        }

        LOG_DEBUG("flush() - Phase 1 complete: estimated sizes using deflateBound");

        // Update SoA with estimated metadata for arrays
        for (size_t i = 0; i < array_metadata.size(); i++) {
            if (!array_metadata[i].block_infos.empty()) {
                m_cold.block_infos[i] = array_metadata[i].block_infos;
                m_cold.compressed_sizes[i] = array_metadata[i].compressed_size;
            }
        }

        // Set estimated metadata block info for all metadata entries BEFORE calculating header size
        // This ensures calculateHeaderSize() uses correct block counts
        if (!metadata_indices.empty()) {
            for (size_t idx : metadata_indices) {
                m_cold.block_infos[idx] = metadata_estimated_blocks;
                m_cold.compressed_sizes[idx] = metadata_estimated_size;
                m_cold.compressions[idx] = m_config.metadata_compression;
            }
        }

        // Calculate header size (will use estimated block_infos)
        size_t header_size = calculateHeaderSize();

        // Calculate file positions for each array entry (using estimated sizes)
        std::vector<size_t> file_positions(m_hot.keys.size());
        size_t data_offset = header_size;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (!m_cold.stored_in_metadata_flags[i]) {
                file_positions[i] = data_offset;
                data_offset += m_cold.compressed_sizes[i];
            }
        }

        // Metadata block position (after all array data)
        size_t metadata_position = data_offset;

        // Calculate total file size (using estimated metadata size)
        size_t total_file_size = metadata_position + metadata_estimated_size;

        LOG_DEBUG("flush() - Phase 2 complete: calculated file layout, header_size=", header_size,
                  ", total_size=", total_file_size);

        // Track actual block_infos (estimates may differ from actual)
        std::vector<std::vector<BlockInfo>> actual_blocks(m_hot.keys.size());
        std::vector<std::vector<char>> compressed_data(m_hot.keys.size());  // Store compressed data
        std::mutex file_mutex;  // For thread-safe file writes

        // Compress and write metadata block first (to get actual block_infos)
        std::vector<char> metadata_compressed;
        std::vector<BlockInfo> metadata_blocks;
        if (!metadata_indices.empty()) {
            m_compress_buffer.clear();
            std::vector<char> temp_buffer;
            metadata_blocks = compressBlocksBuffered(
                metadata_str.data(),
                metadata_str.size(),
                m_config.metadata_compression,
                m_config.metadata_max_block_size,
                metadata_compressed,
                temp_buffer,
                m_thread_pool.get()
            );

            // ALL metadata entries point to the SAME metadata block
            // Set block infos for all metadata entries to point to the shared metadata block
            for (size_t idx : metadata_indices) {
                m_cold.file_positions[idx] = metadata_position;
                m_cold.compressed_sizes[idx] = metadata_compressed.size();
                m_cold.block_infos[idx] = metadata_blocks;
                m_cold.compressions[idx] = m_config.metadata_compression;
            }

            // Store metadata block info in base layer registry (for v1 format loading)
            size_t base_layer_idx = m_layer_metadata_registry.get_layer_index("__base__");
            m_layer_metadata_registry.block_positions[base_layer_idx] = metadata_position;
            m_layer_metadata_registry.block_sizes[base_layer_idx] = static_cast<uint32_t>(metadata_compressed.size());
            m_layer_metadata_registry.compressions[base_layer_idx] = m_config.metadata_compression;
        }

        // ---------------------------------------------------------------------
        // Unified write path (local file, S3, or HTTP).
        //
        // Assemble the ENTIRE file image in a single contiguous byte buffer using
        // ACTUAL (post-compression) sizes and offsets, then hand it to the sink
        // for the destination type. This replaces the previous split local/S3
        // implementations, which diverged: the S3 path placed data at ESTIMATED
        // offsets while recording ACTUAL offsets in the index (corrupt reads) and
        // never wrote per-layer STARMeta blocks (lost metadata). One assembler for
        // all destinations guarantees the bytes match the header regardless of
        // where they end up.
        // ---------------------------------------------------------------------

        // HTTP/vsicurl is read-only — there is no write verb. Fail loudly rather
        // than silently producing nothing.
        if (m_path_info.type == FilePathInfo::HTTP) {
            throw std::runtime_error(
                "Cannot write to an HTTP (/vsicurl) source: HTTP is read-only. "
                "Write to a local path or an S3 (/vsis3) URL, or use save_to().");
        }

        // Phase 3a: compress every dirty array to get ACTUAL block infos + bytes.
        // (Non-dirty arrays are copied from the existing file at their old offsets.)
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i]) continue;

            if (m_hot.dirty_flags[i]) {
                std::vector<char> serialize_buffer;
                if (!array_metadata[i].serialized_data.empty()) {
                    serialize_buffer = array_metadata[i].serialized_data;  // small: cached
                } else {
                    const ValueVariant& var = m_data_storage[m_hot.data_indices[i]];
                    std::visit([this, &serialize_buffer](auto&& arr) {
                        using T = typename std::decay_t<decltype(arr)>::value_type;
                        serialize_buffer = serialize_array_data<T>(arr.data(), m_config.compression);
                    }, var);
                }

                std::vector<char> temp_buffer;
                actual_blocks[i] = compressBlocksBuffered(
                    serialize_buffer.data(), serialize_buffer.size(),
                    m_config.compression, m_config.block_size,
                    compressed_data[i], temp_buffer, m_thread_pool.get());
            } else {
                // Non-dirty: copy the already-compressed bytes from the old file.
                actual_blocks[i] = m_cold.block_infos[i];
                compressed_data[i].resize(m_cold.compressed_sizes[i]);
                std::ifstream in(m_filename, std::ios::binary);
                if (in) {
                    in.seekg(m_cold.file_positions[i]);
                    in.read(compressed_data[i].data(), compressed_data[i].size());
                }
                // Re-base block offsets to a contiguous layout starting at 0.
                size_t current_offset = 0;
                for (auto& block : actual_blocks[i]) {
                    block.offset = current_offset;
                    current_offset += block.compressed_size;
                }
            }
        }

        // Phase 3b: fold actual block infos + sizes back into the SoA and compute
        // ACTUAL contiguous array file positions (right after the header).
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] || actual_blocks[i].empty()) continue;
            m_cold.block_infos[i] = actual_blocks[i];
            m_cold.compressed_sizes[i] = 0;
            for (const auto& block : actual_blocks[i]) {
                m_cold.compressed_sizes[i] += block.compressed_size;
            }
        }
        size_t data_offset_actual = header_size;
        size_t array_data_total = 0;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i]) continue;
            m_cold.file_positions[i] = data_offset_actual;
            data_offset_actual += m_cold.compressed_sizes[i];
            array_data_total += m_cold.compressed_sizes[i];
        }

        // Phase 3c: build per-layer STARMeta blocks (compressed) so their ACTUAL
        // positions/sizes are known before we serialize the layer registry. Blocks
        // are laid out contiguously immediately after the array data.
        std::vector<std::vector<char>> layer_meta_blocks(m_layer_metadata_registry.layer_names.size());
        {
            size_t meta_cursor = data_offset_actual;  // metadata blocks follow array data
            for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
                const std::string& layer_name = m_layer_metadata_registry.layer_names[i];
                if (m_layer_metadata_registry.key_indices[i].empty()) {
                    m_layer_metadata_registry.block_positions[i] = 0;
                    m_layer_metadata_registry.block_sizes[i] = 0;
                    m_layer_metadata_registry.compressions[i] = CompressionAlgorithm::NONE;
                    continue;
                }
                std::ostringstream metadata_stream;
                serialize_layer_metadata_block(metadata_stream, layer_name);
                std::string mstr = metadata_stream.str();
                layer_meta_blocks[i] = compress_single_block(
                    std::vector<char>(mstr.begin(), mstr.end()),
                    m_config.metadata_compression);

                m_layer_metadata_registry.block_positions[i] = meta_cursor;
                m_layer_metadata_registry.block_sizes[i] =
                    static_cast<uint32_t>(layer_meta_blocks[i].size());
                m_layer_metadata_registry.compressions[i] = m_config.metadata_compression;
                meta_cursor += layer_meta_blocks[i].size();
            }
        }

        // Phase 3d: serialize the header section (fixed header + key registry +
        // layer registry + presence bitmaps + array index) sequentially — no
        // backward seeking, since all positions are now final.
        std::ostringstream header_stream;
        m_file_header.header_size = header_size;
        size_t data_entry_count = 0;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 0) data_entry_count++;
        }
        m_file_header.entry_count = data_entry_count;
        m_file_header.layer_count = static_cast<uint32_t>(m_layer_metadata_registry.layer_names.size() - 1);
        m_file_header.key_registry_count = static_cast<uint32_t>(m_key_registry.names.size());
        m_file_header.write(header_stream);

        // Global key registry
        for (size_t i = 0; i < m_key_registry.names.size(); ++i) {
            uint16_t key_len = static_cast<uint16_t>(m_key_registry.names[i].size());
            header_stream.write(reinterpret_cast<const char*>(&key_len), sizeof(uint16_t));
            header_stream.write(m_key_registry.names[i].c_str(), key_len);
            header_stream.write(reinterpret_cast<const char*>(&m_key_registry.hashes[i]), sizeof(uint64_t));
        }

        // Layer metadata registry (with ACTUAL block positions/sizes from 3c)
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            uint16_t layer_name_len = static_cast<uint16_t>(m_layer_metadata_registry.layer_names[i].size());
            header_stream.write(reinterpret_cast<const char*>(&layer_name_len), sizeof(uint16_t));
            header_stream.write(m_layer_metadata_registry.layer_names[i].c_str(), layer_name_len);
            header_stream.write(reinterpret_cast<const char*>(&m_layer_metadata_registry.block_positions[i]), sizeof(uint64_t));
            header_stream.write(reinterpret_cast<const char*>(&m_layer_metadata_registry.block_sizes[i]), sizeof(uint32_t));
            uint8_t compression_byte = static_cast<uint8_t>(m_layer_metadata_registry.compressions[i]);
            header_stream.write(reinterpret_cast<const char*>(&compression_byte), sizeof(uint8_t));
            uint16_t metadata_key_count = static_cast<uint16_t>(m_layer_metadata_registry.key_indices[i].size());
            header_stream.write(reinterpret_cast<const char*>(&metadata_key_count), sizeof(uint16_t));
            std::vector<uint16_t> sorted_indices(m_layer_metadata_registry.key_indices[i].begin(),
                                                 m_layer_metadata_registry.key_indices[i].end());
            std::sort(sorted_indices.begin(), sorted_indices.end());
            header_stream.write(reinterpret_cast<const char*>(sorted_indices.data()),
                               sorted_indices.size() * sizeof(uint16_t));
        }

        // Layer presence bitmaps (same order as the registry)
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            const std::string& layer_name = m_layer_metadata_registry.layer_names[i];
            const auto& bitmap = m_layer_presence[layer_name];
            header_stream.write(reinterpret_cast<const char*>(bitmap.data()), bitmap.size() * sizeof(uint64_t));
        }

        // Array index (data arrays only) with ACTUAL positions/blocks
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1) continue;
            uint16_t key_idx = m_key_registry.get_index(m_hot.keys[i]);
            header_stream.write(reinterpret_cast<const char*>(&key_idx), sizeof(uint16_t));

            IndexEntry entry;
            entry.position = m_cold.file_positions[i];
            entry.total_bytes = m_cold.compressed_sizes[i];
            entry.datatype = m_hot.dtypes[i];
            entry.shape = m_cold.shapes[i];
            entry.compression = m_cold.compressions[i];
            entry.block_size = m_config.block_size;
            entry.blocks = m_cold.block_infos[i];
            entry.stored_in_metadata = false;  // Always false in v1
            entry.write(header_stream);
        }

        std::string header_str = header_stream.str();
        // The header must fit exactly in the reserved region; if this ever trips,
        // calculateHeaderSize() and the index serialization have diverged.
        if (header_str.size() > header_size) {
            throw std::runtime_error("Internal error: serialized header (" +
                std::to_string(header_str.size()) + ") exceeds reserved header size (" +
                std::to_string(header_size) + ")");
        }

        // Phase 3e: assemble the full file image: [header | array data | metadata blocks].
        size_t total_image_size = data_offset_actual;  // header + arrays
        for (const auto& blk : layer_meta_blocks) total_image_size += blk.size();

        std::vector<char> image(total_image_size, 0);
        std::memcpy(image.data(), header_str.data(), header_str.size());
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] || compressed_data[i].empty()) continue;
            std::memcpy(image.data() + m_cold.file_positions[i],
                       compressed_data[i].data(), compressed_data[i].size());
        }
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            if (layer_meta_blocks[i].empty()) continue;
            std::memcpy(image.data() + m_layer_metadata_registry.block_positions[i],
                       layer_meta_blocks[i].data(), layer_meta_blocks[i].size());
        }

        // Phase 3f: sink the image to the destination.
        if (m_capture_image != nullptr) {
            // In-memory sink (write_bytes): hand the assembled image to the caller
            // instead of writing to disk/S3.
            *m_capture_image = std::move(image);
        } else if (m_path_info.type == FilePathInfo::S3) {
#ifdef ENABLE_S3
            if (!m_s3_credentials) {
                throw std::runtime_error("S3 credentials not available for writing to: " + m_filename);
            }
            S3Writer writer(m_path_info.bucket, m_path_info.key, m_path_info.region, *m_s3_credentials);
            writer.putObject(image.data(), image.size());
            LOG_DEBUG("Successfully uploaded ", image.size(), " bytes to S3: ", m_filename);
#else
            throw std::runtime_error("S3 support not enabled: cannot write to " + m_filename);
#endif
        } else {
            // LOCAL
            std::ofstream out(m_filename, std::ios::binary | std::ios::trunc);
            if (!out) {
                throw std::runtime_error("Failed to open file for writing: " + m_filename);
            }
            out.write(image.data(), image.size());
            out.flush();
            out.close();
            #ifndef _WIN32
            int fd = ::open(m_filename.c_str(), O_RDWR);
            if (fd != -1) {
                ::fsync(fd);
                ::close(fd);
            }
            #endif
            LOG_DEBUG("Wrote ", image.size(), " bytes to local file: ", m_filename);
        }

        LOG_DEBUG("flush() - Phase 3 complete: parallel compress and write finished");

        for (size_t idx : dirty_indices) {
            m_hot.dirty_flags[idx] = false;
            m_hot.locations[idx] = StorageLocation::PERSISTED;
        }
        for (size_t idx : metadata_indices) {
            m_hot.dirty_flags[idx] = false;
            m_hot.locations[idx] = StorageLocation::PERSISTED;
        }

        m_flushed = true;
        m_header_dirty = false;

        // The on-disk bytes just changed (offsets shifted, header rewritten), so
        // the persistent reader — and any whole-file cache it holds — is now
        // stale. Drop it; the next read lazily re-creates it against the new file.
        m_reader.reset();

        // Shrink persistent buffers if they've grown too large (memory optimization)
        if (m_serialize_buffer.capacity() > m_config.buffer_shrink_threshold) {
            LOG_TRACE("Shrinking serialize_buffer from ", m_serialize_buffer.capacity(), " bytes");
            m_serialize_buffer.clear();
            m_serialize_buffer.shrink_to_fit();
        }
        if (m_compress_buffer.capacity() > m_config.buffer_shrink_threshold) {
            LOG_TRACE("Shrinking compress_buffer from ", m_compress_buffer.capacity(), " bytes");
            m_compress_buffer.clear();
            m_compress_buffer.shrink_to_fit();
        }

        LOG_DEBUG("flush() completed - wrote ", dirty_indices.size(), " entries, ",
                  metadata_indices.size(), " metadata entries");
    }

public:
    /**
     * @brief Save to a different file (local or S3)
     *
     * Allows saving the current state to a different file without modifying m_filename.
     * This enables:
     * - Read-only files to be saved elsewhere
     * - Local → S3 migration
     * - S3 → Local migration
     * - S3 → S3 copy
     *
     * Streams unloaded data directly from source without loading into memory.
     *
     * @param target_path Path to save to (local, s3://... / /vsis3/...)
     */
private:
    // Load every entry (array namespace + metadata block) into memory and mark
    // it dirty so a subsequent flush() rewrites all data to the target file.
    // Without this, flush() skips clean entries and the target's index would
    // reference block data that was never written (unreadable keys after save_to).
    void stage_all_entries_for_resave() {
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_hot.data_indices[i] == SIZE_MAX) {
                load_entry(i);  // loads array entries; metadata entries load the block
            }
            if (m_hot.data_indices[i] != SIZE_MAX) {
                m_hot.dirty_flags[i] = true;
            }
        }
        m_flushed = false;  // force flush_internal() to run
    }

public:
    void save_to(const std::string& target_path) {
#ifdef ENABLE_S3
        // Check if trying to save to source in read-only mode
        if (target_path == m_filename && m_file_mode == FileMode::READ_ONLY) {
            throw std::runtime_error(
                "Cannot save to source file in read-only mode. Use save_to() with different path.");
        }

        // Load and dirty every entry (array + metadata) while still pointing at
        // the SOURCE, so all data is in memory before we redirect to the target.
        stage_all_entries_for_resave();

        // Temporarily swap filename and mode for flush
        std::string original_filename = m_filename;
        FileMode original_mode = m_file_mode;
        FilePathInfo original_path_info = m_path_info;
        std::unique_ptr<S3Credentials> original_creds;
        if (m_s3_credentials) {
            original_creds = std::make_unique<S3Credentials>(*m_s3_credentials);
        }

        try {
            m_filename = target_path;
            m_file_mode = FileMode::READ_WRITE;  // Allow write for save_to

            // Parse target path and resolve credentials (cached)
            m_path_info = parseFilePath(target_path);
            if (m_path_info.type == FilePathInfo::S3) {
                // Reuse credentials if same region, otherwise resolve new
                if (original_creds && original_path_info.region == m_path_info.region) {
                    m_s3_credentials = std::make_unique<S3Credentials>(*original_creds);
                } else {
                    m_s3_credentials = std::make_unique<S3Credentials>(S3Credentials::resolve());
                }
            }

            // Call flush with new target
            flush();

            // Restore original state
            m_filename = original_filename;
            m_file_mode = original_mode;
            m_path_info = original_path_info;
            if (original_creds) {
                m_s3_credentials = std::move(original_creds);
            }
        } catch (...) {
            // Restore original state on error
            m_filename = original_filename;
            m_file_mode = original_mode;
            m_path_info = original_path_info;
            if (original_creds) {
                m_s3_credentials = std::move(original_creds);
            }
            throw;
        }
#else
        // Without S3 support, just use flush with temp filename swap.
        // Load and dirty every entry while still pointing at the source.
        stage_all_entries_for_resave();

        std::string original_filename = m_filename;
        FileMode original_mode = m_file_mode;
        try {
            m_filename = target_path;
            m_file_mode = FileMode::READ_WRITE;  // Allow write even if source is read-only
            flush_internal();
            m_filename = original_filename;
            m_file_mode = original_mode;
        } catch (...) {
            m_filename = original_filename;
            m_file_mode = original_mode;
            throw;
        }
#endif
    }

    /**
     * @brief Serialize the dataset to an in-memory byte buffer.
     *
     * The byte-array counterpart of save_to(): returns a complete .stards image
     * (the exact bytes that would be written to a file) instead of writing to a
     * path. Works on any dataset — including read-only ones and datasets opened
     * with open_bytes() — since it never touches the source file. Round-trips
     * with open_bytes(): `open_bytes(ds->write_bytes())` reconstructs the dataset.
     *
     * @return A complete .stards image as a byte array.
     */
    std::vector<char> write_bytes() {
        // Load + dirty every entry so flush_internal() rewrites all data into the
        // captured image (clean entries are otherwise skipped — see save_to()).
        stage_all_entries_for_resave();

        std::vector<char> image;
        // Redirect the write to memory and force a full (write-mode) flush,
        // restoring all mutated state afterward so the dataset is unchanged.
        std::vector<char>* prev_capture = m_capture_image;
        FileMode original_mode = m_file_mode;
        bool original_flushed = m_flushed;
        m_capture_image = &image;
        m_file_mode = FileMode::READ_WRITE;  // allow write even if source is read-only
        try {
            flush_internal();
        } catch (...) {
            m_capture_image = prev_capture;
            m_file_mode = original_mode;
            m_flushed = original_flushed;
            throw;
        }
        m_capture_image = prev_capture;
        m_file_mode = original_mode;
        // Restore the flushed flag: capturing to memory did not persist the
        // source file, so a later real flush()/save_to() must still run.
        m_flushed = original_flushed;
        return image;
    }

    /**
     * @brief Check if file is in read-only mode
     * @return True if read-only
     */
    bool is_read_only() const {
        // Read-only mode is independent of cloud support: a local or in-memory
        // (open_bytes) dataset opened READ_ONLY must report and enforce it too.
        return m_file_mode == FileMode::READ_ONLY;
    }

    /**
     * @brief Get current filename
     * @return Filename
     */
    std::string get_filename() const {
        return m_filename;
    }

    /**
     * @brief Get file header with version information
     * @return Reference to FileHeader
     */
    const FileHeader& get_file_header() const {
        return m_file_header;
    }

    /**
     * @brief Read-time options (e.g. layer inheritance) for this dataset.
     *
     * These affect only how the in-memory dataset resolves reads; nothing is
     * persisted. They are safe to change at any time, including on a read-only
     * dataset, and take effect immediately for existing and future LayerViews.
     */
    const OpenOptions& open_options() const { return m_open_options; }
    void set_open_options(const OpenOptions& opts) { m_open_options = opts; }

    // Whether LayerView lookups fall back to the base layer. Off by default.
    bool layer_inheritance() const { return m_open_options.layer_inheritance; }
    void set_layer_inheritance(bool on) { m_open_options.layer_inheritance = on; }


    /**
     * @brief Create a new Star dataset file
     *
     * Creates a new file with the specified configuration. If the file already exists,
     * it will be overwritten. The file will be created on the first flush() or when
     * the object is destroyed.
     *
     * @param filename Path to create (local, s3://... / /vsis3/...)
     * @param config Configuration for compression, block sizes, metadata
     * @return New StarDataset instance
     */
    static std::shared_ptr<StarDataset> create(const std::string& filename, const StarConfig& config = StarConfig()) {
        // If file exists, delete it to allow overwrite
        std::ifstream check_file(filename, std::ios::binary);
        if (check_file.good()) {
            check_file.close();
            // Delete existing file
            std::remove(filename.c_str());
        }

        // Use make_shared to enable shared_from_this()
        return std::make_shared<StarDataset>(filename, FileMode::READ_WRITE, &config);
    }

    /**
     * @brief Open an existing Star dataset file
     *
     * Opens an existing file and reads its configuration from the file header.
     * The configuration used when the file was created is preserved.
     *
     * If mode is READ_WRITE and file doesn't exist, it will be created.
     * If mode is READ_ONLY and file doesn't exist, an error is thrown.
     *
     * @param filename Path to open (local, s3:// or /vsis3/, https:// or /vsicurl/)
     * @param mode FileMode enum (READ_WRITE/READ_ONLY)
     * @return Opened StarDataset instance
     * @throws std::runtime_error if file doesn't exist in READ_ONLY mode or is corrupt
     */
    static std::shared_ptr<StarDataset> open(const std::string& filename,
                                             FileMode mode = FileMode::READ_WRITE,
                                             const OpenOptions& opts = {}) {
        // Parse the path so remote schemes (S3, HTTP) are handled appropriately;
        // a plain ifstream can only see local files and would wrongly report a
        // remote URL as "does not exist".
        FilePathInfo path_info = parseFilePath(filename);
        const bool is_remote = (path_info.type != FilePathInfo::LOCAL);

        // Local existence is free to check (no network); do it up front so a
        // missing local file gives a precise message. For REMOTE files we do NOT
        // issue a HEAD just to probe existence — instead we construct the dataset
        // (which reads the header via a single ranged GET on the reused
        // connection) and infer existence from whether a valid header was read.
        if (!is_remote) {
            std::ifstream check_file(filename, std::ios::binary);
            bool file_exists = check_file.good();
            check_file.close();

            if (!file_exists && mode == FileMode::READ_ONLY) {
                throw std::runtime_error("File does not exist: " + filename +
                    ". Cannot open non-existent file in read-only mode.");
            }
            if (!file_exists && mode == FileMode::READ_WRITE) {
                // File will be created on first flush.
                return std::make_shared<StarDataset>(filename, mode, nullptr, opts);
            }
        }
#if !defined(ENABLE_CURL) && !defined(ENABLE_S3)
        if (is_remote) {
            throw std::runtime_error("Remote URL support not enabled, cannot open: " + filename);
        }
#endif

        // Construct: loadIndex() reads the header through the persistent reader
        // (one ranged request, no HEAD). If nothing valid was read, m_header_size
        // stays 0 -> the object is absent/empty/corrupt.
        auto store = std::make_shared<StarDataset>(filename, mode, nullptr, opts);

        if (store->m_header_size == 0) {
            // No readable header. For a REMOTE read-write target this means the
            // object doesn't exist yet -> treat as a new file (created on flush).
            if (is_remote && mode == FileMode::READ_WRITE) {
                return store;
            }
            if (mode == FileMode::READ_ONLY) {
                throw std::runtime_error("File does not exist or is unreadable: " +
                    filename + ". Cannot open in read-only mode.");
            }
            throw std::runtime_error("Failed to load file: " + filename +
                ". File may be corrupt or not a valid STAR file.");
        }

        return store;
    }

    /**
     * @brief Open an existing Star dataset file (string mode overload)
     *
     * @param filename Path to open (local, s3:// or /vsis3/, https:// or /vsicurl/)
     * @param mode_str String mode ("r", "w", "rw", "a")
     * @return Opened StarDataset instance
     * @throws std::runtime_error if file doesn't exist or is invalid
     */
    static std::shared_ptr<StarDataset> open(const std::string& filename, const std::string& mode_str,
                                             const OpenOptions& opts = {}) {
        return open(filename, parseModeString(mode_str), opts);
    }

    /**
     * @brief Open a dataset from an in-memory byte buffer.
     *
     * Mirrors open(), but the source is a byte array holding a complete .stards
     * image (e.g. bytes received over a socket or pulled from a database) instead
     * of a path. The dataset is READ_ONLY — there is no backing file to flush to;
     * use write_bytes() to serialize modifications back out to a new byte array.
     *
     * @param bytes A complete .stards image.
     * @param opts  Read-time options (e.g. layer_inheritance).
     * @return Opened StarDataset backed by the provided bytes.
     * @throws std::runtime_error if the bytes are not a valid STAR image.
     */
    static std::shared_ptr<StarDataset> open_bytes(std::vector<char> bytes,
                                                   const OpenOptions& opts = {}) {
        if (bytes.empty()) {
            throw std::runtime_error("open_bytes: empty byte buffer is not a valid STAR image.");
        }
        // The in-memory source is READ_ONLY: there is no path to flush back to.
        auto store = std::make_shared<StarDataset>("<memory>", FileMode::READ_ONLY,
                                                   nullptr, opts, &bytes);
        if (store->m_header_size == 0) {
            throw std::runtime_error(
                "open_bytes: byte buffer is not a valid STAR image (bad or missing header).");
        }
        return store;
    }

    /**
     * @brief Convenience overload taking a raw pointer + length (e.g. from a
     *        C buffer, Python bytes, or a mapped region).
     */
    static std::shared_ptr<StarDataset> open_bytes(const void* data, size_t size,
                                                   const OpenOptions& opts = {}) {
        const char* p = static_cast<const char*>(data);
        return open_bytes(std::vector<char>(p, p + size), opts);
    }

    /**
     * @brief Constructor with compression options
     * @param fname Filename to use for storage
     * @param mode File open mode (READ_WRITE or READ_ONLY)
     * @param config Optional configuration (nullptr to load from file)
     *
     * NOTE: Constructor is public to enable std::make_shared, but you should
     * use the static factory methods create() and open() instead.
     */
    StarDataset(const std::string& fname, FileMode mode, const StarConfig* config,
                const OpenOptions& open_options = {},
                std::vector<char>* memory_source = nullptr)
        : m_filename(fname), m_header_dirty(true), meta(this),
          m_file_mode(mode),
          m_config(config ? *config : StarConfig()),
          m_open_options(open_options)
    {
        // In-memory dataset (open_bytes / a fresh write_bytes target): take the
        // bytes as the read source and skip all path parsing / credential setup.
        if (memory_source != nullptr) {
            m_memory_source = std::move(*memory_source);
            m_path_info.type = FilePathInfo::MEMORY;
        } else {
#ifdef ENABLE_S3
            // Parse path once at construction (minimize allocations)
            m_path_info = parseFilePath(fname);

            // Resolve credentials once (cache hot data)
            if (m_path_info.type == FilePathInfo::S3) {
                m_s3_credentials = std::make_unique<S3Credentials>(S3Credentials::resolve());
            }
#else
            m_path_info.type = FilePathInfo::LOCAL;
            m_path_info.path = fname;
#endif
        }

        // PRE-ALLOCATE CAPACITY FOR VECTORS (Performance optimization)
        // Reduces O(N²) reallocation overhead when storing many arrays
        constexpr size_t INITIAL_CAPACITY = 128;  // Tune based on typical workload

        // Reserve HotStorage vectors (6 vectors)
        m_hot.keys.reserve(INITIAL_CAPACITY);              // Key names
        m_hot.dtypes.reserve(INITIAL_CAPACITY);            // Data types
        m_hot.locations.reserve(INITIAL_CAPACITY);         // Storage locations
        m_hot.dirty_flags.reserve(INITIAL_CAPACITY);       // Dirty flags
        m_hot.loaded_flags.reserve(INITIAL_CAPACITY);      // Load status
        m_hot.data_indices.reserve(INITIAL_CAPACITY);      // Indices into m_data_storage

        // Reserve ColdStorage vectors (7 vectors)
        m_cold.shapes.reserve(INITIAL_CAPACITY);                    // Array dimensions
        m_cold.file_positions.reserve(INITIAL_CAPACITY);            // File offsets
        m_cold.compressed_sizes.reserve(INITIAL_CAPACITY);          // Compressed sizes
        m_cold.uncompressed_sizes.reserve(INITIAL_CAPACITY);        // Uncompressed sizes
        m_cold.compressions.reserve(INITIAL_CAPACITY);              // Compression algorithms
        m_cold.block_infos.reserve(INITIAL_CAPACITY);               // Block metadata (outer vector)
        m_cold.stored_in_metadata_flags.reserve(INITIAL_CAPACITY);  // Storage location flags

        // Reserve data storage vector (CRITICAL: holds actual array data)
        // This stores the ValueVariant objects containing NDArray<T> instances
        m_data_storage.reserve(INITIAL_CAPACITY);

        // Initialize thread pool from global configuration
        size_t threads = (g_num_threads == 0) ? std::thread::hardware_concurrency() : g_num_threads;
        if (threads == 0) threads = 1;  // Fallback if hardware_concurrency returns 0
        if (threads > 1) {
            m_thread_pool = std::make_unique<ThreadPool>(threads);
        }
        // If threads == 1, m_thread_pool remains nullptr (single-threaded mode)

        loadIndex();

        // Ensure base layer exists in registry (for new files)
        if (m_layer_metadata_registry.layer_names.empty()) {
            m_layer_metadata_registry.add_layer("__base__");
            m_layer_metadata_loaded.resize(1, false);
            m_layer_metadata_indices.resize(1);

            // Initialize empty bitmap for base layer (will grow as arrays are added)
            m_layer_presence["__base__"] = std::vector<uint64_t>();
        }
    }

    // Delete copy and move constructors (non-copyable due to std::shared_mutex)
    StarDataset(const StarDataset&) = delete;
    StarDataset& operator=(const StarDataset&) = delete;
    StarDataset(StarDataset&&) = delete;
    StarDataset& operator=(StarDataset&&) = delete;

public:
    /**
     * @brief Destructor - writes all pending changes to disk (RAII)
     */
    ~StarDataset() {
        LOG_DEBUG("~StarDataset() destructor called");
        try {
            flush_quiet();  // best-effort; never throws on read-only
        } catch (const std::exception& e) {
            std::cerr << "Exception in ~StarDataset(): " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in ~StarDataset()" << std::endl;
        }
        LOG_DEBUG("~StarDataset() destructor finished");
    }

    /**
     * @brief Flush all pending writes to disk
     *
     * This is automatically called on context manager exit or object destruction.
     * You can call this manually to ensure data is persisted without closing the dataset.
     */
    void close() {
        // Cleanup semantics (mirrors the destructor): best-effort flush that is a
        // no-op for read-only datasets rather than an error.
        flush_quiet();
    }

    //==========================================================================
    // Layer Management API
    //==========================================================================

    /**
     * @brief Get existing layer view
     * @param layer_name Name of the layer
     * @return Shared pointer to LayerView
     * @throws std::runtime_error if layer doesn't exist
     */
    std::shared_ptr<LayerView> get_layer(const std::string& layer_name);

    /**
     * @brief Create new layer and return view
     * @param layer_name Name of new layer
     * @return Shared pointer to LayerView
     * @throws std::runtime_error if layer already exists
     */
    std::shared_ptr<LayerView> create_layer(const std::string& layer_name);

    /**
     * @brief Check if layer exists
     * @param layer_name Layer name to check
     * @return true if layer exists, false otherwise
     */
    bool has_layer(const std::string& layer_name) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return has_layer_unlocked(layer_name);
    }

    /**
     * @brief Set layer presence for a data array key
     * @param layer_name Layer name
     * @param key Data array key
     * @param present Whether the key is present in this layer
     */
    void set_layer_presence(const std::string& layer_name, const std::string& key, bool present) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        LOG_DEBUG("set_layer_presence: layer=", layer_name, " key=", key, " present=", present);

        // Get key index from KEY REGISTRY (not m_key_to_index which is m_hot.keys index)
        // This is critical for v1 format where bitmaps are indexed by registry key indices
        uint16_t key_idx = m_key_registry.get_or_create(key);

        // Get or create layer presence bitmap
        auto& bitmap = m_layer_presence[layer_name];
        size_t word_idx = key_idx / 64;
        size_t bit_idx = key_idx % 64;

        // Resize bitmap if needed
        if (word_idx >= bitmap.size()) {
            bitmap.resize(word_idx + 1, 0);
        }

        // Set or clear bit
        if (present) {
            bitmap[word_idx] |= (uint64_t(1) << bit_idx);
            LOG_DEBUG("  Set bit ", bit_idx, " (key_idx=", key_idx, ") in word ", word_idx, " for layer ", layer_name);
        } else {
            bitmap[word_idx] &= ~(uint64_t(1) << bit_idx);
        }

        m_header_dirty = true;
    }

private:
    // Internal helper without locking (call when already holding lock)
    bool has_layer_unlocked(const std::string& layer_name) const {
        return m_layer_metadata_registry.contains_layer(layer_name) || layer_name == "__base__";
    }

    /**
     * @brief Resolve metadata inheritance from header (O(1) - no loading)
     * @param key Key to look up
     * @param layer_name Layer to start search from
     * @return Layer index where key exists (checks layer first, then base)
     * @throws std::runtime_error if key not found in layer or base
     */
    size_t resolve_metadata_inheritance(const std::string& key, const std::string& layer_name) const {
        // O(1) lookup: key → key_index
        if (!m_key_registry.contains(key)) {
            throw std::runtime_error("Key not found in registry: " + key);
        }
        uint16_t key_index = m_key_registry.get_index(key);

        // O(1) lookup: layer_name → layer_index
        size_t layer_idx = m_layer_metadata_registry.get_layer_index(layer_name);

        // O(1) check: does layer have this key?
        const auto& layer_indices = m_layer_metadata_registry.key_indices[layer_idx];
        if (layer_indices.count(key_index) > 0) {
            return layer_idx;
        }

        // Fall back to base only if inheritance is enabled (off by default). For a
        // non-base layer with inheritance disabled, the key is simply not present.
        if (layer_name != "__base__" && !m_open_options.layer_inheritance) {
            throw std::runtime_error("Key not found in layer: " + key);
        }

        size_t base_idx = m_layer_metadata_registry.get_layer_index("__base__");
        const auto& base_indices = m_layer_metadata_registry.key_indices[base_idx];
        if (base_indices.count(key_index) > 0) {
            return base_idx;
        }

        throw std::runtime_error("Key not found in layer or base: " + key);
    }

    /**
     * @brief Check if metadata key exists in specific layer (header-only, O(1))
     * @param key Key to check
     * @param layer_name Layer name
     * @return true if key exists in layer
     */
    bool has_metadata_key_in_layer(const std::string& key, const std::string& layer_name) const {
        if (!m_key_registry.contains(key)) {
            return false;
        }
        uint16_t key_index = m_key_registry.get_index(key);

        if (!m_layer_metadata_registry.contains_layer(layer_name)) {
            return false;
        }
        size_t layer_idx = m_layer_metadata_registry.get_layer_index(layer_name);

        const auto& indices = m_layer_metadata_registry.key_indices[layer_idx];
        return indices.count(key_index) > 0;
    }

public:

    /**
     * @brief Get list of all layer names
     * @return Vector of layer names
     */
    std::vector<std::string> list_layers() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        // Filter out __base__ layer (internal use only)
        std::vector<std::string> result;
        for (const auto& name : m_layer_metadata_registry.layer_names) {
            if (name != "__base__") {
                result.push_back(name);
            }
        }
        return result;
    }

    /**
     * @brief Check if key exists in specific layer using bit-mask (O(1))
     * @param key Key to check
     * @param layer_name Layer name
     * @return true if key exists in layer, false otherwise
     */
    bool key_in_layer(const std::string& key, const std::string& layer_name) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_key_to_index.find(key);
        if (it != m_key_to_index.end()) {
            // Key found in array storage
            size_t idx = it->second;

            // Check if this is a metadata key (stored in metadata block, not as separate array)
            if (m_cold.stored_in_metadata_flags[idx] == 1) {
                // Metadata key - check layer metadata registry
                return has_metadata_key_in_layer(key, layer_name);
            }

            // Data array - check layer-specific bitmap using KEY REGISTRY index
            // Get key index from registry (v1 format uses registry indices for bitmaps)
            uint16_t key_idx = m_key_registry.get_index(key);

            auto layer_it = m_layer_presence.find(layer_name);
            if (layer_it == m_layer_presence.end()) {
                return false;
            }

            const auto& bitmap = layer_it->second;
            size_t word_idx = key_idx / 64;
            size_t bit_idx = key_idx % 64;

            if (word_idx >= bitmap.size()) {
                return false;
            }

            return (bitmap[word_idx] & (1ULL << bit_idx)) != 0;
        }

        // Key not in array storage, check if it's a metadata-only key
        return has_metadata_key_in_layer(key, layer_name);
    }

    /**
     * @brief Store array as separate compressed array (not in metadata block)
     *
     * Use this for large arrays that need slicing support. Small arrays
     * should use meta.put() instead.
     *
     * @param key Array key
     * @param value NDArray to store
     */
    template<typename T>
    void put(const std::string& key, NDArray<T>&& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_flushed = false;  // Data changed, needs flush

        auto it = m_key_to_index.find(key);

        using ElementType = typename std::remove_reference_t<decltype(value)>::value_type;
        DataType dtype = TypeToDataType<ElementType>::value;
        auto shape_vec = std::vector<size_t>(value.shape().begin(), value.shape().end());
        uint32_t uncompressed_size = value.size() * sizeof(ElementType);

        LOG_DEBUG("put() storing key '", key, "' as separate array with ", uncompressed_size, " bytes (", value.size(), " elements)");

        if (it != m_key_to_index.end()) {
            // Update existing entry
            size_t idx = it->second;

            m_hot.dtypes[idx] = dtype;
            m_hot.locations[idx] = StorageLocation::PENDING;
            m_hot.dirty_flags[idx] = true;
            m_hot.loaded_flags[idx] = true;

            m_cold.shapes[idx] = std::move(shape_vec);
            m_cold.file_positions[idx] = 0;
            m_cold.uncompressed_sizes[idx] = uncompressed_size;
            m_cold.compressions[idx] = m_config.compression;
            m_cold.block_infos[idx].clear();
            m_cold.stored_in_metadata_flags[idx] = 0;  // NOT in metadata

            // Update data storage
            if (m_hot.data_indices[idx] == SIZE_MAX) {
                m_hot.data_indices[idx] = m_data_storage.size();
                m_data_storage.emplace_back(std::forward<NDArray<T>>(value));
            } else {
                m_data_storage[m_hot.data_indices[idx]] = std::forward<NDArray<T>>(value);
            }
        } else {
            // Create new entry

            // Performance optimization: Pre-emptively expand all vectors together
            // to avoid 14 independent reallocations (each copying all existing data)
            if (m_hot.keys.size() >= m_hot.keys.capacity()) {
                size_t new_capacity = m_hot.keys.capacity() * 2;

                // Expand HotStorage vectors (6 vectors)
                m_hot.keys.reserve(new_capacity);
                m_hot.dtypes.reserve(new_capacity);
                m_hot.locations.reserve(new_capacity);
                m_hot.dirty_flags.reserve(new_capacity);
                m_hot.loaded_flags.reserve(new_capacity);
                m_hot.data_indices.reserve(new_capacity);

                // Expand ColdStorage vectors (7 vectors)
                m_cold.shapes.reserve(new_capacity);
                m_cold.file_positions.reserve(new_capacity);
                m_cold.compressed_sizes.reserve(new_capacity);
                m_cold.uncompressed_sizes.reserve(new_capacity);
                m_cold.compressions.reserve(new_capacity);
                m_cold.block_infos.reserve(new_capacity);
                m_cold.stored_in_metadata_flags.reserve(new_capacity);

                // CRITICAL: Expand data storage vector (holds actual array data)
                m_data_storage.reserve(new_capacity);
            }

            size_t idx = m_hot.keys.size();
            size_t data_idx = m_data_storage.size();

            m_hot.keys.push_back(key);
            m_hot.dtypes.push_back(dtype);
            m_hot.locations.push_back(StorageLocation::PENDING);
            m_hot.dirty_flags.push_back(true);
            m_hot.loaded_flags.push_back(true);
            m_hot.data_indices.push_back(data_idx);

            m_cold.shapes.emplace_back(std::move(shape_vec));
            m_cold.file_positions.push_back(0);
            m_cold.compressed_sizes.push_back(0);
            m_cold.uncompressed_sizes.push_back(uncompressed_size);
            m_cold.compressions.push_back(m_config.compression);
            m_cold.block_infos.emplace_back();
            m_cold.stored_in_metadata_flags.push_back(0);  // NOT in metadata

            m_data_storage.emplace_back(std::forward<NDArray<T>>(value));
            m_key_to_index[key] = idx;

            // Add to key registry for v1 format and get its index
            uint16_t key_idx = m_key_registry.get_or_create(key);

            // Grow layer presence bitmaps for all layers based on KEY REGISTRY index
            size_t new_bitmap_words = (key_idx + 64) / 64;  // Round up to nearest 64-bit word
            for (auto& [layer_name, bitmap] : m_layer_presence) {
                if (bitmap.size() < new_bitmap_words) {
                    bitmap.resize(new_bitmap_words, 0);
                }
            }

            // Set bit for this array in base layer bitmap by default using KEY REGISTRY index
            // (LayerView::put will clear this and set the layer-specific bit instead)
            size_t word_idx = key_idx / 64;
            size_t bit_idx = key_idx % 64;
            m_layer_presence["__base__"][word_idx] |= (uint64_t(1) << bit_idx);
        }

        m_header_dirty = true;
        m_flushed = false;  // Need to flush new data
    }

    /**
     * @brief Store array as separate compressed array (const lvalue overload)
     */
    template<typename T>
    void put(const std::string& key, const NDArray<T>& value) {
        NDArray<T> value_copy = value;
        put(key, std::move(value_copy));
    }

    /**
     * @brief Get array from storage
     *
     * Retrieves array from either metadata block or separate storage.
     *
     * @param key Array key
     * @return NDArray
     */
    template<typename T>
    NDArray<T> get(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_key_to_index.find(key);
        if (it == m_key_to_index.end()) {
            throw std::runtime_error("Key not found: " + key);
        }

        size_t idx = it->second;

        // Verify type matches
        if (m_hot.dtypes[idx] != TypeToDataType<T>::value) {
            throw std::runtime_error("Type mismatch for key: " + key);
        }

        // Check if data is loaded
        if (m_hot.data_indices[idx] == SIZE_MAX) {
            // Need to load from disk
            lock.unlock();
            std::unique_lock<std::shared_mutex> write_lock(m_mutex);

            // Check again after acquiring write lock
            if (m_hot.data_indices[idx] == SIZE_MAX) {
                // Load the data (this will populate m_data_storage)
                load_entry(idx);
            }

            write_lock.unlock();
            lock.lock();
        }

        // Return copy of the data
        return std::get<NDArray<T>>(m_data_storage[m_hot.data_indices[idx]]);
    }


    /**
     * @brief Gets list of keys in metadata block
     * @return Vector of key names (excludes layer-prefixed internal keys)
     */
    std::vector<std::string> get_metadata_keys() const {
        // v1 format: Collect metadata keys from all layer registries (base + layers)
        std::set<std::string> key_set;  // Use set to avoid duplicates
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            for (uint16_t key_idx : m_layer_metadata_registry.key_indices[i]) {
                key_set.insert(m_key_registry.names[key_idx]);
            }
        }
        return std::vector<std::string>(key_set.begin(), key_set.end());
    }

private:
    /**
     * @brief Gets ALL keys from hot storage (internal use only)
     * @return Vector of all key names
     *
     * Note: Returns ALL keys in m_hot.keys, including layer-prefixed internal keys.
     * LayerMetadataAccessor will filter these to find its layer-specific keys.
     */
    std::vector<std::string> get_all_metadata_keys_internal() const {
        return m_hot.keys;
    }

public:

    /**
     * @brief Gets count of entries in metadata block
     * @return Number of metadata entries (excludes layer-prefixed internal keys)
     */
    size_t get_metadata_count() const {
        // v1 format: Count metadata keys from all layer registries (base + layers)
        size_t count = 0;
        for (size_t i = 0; i < m_layer_metadata_registry.layer_names.size(); ++i) {
            count += m_layer_metadata_registry.key_indices[i].size();
        }
        return count;
    }

    /**
     * @brief Gets all keys (metadata block + separate arrays)
     * @return Vector of all keys in the store
     */
    std::vector<std::string> get_all_keys() {
        // Simply return all keys from SoA (both metadata and separate storage)
        return m_hot.keys;
    }

    /**
     * @brief Gets n-dimensional slice of an array
     * 
     * IMPORTANT: This function only works with arrays stored as separate compressed arrays
     * with block structure. Arrays stored in the metadata block cannot be sliced and must
     * be accessed as complete units using meta.get() instead.
     *
     * @param key Array key in store (must be separately stored with blocks)
     * @param slices Vector of slices, one per dimension
     *               Empty = entire dimension, unfilled dimensions = full slice
     * @return NDArray containing the requested slice
     * @throws std::runtime_error if key not found or stored in metadata block
     *
     * Examples:
     *   // 1D: elements 1000-2000 (step defaults to 1)
     *   get_slice<double>("large_timeseries", {{1000, 2000}})
     *
     *   // 2D: rows 10-20, all columns
     *   get_slice<float>("image_data", {{10, 20}, {0, width}})
     *
     *   // Using helper functions for clarity
     *   get_slice<float>("matrix", {slice_range(10, 20), slice_all(width)})
     *
     *   // 3D: hyperslab
     *   get_slice<uint16_t>("volume", {{0, 10}, {5, 15}, {0, depth}})
     *
     *   // For small arrays in metadata block, use meta.get() instead:
     *   auto small_array = store.meta.get("small_data");
     */
    template<typename T>
    NDArray<T> get_slice(const std::string& key, const std::vector<Slice>& slices) {
        // Validation using SoA
        auto it = m_key_to_index.find(key);
        if (it == m_key_to_index.end()) {
            // Check if it's a metadata-only key
            if (meta.contains(key)) {
                throw std::runtime_error(
                    "Array '" + key + "' is stored in the metadata block and cannot be sliced. "
                    "Metadata block items are designed for complete access only. "
                    "Use meta.get(\"" + key + "\") to retrieve the full array.");
            }
            throw std::runtime_error("Key not found: " + key);
        }

        size_t idx = it->second;

        // Validate type
        if (m_hot.dtypes[idx] != TypeToDataType<T>::value) {
            throw std::runtime_error("Type mismatch for key: " + key);
        }

        LOG_TRACE("get_slice: Found entry with ", m_cold.block_infos[idx].size(), " blocks, ",
                 "stored_in_metadata=", (int)m_cold.stored_in_metadata_flags[idx]);

        // Only support slicing for 1D, 2D, and 3D arrays
        if (m_cold.shapes[idx].size() > 3) {
            throw std::runtime_error(
                "Slicing is only supported for 1D, 2D, and 3D arrays. "
                "Array '" + key + "' has " + std::to_string(m_cold.shapes[idx].size()) + " dimensions.");
        }

        // Byte-shuffled arrays regroup bytes across the WHOLE array, so per-block
        // element extraction (what get_slice does) can't reconstruct individual
        // elements without the full array. Slicing is therefore unsupported for
        // *_SHUFFLE codecs; read the whole array with get<T>() instead.
        if (uses_shuffle(m_cold.compressions[idx])) {
            throw std::runtime_error(
                "Array '" + key + "' uses a byte-shuffle compression codec and cannot be "
                "sliced. Use get<T>(\"" + key + "\") to read the full array.");
        }

        // Metadata block items must always be accessed as complete units
        // This is by design: metadata items are small and optimized for full access,
        // while slicing is designed for large block-compressed arrays
        if (m_cold.stored_in_metadata_flags[idx] || m_cold.block_infos[idx].empty()) {
            throw std::runtime_error(
                "Array '" + key + "' is stored in the metadata block and cannot be sliced. "
                "Metadata block items are designed for complete access only. "
                "Use meta.get(\"" + key + "\") to retrieve the full array.");
        }

        // Create IndexEntry from SoA for helper methods
        IndexEntry entry;
        entry.datatype = m_hot.dtypes[idx];
        entry.shape = m_cold.shapes[idx];
        entry.position = m_cold.file_positions[idx];
        entry.total_bytes = m_cold.compressed_sizes[idx];
        entry.compression = m_cold.compressions[idx];
        entry.blocks = m_cold.block_infos[idx];
        entry.block_size = m_config.block_size;
        entry.stored_in_metadata = m_cold.stored_in_metadata_flags[idx];

        // Create slice specification (pure)
        SliceSpec spec = createSliceSpec(entry.shape, slices, sizeof(T));

        // Map to blocks (pure)
        BlockMap block_map = createBlockMap(entry, spec);
        LOG_TRACE("get_slice: BlockMap has ", block_map.block_indices.size(), " blocks");

        // Create extraction plan (pure)
        ExtractionPlan plan = createExtractionPlan(spec, block_map, entry.shape, entry.blocks, sizeof(T));
        LOG_TRACE("get_slice: ExtractionPlan has ", plan.ranges.size(), " ranges");

        // Read blocks
        std::vector<char> compressed = readBlocks(entry, block_map);
        LOG_TRACE("get_slice: Read ", compressed.size(), " compressed bytes");

        // Adjust block offsets for decompression
        // The compressed buffer contains selected blocks sequentially,
        // but decompressBlocks expects offsets to match the buffer layout
        std::vector<BlockInfo> adjusted_blocks;
        size_t current_offset = 0;
        for (size_t block_idx : block_map.block_indices) {
            BlockInfo adjusted = entry.blocks[block_idx];
            adjusted.offset = current_offset;
            adjusted_blocks.push_back(adjusted);
            current_offset += adjusted.compressed_size;
        }

        // Create sequential indices for adjusted blocks (0, 1, 2, ...)
        std::vector<size_t> sequential_indices;
        for (size_t i = 0; i < adjusted_blocks.size(); ++i) {
            sequential_indices.push_back(i);
        }

        // Decompress blocks (with parallel decompression if thread pool available)
        std::vector<char> decompressed = decompressBlocks(
            compressed,
            adjusted_blocks,
            entry.compression,
            sequential_indices,
            m_thread_pool.get());

        // Extract sliced elements (pure)
        std::vector<char> extracted = extractElements(decompressed, plan, adjusted_blocks, sequential_indices, sizeof(T));

        // Create output array
        NDArray<T> result(spec.output_shape);
        if (!extracted.empty()) {
            // Check if any slice has step > 1
            bool has_step = false;
            for (const auto& slice : spec.slices) {
                if (slice.step > 1) {
                    has_step = true;
                    break;
                }
            }

            if (!has_step) {
                // Simple contiguous copy
                std::memcpy(result.data().data(), extracted.data(), extracted.size());
            } else {
                // Apply step filtering
                const T* src = reinterpret_cast<const T*>(extracted.data());
                T* dst = result.data().data();

                if (m_cold.shapes[idx].size() == 1) {
                    // 1D case with step
                    size_t step = spec.slices[0].step;
                    size_t src_count = (spec.slices[0].stop - spec.slices[0].start);
                    for (size_t i = 0, j = 0; i < src_count; i += step, ++j) {
                        dst[j] = src[i];
                    }
                } else if (m_cold.shapes[idx].size() == 2) {
                    // 2D case - apply step to each dimension
                    size_t step0 = spec.slices[0].step;
                    size_t step1 = spec.slices[1].step;
                    size_t src_rows = (spec.slices[0].stop - spec.slices[0].start);
                    size_t src_cols = (spec.slices[1].stop - spec.slices[1].start);

                    size_t dst_idx = 0;
                    for (size_t i = 0; i < src_rows; i += step0) {
                        for (size_t j = 0; j < src_cols; j += step1) {
                            dst[dst_idx++] = src[i * src_cols + j];
                        }
                    }
                } else if (m_cold.shapes[idx].size() == 3) {
                    // 3D case - apply step to each dimension
                    size_t step0 = spec.slices[0].step;
                    size_t step1 = spec.slices[1].step;
                    size_t step2 = spec.slices[2].step;
                    size_t src_dim0 = (spec.slices[0].stop - spec.slices[0].start);
                    size_t src_dim1 = (spec.slices[1].stop - spec.slices[1].start);
                    size_t src_dim2 = (spec.slices[2].stop - spec.slices[2].start);

                    size_t dst_idx = 0;
                    for (size_t i = 0; i < src_dim0; i += step0) {
                        for (size_t j = 0; j < src_dim1; j += step1) {
                            for (size_t k = 0; k < src_dim2; k += step2) {
                                dst[dst_idx++] = src[i * src_dim1 * src_dim2 + j * src_dim2 + k];
                            }
                        }
                    }
                }
            }
        }

        return result;
    }

    /**
     * @brief Checks if an array supports slicing
     *
     * Arrays stored in the metadata block cannot be sliced - they must be
     * accessed as complete units. Only separately stored arrays with block
     * compression support efficient slicing.
     *
     * @param key Array key to check
     * @return True if array supports get_slice(), false if only meta.get() works
     *
     * Example:
     *   if (store.is_sliceable("large_data")) {
     *       auto slice = store.get_slice<double>("large_data", {{0, 1000}});
     *   } else {
     *       auto full = store.meta.get("small_data");
     *   }
     */
    bool is_sliceable(const std::string& key) const {
        auto it = m_key_to_index.find(key);
        if (it == m_key_to_index.end()) {
            // Not in array storage - check if it's metadata
            if (meta.contains(key)) {
                return false;  // Metadata is not sliceable
            }
            return false;  // Key doesn't exist
        }
        size_t idx = it->second;

        // Sliceable if stored separately with block structure, and not byte-shuffled
        // (shuffle precludes per-block element extraction — see get_slice()).
        return !m_cold.stored_in_metadata_flags[idx]
               && !m_cold.block_infos[idx].empty()
               && !uses_shuffle(m_cold.compressions[idx]);
    }

    /**
     * @brief Get the element data type of a stored array.
     *
     * Looks up the array namespace (values stored via put()/put<T>()). This lets
     * callers dispatch on the concrete type without probing every get<T>()
     * overload. For metadata-namespace values use meta.get(key)->dtype instead.
     *
     * @param key Array key
     * @return DataType of the stored array
     * @throws std::runtime_error if the key is not an array (not found in array storage)
     */
    DataType dtype_of(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_key_to_index.find(key);
        if (it == m_key_to_index.end()) {
            throw std::runtime_error("Key not found in array storage: " + key);
        }
        return m_hot.dtypes[it->second];
    }

    /**
     * @brief Check whether a key exists in the dataset.
     *
     * Returns true if `key` names either a stored array (array namespace) or a
     * metadata-block value (metadata namespace). Layer-prefixed internal keys are
     * matched as-is. Use this for membership tests; it does not load any data.
     *
     * @param key Key to look up
     * @return true if the key exists in either namespace
     */
    bool contains(const std::string& key) const {
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            if (m_key_to_index.find(key) != m_key_to_index.end()) {
                return true;
            }
        }
        // meta has its own synchronization; check the metadata namespace too.
        return meta.contains(key);
    }

    /**
     * @brief Checks if metadata block has been loaded
     * @return True if loaded
     */
    bool is_metadata_loaded() const {
        return m_metadata_loaded;
    }

private:
    // =========================================================================
    // N-Dimensional Array Slicing 
    // =========================================================================

    /**
     * @brief Converts n-dimensional indices to flat index (row-major)
     */
    inline size_t flatIndex(const std::vector<size_t>& indices,
                           const std::vector<size_t>& shape) const {
        size_t flat = 0;
        size_t stride = 1;

        // Row-major order (C/C++ convention)
        for (int dim = (int)shape.size() - 1; dim >= 0; --dim) {
            flat += indices[dim] * stride;
            stride *= shape[dim];
        }
        return flat;
    }

    /**
     * @brief Creates SliceSpec from user slices + array shape
     *
     * Algorithm:
     * 1. Validate slice count matches dimensions
     * 2. Normalize slices (handle defaults, bounds)
     * 3. Calculate output shape
     * 4. Determine contiguity properties
     */
    SliceSpec createSliceSpec(
        const std::vector<size_t>& shape,
        const std::vector<Slice>& slices,
        size_t element_size) const {

        SliceSpec spec;
        spec.element_size = element_size;
        spec.is_contiguous = true;
        spec.is_full_rows = false;

        // Validate dimensions
        if (slices.size() > shape.size()) {
            throw std::runtime_error("Too many slice dimensions");
        }

        // Normalize slices and calculate output shape
        spec.slices.reserve(shape.size());
        spec.output_shape.reserve(shape.size());

        for (size_t dim = 0; dim < shape.size(); ++dim) {
            Slice s;
            if (dim < slices.size()) {
                s = slices[dim];
                // Validate bounds
                if (s.start >= shape[dim] || s.stop > shape[dim] || s.start >= s.stop) {
                    throw std::runtime_error("Slice out of bounds for dimension " + std::to_string(dim));
                }
            } else {
                // Full slice for unspecified dimensions
                s = Slice{0, shape[dim], 1};
            }

            spec.slices.push_back(s);
            spec.output_shape.push_back(s.length());

            // Check contiguity
            if (s.start != 0 || s.stop != shape[dim] || s.step != 1) {
                if (dim == 0) {
                    // First dimension: partial is OK
                } else {
                    // Other dimensions: partial means not fully contiguous
                    if (s.start != 0 || s.stop != shape[dim]) {
                        spec.is_contiguous = false;
                    }
                }
            }
        }

        // Check for full rows (2D optimization)
        if (shape.size() == 2 && slices.size() >= 2) {
            if (spec.slices[1].start == 0 &&
                spec.slices[1].stop == shape[1] &&
                spec.slices[1].step == 1) {
                spec.is_full_rows = true;
            }
        }

        // Calculate total elements
        spec.total_elements = 1;
        for (size_t dim_size : spec.output_shape) {
            spec.total_elements *= dim_size;
        }

        return spec;
    }

    /**
     * @brief Maps n-dimensional slice to block indices
     *
     * Algorithm:
     * 1. Convert multi-dim slices to flat element ranges
     * 2. Calculate elements_per_block
     * 3. Map element ranges to block ranges
     * 4. Detect contiguous blocks
     * 5. Build block read plan
     */
    BlockMap createBlockMap(
        const IndexEntry& entry,
        const SliceSpec& spec) const {

        BlockMap block_map;
        block_map.total_compressed_bytes = 0;
        block_map.blocks_contiguous = false;
        block_map.contiguous_start_offset = 0;

        if (entry.blocks.empty()) {
            return block_map;
        }

        // Calculate elements per block
        size_t elements_per_block = entry.block_size / spec.element_size;
        if (elements_per_block == 0) {
            throw std::runtime_error("Block size too small for element type");
        }

        // Calculate total elements in array
        size_t total_elements = 1;
        for (size_t dim_size : entry.shape) {
            total_elements *= dim_size;
        }

        // For contiguous slices starting at first element
        if (spec.is_contiguous && spec.slices[0].start == 0) {
            // Calculate flat element range
            size_t start_element = 0;
            size_t end_element = spec.total_elements;

            // Find blocks covering this range
            size_t first_block = start_element / elements_per_block;
            size_t last_block = (end_element - 1) / elements_per_block;

            for (size_t i = first_block; i <= last_block && i < entry.blocks.size(); ++i) {
                block_map.block_indices.push_back(i);
                block_map.block_offsets.push_back(entry.blocks[i].offset);
                block_map.block_sizes.push_back(entry.blocks[i].compressed_size);
                block_map.total_compressed_bytes += entry.blocks[i].compressed_size;
            }
        } else {
            // Non-contiguous or partial: need to find affected blocks
            // For now, implement simple row-based slicing (most common case)
            if (entry.shape.size() == 2 && spec.is_full_rows) {
                // 2D array, taking full rows
                size_t row_size = entry.shape[1];
                size_t start_element = spec.slices[0].start * row_size;
                size_t end_element = spec.slices[0].stop * row_size;

                size_t first_block = start_element / elements_per_block;
                size_t last_block = (end_element - 1) / elements_per_block;

                for (size_t i = first_block; i <= last_block && i < entry.blocks.size(); ++i) {
                    block_map.block_indices.push_back(i);
                    block_map.block_offsets.push_back(entry.blocks[i].offset);
                    block_map.block_sizes.push_back(entry.blocks[i].compressed_size);
                    block_map.total_compressed_bytes += entry.blocks[i].compressed_size;
                }
            } else {
                // General case: analyze all dimensions
                // For initial implementation, use conservative approach
                std::set<size_t> affected_blocks;

                // Calculate which blocks might be affected
                // This is simplified - full implementation would trace exact elements
                std::vector<size_t> start_indices;
                std::vector<size_t> end_indices;
                for (const auto& s : spec.slices) {
                    start_indices.push_back(s.start);
                    end_indices.push_back(s.stop - 1);
                }

                size_t start_flat = flatIndex(start_indices, entry.shape);
                size_t end_flat = flatIndex(end_indices, entry.shape);

                size_t first_block = start_flat / elements_per_block;
                size_t last_block = end_flat / elements_per_block;

                for (size_t i = first_block; i <= last_block && i < entry.blocks.size(); ++i) {
                    affected_blocks.insert(i);
                }

                for (size_t block_idx : affected_blocks) {
                    block_map.block_indices.push_back(block_idx);
                    block_map.block_offsets.push_back(entry.blocks[block_idx].offset);
                    block_map.block_sizes.push_back(entry.blocks[block_idx].compressed_size);
                    block_map.total_compressed_bytes += entry.blocks[block_idx].compressed_size;
                }
            }
        }

        // Check if blocks are contiguous
        if (!block_map.block_indices.empty()) {
            block_map.blocks_contiguous = true;
            for (size_t i = 1; i < block_map.block_indices.size(); ++i) {
                if (block_map.block_indices[i] != block_map.block_indices[i-1] + 1) {
                    block_map.blocks_contiguous = false;
                    break;
                }
            }

            // Check if contiguous in file
            if (block_map.blocks_contiguous && block_map.block_indices.size() > 1) {
                for (size_t i = 1; i < block_map.block_indices.size(); ++i) {
                    size_t prev_idx = block_map.block_indices[i-1];
                    size_t curr_idx = block_map.block_indices[i];
                    const BlockInfo& prev = entry.blocks[prev_idx];
                    const BlockInfo& curr = entry.blocks[curr_idx];

                    if (curr.offset != prev.offset + prev.compressed_size) {
                        block_map.blocks_contiguous = false;
                        break;
                    }
                }
            }

            if (block_map.blocks_contiguous) {
                block_map.contiguous_start_offset = entry.blocks[block_map.block_indices[0]].offset;
            }
        }

        return block_map;
    }

    /**
     * @brief Creates plan to extract elements from decompressed blocks
     *
     * Handles both contiguous and non-contiguous slicing:
     * - Contiguous: single range per block
     * - Non-contiguous (e.g., partial columns in 2D): multiple ranges for each "row"
     */
    ExtractionPlan createExtractionPlan(
        const SliceSpec& spec,
        const BlockMap& block_map,
        const std::vector<size_t>& shape,
        const std::vector<BlockInfo>& blocks,
        size_t element_size) const {

        ExtractionPlan plan;
        // When step > 1, we extract more elements than the final output
        // Calculate the extraction size (stop - start for each dimension)
        size_t extraction_elements = 1;
        for (const auto& slice : spec.slices) {
            extraction_elements *= (slice.stop - slice.start);
        }
        plan.total_elements = extraction_elements;

        if (block_map.block_indices.empty() || shape.empty()) {
            return plan;
        }

        // Calculate elements per block from uncompressed size
        size_t elements_per_block = blocks[0].uncompressed_size / element_size;

        // Calculate total elements in array
        size_t total_elements = 1;
        for (size_t dim_size : shape) {
            total_elements *= dim_size;
        }

        size_t output_offset = 0;


        // Case 1: Full contiguous slice (1D or full array access)
        if (spec.is_contiguous && shape.size() == 1) {
            // 1D contiguous slicing
            size_t start_element_global = spec.slices[0].start;
            size_t end_element_global = spec.slices[0].stop;

            for (size_t i = 0; i < block_map.block_indices.size(); ++i) {
                size_t block_idx_in_array = block_map.block_indices[i];
                size_t block_start_element = block_idx_in_array * elements_per_block;
                size_t block_end_element = std::min(block_start_element + elements_per_block, total_elements);

                if (block_end_element > start_element_global && block_start_element < end_element_global) {
                    size_t overlap_start = std::max(block_start_element, start_element_global);
                    size_t overlap_end = std::min(block_end_element, end_element_global);

                    ExtractionPlan::ElementRange range;
                    range.block_idx = i;
                    range.element_start = overlap_start - block_start_element;
                    range.element_count = overlap_end - overlap_start;
                    range.output_offset = output_offset;
                    plan.ranges.push_back(range);

                    output_offset += range.element_count;
                }
            }
        }
        // Case 2: 2D full rows (each row is contiguous)
        else if (spec.is_full_rows && shape.size() == 2) {
            size_t row_size = shape[1];
            size_t start_row = spec.slices[0].start;
            size_t end_row = spec.slices[0].stop;

            for (size_t row = start_row; row < end_row; ++row) {
                size_t row_start_element = row * row_size;
                size_t row_end_element = row_start_element + row_size;

                // Find which block(s) contain this row
                for (size_t i = 0; i < block_map.block_indices.size(); ++i) {
                    size_t block_idx_in_array = block_map.block_indices[i];
                    size_t block_start_element = block_idx_in_array * elements_per_block;
                    size_t block_end_element = std::min(block_start_element + elements_per_block, total_elements);

                    if (block_end_element > row_start_element && block_start_element < row_end_element) {
                        size_t overlap_start = std::max(block_start_element, row_start_element);
                        size_t overlap_end = std::min(block_end_element, row_end_element);

                        ExtractionPlan::ElementRange range;
                        range.block_idx = i;
                        range.element_start = overlap_start - block_start_element;
                        range.element_count = overlap_end - overlap_start;
                        range.output_offset = output_offset;
                        plan.ranges.push_back(range);

                        output_offset += range.element_count;
                    }
                }
            }
        }
        // Case 3: Multi-dimensional non-contiguous (e.g., partial columns in 2D, 3D hyperslabs)
        else {
            // General case: iterate through all combinations and extract contiguous chunks
            // For 2D: iterate rows, extract column range for each row
            // For nD: recursively extract contiguous innermost dimension chunks

            if (shape.size() == 2) {
                // 2D partial column slicing
                size_t row_size = shape[1];
                size_t start_row = spec.slices[0].start;
                size_t end_row = spec.slices[0].stop;
                size_t start_col = spec.slices[1].start;
                size_t end_col = spec.slices[1].stop;
                size_t col_count = end_col - start_col;

                // Extract each row's column subset
                for (size_t row = start_row; row < end_row; ++row) {
                    size_t row_col_start = row * row_size + start_col;
                    size_t row_col_end = row * row_size + end_col;

                    // Find which block contains this chunk
                    for (size_t i = 0; i < block_map.block_indices.size(); ++i) {
                        size_t block_idx_in_array = block_map.block_indices[i];
                        size_t block_start_element = block_idx_in_array * elements_per_block;
                        size_t block_end_element = std::min(block_start_element + elements_per_block, total_elements);

                        if (block_end_element > row_col_start && block_start_element < row_col_end) {
                            size_t overlap_start = std::max(block_start_element, row_col_start);
                            size_t overlap_end = std::min(block_end_element, row_col_end);

                            ExtractionPlan::ElementRange range;
                            range.block_idx = i;
                            range.element_start = overlap_start - block_start_element;
                            range.element_count = overlap_end - overlap_start;
                            range.output_offset = output_offset;
                            plan.ranges.push_back(range);

                            output_offset += range.element_count;
                        }
                    }
                }
            }
            // Case 4: 3D hyperslabs
            else if (shape.size() == 3) {
                // 3D slicing: iterate through outer two dimensions, extract innermost dimension
                size_t dim0_size = shape[0];
                size_t dim1_size = shape[1];
                size_t dim2_size = shape[2];

                size_t start_dim0 = spec.slices[0].start;
                size_t end_dim0 = spec.slices[0].stop;
                size_t start_dim1 = spec.slices[1].start;
                size_t end_dim1 = spec.slices[1].stop;
                size_t start_dim2 = spec.slices[2].start;
                size_t end_dim2 = spec.slices[2].stop;

                // Iterate through outer dimensions
                for (size_t i = start_dim0; i < end_dim0; ++i) {
                    for (size_t j = start_dim1; j < end_dim1; ++j) {
                        // Calculate flat index for this "row" in innermost dimension
                        size_t row_start = (i * dim1_size * dim2_size) + (j * dim2_size) + start_dim2;
                        size_t row_end = (i * dim1_size * dim2_size) + (j * dim2_size) + end_dim2;

                        // Find which block(s) contain this range
                        for (size_t b = 0; b < block_map.block_indices.size(); ++b) {
                            size_t block_idx_in_array = block_map.block_indices[b];
                            size_t block_start_element = block_idx_in_array * elements_per_block;
                            size_t block_end_element = std::min(block_start_element + elements_per_block, total_elements);

                            if (block_end_element > row_start && block_start_element < row_end) {
                                size_t overlap_start = std::max(block_start_element, row_start);
                                size_t overlap_end = std::min(block_end_element, row_end);

                                ExtractionPlan::ElementRange range;
                                range.block_idx = b;
                                range.element_start = overlap_start - block_start_element;
                                range.element_count = overlap_end - overlap_start;
                                range.output_offset = output_offset;
                                plan.ranges.push_back(range);

                                output_offset += range.element_count;
                            }
                        }
                    }
                }
            }
            else {
                throw std::runtime_error("Slicing only supported for 1D, 2D, and 3D arrays");
            }
        }

        // Sanity check: output_offset should equal total_elements
        if (output_offset != spec.total_elements) {
            LOG_TRACE("createExtractionPlan: WARNING! output_offset=", output_offset,
                     " != spec.total_elements=", spec.total_elements);
        }

        return plan;
    }

    /**
     * @brief Reads blocks according to BlockMap
     *
     * Optimization:
     * - If blocks_contiguous: single seekg + read
     * - Else: batch multiple seeks and reads
     * - For S3: minimize range requests
     */
    std::vector<char> readBlocks(
        const IndexEntry& entry,
        const BlockMap& block_map) {

        std::vector<char> result;
        if (block_map.block_indices.empty()) {
            return result;
        }

        // All reads go through the dataset's persistent reader (one reused
        // connection for remote files; no per-read HEAD/handshake).
        if (!reader().good()) {
            throw std::runtime_error("Failed to open file for reading blocks: " + m_filename);
        }

        if (block_map.blocks_contiguous) {
            // Contiguous blocks: fetch the whole span in a SINGLE ranged request.
            result = read_range(entry.position + block_map.contiguous_start_offset,
                                block_map.total_compressed_bytes);
            if (result.size() != block_map.total_compressed_bytes) {
                throw std::runtime_error("Short read of contiguous blocks (expected " +
                    std::to_string(block_map.total_compressed_bytes) + ", got " +
                    std::to_string(result.size()) + ")");
            }
            return result;
        }

        // Non-contiguous blocks (e.g. a partial slice): coalesce the requested
        // blocks into the fewest maximal contiguous spans, fetch each span in one
        // request, then copy the individual blocks out. This turns "one request
        // per block" into "one request per run of adjacent blocks".
        result.resize(block_map.total_compressed_bytes);

        // Sort the requested block indices by their file offset so adjacent
        // blocks can be merged into a single range.
        std::vector<size_t> ordered = block_map.block_indices;
        std::sort(ordered.begin(), ordered.end(), [&](size_t a, size_t b) {
            return entry.blocks[a].offset < entry.blocks[b].offset;
        });

        // Map each block index -> where its bytes go in `result` (result is laid
        // out in the caller's original block_indices order).
        std::unordered_map<size_t, size_t> out_offset;
        {
            size_t running = 0;
            for (size_t bi : block_map.block_indices) {
                out_offset[bi] = running;
                running += entry.blocks[bi].compressed_size;
            }
        }

        size_t run_start = 0;   // index into `ordered` where the current run begins
        while (run_start < ordered.size()) {
            size_t run_end = run_start;  // inclusive
            size_t span_begin = entry.blocks[ordered[run_start]].offset;
            size_t span_end   = span_begin + entry.blocks[ordered[run_start]].compressed_size;

            // Extend the run while the next block starts exactly where this one
            // ended (truly contiguous on disk).
            while (run_end + 1 < ordered.size()) {
                const BlockInfo& next = entry.blocks[ordered[run_end + 1]];
                if (next.offset == span_end) {
                    span_end += next.compressed_size;
                    run_end++;
                } else {
                    break;
                }
            }

            // Fetch this run in a single request.
            std::vector<char> span = read_range(entry.position + span_begin,
                                                span_end - span_begin);
            if (span.size() != span_end - span_begin) {
                throw std::runtime_error("Short read of block run (expected " +
                    std::to_string(span_end - span_begin) + ", got " +
                    std::to_string(span.size()) + ")");
            }

            // Copy each block in the run to its slot in `result`.
            for (size_t k = run_start; k <= run_end; ++k) {
                const BlockInfo& blk = entry.blocks[ordered[k]];
                size_t within_span = blk.offset - span_begin;
                std::memcpy(result.data() + out_offset[ordered[k]],
                            span.data() + within_span, blk.compressed_size);
            }

            run_start = run_end + 1;
        }

        return result; // NRVO avoids copy
    }

    /**
     * @brief Extracts sliced elements from decompressed blocks
     *
     * Algorithm:
     * 1. Pre-allocate output buffer (exact size)
     * 2. For each range in ExtractionPlan:
     *    - Memcpy from block to output
     * 3. Return contiguous result
     */
    std::vector<char> extractElements(
        const std::vector<char>& decompressed,
        const ExtractionPlan& plan,
        const std::vector<BlockInfo>& blocks,
        const std::vector<size_t>& block_indices,
        size_t element_size) const {

        LOG_TRACE("extractElements: decompressed size=", decompressed.size(),
                 " plan.ranges=", plan.ranges.size(),
                 " element_size=", element_size);

        std::vector<char> result;
        if (plan.ranges.empty()) {
            LOG_TRACE("extractElements: No ranges to extract");
            return result;
        }

        // Pre-allocate exact size
        size_t total_bytes = plan.total_elements * element_size;
        result.resize(total_bytes);
        LOG_TRACE("extractElements: Allocated result buffer of ", total_bytes, " bytes");

        // Build block offset table in decompressed buffer
        // The decompressed data contains blocks appended sequentially
        std::vector<size_t> block_offsets;
        block_offsets.push_back(0);
        for (size_t i = 0; i < block_indices.size(); ++i) {
            size_t block_idx = block_indices[i];
            size_t offset = block_offsets.back() + blocks[block_idx].uncompressed_size;
            block_offsets.push_back(offset);
            LOG_TRACE("extractElements: Block ", i, " (idx=", block_idx, ") offset=",
                     block_offsets[i], " size=", blocks[block_idx].uncompressed_size);
        }

        // Extract each range
        for (size_t r = 0; r < plan.ranges.size(); ++r) {
            const auto& range = plan.ranges[r];
            LOG_TRACE("extractElements: Range ", r, " block_idx=", range.block_idx,
                     " element_start=", range.element_start,
                     " element_count=", range.element_count,
                     " output_offset=", range.output_offset);

            // range.block_idx is the index in the decompressed buffer (0, 1, 2, ...)
            // which corresponds to block_indices[range.block_idx] in the original array
            if (range.block_idx >= block_offsets.size() - 1) {
                LOG_TRACE("extractElements: Skipping range - block_idx out of bounds");
                continue; // Safety check
            }

            size_t block_start_in_decompressed = block_offsets[range.block_idx];
            size_t src_offset = block_start_in_decompressed + (range.element_start * element_size);
            size_t dst_offset = range.output_offset * element_size;
            size_t copy_bytes = range.element_count * element_size;

            LOG_TRACE("extractElements: Copying ", copy_bytes, " bytes from src_offset=",
                     src_offset, " to dst_offset=", dst_offset);

            if (src_offset + copy_bytes <= decompressed.size() &&
                dst_offset + copy_bytes <= result.size()) {
                std::memcpy(result.data() + dst_offset,
                           decompressed.data() + src_offset,
                           copy_bytes);
                LOG_TRACE("extractElements: Copy successful");
            } else {
                LOG_TRACE("extractElements: Copy SKIPPED - bounds check failed");
            }
        }

        return result; // NRVO avoids copy
    }

public:
    /**
     * @brief Returns an iterator to the beginning of the index
     * @return Iterator to the first key-value pair
     */
    iterator begin() {
        return m_hot.keys.begin();
    }

    /**
     * @brief Returns an iterator to the end of the index
     * @return Iterator to one past the last key-value pair
     */
    iterator end() {
        return m_hot.keys.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the index
     * @return Const iterator to the first key-value pair
     */
    const_iterator begin() const {
        return m_hot.keys.begin();
    }

    /**
     * @brief Returns a const iterator to the end of the index
     * @return Iterator to one past the last key-value pair
     */
    const_iterator end() const {
        return m_hot.keys.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the index
     * @return Const iterator to the first key-value pair
     */
    const_iterator cbegin() const {
        return m_hot.keys.cbegin();
    }

    /**
     * @brief Returns a const iterator to the end of the index
     * @return Const iterator to one past the last key-value pair
     */
    const_iterator cend() const {
        return m_hot.keys.cend();
    }

    /**
     * @brief Removes a key-value pair from the store
     * @param key Key to remove
     */
    /**
     * @brief Returns the number of key-value pairs in the store
     * @return Number of key-value pairs
     */
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_hot.keys.size();
    }
};  // end of StarDataset class

//==============================================================================
// LayerView Method Implementations
//==============================================================================

template<typename T>
NDArray<T> LayerView::get(const std::string& key) {
    // Try to get from this layer first (using layer-prefixed key)
    // If not found, fall back to base layer
    std::string storage_key;
    if (m_layer_name == "__base__") {
        // Base layer uses unprefixed key
        storage_key = key;
    } else {
        // Non-base layer uses prefixed key: "__layer_<name>__:<key>"
        storage_key = "__layer_" + m_layer_name + "__:" + key;
    }

    // Try to get from this layer's storage
    try {
        return m_base->get<T>(storage_key);
    } catch (const std::runtime_error&) {
        // Not found in this layer. Fall back to the base layer only if inheritance
        // is enabled (OpenOptions.layer_inheritance); it is off by default.
        if (m_layer_name != "__base__" && m_base->layer_inheritance()) {
            try {
                return m_base->get<T>(key);  // Base uses unprefixed key
            } catch (const std::runtime_error&) {
                // Not in base either
                throw std::runtime_error("Key not found in layer or base: " + key);
            }
        } else {
            // Inheritance disabled, or already in the base layer: rethrow / report miss.
            if (m_layer_name != "__base__") {
                throw std::runtime_error("Key not found in layer: " + key);
            }
            throw;
        }
    }
}

template<typename T>
void LayerView::put(const std::string& key, NDArray<T>&& value) {
    // For non-base layers, use a layer-prefixed internal key to store separate data
    // Format: "__layer_<name>__:<key>"
    // This allows each layer to have its own version of the same logical key
    std::string storage_key;
    if (m_layer_name == "__base__") {
        storage_key = key;
    } else {
        storage_key = "__layer_" + m_layer_name + "__:" + key;
    }

    // Store with the internal key
    m_base->put(storage_key, std::move(value));

    // Update layer presence bitmaps using the LOGICAL key (not the storage key)
    // This allows queries to know which layers have this logical key
    if (m_layer_name != "__base__") {
        // For non-base layers, clear base presence and set layer presence
        m_base->set_layer_presence("__base__", key, false);
        m_base->set_layer_presence(m_layer_name, key, true);
    } else {
        // For base layer, set base presence
        m_base->set_layer_presence("__base__", key, true);
    }
}

//==============================================================================
// MetadataAccessor Method Implementations
//==============================================================================

// Helper functions for extracting type info from variant
static inline DataType extract_dtype_from_variant(const ValueVariant& var) {
    if (std::holds_alternative<NDArray<int8_t>>(var)) return DataType::INT8;
    if (std::holds_alternative<NDArray<int16_t>>(var)) return DataType::INT16;
    if (std::holds_alternative<NDArray<int32_t>>(var)) return DataType::INT32;
    if (std::holds_alternative<NDArray<int64_t>>(var)) return DataType::INT64;
    if (std::holds_alternative<NDArray<uint8_t>>(var)) return DataType::UINT8;
    if (std::holds_alternative<NDArray<uint16_t>>(var)) return DataType::UINT16;
    if (std::holds_alternative<NDArray<uint32_t>>(var)) return DataType::UINT32;
    if (std::holds_alternative<NDArray<uint64_t>>(var)) return DataType::UINT64;
    if (std::holds_alternative<NDArray<float>>(var)) return DataType::FLOAT32;
    if (std::holds_alternative<NDArray<double>>(var)) return DataType::FLOAT64;
    if (std::holds_alternative<NDArray<std::string>>(var)) return DataType::STRING;
    throw std::runtime_error("Unknown type in ValueVariant");
}

static inline std::vector<size_t> extract_shape_from_variant(const ValueVariant& var) {
    return std::visit([](auto&& arr) -> std::vector<size_t> {
        return std::vector<size_t>(arr.shape().begin(), arr.shape().end());
    }, var);
}

template<typename V>
void MetadataAccessor::put(const std::string& key, const V& value) {
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    using ElementType = typename V::value_type;
    DataType dtype = TypeToDataType<ElementType>::value;
    auto shape_vec = std::vector<size_t>(value.shape().begin(), value.shape().end());

    // Get or create key index in key registry
    uint16_t key_idx = m_store->m_key_registry.get_or_create(key);
    size_t base_layer_idx = m_store->m_layer_metadata_registry.get_layer_index("__base__");

    // Metadata uses completely separate namespace from arrays - only in layer metadata system
    auto& layer_metadata_map = m_store->m_layer_metadata_indices[base_layer_idx];
    auto it = layer_metadata_map.find(key_idx);

    if (it != layer_metadata_map.end()) {
        // Update existing metadata entry (only in layer metadata storage, not in m_hot.keys)
        size_t storage_idx = it->second;
        m_store->m_data_storage[storage_idx] = value;
        m_store->m_metadata_dtypes[storage_idx] = dtype;
        m_store->m_metadata_shapes[storage_idx] = shape_vec;
    } else {
        // Create new metadata entry (only in layer metadata storage, NOT in m_hot.keys)
        size_t storage_idx = m_store->m_data_storage.size();
        m_store->m_data_storage.push_back(value);
        m_store->m_metadata_dtypes[storage_idx] = dtype;
        m_store->m_metadata_shapes[storage_idx] = shape_vec;

        // Track in layer metadata system (separate from arrays)
        layer_metadata_map[key_idx] = storage_idx;
        m_store->m_layer_metadata_registry.key_indices[base_layer_idx].insert(key_idx);
    }

    m_store->m_flushed = false;
    m_store->m_header_dirty = true;
}

inline std::shared_ptr<MetadataValue> MetadataAccessor::get(const std::string& key) {
    // v1 format: Metadata is stored in per-layer blocks, not in m_hot/m_cold
    // Delegate to base layer's accessor
    LayerMetadataAccessor base_accessor(m_store, "__base__");
    return base_accessor.get(key);
}

template<typename V>
void MetadataAccessor::put_batch(const std::map<std::string, V>& values) {
    // v1 format: Use layer metadata system (separate namespace from arrays)
    for (const auto& [key, value] : values) {
        // Delegate to single put() which handles its own locking
        put(key, value);
    }
}

inline std::map<std::string, MetadataValue> MetadataAccessor::get_batch(
    const std::vector<std::string>& keys) {
    // v1 format: Use layer metadata system (separate namespace from arrays)
    std::map<std::string, MetadataValue> results;

    for (const auto& key : keys) {
        auto value = get(key);
        if (value) {
            results[key] = *value;
        }
    }

    return results;
}

inline std::map<std::string, MetadataValue> MetadataAccessor::get_all() {
    // v1 format: Get all metadata from base layer
    LayerMetadataAccessor base_accessor(m_store, "__base__");

    // Load base layer metadata
    m_store->load_layer_metadata("__base__");

    std::map<std::string, MetadataValue> results;
    size_t base_layer_idx = m_store->m_layer_metadata_registry.get_layer_index("__base__");

    // Iterate through all keys in base layer
    for (uint16_t key_idx : m_store->m_layer_metadata_registry.key_indices[base_layer_idx]) {
        const std::string& key = m_store->m_key_registry.names[key_idx];
        auto value = base_accessor.get(key);
        if (value) {
            results[key] = *value;
        }
    }

    return results;
}

inline void MetadataAccessor::remove(const std::string& key) {
    // v1 format: Remove from layer metadata system (separate namespace from arrays)
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    LOG_DEBUG("MetadataAccessor::remove called for key: ", key);

    // Check if key exists first
    if (!m_store->m_key_registry.contains(key)) {
        LOG_DEBUG("Key not in registry, returning");
        return;  // Key doesn't exist, nothing to remove
    }

    LOG_DEBUG("Key found in registry");
    uint16_t key_idx = m_store->m_key_registry.get_index(key);
    LOG_DEBUG("key_idx: ", key_idx);

    size_t base_layer_idx = m_store->m_layer_metadata_registry.get_layer_index("__base__");
    LOG_DEBUG("base_layer_idx: ", base_layer_idx);

    // Remove from layer metadata indices
    auto& layer_metadata_map = m_store->m_layer_metadata_indices[base_layer_idx];
    auto it = layer_metadata_map.find(key_idx);
    if (it != layer_metadata_map.end()) {
        LOG_DEBUG("Found in layer metadata map, removing");
        // Note: We don't remove from m_data_storage to avoid shifting indices
        // Just remove the mapping
        layer_metadata_map.erase(it);
        m_store->m_layer_metadata_registry.key_indices[base_layer_idx].erase(key_idx);
    } else {
        LOG_DEBUG("Not found in layer metadata map");
    }

    m_store->m_flushed = false;
    m_store->m_header_dirty = true;
    LOG_DEBUG("MetadataAccessor::remove completed");
}

inline void MetadataAccessor::clear() {
    // v1 format: Clear only metadata from base layer (separate namespace from arrays)
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    size_t base_layer_idx = m_store->m_layer_metadata_registry.get_layer_index("__base__");

    // Clear layer metadata indices
    m_store->m_layer_metadata_indices[base_layer_idx].clear();
    m_store->m_layer_metadata_registry.key_indices[base_layer_idx].clear();

    m_store->m_flushed = false;
    m_store->m_header_dirty = true;
}

inline bool MetadataAccessor::contains(const std::string& key) const {
    // v1 format: Check layer metadata registry (separate namespace from arrays)
    LayerMetadataAccessor base_accessor(m_store, "__base__");
    return base_accessor.contains(key);
}

//==============================================================================
// Layer Management Method Implementations
//==============================================================================

inline std::shared_ptr<LayerView> StarDataset::get_layer(const std::string& layer_name) {
    std::shared_lock<std::shared_mutex> lock(m_mutex);

    if (!has_layer(layer_name)) {
        throw std::runtime_error("Layer not found: " + layer_name);
    }

    return std::make_shared<LayerView>(shared_from_this(), layer_name);
}

inline std::shared_ptr<LayerView> StarDataset::create_layer(const std::string& layer_name) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);

    if (has_layer_unlocked(layer_name)) {
        throw std::runtime_error("Layer already exists: " + layer_name);
    }

    // Add layer to registry
    size_t layer_idx = m_layer_metadata_registry.add_layer(layer_name);

    // Resize tracking structures
    if (m_layer_metadata_loaded.size() <= layer_idx) {
        m_layer_metadata_loaded.resize(layer_idx + 1, false);
    }
    if (m_layer_metadata_indices.size() <= layer_idx) {
        m_layer_metadata_indices.resize(layer_idx + 1);
    }

    // Initialize empty bitmap for data arrays
    size_t bitmap_words = (m_hot.keys.size() + 63) / 64;
    m_layer_presence[layer_name] = std::vector<uint64_t>(bitmap_words, 0);

    m_header_dirty = true;  // Header changed
    m_flushed = false;      // Need to flush header changes

    return std::make_shared<LayerView>(shared_from_this(), layer_name);
}

//==============================================================================
// LayerMetadataAccessor Method Implementations
//==============================================================================

template<typename V>
inline void LayerMetadataAccessor::put(const std::string& key, const V& value) {
    // v1 format: Store in layer-specific metadata block
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    // Get or create key index in global registry
    uint16_t key_idx = m_store->m_key_registry.get_or_create(key);

    // Get layer index
    size_t layer_idx = m_store->m_layer_metadata_registry.get_layer_index(m_layer_name);

    // Ensure this layer's metadata is loaded (or mark as loaded if it's a new layer)
    if (!m_store->m_layer_metadata_loaded[layer_idx]) {
        // For new layers with no persisted data, just mark as loaded
        uint32_t block_size = m_store->m_layer_metadata_registry.block_sizes[layer_idx];
        if (block_size == 0) {
            // New layer, no data to load
            m_store->m_layer_metadata_loaded[layer_idx] = true;
        } else {
            // Existing layer, load from disk
            m_store->load_layer_metadata(m_layer_name);
        }
    }

    // Get or create storage index
    auto& layer_metadata_map = m_store->m_layer_metadata_indices[layer_idx];
    size_t storage_idx;

    auto it = layer_metadata_map.find(key_idx);
    if (it != layer_metadata_map.end()) {
        // Update existing entry
        storage_idx = it->second;
        if (storage_idx >= m_store->m_data_storage.size()) {
            throw std::runtime_error("Invalid storage index when updating metadata: " + key);
        }
        m_store->m_data_storage[storage_idx] = value;
    } else {
        // Create new entry
        storage_idx = m_store->m_data_storage.size();
        m_store->m_data_storage.push_back(value);
        layer_metadata_map[key_idx] = storage_idx;
    }

    // Store dtype and shape
    using ElementType = typename V::value_type;
    m_store->m_metadata_dtypes[storage_idx] = TypeToDataType<ElementType>::value;
    m_store->m_metadata_shapes[storage_idx] = std::vector<size_t>(value.shape().begin(), value.shape().end());

    // Add key to layer's key_indices set
    m_store->m_layer_metadata_registry.key_indices[layer_idx].insert(key_idx);

    m_store->m_header_dirty = true;
    m_store->m_flushed = false;  // New/changed data must be persisted on next flush
}

inline std::shared_ptr<MetadataValue> LayerMetadataAccessor::get(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    // Get key index from registry
    uint16_t key_idx;
    try {
        key_idx = m_store->m_key_registry.get_index(key);
    } catch (...) {
        return nullptr;  // Key doesn't exist anywhere
    }

    // Get layer indices
    size_t layer_idx = m_store->m_layer_metadata_registry.get_layer_index(m_layer_name);
    size_t base_layer_idx = m_store->m_layer_metadata_registry.get_layer_index("__base__");

    // Check current layer first (for override)
    if (m_layer_name != "__base__") {
        // Ensure current layer metadata is loaded
        if (!m_store->m_layer_metadata_loaded[layer_idx]) {
            m_store->load_layer_metadata(m_layer_name);
        }

        const auto& layer_key_indices = m_store->m_layer_metadata_registry.key_indices[layer_idx];
        if (layer_key_indices.find(key_idx) != layer_key_indices.end()) {
            // Key exists in current layer - return it
            const auto& layer_metadata_map = m_store->m_layer_metadata_indices[layer_idx];
            auto it = layer_metadata_map.find(key_idx);
            if (it != layer_metadata_map.end()) {
                size_t storage_idx = it->second;
                if (storage_idx >= m_store->m_data_storage.size()) {
                    throw std::runtime_error("Invalid storage index for key: " + key + " (index=" + std::to_string(storage_idx) + ", storage_size=" + std::to_string(m_store->m_data_storage.size()) + ")");
                }
                const ValueVariant& var = m_store->m_data_storage[storage_idx];

                auto meta = std::make_shared<MetadataValue>();
                meta->data = var;
                meta->dtype = m_store->m_metadata_dtypes[storage_idx];
                meta->shape = m_store->m_metadata_shapes[storage_idx];
                return meta;
            }
        }
    }

    // Fall back to base layer only if inheritance is enabled (off by default).
    // For a non-base layer with inheritance disabled, a miss stays a miss.
    if (m_layer_name != "__base__" && !m_store->layer_inheritance()) {
        return nullptr;
    }

    if (!m_store->m_layer_metadata_loaded[base_layer_idx]) {
        m_store->load_layer_metadata("__base__");
    }

    const auto& base_key_indices = m_store->m_layer_metadata_registry.key_indices[base_layer_idx];
    if (base_key_indices.find(key_idx) == base_key_indices.end()) {
        return nullptr;  // Key not in base layer either
    }

    const auto& base_metadata_map = m_store->m_layer_metadata_indices[base_layer_idx];
    auto it = base_metadata_map.find(key_idx);
    if (it == base_metadata_map.end()) {
        return nullptr;  // Not loaded yet (shouldn't happen after load_layer_metadata)
    }

    size_t storage_idx = it->second;
    if (storage_idx >= m_store->m_data_storage.size()) {
        throw std::runtime_error("Invalid storage index for key: " + key + " (index=" + std::to_string(storage_idx) + ", storage_size=" + std::to_string(m_store->m_data_storage.size()) + ")");
    }
    const ValueVariant& var = m_store->m_data_storage[storage_idx];

    auto meta = std::make_shared<MetadataValue>();
    meta->data = var;
    meta->dtype = m_store->m_metadata_dtypes[storage_idx];
    meta->shape = m_store->m_metadata_shapes[storage_idx];
    return meta;
}

inline bool LayerMetadataAccessor::contains(const std::string& key) const {
    // v1 format: Check layer metadata registry (O(1) header-only)
    try {
        m_store->resolve_metadata_inheritance(key, m_layer_name);
        return true;
    } catch (...) {
        return false;
    }
}

inline void LayerMetadataAccessor::remove(const std::string& key) {
    // Mirror put(): the key is stored under its plain name in THIS layer's
    // metadata index (not a "__layer_..." prefixed key in the base layer).
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    if (!m_store->m_key_registry.contains(key)) {
        return;  // Key doesn't exist, nothing to remove
    }
    uint16_t key_idx = m_store->m_key_registry.get_index(key);

    size_t layer_idx = m_store->m_layer_metadata_registry.get_layer_index(m_layer_name);

    // Remove the mapping from this layer's storage index and key set.
    // (We intentionally leave m_data_storage untouched to avoid shifting the
    // storage indices that other keys still reference.)
    auto& layer_metadata_map = m_store->m_layer_metadata_indices[layer_idx];
    auto it = layer_metadata_map.find(key_idx);
    if (it != layer_metadata_map.end()) {
        layer_metadata_map.erase(it);
        m_store->m_layer_metadata_registry.key_indices[layer_idx].erase(key_idx);
        m_store->m_flushed = false;
        m_store->m_header_dirty = true;
    }
}

inline std::vector<std::string> LayerMetadataAccessor::keys() const {
    // v1 format: Return all keys visible to this layer (layer + base, with layer overriding base)
    std::vector<std::string> result;
    std::set<std::string> seen;

    size_t layer_idx = m_store->m_layer_metadata_registry.get_layer_index(m_layer_name);
    size_t base_layer_idx = m_store->m_layer_metadata_registry.get_layer_index("__base__");

    // Get all key indices for this layer
    const auto& layer_key_indices = m_store->m_layer_metadata_registry.key_indices[layer_idx];
    for (uint16_t key_idx : layer_key_indices) {
        const std::string& key_name = m_store->m_key_registry.names[key_idx];
        result.push_back(key_name);
        seen.insert(key_name);
    }

    // Add base keys that aren't overridden — only when inheritance is enabled
    // (off by default).
    if (m_layer_name != "__base__" && m_store->layer_inheritance()) {
        const auto& base_key_indices = m_store->m_layer_metadata_registry.key_indices[base_layer_idx];
        for (uint16_t key_idx : base_key_indices) {
            const std::string& key_name = m_store->m_key_registry.names[key_idx];
            if (seen.find(key_name) == seen.end()) {
                result.push_back(key_name);
            }
        }
    }

    return result;
}

//==============================================================================
// LayerView Method Implementations
//==============================================================================

inline bool LayerView::contains(const std::string& key) const {
    if (m_base->key_in_layer(key, m_layer_name)) return true;
    // Consult the base layer only if inheritance is enabled (off by default).
    if (m_layer_name != "__base__" && !m_base->layer_inheritance()) return false;
    return m_base->key_in_layer(key, "__base__");
}

inline std::vector<std::string> LayerView::keys() const {
    std::vector<std::string> result;
    std::set<std::string> seen;

    // Get array keys
    auto base_keys = m_base->m_hot.keys;
    for (const auto& k : base_keys) {
        if (contains(k)) {
            result.push_back(k);
            seen.insert(k);
        }
    }

    // Get metadata keys from layer and base
    size_t layer_idx = m_base->m_layer_metadata_registry.get_layer_index(m_layer_name);
    size_t base_idx = m_base->m_layer_metadata_registry.get_layer_index("__base__");

    // Add layer metadata keys
    for (uint16_t key_idx : m_base->m_layer_metadata_registry.key_indices[layer_idx]) {
        const std::string& key = m_base->m_key_registry.names[key_idx];
        if (seen.find(key) == seen.end()) {
            result.push_back(key);
            seen.insert(key);
        }
    }

    // Add base metadata keys — only when inheritance is enabled (off by default)
    // and this isn't already the base layer.
    if (m_layer_name == "__base__" || m_base->layer_inheritance()) {
        for (uint16_t key_idx : m_base->m_layer_metadata_registry.key_indices[base_idx]) {
            const std::string& key = m_base->m_key_registry.names[key_idx];
            if (seen.find(key) == seen.end()) {
                result.push_back(key);
            }
        }
    }

    return result;
}

} // namespace star
