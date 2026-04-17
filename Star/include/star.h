#pragma once 

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
#include <variant>
#include <vector>
#include <chrono>
#include <shared_mutex>
#include <mutex>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>
#include <future>


// force it for now
#define ENABLE_CURL 1
#define ENABLE_ZLIB 1

#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

#ifdef ENABLE_LZ4
#include <lz4.h>
#endif

// Library version constants
#ifndef STAR_VERSION_MAJOR
#define STAR_VERSION_MAJOR 1
#endif
#ifndef STAR_VERSION_MINOR
#define STAR_VERSION_MINOR 0
#endif
#ifndef STAR_VERSION_PATCH
#define STAR_VERSION_PATCH 0
#endif

//==============================================================================
// STAR Namespace
//==============================================================================

namespace star {

inline const std::string PROJECT_NAME = "STARDS 0.2.0";
inline const char* MAGIC_STRING = "STARDS";
inline const size_t MAGIC_STRING_LENGTH = 6;

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
// File Format Structures
//==============================================================================

/**
 * @brief File header structure (23 bytes fixed size)
 *
 * Layout:
 * - magic[6]: "STARDS" magic string (6 bytes)
 * - format_version: File format version (1 byte, currently 2)
 * - header_size: Total size of header + index section (8 bytes)
 * - entry_count: Number of entries in the index (8 bytes)
 *
 * Note: Software/library version is NOT stored in the file, only the format version.
 */
struct FileHeader {
    char magic[MAGIC_STRING_LENGTH] = {'S','T','A','R','D','S'};
    uint8_t format_version = 2;
    uint64_t header_size = 0;
    uint64_t entry_count = 0;

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
        return MAGIC_STRING_LENGTH + sizeof(uint8_t) + sizeof(uint64_t) * 2;  // 23 bytes
    }

    /**
     * @brief Write header to stream
     */
    void write(std::ostream& os) const {
        os.write(magic, MAGIC_STRING_LENGTH);
        os.write(reinterpret_cast<const char*>(&format_version), sizeof(format_version));
        os.write(reinterpret_cast<const char*>(&header_size), sizeof(header_size));
        os.write(reinterpret_cast<const char*>(&entry_count), sizeof(entry_count));
    }

