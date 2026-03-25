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
#include <variant>
#include <vector>
#include <chrono>
#include <shared_mutex>
#include <mutex>

// force it for now
#define ENABLE_CURL 1
#define ENABLE_ZLIB 1

#ifdef ENABLE_ZLIB
#include <zlib.h>
#endif

inline const std::string PROJECT_NAME = "SCS 0.1.0";
inline const char* MAGIC_STRING = "CLOUDS++";
inline const size_t MAGIC_STRING_LENGTH = 8;


//==============================================================================
// Logging System
//==============================================================================

namespace logger {
    enum LogLevel { TRACE=0, DEBUG, INFO, WARN, ERROR };
    static inline LogLevel current_log_level = TRACE;
    
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

    // Helper to check if scalar
    bool is_scalar() const {
        return shape.empty();
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
    }

    size_t serialized_size() const {
        return sizeof(position) + sizeof(total_bytes) + sizeof(datatype) +
               sizeof(size_t) + (shape.size() * sizeof(size_t)) +
               sizeof(compression) + sizeof(block_size) + sizeof(size_t) +
               (blocks.size() * (sizeof(size_t) * 3));
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
            // Check if position is valid
            if (pos >= 0 && pos < static_cast<pos_type>(m_content_length)) {
                // Clear current buffer
                setg(nullptr, nullptr, nullptr);
                m_position = static_cast<size_t>(pos);
                return pos;
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
            
            // Check if new position is valid
            if (new_pos >= 0 && new_pos < static_cast<pos_type>(m_content_length)) {
                // Clear current buffer
                setg(nullptr, nullptr, nullptr);
                m_position = static_cast<size_t>(new_pos);
                return new_pos;
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
        curl_easy_perform(m_curl);
        curl_easy_setopt(m_curl, CURLOPT_NOBODY, 0L);
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
// ndarray Class - Modern n-dimensional array implementation
//==============================================================================

/**
 * @brief Modern n-dimensional array class with xtensor-style API
 *
 * This class implements an n-dimensional array stored as a flat 1D vector
 * with operator() indexing, iterators, and static factory methods.
 */
template<typename T>
class ndarray {
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
    ndarray() = default;

    /**
     * @brief Constructor with shape
     * @param shape_in Dimensions of the array
     */
    explicit ndarray(const std::vector<size_t>& shape_in) : m_shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        m_data.resize(total_size);
    }

    /**
     * @brief Constructor with shape and initial value
     * @param shape_in Dimensions of the array
     * @param initial_value Value to initialize all elements with
     */
    ndarray(const std::vector<size_t>& shape_in, const T& initial_value) : m_shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        m_data.resize(total_size, initial_value);
    }

    /**
     * @brief Constructor with data and shape
     * @param data_in Flat data to use
     * @param shape_in Dimensions of the array
     */
    ndarray(const std::vector<T>& data_in, const std::vector<size_t>& shape_in) : m_data(data_in), m_shape(shape_in) {
        calculateStrides();
        if (m_data.size() != computeTotalSize()) {
            throw std::runtime_error("Data size does not match the specified shape");
        }
    }

    /**
     * @brief Copy constructor
     * @param other ndarray to copy from
     */
    ndarray(const ndarray& other) : m_data(other.m_data), m_shape(other.m_shape), m_strides(other.m_strides) {
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
        if (m_shape.empty()) return 0;
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
     * @brief Legacy at() method for backward compatibility (DEPRECATED)
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
    static ndarray<T> zeros(const std::vector<size_t>& shape) {
        return ndarray<T>(shape, T{0});
    }

    /**
     * @brief Create one-initialized array
     */
    static ndarray<T> ones(const std::vector<size_t>& shape) {
        return ndarray<T>(shape, T{1});
    }

    /**
     * @brief Create array filled with specific value
     */
    static ndarray<T> full(const std::vector<size_t>& shape, const T& value) {
        return ndarray<T>(shape, value);
    }

    /**
     * @brief Create uninitialized array
     */
    static ndarray<T> empty(const std::vector<size_t>& shape) {
        return ndarray<T>(shape);
    }

    /**
     * @brief Create range array (numeric types only)
     */
    template<typename U = T>
    static typename std::enable_if<std::is_arithmetic<U>::value, ndarray<T>>::type
    arange(T start, T stop, T step = T{1}) {
        if (step == T{0}) {
            throw std::runtime_error("Step cannot be zero");
        }

        size_t count = static_cast<size_t>(std::ceil((stop - start) / step));
        ndarray<T> result({count});

        T value = start;
        for (size_t i = 0; i < count; ++i) {
            result.m_data[i] = value;
            value += step;
        }

        return result;
    }

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
    void resize(const std::vector<size_t>& new_shape, const T& fill_value = T{}) {
        size_t new_size = 1;
        for (auto dim : new_shape) {
            new_size *= dim;
        }

        m_data.resize(new_size, fill_value);
        m_shape = new_shape;
        calculateStrides();
    }

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
    friend std::ostream& operator<<(std::ostream& os, const ndarray<T>& arr) {
        LOG_TRACE("Writing ndarray<", typeid(T).name(), "> to output stream at position ", os.tellp());
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
    friend std::istream& operator>>(std::istream& is, ndarray<T>& arr) {
        LOG_TRACE("Reading ndarray<", typeid(T).name(), "> from input stream from byte position ", is.tellg());

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

            LOG_TRACE("Finished reading ndarray<std::string>");
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
        if (m_shape.empty()) return 0;
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
    ndarray<int8_t>, ndarray<int16_t>, ndarray<int32_t>, ndarray<int64_t>,
    ndarray<uint8_t>, ndarray<uint16_t>, ndarray<uint32_t>, ndarray<uint64_t>,
    ndarray<float>, ndarray<double>,
    ndarray<std::string>
>;

//==============================================================================
// Block Compression Functions
//==============================================================================

/**
 * @brief Compresses data in blocks and returns block metadata
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
    std::vector<BlockInfo> block_infos;

    size_t num_blocks = (data_size + block_size - 1) / block_size;
    LOG_TRACE("Compressing ", data_size, " bytes into ", num_blocks, " blocks");

    for (size_t i = 0; i < num_blocks; ++i) {
        size_t offset_in = i * block_size;
        size_t block_uncompressed_size = std::min(block_size, data_size - offset_in);

        BlockInfo info;
        info.offset = compressed_output.size();
        info.uncompressed_size = block_uncompressed_size;

        if (algorithm == CompressionAlgorithm::GZIP) {
            #ifdef ENABLE_ZLIB
            uLongf compressed_bound = compressBound(block_uncompressed_size);
            std::vector<Bytef> temp_compressed(compressed_bound);

            int result = compress2(
                temp_compressed.data(),
                &compressed_bound,
                reinterpret_cast<const Bytef*>(data + offset_in),
                block_uncompressed_size,
                Z_BEST_COMPRESSION
            );

            if (result != Z_OK) {
                throw std::runtime_error("Block compression failed");
            }

            info.compressed_size = compressed_bound;

            // Append compressed block to output
            compressed_output.insert(
                compressed_output.end(),
                temp_compressed.begin(),
                temp_compressed.begin() + compressed_bound
            );
            #else
            throw std::runtime_error("zlib not enabled");
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

    return {compressed_output, block_infos};
}

/**
 * @brief Decompresses specific blocks from compressed data
 * @param compressed_data Full compressed data
 * @param blocks Block metadata
 * @param algorithm Compression algorithm used
 * @param block_indices Which blocks to decompress (empty = all)
 * @return Decompressed data
 */
inline std::vector<char> decompressBlocks(
    const std::vector<char>& compressed_data,
    const std::vector<BlockInfo>& blocks,
    CompressionAlgorithm algorithm,
    const std::vector<size_t>& block_indices = {}) {

    std::vector<char> decompressed_output;

    // If no specific blocks requested, decompress all
    std::vector<size_t> indices_to_decompress = block_indices;
    if (indices_to_decompress.empty()) {
        for (size_t i = 0; i < blocks.size(); ++i) {
            indices_to_decompress.push_back(i);
        }
    }

    // Calculate total output size
    size_t total_size = 0;
    for (size_t idx : indices_to_decompress) {
        total_size += blocks[idx].uncompressed_size;
    }
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

    return decompressed_output;
}

//==============================================================================
// SCStore Class
//==============================================================================

/**
 * @brief A cloud-optimized binary key-value store for serializable data types
 * 
 * This class implements a binary key-value store that can persist data to disk in a 
 * cloud-optimized format. It uses a single file with an index section followed by data
 * for efficient cloud storage and retrieval. Large arrays are chunked 
 * for better performance over networks.
 */
class SCStore {
private:
    static constexpr size_t DEFAULT_BLOCK_SIZE = 64 * 1024; // 64KB blocks

    CompressionAlgorithm m_default_compression = CompressionAlgorithm::NONE;
    size_t m_default_block_size = DEFAULT_BLOCK_SIZE;
    static constexpr size_t LARGE_ARRAY_THRESHOLD = 1024;
public:
    // Type aliases for iterators
    using iterator = std::map<std::string, IndexEntry>::iterator;
    using const_iterator = std::map<std::string, IndexEntry>::const_iterator;

    std::string m_filename;
    std::map<std::string, IndexEntry> m_index; // key -> IndexEntry with block metadata
    std::map<std::string, std::shared_ptr<ValueVariant>> m_cache;
    size_t m_header_size = 0; // Size of the header section in bytes
    bool m_header_dirty = false; // Flag to indicate if header size needs recalculation
    mutable std::shared_mutex m_mutex; // Mutex for thread safety

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

        size_t size = MAGIC_STRING_LENGTH;        // magic string
        size += sizeof(uint8_t);                  // format version
        size += sizeof(m_header_size);            // Size of header size field
        size += sizeof(uint64_t);                 // Size of count field

        for (const auto& [key, entry] : m_index) {
            size += sizeof(size_t);               // key size
            size += key.length();                 // Key data
            size += entry.serialized_size();      // IndexEntry serialized size
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

        initial_stream.read(reinterpret_cast<char*>(&m_header_size), sizeof(m_header_size));
        if (m_header_size == 0) {
            LOG_ERROR("Header size is 0, file is empty");
            throw std::runtime_error("Header size is 0, file is empty");
        }
        LOG_TRACE("Found header size: ", m_header_size);

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
        for (size_t i = 0; i < count; i++) {
            std::string key = deserializeKey(header_stream);

            // Read IndexEntry
            IndexEntry entry;
            entry.read(header_stream);
            entry.dirty = false;

            m_index[key] = entry;
            LOG_TRACE("Loaded key ", key, " with position ", entry.position, " and bytes ", entry.total_bytes);
        }
        
        m_header_dirty = false;
        LOG_TRACE("Index loaded successfully");
    }

    void printHeader() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        std::cout << "==== SCS File Header ====" << std::endl;
        std::cout << "Filename: " << m_filename << std::endl;
        std::cout << "Header size: " << m_header_size << std::endl;
        std::cout << "Index entries: " << m_index.size() << std::endl;
        size_t idx = 0;
        for (const auto& [key, entry] : m_index) {
            std::cout << "  [" << idx++ << "] Key: \"" << key << "\"" << std::endl;
            entry.print();
        }
        std::cout << "=========================" << std::endl;
    }


    /**
     * @brief Writes all data to the file
     */
    void flush() {
        // lock for writing
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        // First pass: Serialize and compress all data to get actual sizes
        std::map<std::string, std::vector<char>> compressed_data;

        for (auto& [key, value_ptr] : m_cache) {
            // Serialize the value to get raw data
            std::ostringstream oss;
            std::visit([&oss](const auto& value) {
                oss << value;
            }, *value_ptr);
            std::string serialized = oss.str();

            IndexEntry& entry = m_index[key];

            // Compress in blocks if needed
            if (entry.compression != CompressionAlgorithm::NONE && entry.block_size > 0) {
                auto [compressed, block_infos] = compressBlocks(
                    serialized.data(), serialized.size(),
                    entry.compression, entry.block_size);

                entry.blocks = std::move(block_infos);
                entry.total_bytes = compressed.size();
                compressed_data[key] = std::move(compressed);

                LOG_TRACE("Compressed key '", key, "': ", serialized.size(), " -> ", entry.total_bytes, " bytes");
            } else {
                // No compression
                BlockInfo single_block;
                single_block.offset = 0;
                single_block.compressed_size = serialized.size();
                single_block.uncompressed_size = serialized.size();
                entry.blocks = {single_block};
                entry.total_bytes = serialized.size();
                compressed_data[key] = std::vector<char>(serialized.begin(), serialized.end());
            }
        }

        // Recalculate header size with updated entry sizes
        m_header_size = calculateHeaderSize();
        m_header_dirty = false;

        // Second pass: Write everything to file
        std::fstream file(m_filename, std::ios::binary | std::ios::out);

        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing");
        }

        LOG_TRACE("Writing header section");

        file.write(MAGIC_STRING, MAGIC_STRING_LENGTH);

        // Write format version
        uint8_t format_version = 2; // v2 with block compression
        file.write(reinterpret_cast<const char*>(&format_version), sizeof(format_version));
        LOG_TRACE("Writing format version: ", (int)format_version);

        // Write header size
        LOG_TRACE("Writing header size: ", m_header_size);
        file.write(reinterpret_cast<const char*>(&m_header_size), sizeof(m_header_size));

        // Write index section
        uint64_t count = m_index.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(uint64_t));

        LOG_TRACE("Writing ", count, " entries in index");
        size_t pos = m_header_size;

        for (auto& [key, index_entry] : m_index) {
            LOG_TRACE("Serializing key ", key);
            size_t len = key.length();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(key.data(), len);

            // Update position and clear dirty flag
            index_entry.position = pos;
            index_entry.dirty = false;

            // Write IndexEntry
            LOG_TRACE("Writing index entry:  key=", key, ", position=", pos, ", bytes=", index_entry.total_bytes);
            index_entry.write(file);

            pos += index_entry.total_bytes;
        }

        LOG_TRACE("File position after writing index: ", file.tellp());
        LOG_TRACE("Computed Header size: ", m_header_size);

        // sanity check the file position
        if(file.tellp() != m_header_size) {
            LOG_ERROR("File position mismatch after writing index, expected ", m_header_size, " but got ", file.tellp());
            throw std::runtime_error("File position mismatch after writing index");
        }

        // Write data after the header
        for (const auto& [key, data] : compressed_data) {
            size_t current_pos = file.tellp();
            const IndexEntry& entry = m_index[key];

            if(entry.position != current_pos) {
                LOG_ERROR("Index position mismatch for key ", key, ", expected ", entry.position, " but got ", current_pos);
                throw std::runtime_error("Index position mismatch");
            }

            LOG_TRACE("Writing data for key: key=", key, ", position=", entry.position, ", bytes=", entry.total_bytes);

            // Write the pre-compressed data
            file.write(data.data(), data.size());
        }

        file.flush();
        file.close();
        LOG_TRACE("Flushed and closed file");
    }
    

    /**
     * @brief Constructor with compression options
     * @param fname Filename to use for storage
     * @param compression Default compression algorithm
     * @param block_size Default block size for compression
     */
    SCStore(const std::string& fname, CompressionAlgorithm compression = CompressionAlgorithm::NONE, size_t block_size = DEFAULT_BLOCK_SIZE)
        : m_filename(fname), m_header_dirty(true), m_default_compression(compression), m_default_block_size(block_size) {
        loadIndex();
    }

    /**
     * @brief Destructor - writes all pending changes to disk (RAII)
     */
    ~SCStore() {
        try {
            flush();
        } catch (const std::exception& e) {
            // Log error but don't throw from destructor
            std::cerr << "Error flushing store in destructor: " << e.what() << std::endl;
        }
    }

    /**
     * @brief Sets the default compression type and block size for new keys
     * @param compression Compression algorithm
     * @param block_size Block size for compression
     */
    void set_default_compression(CompressionAlgorithm compression, size_t block_size = DEFAULT_BLOCK_SIZE) {
        m_default_compression = compression;
        m_default_block_size = block_size;
    }

    /**
     * @brief Stores a value with the given key
     * @param key Key to store
     * @param value Value to store (NDArray)
     * @param compression Compression algorithm (uses default if NONE)
     * @param block_size Block size for compression (uses default if 0)
     */
    template<typename V>
    void put(const std::string& key, const V& value,
             CompressionAlgorithm compression = CompressionAlgorithm::NONE,
             size_t block_size = 0) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);

        // Use defaults if not specified
        if (compression == CompressionAlgorithm::NONE) {
            compression = m_default_compression;
        }
        if (block_size == 0) {
            block_size = m_default_block_size;
        }

        // Cache the value
        m_cache[key] = std::make_shared<ValueVariant>(value);

        // Extract type information
        using ElementType = typename V::value_type; // Assumes V is ndarray<T>
        DataType dtype = TypeToDataType<ElementType>::value;
        const auto& value_shape = value.shape();
        std::vector<size_t> shape(value_shape.begin(), value_shape.end());

        LOG_TRACE("Storing key '", key, "' as ", datatype_to_string(dtype),
                  " array with ", shape.size(), " dimensions");

        // Serialize to get size
        std::ostringstream oss;
        oss << value;
        std::string serialized = oss.str();

        // Compute compressed size if needed
        size_t total_bytes = serialized.size();
        std::vector<BlockInfo> blocks;

        if (compression != CompressionAlgorithm::NONE && serialized.size() > block_size) {
            // Will compress in blocks
            auto [compressed, block_infos] = compressBlocks(
                serialized.data(), serialized.size(), compression, block_size);
            total_bytes = compressed.size();
            blocks = std::move(block_infos);

            LOG_TRACE("Compressed ", serialized.size(), " -> ", total_bytes, " bytes in ",
                      blocks.size(), " blocks");
        } else {
            // Single block, no compression
            BlockInfo single_block;
            single_block.offset = 0;
            single_block.compressed_size = serialized.size();
            single_block.uncompressed_size = serialized.size();
            blocks.push_back(single_block);
        }

        // Create index entry
        IndexEntry entry;
        entry.position = 0;  // Will be updated during flush
        entry.total_bytes = total_bytes;
        entry.datatype = dtype;
        entry.shape = std::move(shape);
        entry.compression = compression;
        entry.block_size = block_size;
        entry.blocks = std::move(blocks);
        entry.dirty = true;

        m_index[key] = std::move(entry);
        m_header_dirty = true;
    }

    /**
     * @brief Retrieves a value by key
     * @param key Key to look up
     * @return Shared pointer to the value, or nullptr if not found
     */
    template<typename V>
    std::shared_ptr<V> get(const std::string& key) {
        LOG_TRACE("Getting value for key ", key);
        
        // First check if the key exists with a shared lock
        {
            std::shared_lock<std::shared_mutex> lock(m_mutex);
            if (!(m_index.count(key) > 0)) return nullptr;
            
            // Check if the value is already in the cache
            auto cache_it = m_cache.find(key);
            if (cache_it != m_cache.end()) {
                // Value is in cache, try to get it
                if (auto val = std::get_if<V>(m_cache[key].get())) {
                    return std::make_shared<V>(*val);
                }
                LOG_TRACE("Value type mismatch for key ", key);
                return nullptr;
            }
        }
        
        { // Not in cache, need to load from file with exclusive lock
            std::unique_lock<std::shared_mutex> lock(m_mutex);
        
            // Check again in case another thread loaded it while we were waiting
            if (m_cache.find(key) != m_cache.end()) {
                if (auto val = std::get_if<V>(m_cache[key].get())) {
                    return std::make_shared<V>(*val);
                }
                LOG_TRACE("Value type mismatch for key ", key);
                return nullptr;
            }
        
            LOG_TRACE("Value not in cache, loading from file for key ", key);
            
            std::unique_ptr<std::istream> file;
            
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
            
            if (!file->good()) return nullptr;

            // Get the index entry
            const IndexEntry& entry = m_index[key];
            if (entry.position > 0) {
                file->seekg(entry.position, std::ios::beg);
                LOG_TRACE("Position after seek: ", file->tellg());

                // Read compressed data
                std::vector<char> compressed_data(entry.total_bytes);
                file->read(compressed_data.data(), entry.total_bytes);
                if (static_cast<size_t>(file->gcount()) != entry.total_bytes) {
                    throw std::runtime_error("Failed to read data. Expected " +
                                           std::to_string(entry.total_bytes) + " bytes, got " +
                                           std::to_string(file->gcount()));
                }

                // Decompress if needed
                std::vector<char> decompressed;
                if (entry.compression != CompressionAlgorithm::NONE && !entry.blocks.empty()) {
                    decompressed = decompressBlocks(compressed_data, entry.blocks, entry.compression);
                    LOG_TRACE("Decompressed ", compressed_data.size(), " -> ", decompressed.size(), " bytes");
                } else {
                    decompressed = std::move(compressed_data);
                }

                // Deserialize from decompressed data
                std::istringstream iss(std::string(decompressed.data(), decompressed.size()));
                std::shared_ptr<V> value_ptr = std::make_shared<V>();
                iss >> *value_ptr;

                m_cache[key] = std::make_shared<ValueVariant>(*value_ptr);
                LOG_TRACE("Deserialized value of type ", typeid(V).name());
                return value_ptr;
            } else {
                LOG_TRACE("Value not yet written to file for key ", key);
                return nullptr;
            }
        }
    }

    /**
     * @brief Checks if a key exists in the store
     * @param key Key to check
     * @return True if the key exists, false otherwise
     */
    bool contains(const std::string& key) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_index.count(key) > 0;
    }