    /**
     * @brief Read header from stream
     */
    void read(std::istream& is) {
        is.read(magic, MAGIC_STRING_LENGTH);
        is.read(reinterpret_cast<char*>(&format_version), sizeof(format_version));
        is.read(reinterpret_cast<char*>(&header_size), sizeof(header_size));
        is.read(reinterpret_cast<char*>(&entry_count), sizeof(entry_count));
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
    enum LogLevel { TRACE=0, DEBUG, INFO, WARN, ERROR };
    static inline LogLevel current_log_level = ERROR;
    
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
            
            std::stringstream ss;
            ss << "[" << PROJECT_NAME << "][" << LOG_LEVEL_STRINGS[level] << "]"
               << "[" << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << "]"
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
      do { if (logger::TRACE >= logger::current_log_level) logger::log_internal(logger::TRACE, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_DEBUG(...) \
      do { if (logger::DEBUG >= logger::current_log_level) logger::log_internal(logger::DEBUG, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_INFO(...) \
      do { if (logger::INFO >= logger::current_log_level) logger::log_internal(logger::INFO, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_WARN(...) \
      do { if (logger::WARN >= logger::current_log_level) logger::log_internal(logger::WARN, __LINE__, __func__, __VA_ARGS__); } while(0)
  #define LOG_ERROR(...) \
      do { if (logger::ERROR >= logger::current_log_level) logger::log_internal(logger::ERROR, __LINE__, __func__, __VA_ARGS__); } while(0)



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
    size_t offset;              // Offset within this key's data section
    size_t compressed_size;     // Compressed size (may equal uncompressed if not compressed)
    size_t uncompressed_size;   // Original block size

    void write(std::ostream& os) const {
        os.write(reinterpret_cast<const char*>(&offset), sizeof(offset));
        os.write(reinterpret_cast<const char*>(&compressed_size), sizeof(compressed_size));
        os.write(reinterpret_cast<const char*>(&uncompressed_size), sizeof(uncompressed_size));
    }

    void read(std::istream& is) {
        is.read(reinterpret_cast<char*>(&offset), sizeof(offset));
        is.read(reinterpret_cast<char*>(&compressed_size), sizeof(compressed_size));
        is.read(reinterpret_cast<char*>(&uncompressed_size), sizeof(uncompressed_size));
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
    size_t length() const { return (stop - start + step - 1) / step; }
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
 */
struct IndexEntry {
    size_t position;                    // Position in file where data starts
    size_t total_bytes;                 // Total bytes (all blocks + metadata)
    DataType datatype;                  // Base element type
    std::vector<size_t> shape;          // Array dimensions (empty = scalar)
    CompressionAlgorithm compression;   // Compression algorithm
    size_t block_size;                  // Uncompressed block size (0 = no blocking)
    std::vector<BlockInfo> blocks;      // Per-block metadata
    bool dirty;                         // In-memory changes not yet flushed
    bool is_metadata_block = false;     // True if this is the special metadata block container
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

    void write(std::ostream& os) const {
        os.write(reinterpret_cast<const char*>(&position), sizeof(position));
        os.write(reinterpret_cast<const char*>(&total_bytes), sizeof(total_bytes));
        os.write(reinterpret_cast<const char*>(&datatype), sizeof(datatype));

        // Write shape (ndim = 0 means scalar)
        size_t ndim = shape.size();
        os.write(reinterpret_cast<const char*>(&ndim), sizeof(ndim));
        if (ndim > 0) {
            os.write(reinterpret_cast<const char*>(shape.data()), sizeof(size_t) * ndim);
        }

        os.write(reinterpret_cast<const char*>(&compression), sizeof(compression));
        os.write(reinterpret_cast<const char*>(&block_size), sizeof(block_size));

        // Write number of blocks
        size_t num_blocks = blocks.size();
        os.write(reinterpret_cast<const char*>(&num_blocks), sizeof(num_blocks));

        // Write each block info
        for (const auto& block : blocks) {
            block.write(os);
        }

        // Write stored_in_metadata flag
        uint8_t stored_flag = stored_in_metadata ? 1 : 0;
        os.write(reinterpret_cast<const char*>(&stored_flag), sizeof(stored_flag));
    }

    void read(std::istream& is) {
        is.read(reinterpret_cast<char*>(&position), sizeof(position));
        is.read(reinterpret_cast<char*>(&total_bytes), sizeof(total_bytes));
        is.read(reinterpret_cast<char*>(&datatype), sizeof(datatype));

        // Read shape (ndim = 0 means scalar)
        size_t ndim;
        is.read(reinterpret_cast<char*>(&ndim), sizeof(ndim));
        if (ndim > 0) {
            shape.resize(ndim);
            is.read(reinterpret_cast<char*>(shape.data()), sizeof(size_t) * ndim);
        } else {
            shape.clear(); // Explicitly make empty for scalar
        }

        is.read(reinterpret_cast<char*>(&compression), sizeof(compression));
        is.read(reinterpret_cast<char*>(&block_size), sizeof(block_size));

        // Read number of blocks
        size_t num_blocks;
        is.read(reinterpret_cast<char*>(&num_blocks), sizeof(num_blocks));

        // Read each block info
        blocks.resize(num_blocks);
        for (auto& block : blocks) {
            block.read(is);
        }

        // Read stored_in_metadata flag
        uint8_t stored_flag = 0;
        is.read(reinterpret_cast<char*>(&stored_flag), sizeof(stored_flag));
        stored_in_metadata = (stored_flag != 0);
    }

    size_t serialized_size() const {
        return sizeof(position) + sizeof(total_bytes) + sizeof(datatype) +
               sizeof(size_t) + (shape.size() * sizeof(size_t)) +
               sizeof(compression) + sizeof(block_size) + sizeof(size_t) +
               (blocks.size() * (sizeof(size_t) * 3)) +
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
#include <curl/curl.h>
#include <string>
#include <vector>
#include <streambuf>
#include <iostream>
#include <memory>

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
            stream->m_content_length = std::stoul(length);
        }
        return bytes;
    }
    
    bool fetchRange(size_t start, size_t end) {
        // m_buffer.resize(end - start + 1);
        
        // Set range header
        std::string range = "Range: bytes=" + std::to_string(start) + "-" + std::to_string(end);
        LOG_TRACE("Fetching range: ", range);
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, range.c_str());
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, headers);
        
        // Perform request
        CURLcode res = curl_easy_perform(m_curl);
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
        CURLcode res = curl_easy_perform(m_curl);
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
    enum Type { LOCAL, HTTP, S3 };
    Type type;
    std::string path;      // For LOCAL/HTTP
    std::string bucket;    // For S3
    std::string key;       // For S3
    std::string region;    // For S3
};

//==============================================================================
// S3 Support - AWS Authentication and Streaming
//==============================================================================

#ifdef ENABLE_S3
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <optional>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <ctime>
#include <tuple>
#ifndef _WIN32
#include <dirent.h>
#endif

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

    // URL encode a string (for S3 object keys)
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
        // Create canonical request
        std::string canonical_uri = "/" + urlEncode(key);
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
            std::time_t expires_time = mktime(&tm);

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
     * @param start_url The SSO start URL from AWS config
     * @return SSOToken if found and valid, empty optional otherwise
     */
    static std::optional<SSOToken> readToken(const std::string& start_url) {
        std::string cache_dir = getSSOCacheDir();
        if (cache_dir.empty()) {
            return std::nullopt;
        }

        // List all .json files in cache directory
        #ifdef _WIN32
        // not impemented 
        return std::nullopt;
        #else
        // Unix-like systems
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

        CURLcode res = curl_easy_perform(curl);
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

        // Read cached SSO token
        auto token = AWSTokenCache::readToken(sso_start_url);
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
 * @brief Parse a file path and determine its type (local, HTTP, or S3)
 */
inline FilePathInfo parseFilePath(const std::string& filename) {
    FilePathInfo info;

    if (filename.substr(0, 7) == "/vsis3/") {
        // S3 path: /vsis3/bucket/path/to/key
        info.type = FilePathInfo::S3;
        std::string remainder = filename.substr(7);
        size_t slash = remainder.find('/');
        if (slash == std::string::npos) {
            throw std::runtime_error("Invalid S3 path format. Expected: /vsis3/bucket/key");
        }
        info.bucket = remainder.substr(0, slash);
        info.key = remainder.substr(slash + 1);

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
    } else if (filename.substr(0, 9) == "/vsicurl/") {
        // HTTP path
        info.type = FilePathInfo::HTTP;
        info.path = filename.substr(9);
        return info;
    } else {
        // Local file path
        info.type = FilePathInfo::LOCAL;
        info.path = filename;
        return info;
    }
}

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
                data->content_length = std::stoul(length);
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
        return "https://" + m_bucket + ".s3." + m_region + ".amazonaws.com/" + m_key;
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::gmtime(&now_time);

        char buffer[17];
        std::strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", &tm);
        return std::string(buffer);
    }

    bool fetchRange(size_t start, size_t end) {
        m_buffer.clear();

        std::string url = getS3Url();
        std::string timestamp = getCurrentTimestamp();
        std::string host = m_bucket + ".s3." + m_region + ".amazonaws.com";

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

        CURLcode res = curl_easy_perform(m_curl);
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
        std::string host = m_bucket + ".s3." + m_region + ".amazonaws.com";

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

        CURLcode res = curl_easy_perform(m_curl);
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
            host = m_bucket + ".s3." + m_region + ".amazonaws.com";

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

            res = curl_easy_perform(m_curl);
            curl_slist_free_all(headers);
            curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);

            if (res != CURLE_OK) {
                LOG_ERROR("S3 HEAD request (retry) failed: ", curl_easy_strerror(res));
                throw std::runtime_error("S3 HEAD request failed: " + std::string(curl_easy_strerror(res)));
            }

            curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &response_code);
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
        return "https://" + m_bucket + ".s3." + m_region + ".amazonaws.com/" + m_key;
    }

    std::string getCurrentTimestamp() const {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* now_tm = std::gmtime(&now_c);

        char buffer[32];
        std::strftime(buffer, sizeof(buffer), "%Y%m%dT%H%M%SZ", now_tm);
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
            headers["host"] = m_bucket + ".s3." + m_region + ".amazonaws.com";
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
            CURLcode res = curl_easy_perform(m_curl);

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
     * @brief Calculate total bytes needed to store the array
     * @return Total bytes (uncompressed)
     */
    size_t totalBytes() const {
        // Compute uncompressed size
        size_t uncompressed_size = 0;
        if constexpr (std::is_same<T, std::string>::value) {
            size_t string_bytes = 0;
            for (const auto& str : m_data) {
                string_bytes += str.size() + sizeof(size_t); // String length + string data
            }
            uncompressed_size = sizeof(size_t) +                      // Number of dimensions
                                sizeof(size_t) * m_shape.size() +     // Shape dimensions
                                sizeof(size_t) * m_strides.size() +   // Strides
                                string_bytes;                         // Actual string data
        } else {
            uncompressed_size = sizeof(size_t) +                      // Number of dimensions
                                sizeof(size_t) * m_shape.size() +     // Shape dimensions
                                sizeof(size_t) * m_strides.size() +   // Strides
                                sizeof(T) * m_data.size();            // Actual data
        }

        LOG_TRACE("Uncompressed size: ", uncompressed_size);
        return uncompressed_size;
    }

    /**
     * @brief Write array to output stream
     * @param os Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const NDArray<T>& arr) {
        LOG_TRACE("Writing NDArray<", typeid(T).name(), "> to output stream at position ", os.tellp());
        size_t total_bytes = 0;
        size_t size_to_write = 0;

        // Write number of dimensions
        size_t num_dims = arr.m_shape.size();
        size_to_write = sizeof(num_dims);
        os.write(reinterpret_cast<const char*>(&num_dims), size_to_write);
        LOG_TRACE("Wrote number of dimensions: ", num_dims, " with total number of elements ", sizeof(num_dims));
        total_bytes += size_to_write;

        // Write shape
        size_to_write = sizeof(size_t) * num_dims;
        os.write(reinterpret_cast<const char*>(arr.m_shape.data()), size_to_write);
        LOG_TRACE("Wrote shape of size ", sizeof(size_t) * num_dims);
        total_bytes += size_to_write;

        // Write strides
        size_to_write = sizeof(size_t) * num_dims;
        os.write(reinterpret_cast<const char*>(arr.m_strides.data()), size_to_write);
        LOG_TRACE("Wrote strides of size ", sizeof(size_t) * num_dims);
        total_bytes += size_to_write;

        if constexpr (std::is_same<T, std::string>::value) {
            // Write data (strings need special handling)
            for (const auto& str : arr.m_data) {
                size_t str_len = str.size();
                os.write(reinterpret_cast<const char*>(&str_len), sizeof(str_len));
                total_bytes += sizeof(str_len);
                os.write(str.data(), str_len);
            }
        } else {
            // Write data in chunks
            constexpr size_t CHUNK_SIZE = 1024; // 1KB chunks
            size_to_write = sizeof(T) * arr.m_data.size();
            const char* data_ptr = reinterpret_cast<const char*>(arr.m_data.data());
            total_bytes += size_to_write;

            size_t bytes_written = 0;
            while (bytes_written < size_to_write) {
                size_t bytes_to_write = std::min(CHUNK_SIZE, size_to_write - bytes_written);
                os.write(data_ptr + bytes_written, bytes_to_write);
                bytes_written += bytes_to_write;

                LOG_TRACE("Wrote data chunk of size ", bytes_to_write,
                          " (", bytes_written, "/", size_to_write, " bytes)");

            }
            LOG_TRACE("Completed writing data of size ", size_to_write);
            LOG_TRACE("Total bytes written: ", total_bytes);
        }
        return os;
    }

    /**
     * @brief Read array from input stream
     * @param is Input stream
     */
    friend std::istream& operator>>(std::istream& is, NDArray<T>& arr) {
        LOG_TRACE("Reading NDArray<", typeid(T).name(), "> from input stream from byte position ", is.tellg());

        // Read number of dimensions
        size_t num_dims;
        is.read(reinterpret_cast<char*>(&num_dims), sizeof(num_dims));
        LOG_TRACE("Read number of dimensions: ", num_dims);

        // Read shape
        arr.m_shape.resize(num_dims);
        is.read(reinterpret_cast<char*>(arr.m_shape.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Read shape array of ", sizeof(size_t) * num_dims, " bytes");

        // Read strides
        arr.m_strides.resize(num_dims);
        is.read(reinterpret_cast<char*>(arr.m_strides.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Read strides array of ", sizeof(size_t) * num_dims, " bytes");

        // Calculate total size and read data
        size_t total_size = arr.computeTotalSize();
        if constexpr (std::is_same<T, std::string>::value) {
            arr.m_data.resize(total_size);
            for (size_t i = 0; i < total_size; ++i) {
                size_t str_len;
                is.read(reinterpret_cast<char*>(&str_len), sizeof(str_len));

                std::string str(str_len, '\0');
                is.read(&str[0], str_len);
                arr.m_data[i] = std::move(str);
                LOG_TRACE("Read string ", i, " with length ", str_len);
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
    // Main data compression settings
    CompressionAlgorithm compression = CompressionAlgorithm::LZ4;
    size_t block_size = 1024 * 1024;  // 1MB default

    // Metadata block settings
    bool metadata_block_enabled = true;
    size_t metadata_max_block_size = 64 * 1024;    // 64KB max total metadata block
    CompressionAlgorithm metadata_compression = CompressionAlgorithm::LZ4;
    std::set<std::string> metadata_force_separate_keys;  // Keys to never store in metadata

    // Buffer management (memory optimization)
    size_t buffer_shrink_threshold = 1024 * 1024;  // 1MB - shrink buffers above this size

    // Arena allocation
    size_t arena_chunk_size = 1 * 1024 * 1024;  // 1MB chunks
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
    std::vector<bool> loaded_flags;             // Track if loaded in memory (for efficient saveTo)
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

// Forward declaration
class StarDataset;

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
    bool contains(const std::string& key);
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
class StarDataset {
public:
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
    std::vector<ValueVariant> m_data_storage;                   // Actual data when loaded
    std::unordered_map<std::string_view, size_t> m_key_to_index;  // Fast key -> index lookup (views into m_hot.keys)
    bool m_metadata_loaded = false;                             // Metadata block loaded flag

    // Pre-allocated buffers (reduces allocations during flush)
    std::vector<char> m_serialize_buffer;                       // Reusable buffer for serialization
    std::vector<char> m_compress_buffer;                        // Reusable buffer for compression

    // Metadata API accessor
    MetadataAccessor meta;                                      // Explicit metadata operations

#ifdef ENABLE_S3
    // S3 support - cached for performance (avoid repeated parsing/resolution)
    FileMode m_file_mode;                                       // Read-only or read-write mode
    FilePathInfo m_path_info;                                   // Cached parsed path info
    std::unique_ptr<S3Credentials> m_s3_credentials;           // Cached S3 credentials
#endif

    // Configuration and state
    StarConfig m_config;                                        // Current configuration for this dataset
    bool m_flushed = false;                                     // Track if already flushed (prevents redundant flushes)
    std::unique_ptr<ThreadPool> m_thread_pool;                  // Thread pool for parallel operations

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

        // Open file and seek to position
        std::ifstream in(m_filename, std::ios::binary);
        if (!in.good()) {
            throw std::runtime_error("Failed to open file for reading entry");
        }

        in.seekg(position);

        // Read compressed data
        std::vector<char> compressed_data(compressed_size);
        in.read(compressed_data.data(), compressed_size);
        in.close();

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

        size_t size = FileHeader::size();  // Fixed header size

        // Count ALL entries (both metadata and arrays)
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            size += sizeof(size_t);               // key size
            size += m_hot.keys[i].length();       // Key data

            // Use IndexEntry::serialized_size() to get exact size
            // Create temporary IndexEntry to calculate size
            IndexEntry temp_entry;
            temp_entry.position = m_cold.file_positions[i];
            temp_entry.total_bytes = m_cold.compressed_sizes[i];
            temp_entry.datatype = m_hot.dtypes[i];
            temp_entry.shape = m_cold.shapes[i];
            temp_entry.compression = m_cold.compressions[i];
            temp_entry.block_size = m_config.block_size;
            temp_entry.blocks = m_cold.block_infos[i];
            temp_entry.stored_in_metadata = (m_cold.stored_in_metadata_flags[i] != 0);

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

        std::unique_ptr<std::istream> file;

#ifdef ENABLE_S3
        // Use cached path info (parsed once in constructor)
        switch (m_path_info.type) {
            case FilePathInfo::S3:
                LOG_TRACE("Opening S3 stream for bucket: ", m_path_info.bucket, ", key: ", m_path_info.key);
                if (!m_s3_credentials) {
                    throw std::runtime_error("S3 credentials not available");
                }
                file = std::make_unique<S3Stream>(m_path_info.bucket, m_path_info.key,
                                                 m_path_info.region, *m_s3_credentials);
                break;
            case FilePathInfo::HTTP:
                #ifdef ENABLE_CURL
                LOG_TRACE("Opening HTTP stream for URL: ", m_path_info.path);
                file = std::make_unique<HttpStream>(m_path_info.path);
                #else
                throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
                #endif
                break;
            case FilePathInfo::LOCAL:
                file = std::make_unique<std::ifstream>(m_path_info.path, std::ios::binary);
                break;
        }
#else
        // Check if this is a HTTP URL
        if (m_filename.substr(0, 9) == "/vsicurl/") {
            #ifdef ENABLE_CURL
            std::string url = m_filename.substr(9);
            LOG_TRACE("Opening HTTP stream for URL: ", url);
            file = std::make_unique<HttpStream>(url);
            #else
            throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
            #endif
        } else {
            file = std::make_unique<std::ifstream>(m_filename, std::ios::binary);
        }
#endif

        if (!file->good()) {
            return;
        }

        // First read the initial 512 bytes into a heap-allocated buffer
        constexpr size_t INITIAL_READ_SIZE = 2056;
        char initial_buffer[INITIAL_READ_SIZE];
        file->read(initial_buffer, INITIAL_READ_SIZE);
        size_t bytes_read = file->gcount();
        
        LOG_TRACE("Read initial ", bytes_read, " bytes");

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

        // Read format version (added in v2)
        uint8_t format_version = 1; // Default to v1 for old files
        initial_stream.read(reinterpret_cast<char*>(&format_version), sizeof(format_version));
        LOG_TRACE("Found format version: ", (int)format_version);
        m_file_header.format_version = format_version;

        initial_stream.read(reinterpret_cast<char*>(&m_header_size), sizeof(m_header_size));
        if (m_header_size == 0) {
            LOG_ERROR("Header size is 0, file is empty");
            throw std::runtime_error("Header size is 0, file is empty");
        }
        LOG_TRACE("Found header size: ", m_header_size);
        m_file_header.header_size = m_header_size;

        std::stringstream header_stream;

        // If header is larger than our initial read, read the full header
        if (m_header_size > INITIAL_READ_SIZE) {
            LOG_TRACE("Header size (", m_header_size, ") exceeds initial read (", INITIAL_READ_SIZE, "), reading full header");
            file->seekg(0);  // Go back to beginning of file
            auto header_buffer = std::make_unique<char[]>(m_header_size);
            file->read(header_buffer.get(), m_header_size);
            header_stream.write(header_buffer.get(), m_header_size);
        } else {
            // Use the data we already read
            LOG_TRACE("Using initial read for header, size: ", m_header_size);
            header_stream.write(initial_buffer, m_header_size);
        }

        // Reset stream position to after header size
        header_stream.seekg(MAGIC_STRING_LENGTH+sizeof(format_version)+sizeof(m_header_size), std::ios::beg);
        size_t count;
        header_stream.read(reinterpret_cast<char*>(&count), sizeof(count));

        LOG_TRACE("Found ", count, " entries in index");
        m_file_header.entry_count = count;

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
            std::string key = deserializeKey(header_stream);

            // Read IndexEntry
            IndexEntry entry;
            entry.read(header_stream);

            // Populate SoA arrays from IndexEntry
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

            m_key_to_index[std::string_view(m_hot.keys[idx])] = idx;

            LOG_TRACE("Loaded key ", key, " at index ", idx, " with position ", entry.position, " and bytes ", entry.total_bytes);
        }

        m_header_dirty = false;
        m_file_header.entry_count = count;
        LOG_TRACE("Index loaded successfully with ", count, " entries");
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
#endif

    /**
     * @brief Serializes metadata block to stream
     * @param os Output stream
     */
    void serialize_metadata_block(std::ostream& os) const {
        // Magic header
        const char magic[8] = {'S','C','S','M','E','T','A','B'};
        os.write(magic, 8);

        // Format version
        uint8_t version = 1;
        os.write(reinterpret_cast<const char*>(&version), 1);

        // Count entries stored in metadata block (using SoA)
        uint32_t count = 0;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1 && m_hot.data_indices[i] != SIZE_MAX) {
                count++;
            }
        }
        os.write(reinterpret_cast<const char*>(&count), 4);

        // Serialize all metadata block entries from SoA
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            // Only serialize items in metadata block with loaded data
            if (m_cold.stored_in_metadata_flags[i] != 1 || m_hot.data_indices[i] == SIZE_MAX) {
                continue;
            }

            const std::string& key = m_hot.keys[i];
            const auto& variant = m_data_storage[m_hot.data_indices[i]];

            // Key
            uint16_t key_len = static_cast<uint16_t>(key.size());
            os.write(reinterpret_cast<const char*>(&key_len), 2);
            os.write(key.data(), key_len);

            // Type and value (visit variant)
            std::visit([&](auto&& arr) {
                using T = typename std::decay_t<decltype(arr)>::value_type;
                DataType dtype = TypeToDataType<T>::value;
                uint8_t dtype_byte = static_cast<uint8_t>(dtype);
                os.write(reinterpret_cast<const char*>(&dtype_byte), 1);

                // Shape
                const auto& arr_shape = arr.shape();
                uint8_t ndim = static_cast<uint8_t>(arr_shape.size());
                os.write(reinterpret_cast<const char*>(&ndim), 1);
                if (ndim > 0) {
                    for (size_t dim : arr_shape) {
                        os.write(reinterpret_cast<const char*>(&dim), sizeof(size_t));
                    }
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
     * @brief Deserializes metadata block from stream
     * @param is Input stream
     */
    void deserialize_metadata_block(std::istream& is) {
        // Stub: In SoA design, metadata is loaded by loadIndex()
        // This method is kept for backwards compatibility with tests
        LOG_WARN("deserialize_metadata_block() is deprecated - metadata loaded via loadIndex()");
        return;
    }

    /**
     * @brief Loads metadata block from file if not already loaded (supports HTTP/remote URLs)
     */
    void load_metadata_block() {
        if (m_metadata_loaded) {
            return;
        }

        // Find a metadata entry to get the metadata block position and block info
        size_t metadata_block_position = 0;
        size_t metadata_block_size = 0;
        std::vector<BlockInfo> metadata_blocks;
        CompressionAlgorithm metadata_compression = CompressionAlgorithm::LZ4;
        bool found_metadata = false;

        for (size_t i = 0; i < m_cold.stored_in_metadata_flags.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1) {
                metadata_block_position = m_cold.file_positions[i];
                metadata_block_size = m_cold.compressed_sizes[i];
                metadata_blocks = m_cold.block_infos[i];
                metadata_compression = m_cold.compressions[i];
                found_metadata = true;
                break;
            }
        }

        if (!found_metadata) {
            LOG_TRACE("No metadata entries found");
            m_metadata_loaded = true;
            return;
        }

        LOG_TRACE("Loading metadata block from position ", metadata_block_position,
                 " with size ", metadata_block_size);

        // Open file (supporting S3/HTTP) and read metadata block
        std::unique_ptr<std::istream> file;

#ifdef ENABLE_S3
        switch (m_path_info.type) {
            case FilePathInfo::S3:
                if (!m_s3_credentials) {
                    throw std::runtime_error("S3 credentials not available");
                }
                file = std::make_unique<S3Stream>(m_path_info.bucket, m_path_info.key,
                                                 m_path_info.region, *m_s3_credentials);
                break;
            case FilePathInfo::HTTP:
                #ifdef ENABLE_CURL
                file = std::make_unique<HttpStream>(m_path_info.path);
                #else
                throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
                #endif
                break;
            case FilePathInfo::LOCAL:
                file = std::make_unique<std::ifstream>(m_path_info.path, std::ios::binary);
                break;
        }
#else
        if (m_filename.substr(0, 9) == "/vsicurl/") {
            #ifdef ENABLE_CURL
            std::string url = m_filename.substr(9);
            file = std::make_unique<HttpStream>(url);
            #else
            throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
            #endif
        } else {
            file = std::make_unique<std::ifstream>(m_filename, std::ios::binary);
        }
#endif

        if (!file->good()) {
            throw std::runtime_error("Failed to open file for reading metadata block");
        }

        file->seekg(metadata_block_position);

        // Read compressed metadata block
        std::vector<char> compressed_data(metadata_block_size);
        file->read(compressed_data.data(), metadata_block_size);
        file.reset();  // Close file

        // Decompress metadata block using block info and actual compression from index
        // (with parallel decompression if thread pool available)
        std::vector<char> decompressed_data = decompressBlocks(
            compressed_data,
            metadata_blocks,
            metadata_compression,
            {},  // decompress all blocks
            m_thread_pool.get()
        );

        // Parse metadata block
        std::stringstream metadata_stream;
        metadata_stream.write(decompressed_data.data(), decompressed_data.size());
        metadata_stream.seekg(0);

        // Read magic header
        char magic[8];
        metadata_stream.read(magic, 8);
        if (std::memcmp(magic, "SCSMETAB", 8) != 0) {
            throw std::runtime_error("Invalid metadata block magic header");
        }

        // Read version
        uint8_t version;
        metadata_stream.read(reinterpret_cast<char*>(&version), 1);
        if (version != 1) {
            throw std::runtime_error("Unsupported metadata block version: " +
                                    std::to_string(version));
        }

        // Read count
        uint32_t count;
        metadata_stream.read(reinterpret_cast<char*>(&count), 4);
        LOG_TRACE("Metadata block contains ", count, " entries");

        // Read each entry
        for (uint32_t i = 0; i < count; i++) {
            // Read key
            uint16_t key_len;
            metadata_stream.read(reinterpret_cast<char*>(&key_len), 2);
            std::string key(key_len, '\0');
            metadata_stream.read(&key[0], key_len);

            // Read type
            uint8_t dtype_byte;
            metadata_stream.read(reinterpret_cast<char*>(&dtype_byte), 1);
            DataType dtype = static_cast<DataType>(dtype_byte);

            // Read shape
            uint8_t ndim;
            metadata_stream.read(reinterpret_cast<char*>(&ndim), 1);
            std::vector<size_t> shape(ndim);
            for (uint8_t d = 0; d < ndim; d++) {
                metadata_stream.read(reinterpret_cast<char*>(&shape[d]), sizeof(size_t));
            }

            // Calculate data length
            size_t data_len;
            if (dtype == DataType::STRING) {
                // For strings, read the total length field
                metadata_stream.read(reinterpret_cast<char*>(&data_len), 4);
            } else {
                // For numeric types, calculate from shape
                size_t num_elements = 1;
                for (size_t dim : shape) {
                    num_elements *= dim;
                }
                if (num_elements == 0) num_elements = 1;  // Scalar
                data_len = num_elements * datatype_size(dtype);
            }

            // Deserialize value
            ValueVariant value = deserialize_typed_value(metadata_stream, dtype, shape, data_len);

            // Find this key in the index and store the data
            auto it = m_key_to_index.find(key);
            if (it != m_key_to_index.end()) {
                size_t idx = it->second;
                m_data_storage.push_back(std::move(value));
                m_hot.data_indices[idx] = m_data_storage.size() - 1;
                m_hot.loaded_flags[idx] = true;
                LOG_TRACE("Loaded metadata entry: ", key, " at index ", idx);
            } else {
                LOG_WARN("Metadata entry ", key, " not found in index");
            }
        }

        m_metadata_loaded = true;
        LOG_TRACE("Metadata block loaded successfully");
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

    void printHeader() const {
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

        // Early return if already flushed
        if (m_flushed) {
            LOG_TRACE("flush() - already flushed, skipping");
            return;
        }
        
        #ifdef ENABLE_S3
        // Check read-only mode
        if (m_file_mode == FileMode::READ_ONLY) {
            throw std::runtime_error(
                "Cannot flush in read-only mode. Use saveTo() to write to a different file.");
        }
        #endif

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
                std::visit([&serialize_buffer](auto&& arr) {
                    using T = typename std::decay_t<decltype(arr)>::value_type;
                    size_t byte_size = arr.size() * sizeof(T);
                    serialize_buffer.resize(byte_size);
                    std::memcpy(serialize_buffer.data(), arr.data().data(), byte_size);
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
                    size_t byte_size = arr.size() * sizeof(T);
                    m_serialize_buffer.resize(byte_size);
                    std::memcpy(m_serialize_buffer.data(), arr.data().data(), byte_size);
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
        }

#ifdef ENABLE_S3
        if (m_path_info.type == FilePathInfo::S3) {
            // S3 path: Create output buffer and write in parallel
            std::vector<char> output_buffer(total_file_size);

            // Parallel compress and write arrays to buffer
            if (m_thread_pool) {
                m_thread_pool->parallel_for(0, m_hot.keys.size(), [&](size_t i) {
                    if (m_cold.stored_in_metadata_flags[i]) return;

                    std::vector<char> array_data;
                    std::vector<BlockInfo> blocks;

                    if (m_hot.dirty_flags[i]) {
                        // Use cached data if available, otherwise re-serialize
                        std::vector<char> serialize_buffer;
                        if (!array_metadata[i].serialized_data.empty()) {
                            // Small array: use cached data
                            serialize_buffer = array_metadata[i].serialized_data;
                        } else {
                            // Large array: re-serialize (wasn't cached to save memory)
                            const ValueVariant& var = m_data_storage[m_hot.data_indices[i]];
                            std::visit([&serialize_buffer](auto&& arr) {
                                using T = typename std::decay_t<decltype(arr)>::value_type;
                                size_t byte_size = arr.size() * sizeof(T);
                                serialize_buffer.resize(byte_size);
                                std::memcpy(serialize_buffer.data(), arr.data().data(), byte_size);
                            }, var);
                        }

                        std::vector<char> temp_buffer;
                        blocks = compressBlocksBuffered(
                            serialize_buffer.data(),
                            serialize_buffer.size(),
                            m_config.compression,
                            m_config.block_size,
                            array_data,
                            temp_buffer,
                            m_thread_pool.get()  // Enable for large single arrays
                        );

                    } else {
                        // Copy from old file (non-dirty)
                        blocks = m_cold.block_infos[i];
                        array_data.resize(m_cold.compressed_sizes[i]);

                        std::ifstream in(m_filename, std::ios::binary);
                        if (in) {
                            in.seekg(m_cold.file_positions[i]);  // Read from OLD position
                            in.read(array_data.data(), array_data.size());
                        }

                        // Adjust block offsets for new contiguous layout
                        // The copied data is now a sequential buffer starting at offset 0
                        size_t current_offset = 0;
                        for (auto& block : blocks) {
                            block.offset = current_offset;
                            current_offset += block.compressed_size;
                        }
                    }

                    // Store actual block infos
                    actual_blocks[i] = blocks;

                    // Write to buffer at designated position (no lock needed - different offsets)
                    std::memcpy(output_buffer.data() + file_positions[i],
                               array_data.data(),
                               array_data.size());
                });
            } else {
                // Serial fallback
                for (size_t i = 0; i < m_hot.keys.size(); i++) {
                    if (m_cold.stored_in_metadata_flags[i]) continue;

                    std::vector<char> array_data;
                    std::vector<BlockInfo> blocks;

                    if (m_hot.dirty_flags[i]) {
                        // Use cached data if available, otherwise re-serialize
                        std::vector<char> serialize_buffer;
                        if (!array_metadata[i].serialized_data.empty()) {
                            // Small array: use cached data
                            serialize_buffer = array_metadata[i].serialized_data;
                        } else {
                            // Large array: re-serialize (wasn't cached to save memory)
                            const ValueVariant& var = m_data_storage[m_hot.data_indices[i]];
                            std::visit([&serialize_buffer](auto&& arr) {
                                using T = typename std::decay_t<decltype(arr)>::value_type;
                                size_t byte_size = arr.size() * sizeof(T);
                                serialize_buffer.resize(byte_size);
                                std::memcpy(serialize_buffer.data(), arr.data().data(), byte_size);
                            }, var);
                        }

                        m_compress_buffer.clear();
                        std::vector<char> temp_buffer;
                        blocks = compressBlocksBuffered(
                            serialize_buffer.data(),
                            serialize_buffer.size(),
                            m_config.compression,
                            m_config.block_size,
                            array_data,
                            temp_buffer,
                            m_thread_pool.get()  // Use thread pool for block-level parallelism
                        );

                    } else {
                        blocks = m_cold.block_infos[i];
                        array_data.resize(m_cold.compressed_sizes[i]);

                        std::ifstream in(m_filename, std::ios::binary);
                        if (in) {
                            in.seekg(m_cold.file_positions[i]);  // Read from OLD position
                            in.read(array_data.data(), array_data.size());
                        }

                        // Adjust block offsets for new contiguous layout
                        // The copied data is now a sequential buffer starting at offset 0
                        size_t current_offset = 0;
                        for (auto& block : blocks) {
                            block.offset = current_offset;
                            current_offset += block.compressed_size;
                        }
                    }

                    actual_blocks[i] = blocks;
                    std::memcpy(output_buffer.data() + file_positions[i],
                               array_data.data(),
                               array_data.size());
                }
            }

            // Write metadata to buffer
            if (!metadata_compressed.empty()) {
                std::memcpy(output_buffer.data() + metadata_position,
                           metadata_compressed.data(),
                           metadata_compressed.size());
            }

            // Update SoA with actual block infos and sizes
            for (size_t i = 0; i < m_hot.keys.size(); i++) {
                if (!m_cold.stored_in_metadata_flags[i] && !actual_blocks[i].empty()) {
                    m_cold.block_infos[i] = actual_blocks[i];
                    m_cold.compressed_sizes[i] = 0;
                    for (const auto& block : actual_blocks[i]) {
                        m_cold.compressed_sizes[i] += block.compressed_size;
                    }
                    m_cold.file_positions[i] = file_positions[i];
                }
            }

            // NOW write header with ACTUAL block infos
            std::ostringstream header_stream;
            m_file_header.header_size = header_size;
            m_file_header.entry_count = m_hot.keys.size();
            m_file_header.write(header_stream);

            // Write index with actual block infos
            for (size_t i = 0; i < m_hot.keys.size(); i++) {
                size_t key_len = m_hot.keys[i].length();
                header_stream.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                header_stream.write(m_hot.keys[i].data(), key_len);

                IndexEntry entry;
                entry.position = m_cold.file_positions[i];  // Use m_cold, not file_positions!
                entry.total_bytes = m_cold.compressed_sizes[i];
                entry.datatype = m_hot.dtypes[i];
                entry.shape = m_cold.shapes[i];
                entry.compression = m_cold.compressions[i];
                entry.block_size = m_config.block_size;
                entry.blocks = m_cold.block_infos[i];  // ACTUAL blocks
                entry.stored_in_metadata = (m_cold.stored_in_metadata_flags[i] != 0);

                entry.write(header_stream);
            }

            std::string header_str = header_stream.str();
            std::memcpy(output_buffer.data(), header_str.data(), header_str.size());

            // Upload to S3
            if (!m_s3_credentials) {
                throw std::runtime_error("S3 credentials not available for writing to: " + m_filename);
            }

            S3Writer writer(m_path_info.bucket, m_path_info.key, m_path_info.region, *m_s3_credentials);
            writer.putObject(output_buffer.data(), output_buffer.size());

            LOG_DEBUG("Successfully uploaded ", output_buffer.size(), " bytes to S3: ", m_filename);

        } else {
#endif
            // Local file path: Open file and write in parallel
            std::ofstream out(m_filename, std::ios::binary);
            if (!out) {
                throw std::runtime_error("Failed to open file for writing: " + m_filename);
            }

            // Skip header space (write it later with actual block_infos)
            out.seekp(header_size);

            // Parallel compress and write arrays
            if (m_thread_pool) {
                m_thread_pool->parallel_for(0, m_hot.keys.size(), [&](size_t i) {
                    if (m_cold.stored_in_metadata_flags[i]) return;

                    std::vector<char> array_data;
                    std::vector<BlockInfo> blocks;

                    if (m_hot.dirty_flags[i]) {
                        // Use cached data if available, otherwise re-serialize
                        std::vector<char> serialize_buffer;
                        if (!array_metadata[i].serialized_data.empty()) {
                            // Small array: use cached data
                            serialize_buffer = array_metadata[i].serialized_data;
                        } else {
                            // Large array: re-serialize (wasn't cached to save memory)
                            const ValueVariant& var = m_data_storage[m_hot.data_indices[i]];
                            std::visit([&serialize_buffer](auto&& arr) {
                                using T = typename std::decay_t<decltype(arr)>::value_type;
                                size_t byte_size = arr.size() * sizeof(T);
                                serialize_buffer.resize(byte_size);
                                std::memcpy(serialize_buffer.data(), arr.data().data(), byte_size);
                            }, var);
                        }

                        std::vector<char> temp_buffer;
                        blocks = compressBlocksBuffered(
                            serialize_buffer.data(),
                            serialize_buffer.size(),
                            m_config.compression,
                            m_config.block_size,
                            array_data,
                            temp_buffer,
                            m_thread_pool.get()  // Enable for large single arrays
                        );

                    } else {
                        // Copy from old file
                        blocks = m_cold.block_infos[i];
                        array_data.resize(m_cold.compressed_sizes[i]);

                        std::ifstream in(m_filename, std::ios::binary);
                        if (in) {
                            in.seekg(m_cold.file_positions[i]);  // Read from OLD position
                            in.read(array_data.data(), array_data.size());
                        }
                    }

                    // Store actual block infos
                    actual_blocks[i] = blocks;

                    // Write to file at designated position
                    {
                        std::lock_guard<std::mutex> lock(file_mutex);
                        out.seekp(file_positions[i]);
                        out.write(array_data.data(), array_data.size());
                    }
                });
            } else {
                // Serial fallback
                for (size_t i = 0; i < m_hot.keys.size(); i++) {
                    if (m_cold.stored_in_metadata_flags[i]) continue;

                    std::vector<char> array_data;
                    std::vector<BlockInfo> blocks;

                    if (m_hot.dirty_flags[i]) {
                        // Use cached data if available, otherwise re-serialize
                        std::vector<char> serialize_buffer;
                        if (!array_metadata[i].serialized_data.empty()) {
                            // Small array: use cached data
                            serialize_buffer = array_metadata[i].serialized_data;
                        } else {
                            // Large array: re-serialize (wasn't cached to save memory)
                            const ValueVariant& var = m_data_storage[m_hot.data_indices[i]];
                            std::visit([&serialize_buffer](auto&& arr) {
                                using T = typename std::decay_t<decltype(arr)>::value_type;
                                size_t byte_size = arr.size() * sizeof(T);
                                serialize_buffer.resize(byte_size);
                                std::memcpy(serialize_buffer.data(), arr.data().data(), byte_size);
                            }, var);
                        }

                        m_compress_buffer.clear();
                        std::vector<char> temp_buffer;
                        blocks = compressBlocksBuffered(
                            serialize_buffer.data(),
                            serialize_buffer.size(),
                            m_config.compression,
                            m_config.block_size,
                            array_data,
                            temp_buffer,
                            m_thread_pool.get()  // Use thread pool for block-level parallelism
                        );

                    } else {
                        blocks = m_cold.block_infos[i];
                        array_data.resize(m_cold.compressed_sizes[i]);

                        std::ifstream in(m_filename, std::ios::binary);
                        if (in) {
                            in.seekg(m_cold.file_positions[i]);  // Read from OLD position
                            in.read(array_data.data(), array_data.size());
                        }

                        // Adjust block offsets for new contiguous layout
                        // The copied data is now a sequential buffer starting at offset 0
                        size_t current_offset = 0;
                        for (auto& block : blocks) {
                            block.offset = current_offset;
                            current_offset += block.compressed_size;
                        }
                    }

                    actual_blocks[i] = blocks;
                    out.seekp(file_positions[i]);
                    out.write(array_data.data(), array_data.size());
                }
            }

            // Write metadata block
            if (!metadata_compressed.empty()) {
                out.seekp(metadata_position);
                out.write(metadata_compressed.data(), metadata_compressed.size());
            }

            // Update SoA with actual block infos and sizes
            for (size_t i = 0; i < m_hot.keys.size(); i++) {
                if (!m_cold.stored_in_metadata_flags[i] && !actual_blocks[i].empty()) {
                    m_cold.block_infos[i] = actual_blocks[i];
                    m_cold.compressed_sizes[i] = 0;
                    for (const auto& block : actual_blocks[i]) {
                        m_cold.compressed_sizes[i] += block.compressed_size;
                    }
                    m_cold.file_positions[i] = file_positions[i];
                }
            }

            // NOW write header with actual block infos
            out.seekp(0);
            m_file_header.header_size = header_size;
            m_file_header.entry_count = m_hot.keys.size();
            m_file_header.write(out);

            // Write index with actual block infos
            for (size_t i = 0; i < m_hot.keys.size(); i++) {
                size_t key_len = m_hot.keys[i].length();
                out.write(reinterpret_cast<const char*>(&key_len), sizeof(key_len));
                out.write(m_hot.keys[i].data(), key_len);

                IndexEntry entry;
                entry.position = m_cold.file_positions[i];  // Use m_cold, not file_positions!
                entry.total_bytes = m_cold.compressed_sizes[i];
                entry.datatype = m_hot.dtypes[i];
                entry.shape = m_cold.shapes[i];
                entry.compression = m_cold.compressions[i];
                entry.block_size = m_config.block_size;
                entry.blocks = m_cold.block_infos[i];  // ACTUAL blocks
                entry.stored_in_metadata = (m_cold.stored_in_metadata_flags[i] != 0);

                entry.write(out);
            }

            out.close();
#ifdef ENABLE_S3
        }
#endif

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
     * @param target_path Path to save to (can be local or /vsis3/...)
     */
    void saveTo(const std::string& target_path) {
#ifdef ENABLE_S3
        // Check if trying to save to source in read-only mode
        if (target_path == m_filename && m_file_mode == FileMode::READ_ONLY) {
            throw std::runtime_error(
                "Cannot save to source file in read-only mode. Use saveTo() with different path.");
        }

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
            m_file_mode = FileMode::READ_WRITE;  // Allow write for saveTo

            // Ensure metadata block is loaded before saving
            // (so all existing data gets written to target file)
            if (!m_metadata_loaded && has_metadata_block()) {
                load_metadata_block();
                // Mark all loaded metadata entries as dirty so they get written to target
                for (size_t i = 0; i < m_hot.keys.size(); i++) {
                    if (m_cold.stored_in_metadata_flags[i] == 1 && m_hot.loaded_flags[i]) {
                        m_hot.dirty_flags[i] = true;
                    }
                }
            }

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
        // Without S3 support, just use flush with temp filename swap
        // Ensure metadata block is loaded before saving
        if (!m_metadata_loaded && has_metadata_block()) {
            load_metadata_block();
        }

        std::string original_filename = m_filename;
        try {
            m_filename = target_path;
            flush();
            m_filename = original_filename;
        } catch (...) {
            m_filename = original_filename;
            throw;
        }
#endif
    }

    /**
     * @brief Check if file is in read-only mode
     * @return True if read-only
     */
    bool isReadOnly() const {
#ifdef ENABLE_S3
        return m_file_mode == FileMode::READ_ONLY;
#else
        return false;  // Read-only mode not available without S3 support
#endif
    }

    /**
     * @brief Get current filename
     * @return Filename
     */
    std::string getFilename() const {
        return m_filename;
    }

    /**
     * @brief Get file header with version information
     * @return Reference to FileHeader
     */
    const FileHeader& getFileHeader() const {
        return m_file_header;
    }


    /**
     * @brief Create a new Star dataset file
     *
     * Creates a new file with the specified configuration. If the file already exists,
     * it will be overwritten. The file will be created on the first flush() or when
     * the object is destroyed.
     *
     * @param filename Path to create (local or /vsis3/...)
     * @param config Configuration for compression, block sizes, metadata
     * @return New StarDataset instance
     */
    static std::unique_ptr<StarDataset> create(const std::string& filename, const StarConfig& config = StarConfig()) {
        // If file exists, delete it to allow overwrite
        std::ifstream check_file(filename, std::ios::binary);
        if (check_file.good()) {
            check_file.close();
            // Delete existing file
            std::remove(filename.c_str());
        }

        return std::unique_ptr<StarDataset>(new StarDataset(filename, FileMode::READ_WRITE, &config));
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
     * @param filename Path to open (local or /vsis3/...)
     * @param mode FileMode enum (READ_WRITE/READ_ONLY)
     * @return Opened StarDataset instance
     * @throws std::runtime_error if file doesn't exist in READ_ONLY mode or is corrupt
     */
    static std::unique_ptr<StarDataset> open(const std::string& filename, FileMode mode = FileMode::READ_WRITE) {
        // Check if file exists
        bool file_exists = false;

#ifdef ENABLE_S3
        // Parse path to check if it's S3
        FilePathInfo path_info = parseFilePath(filename);

        if (path_info.type == FilePathInfo::S3) {
            // For S3, check existence by trying to open a stream
            try {
                S3Credentials creds = S3Credentials::resolve();
                S3Stream s3_stream(path_info.bucket, path_info.key, path_info.region, creds);
                file_exists = s3_stream.good();
            } catch (...) {
                file_exists = false;
            }
        } else {
#endif
            // For local files, use ifstream
            std::ifstream check_file(filename, std::ios::binary);
            file_exists = check_file.good();
            check_file.close();
#ifdef ENABLE_S3
        }
#endif

        // If file doesn't exist and mode is READ_ONLY, throw error
        if (!file_exists && mode == FileMode::READ_ONLY) {
            throw std::runtime_error("File does not exist: " + filename + ". Cannot open non-existent file in read-only mode.");
        }

        // If file doesn't exist but mode is READ_WRITE, create it
        if (!file_exists && mode == FileMode::READ_WRITE) {
            // File will be created on first flush
            return std::unique_ptr<StarDataset>(new StarDataset(filename, mode, nullptr));
        }

        // File exists, open it
        auto store = std::unique_ptr<StarDataset>(new StarDataset(filename, mode, nullptr));

        // Verify that the file was loaded successfully by checking if we have any index info
        // A valid STAR file should have header_size set after loadIndex()
        if (store->m_header_size == 0) {
            throw std::runtime_error("Failed to load file: " + filename + ". File may be corrupt or not a valid STAR file.");
        }

        return store;
    }

    /**
     * @brief Open an existing Star dataset file (string mode overload)
     *
     * @param filename Path to open (local or /vsis3/...)
     * @param mode_str String mode ("r", "w", "rw", "a")
     * @return Opened StarDataset instance
     * @throws std::runtime_error if file doesn't exist or is invalid
     */
    static std::unique_ptr<StarDataset> open(const std::string& filename, const std::string& mode_str) {
        return open(filename, parseModeString(mode_str));
    }

    // Delete copy and move constructors (non-copyable due to std::shared_mutex)
    StarDataset(const StarDataset&) = delete;
    StarDataset& operator=(const StarDataset&) = delete;
    StarDataset(StarDataset&&) = delete;
    StarDataset& operator=(StarDataset&&) = delete;

private:
    /**
     * @brief Constructor with compression options (private - use factory methods)
     * @param fname Filename to use for storage
     * @param mode File open mode (READ_WRITE or READ_ONLY)
     * @param config Optional configuration (nullptr to load from file)
     */
    StarDataset(const std::string& fname, FileMode mode, const StarConfig* config)
        : m_filename(fname), m_header_dirty(true), meta(this),
          m_config(config ? *config : StarConfig())
#ifdef ENABLE_S3
        , m_file_mode(mode)
#endif
    {
#ifdef ENABLE_S3
        // Parse path once at construction (minimize allocations)
        m_path_info = parseFilePath(fname);

        // Resolve credentials once (cache hot data)
        if (m_path_info.type == FilePathInfo::S3) {
            m_s3_credentials = std::make_unique<S3Credentials>(S3Credentials::resolve());
        }
#endif

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
    }

public:
    /**
     * @brief Destructor - writes all pending changes to disk (RAII)
     */
    ~StarDataset() {
        try {
            flush();
        } catch (const std::exception& e) {
            // Log error but don't throw from destructor
            std::cerr << "Error flushing store in destructor: " << e.what() << std::endl;
        }
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
            m_key_to_index[std::string_view(m_hot.keys[idx])] = idx;
        }

        m_header_dirty = true;
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
     * @return Vector of key names
     */
    std::vector<std::string> get_metadata_keys() const {
        std::vector<std::string> keys;
        // Iterate SoA and collect keys stored in metadata block
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1) {
                keys.push_back(m_hot.keys[i]);
            }
        }
        return keys;
    }

    /**
     * @brief Gets count of entries in metadata block
     * @return Number of metadata entries
     */
    size_t get_metadata_count() const {
        size_t count = 0;
        for (size_t i = 0; i < m_hot.keys.size(); i++) {
            if (m_cold.stored_in_metadata_flags[i] == 1) {
                count++;
            }
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
            return false;  // Key doesn't exist
        }
        size_t idx = it->second;

        // Sliceable if stored separately with block structure
        return !m_cold.stored_in_metadata_flags[idx] && !m_cold.block_infos[idx].empty();
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

        // Pre-allocate exact size (no reallocation)
        result.reserve(block_map.total_compressed_bytes);

        // Open file stream (same pattern as loadIndex)
        std::unique_ptr<std::istream> file;

#ifdef ENABLE_S3
        // Use cached path info (parsed once in constructor)
        switch (m_path_info.type) {
            case FilePathInfo::S3:
                if (!m_s3_credentials) {
                    throw std::runtime_error("S3 credentials not available");
                }
                file = std::make_unique<S3Stream>(m_path_info.bucket, m_path_info.key,
                                                 m_path_info.region, *m_s3_credentials);
                break;
            case FilePathInfo::HTTP:
                #ifdef ENABLE_CURL
                file = std::make_unique<HttpStream>(m_path_info.path);
                #else
                throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
                #endif
                break;
            case FilePathInfo::LOCAL:
                file = std::make_unique<std::ifstream>(m_path_info.path, std::ios::binary);
                break;
        }
#else
        // Check if this is a HTTP URL
        if (m_filename.substr(0, 9) == "/vsicurl/") {
            #ifdef ENABLE_CURL
            std::string url = m_filename.substr(9);
            file = std::make_unique<HttpStream>(url);
            #else
            throw std::runtime_error("CURL support not enabled, cannot open HTTP URL");
            #endif
        } else {
            file = std::make_unique<std::ifstream>(m_filename, std::ios::binary);
        }
#endif

        if (!file->good()) {
            throw std::runtime_error("Failed to open file for reading blocks: " + m_filename);
        }

        if (block_map.blocks_contiguous) {
            // Single I/O operation, linear memory write
            result.resize(block_map.total_compressed_bytes);
            file->seekg(entry.position + block_map.contiguous_start_offset);
            file->read(result.data(), block_map.total_compressed_bytes);
        } else {
            // Batch small operations
            for (size_t i = 0; i < block_map.block_indices.size(); ++i) {
                size_t block_idx = block_map.block_indices[i];
                const BlockInfo& block = entry.blocks[block_idx];

                size_t old_size = result.size();
                result.resize(old_size + block.compressed_size);

                file->seekg(entry.position + block.offset);
                file->read(result.data() + old_size, block.compressed_size);
            }
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

    // Structure of Arrays: single lookup, cache-friendly updates
    auto it = m_store->m_key_to_index.find(key);

    using ElementType = typename V::value_type;
    DataType dtype = TypeToDataType<ElementType>::value;
    auto shape_vec = std::vector<size_t>(value.shape().begin(), value.shape().end());
    uint32_t uncompressed_size = value.size() * sizeof(ElementType);

    if (it != m_store->m_key_to_index.end()) {
        // Update existing entry - SoA pattern: update each array independently
        size_t idx = it->second;

        // Update hot data (frequently accessed)
        m_store->m_hot.dtypes[idx] = dtype;
        m_store->m_hot.locations[idx] = StorageLocation::PENDING;
        m_store->m_hot.dirty_flags[idx] = true;

        // Update cold data (infrequently accessed)
        m_store->m_cold.shapes[idx] = shape_vec;
        m_store->m_cold.file_positions[idx] = 0;  // Metadata block items
        m_store->m_cold.uncompressed_sizes[idx] = uncompressed_size;

        // Update or add data storage
        if (m_store->m_hot.data_indices[idx] == SIZE_MAX) {
            // Not yet allocated - add to storage
            m_store->m_hot.data_indices[idx] = m_store->m_data_storage.size();
            m_store->m_data_storage.push_back(value);
        } else {
            // Update existing data
            m_store->m_data_storage[m_store->m_hot.data_indices[idx]] = value;
        }
    } else {
        // Create new entry - SoA pattern: append to each array

        // Performance optimization: Pre-emptively expand all vectors together
        // to avoid 14 independent reallocations (each copying all existing data)
        if (m_store->m_hot.keys.size() >= m_store->m_hot.keys.capacity()) {
            size_t new_capacity = m_store->m_hot.keys.capacity() * 2;

            // Expand HotStorage vectors (6 vectors)
            m_store->m_hot.keys.reserve(new_capacity);
            m_store->m_hot.dtypes.reserve(new_capacity);
            m_store->m_hot.locations.reserve(new_capacity);
            m_store->m_hot.dirty_flags.reserve(new_capacity);
            m_store->m_hot.loaded_flags.reserve(new_capacity);
            m_store->m_hot.data_indices.reserve(new_capacity);

            // Expand ColdStorage vectors (7 vectors)
            m_store->m_cold.shapes.reserve(new_capacity);
            m_store->m_cold.file_positions.reserve(new_capacity);
            m_store->m_cold.compressed_sizes.reserve(new_capacity);
            m_store->m_cold.uncompressed_sizes.reserve(new_capacity);
            m_store->m_cold.compressions.reserve(new_capacity);
            m_store->m_cold.block_infos.reserve(new_capacity);
            m_store->m_cold.stored_in_metadata_flags.reserve(new_capacity);

            // CRITICAL: Expand data storage vector (holds actual array data)
            m_store->m_data_storage.reserve(new_capacity);
        }

        size_t idx = m_store->m_hot.keys.size();

        // Hot data
        m_store->m_hot.keys.push_back(key);
        m_store->m_hot.dtypes.push_back(dtype);
        m_store->m_hot.locations.push_back(StorageLocation::PENDING);
        m_store->m_hot.dirty_flags.push_back(true);
        m_store->m_hot.loaded_flags.push_back(true);  
        m_store->m_hot.data_indices.push_back(m_store->m_data_storage.size());

        // Cold data
        m_store->m_cold.shapes.push_back(shape_vec);
        m_store->m_cold.file_positions.push_back(0);  // Metadata block items
        m_store->m_cold.compressed_sizes.push_back(0);  // Will be set during flush
        m_store->m_cold.uncompressed_sizes.push_back(uncompressed_size);
        m_store->m_cold.compressions.push_back(m_store->m_config.metadata_compression);
        m_store->m_cold.block_infos.push_back(std::vector<BlockInfo>());

        // Data storage
        m_store->m_data_storage.push_back(value);

        // Index mapping
        m_store->m_key_to_index[std::string_view(m_store->m_hot.keys[idx])] = idx;

        // Mark as metadata block entry
        m_store->m_cold.stored_in_metadata_flags.push_back(1);
    }

    // Always mark as metadata block entry for existing entries
    if (it != m_store->m_key_to_index.end()) {
        m_store->m_cold.stored_in_metadata_flags[it->second] = 1;
    }

    m_store->m_flushed = false;  // Data changed, needs flush
    m_store->m_header_dirty = true;
}

inline std::shared_ptr<MetadataValue> MetadataAccessor::get(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(m_store->m_mutex);

    auto it = m_store->m_key_to_index.find(key);
    if (it == m_store->m_key_to_index.end()) {
        throw std::runtime_error("Key not found: " + key);
    }

    size_t idx = it->second;

    // Load if not already loaded
    if (m_store->m_hot.data_indices[idx] == SIZE_MAX) {
        lock.unlock();
        std::unique_lock<std::shared_mutex> write_lock(m_store->m_mutex);
        m_store->load_entry(idx);
    }

    // Return from m_data_storage
    size_t data_idx = m_store->m_hot.data_indices[idx];
    const ValueVariant& var = m_store->m_data_storage[data_idx];

    auto meta = std::make_shared<MetadataValue>();
    meta->data = var;
    meta->dtype = m_store->m_hot.dtypes[idx];
    meta->shape = m_store->m_cold.shapes[idx];
    return meta;
}

template<typename V>
void MetadataAccessor::put_batch(const std::map<std::string, V>& values) {
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    for (const auto& [key, value] : values) {
        using ElementType = typename V::value_type;
        DataType dtype = TypeToDataType<ElementType>::value;
        std::vector<size_t> shape_vec(value.shape().begin(), value.shape().end());

        auto it = m_store->m_key_to_index.find(key);

        if (it != m_store->m_key_to_index.end()) {
            // Update existing - SoA pattern: update each array independently
            size_t idx = it->second;
            m_store->m_hot.dtypes[idx] = dtype;
            m_store->m_hot.locations[idx] = StorageLocation::PENDING;
            m_store->m_hot.dirty_flags[idx] = true;
            m_store->m_cold.shapes[idx] = shape_vec;

            // Update data storage
            size_t data_idx = m_store->m_hot.data_indices[idx];
            m_store->m_data_storage[data_idx] = value;
        } else {
            // Create new - SoA pattern: append to each array

            // Performance optimization: Pre-emptively expand all vectors together
            if (m_store->m_hot.keys.size() >= m_store->m_hot.keys.capacity()) {
                size_t new_capacity = m_store->m_hot.keys.capacity() * 2;

                // Expand all 14 vectors together
                m_store->m_hot.keys.reserve(new_capacity);
                m_store->m_hot.dtypes.reserve(new_capacity);
                m_store->m_hot.locations.reserve(new_capacity);
                m_store->m_hot.dirty_flags.reserve(new_capacity);
                m_store->m_hot.loaded_flags.reserve(new_capacity);
                m_store->m_hot.data_indices.reserve(new_capacity);

                m_store->m_cold.shapes.reserve(new_capacity);
                m_store->m_cold.file_positions.reserve(new_capacity);
                m_store->m_cold.compressed_sizes.reserve(new_capacity);
                m_store->m_cold.uncompressed_sizes.reserve(new_capacity);
                m_store->m_cold.compressions.reserve(new_capacity);
                m_store->m_cold.block_infos.reserve(new_capacity);
                m_store->m_cold.stored_in_metadata_flags.reserve(new_capacity);

                m_store->m_data_storage.reserve(new_capacity);
            }

            size_t idx = m_store->m_hot.keys.size();
            size_t data_idx = m_store->m_data_storage.size();

            m_store->m_hot.keys.push_back(key);
            m_store->m_hot.dtypes.push_back(dtype);
            m_store->m_hot.locations.push_back(StorageLocation::PENDING);
            m_store->m_hot.dirty_flags.push_back(true);
            m_store->m_hot.loaded_flags.push_back(true);  
            m_store->m_hot.data_indices.push_back(data_idx);

            m_store->m_cold.shapes.push_back(shape_vec);
            m_store->m_cold.file_positions.push_back(0);
            m_store->m_cold.compressed_sizes.push_back(0);
            m_store->m_cold.uncompressed_sizes.push_back(0);
            m_store->m_cold.compressions.push_back(CompressionAlgorithm::NONE);
            m_store->m_cold.block_infos.push_back({});
            m_store->m_cold.stored_in_metadata_flags.push_back(1);  // Always metadata

            m_store->m_data_storage.push_back(value);
            m_store->m_key_to_index[std::string_view(m_store->m_hot.keys[idx])] = idx;
        }

        // Mark as metadata for existing entries too
        if (it != m_store->m_key_to_index.end()) {
            m_store->m_cold.stored_in_metadata_flags[it->second] = 1;
        }
    }

    m_store->m_flushed = false;  // Data changed, needs flush
    m_store->m_header_dirty = true;
}

inline std::map<std::string, MetadataValue> MetadataAccessor::get_batch(
    const std::vector<std::string>& keys) {
    std::shared_lock<std::shared_mutex> lock(m_store->m_mutex);

    std::map<std::string, MetadataValue> results;

    // SoA pattern: batch access with single lock (cache-efficient)
    for (const auto& key : keys) {
        auto it = m_store->m_key_to_index.find(key);
        if (it != m_store->m_key_to_index.end()) {
            size_t idx = it->second;

            // Access hot data (sequential, cache-friendly)
            DataType dtype = m_store->m_hot.dtypes[idx];
            size_t data_idx = m_store->m_hot.data_indices[idx];

            // Build result (cold data accessed only when building)
            MetadataValue meta;
            meta.data = m_store->m_data_storage[data_idx];
            meta.dtype = dtype;
            meta.shape = m_store->m_cold.shapes[idx];

            results[key] = meta;
        } else {
            // Legacy fallback for migration
            auto legacy_meta = get(key);
            if (legacy_meta) {
                results[key] = *legacy_meta;
            }
        }
    }

    return results;
}

inline std::map<std::string, MetadataValue> MetadataAccessor::get_all() {
    std::shared_lock<std::shared_mutex> lock(m_store->m_mutex);

    // Ensure metadata block is loaded (idempotent, cached)
    m_store->load_metadata_block();

    std::map<std::string, MetadataValue> results;

    // SoA pattern: iterate through all entries
    for (size_t i = 0; i < m_store->m_hot.keys.size(); ++i) {
        // Filter for metadata block entries only
        if (m_store->m_cold.stored_in_metadata_flags[i] == 1) {
            const std::string& key = m_store->m_hot.keys[i];

            // Access hot data (cache-efficient)
            DataType dtype = m_store->m_hot.dtypes[i];
            size_t data_idx = m_store->m_hot.data_indices[i];

            // Build MetadataValue
            MetadataValue meta;
            meta.data = m_store->m_data_storage[data_idx];
            meta.dtype = dtype;
            meta.shape = m_store->m_cold.shapes[i];

            results[key] = meta;
        }
    }

    return results;
}

inline void MetadataAccessor::remove(const std::string& key) {
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    // SoA pattern: remove from hot/cold arrays
    auto it = m_store->m_key_to_index.find(key);
    if (it != m_store->m_key_to_index.end()) {
        size_t idx = it->second;

        // Swap with last element and pop (O(1) removal)
        size_t last_idx = m_store->m_hot.keys.size() - 1;
        if (idx != last_idx) {
            // Swap hot data
            m_store->m_hot.keys[idx] = std::move(m_store->m_hot.keys[last_idx]);
            m_store->m_hot.dtypes[idx] = m_store->m_hot.dtypes[last_idx];
            m_store->m_hot.locations[idx] = m_store->m_hot.locations[last_idx];
            m_store->m_hot.dirty_flags[idx] = m_store->m_hot.dirty_flags[last_idx];
            m_store->m_hot.data_indices[idx] = m_store->m_hot.data_indices[last_idx];

            // Swap cold data
            m_store->m_cold.shapes[idx] = std::move(m_store->m_cold.shapes[last_idx]);
            m_store->m_cold.file_positions[idx] = m_store->m_cold.file_positions[last_idx];
            m_store->m_cold.compressed_sizes[idx] = m_store->m_cold.compressed_sizes[last_idx];
            m_store->m_cold.uncompressed_sizes[idx] = m_store->m_cold.uncompressed_sizes[last_idx];
            m_store->m_cold.compressions[idx] = m_store->m_cold.compressions[last_idx];
            m_store->m_cold.block_infos[idx] = std::move(m_store->m_cold.block_infos[last_idx]);
            m_store->m_cold.stored_in_metadata_flags[idx] = m_store->m_cold.stored_in_metadata_flags[last_idx];

            // Update key_to_index for swapped element
            m_store->m_key_to_index[std::string_view(m_store->m_hot.keys[idx])] = idx;
        }

        // Pop last elements
        m_store->m_hot.keys.pop_back();
        m_store->m_hot.dtypes.pop_back();
        m_store->m_hot.locations.pop_back();
        m_store->m_hot.dirty_flags.pop_back();
        m_store->m_hot.data_indices.pop_back();

        m_store->m_cold.shapes.pop_back();
        m_store->m_cold.file_positions.pop_back();
        m_store->m_cold.compressed_sizes.pop_back();
        m_store->m_cold.uncompressed_sizes.pop_back();
        m_store->m_cold.compressions.pop_back();
        m_store->m_cold.block_infos.pop_back();
        m_store->m_cold.stored_in_metadata_flags.pop_back();

        m_store->m_key_to_index.erase(key);
    }

    m_store->m_flushed = false;  // Data changed, needs flush
    m_store->m_header_dirty = true;
}

inline void MetadataAccessor::clear() {
    std::unique_lock<std::shared_mutex> lock(m_store->m_mutex);

    // SoA pattern: clear all hot/cold arrays
    m_store->m_hot.keys.clear();
    m_store->m_hot.dtypes.clear();
    m_store->m_hot.locations.clear();
    m_store->m_hot.dirty_flags.clear();
    m_store->m_hot.data_indices.clear();

    m_store->m_cold.shapes.clear();
    m_store->m_cold.file_positions.clear();
    m_store->m_cold.compressed_sizes.clear();
    m_store->m_cold.uncompressed_sizes.clear();
    m_store->m_cold.compressions.clear();
    m_store->m_cold.block_infos.clear();
    m_store->m_cold.stored_in_metadata_flags.clear();

    m_store->m_data_storage.clear();
    m_store->m_key_to_index.clear();

    m_store->m_flushed = false;  // Data changed, needs flush
    m_store->m_header_dirty = true;
}

inline bool MetadataAccessor::contains(const std::string& key) {
    std::shared_lock<std::shared_mutex> lock(m_store->m_mutex);

    // SoA pattern: single lookup in m_key_to_index
    // All keys (metadata and separate storage) are indexed
    return m_store->m_key_to_index.find(key) != m_store->m_key_to_index.end();
}

} // namespace star