    /**
     * @brief Returns an iterator to the beginning of the index
     * @return Iterator to the first key-value pair
     */
    iterator begin() {
        return m_index.begin();
    }

    /**
     * @brief Returns an iterator to the end of the index
     * @return Iterator to one past the last key-value pair
     */
    iterator end() {
        return m_index.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the index
     * @return Const iterator to the first key-value pair
     */
    const_iterator begin() const {
        return m_index.begin();
    }

    /**
     * @brief Returns a const iterator to the end of the index
     * @return Const iterator to one past the last key-value pair
     */
    const_iterator end() const {
        return m_index.end();
    }

    /**
     * @brief Returns a const iterator to the beginning of the index
     * @return Const iterator to the first key-value pair
     */
    const_iterator cbegin() const {
        return m_index.cbegin();
    }

    /**
     * @brief Returns a const iterator to the end of the index
     * @return Const iterator to one past the last key-value pair
     */
    const_iterator cend() const {
        return m_index.cend();
    }

    /**
     * @brief Removes a key-value pair from the store
     * @param key Key to remove
     */
    void remove(const std::string& key) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        if (m_index.count(key)) {
            m_cache.erase(key);
            m_index.erase(key);
            m_header_dirty = true; // Header size will change with removed key
            lock.unlock(); // Unlock before flush to avoid deadlock
            flush();
        }
    }

    /**
     * @brief Returns the number of key-value pairs in the store
     * @return Number of key-value pairs
     */
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_index.size();
    }
};
