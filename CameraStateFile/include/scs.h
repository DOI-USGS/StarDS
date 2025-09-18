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

const std::string PROJECT_NAME = "SCS 0.1.0";
const char* MAGIC_STRING = "CLOUDS++";
const size_t MAGIC_STRING_LENGTH = 8;

constexpr const char* MAGIC_UNCOMPRESSED = "CLD0";
constexpr const char* MAGIC_GZIPPED = "CLDG";


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
    void set_log_level(LogLevel level) {
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
// NDArray Class - A basic n-dimensional array implementation
//==============================================================================

/**
 * @brief A basic n-dimensional array class similar to NumPy
 * 
 * This class implements a n-dimensional array stored as a flat 1D vector
 * with NumPy-style indexing capabilities.
 */
template<typename T>
class NDArray {
public:
    // Public attributes
    std::vector<T> data;           // Flat 1D storage for array elements
    std::vector<size_t> shape;     // Dimensions of the array
    std::vector<size_t> strides;   // Number of elements to step in each dimension

    /**
     * @brief Default constructor
     */
    NDArray() = default;

    /**
     * @brief Constructor with shape
     * @param shape_in Dimensions of the array
     */
    explicit NDArray(const std::vector<size_t>& shape_in) : shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        data.resize(total_size);
    }

    /**
     * @brief Constructor with shape and initial value
     * @param shape_in Dimensions of the array
     * @param initial_value Value to initialize all elements with
     */
    NDArray(const std::vector<size_t>& shape_in, const T& initial_value) : shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        data.resize(total_size, initial_value);
    }

    /**
     * @brief Constructor with data and shape
     * @param data_in Flat data to use
     * @param shape_in Dimensions of the array
     */
    NDArray(const std::vector<T>& data_in, const std::vector<size_t>& shape_in) : data(data_in), shape(shape_in) {
        calculateStrides();
        if (data.size() != computeTotalSize()) {
            throw std::runtime_error("Data size does not match the specified shape");
        }
    }

    /**
     * @brief Copy constructor
     * @param other NDArray to copy from
     */
    NDArray(const NDArray& other) : data(other.data), shape(other.shape), strides(other.strides) {
    }


    /**
     * @brief Access element using multi-dimensional index
     * @param indices Indices for each dimension
     * @return Reference to the element
     */
    T& at(const std::vector<size_t>& indices) {
        return data[flattenIndex(indices)];
    }

    /**
     * @brief Access element using multi-dimensional index (const version)
     * @param indices Indices for each dimension
     * @return Const reference to the element
     */
    const T& at(const std::vector<size_t>& indices) const {
        return data[flattenIndex(indices)];
    }

    /**
     * @brief Reshape the array to new dimensions
     * @param new_shape New dimensions
     */
    void reshape(const std::vector<size_t>& new_shape) {
        size_t new_size = 1;
        for (auto dim : new_shape) {
            new_size *= dim;
        }
        
        if (new_size != data.size()) {
            throw std::runtime_error("Cannot reshape array to requested dimensions");
        }
        
        shape = new_shape;
        calculateStrides();
    }

    /**
     * @brief Calculate total bytes needed to store the array, optionally compressed
     * @param compression Compression type ("CLD0", "CLDG", etc.)
     * @return Total bytes (compressed if compression is specified)
     */
    size_t totalBytes() const {
        // Compute uncompressed size
        size_t uncompressed_size = 0;
        if constexpr (std::is_same_v<T, std::string>) {
            size_t string_bytes = 0;
            for (const auto& str : data) {
                string_bytes += str.size() + sizeof(size_t); // String length + string data
            }
            uncompressed_size = sizeof(size_t) +                      // Number of dimensions
                                sizeof(size_t) * shape.size() +       // Shape dimensions
                                sizeof(size_t) * strides.size() +     // Strides
                                string_bytes;                         // Actual string data
        } else {
            uncompressed_size = sizeof(size_t) +                      // Number of dimensions
                                sizeof(size_t) * shape.size() +       // Shape dimensions
                                sizeof(size_t) * strides.size() +     // Strides
                                sizeof(T) * data.size();              // Actual data
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
        size_t num_dims = arr.shape.size();
        size_to_write = sizeof(num_dims);
        os.write(reinterpret_cast<const char*>(&num_dims), size_to_write);
        LOG_TRACE("Wrote number of dimensions: ", num_dims, " with total number of elements ", sizeof(num_dims));
        total_bytes += size_to_write;
        
        // Write shape
        size_to_write = sizeof(size_t) * num_dims;
        os.write(reinterpret_cast<const char*>(arr.shape.data()), size_to_write);
        LOG_TRACE("Wrote shape of size ", sizeof(size_t) * num_dims);
        total_bytes += size_to_write;
        
        // Write strides
        size_to_write = sizeof(size_t) * num_dims;
        os.write(reinterpret_cast<const char*>(arr.strides.data()), size_to_write);
        LOG_TRACE("Wrote strides of size ", sizeof(size_t) * num_dims);
        total_bytes += size_to_write;

        if constexpr (std::is_same_v<T, std::string>) {
            // Write data (strings need special handling)
            for (const auto& str : arr.data) {
                size_t str_len = str.size();
                os.write(reinterpret_cast<const char*>(&str_len), sizeof(str_len));
                total_bytes += sizeof(str_len);
                os.write(str.data(), str_len);
            }
        } else {
            // Write data in chunks
            constexpr size_t CHUNK_SIZE = 1024 ; // 1MB chunks
            size_to_write = sizeof(T) * arr.data.size();
            const char* data_ptr = reinterpret_cast<const char*>(arr.data.data());
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
        arr.shape.resize(num_dims);
        is.read(reinterpret_cast<char*>(arr.shape.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Read shape array of ", sizeof(size_t) * num_dims, " bytes");
        
        // Read strides
        arr.strides.resize(num_dims);
        is.read(reinterpret_cast<char*>(arr.strides.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Read strides array of ", sizeof(size_t) * num_dims, " bytes");

        // Calculate total size and read data
        size_t total_size = arr.computeTotalSize();
        if constexpr (std::is_same_v<T, std::string>) {            
            arr.data.resize(total_size);
            for (size_t i = 0; i < total_size; ++i) {
                size_t str_len;
                is.read(reinterpret_cast<char*>(&str_len), sizeof(str_len));
                
                std::string str(str_len, '\0');
                is.read(&str[0], str_len);
                arr.data[i] = std::move(str);
                LOG_TRACE("Read string ", i, " with length ", str_len);
            }
            
            LOG_TRACE("Finished reading NDArray<std::string>");
        } else {
            arr.data.resize(total_size);
            is.read(reinterpret_cast<char*>(arr.data.data()), sizeof(T) * total_size);
            LOG_TRACE("Read data array of size ", sizeof(T) * total_size);
        }
        return is;
    }

    /**
     * @brief Calculate strides based on shape
     */
    void calculateStrides() {
        strides.resize(shape.size());
        if (shape.empty()) return;
        
        strides[shape.size() - 1] = 1;
        for (int i = static_cast<int>(shape.size()) - 2; i >= 0; --i) {
            strides[i] = strides[i + 1] * shape[i + 1];
        }
    }

    /**
     * @brief Compute total size of the array
     * @return Total number of elements
     */
    size_t computeTotalSize() const {
        if (shape.empty()) return 0;
        size_t size = 1;
        for (auto dim : shape) {
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
        if (indices.size() != shape.size()) {
            throw std::runtime_error("Index dimensions do not match array dimensions");
        }
        
        size_t flat_index = 0;
        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i] >= shape[i]) {
                throw std::runtime_error("Index out of bounds");
            }
            flat_index += indices[i] * strides[i];
        }
        
        return flat_index;
    }
};


//==============================================================================
// Type Definitions
//==============================================================================

using ValueVariant = std::variant<
    NDArray<char>, NDArray<int>, NDArray<long>, NDArray<long long>,
    NDArray<float>, NDArray<double>, NDArray<std::string>
>;

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

    std::string m_compression = "none"; // "none" or "CLDG"
    size_t m_block_size = DEFAULT_BLOCK_SIZE;
    static constexpr size_t LARGE_ARRAY_THRESHOLD = 1024;
public: 
    std::string m_filename;
    std::map<std::string, std::tuple<size_t, size_t, bool>> m_index; // key -> {position, bytes, dirty flag}
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
        
        size_t size = sizeof(m_header_size);  // Size of header size field
        size += MAGIC_STRING_LENGTH;          // magic string
        size += 4;                            // compression type
        size += sizeof(uint64_t);             // Size of count field
        
        for (const auto& [key, _] : m_index) {
            size += key.length();             // Key data
            size += sizeof(size_t);           // key size
            size += sizeof(size_t);           // position
            size += sizeof(size_t);           // bytes
            // size += 4;                     // compression type
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

        char compression[4];
        initial_stream.read(compression, 4); 
        std::string compression_string(compression, 4);
        m_compression = compression_string;
        
        LOG_TRACE("Found compression type: ", compression_string);
        if(compression_string != MAGIC_UNCOMPRESSED && compression_string != MAGIC_GZIPPED) {
            LOG_ERROR("Invalid compression type: ", compression_string);
            throw std::runtime_error("Invalid compression type");
        }
        m_compression = compression_string;

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
        header_stream.seekg(MAGIC_STRING_LENGTH+4+sizeof(m_header_size), std::ios::beg); 
        size_t count;
        header_stream.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        LOG_TRACE("Found ", count, " entries in index");
        for (size_t i = 0; i < count; i++) {
            std::string key = deserializeKey(header_stream);
            size_t position;
            size_t bytes;

            header_stream.read(reinterpret_cast<char*>(&position), sizeof(position));
            header_stream.read(reinterpret_cast<char*>(&bytes), sizeof(bytes));
            m_index[key] = std::make_tuple(position, bytes, false);
            LOG_TRACE("Loaded key ", key, " with position ", position, " and bytes ", bytes);
        }
        
        m_header_dirty = false;
        LOG_TRACE("Index loaded successfully");
    }

    void printHeader() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        std::cout << "==== SCS File Header ====" << std::endl;
        std::cout << "Filename: " << m_filename << std::endl;
        std::cout << "Compression: " << m_compression << std::endl;
        std::cout << "Header size: " << m_header_size << std::endl;
        std::cout << "Index entries: " << m_index.size() << std::endl;
        size_t idx = 0;
        for (const auto& [key, entry] : m_index) {
            size_t position, bytes;
            bool dirty;
            std::tie(position, bytes, dirty) = entry;
            std::cout << "  [" << idx++ << "] Key: \"" << key << "\""
                      << ", Position: " << position
                      << ", Bytes: " << bytes
                      << ", Dirty: " << (dirty ? "true" : "false") << std::endl;
        }
        std::cout << "=========================" << std::endl;
    }


    /**
     * @brief Writes all data to the file
     */
    void flush() {
        // lock for writing
        std::unique_lock<std::shared_mutex> lock(m_mutex); 
        
        // Recalculate header size if needed
        if (m_header_dirty) {
            m_header_size = calculateHeaderSize();
            m_header_dirty = false;
        }

        std::fstream file(m_filename, std::ios::binary | std::ios::out);
        
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing");
        }

        LOG_TRACE("Writing header section");
        
        file.write(MAGIC_STRING, MAGIC_STRING_LENGTH);

        // Write compression type
        LOG_TRACE("Writing compression type: ", m_compression);
        file.write(m_compression.data(), m_compression.size());

        // Write header size
        LOG_TRACE("Writing header size: ", m_header_size);
        file.write(reinterpret_cast<const char*>(&m_header_size), sizeof(m_header_size));
        
        // Write index section
        uint64_t count = m_index.size();
        file.write(reinterpret_cast<const char*>(&count), sizeof(uint64_t));
        
        LOG_TRACE("Writing ", count, " entries in index");
        size_t pos = m_header_size;
        
        for (const auto& [key, index_entry] : m_index) {
            LOG_TRACE("Serializing key ", key);
            size_t len = key.length();
            file.write(reinterpret_cast<const char*>(&len), sizeof(len));
            file.write(key.data(), len);
            
            // Write position, size, and dirty flag from index_entry
            auto [_, bytes, dirty] = index_entry;
            m_index[key] = std::make_tuple(pos, bytes, false);

            LOG_TRACE("Writing index entry:  key=", key, ", position=", pos, ", bytes=", bytes);
            file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
            file.write(reinterpret_cast<const char*>(&bytes), sizeof(bytes));
            pos += bytes;
        }

        LOG_TRACE("File position after writing index: ", file.tellp());
        LOG_TRACE("Computed Header size: ", m_header_size);
        
        // sanity check the file position
        if(file.tellp() != m_header_size) {
            LOG_ERROR("File position mismatch after writing index, expected ", m_header_size, " but got ", file.tellp());
            throw std::runtime_error("File position mismatch after writing index");
        }

        // Write data after the header
        for (const auto& [key, value_ptr] : m_cache) {
            size_t current_pos = file.tellp();
            LOG_TRACE("Reading value for key from index: key=", key, ", current position=", current_pos);
            
            // Update index with new position
            auto [indexpos, bytes, dirty] = m_index[key];
            if(indexpos != current_pos) {
                LOG_ERROR("Index position mismatch for key ", key, ", expected ", indexpos, " but got ", current_pos);
                throw std::runtime_error("Index position mismatch");
            }

            LOG_TRACE("Writing value for key: key=", key, ", position=", indexpos, ", bytes=", bytes);
            // need to make the capture happy 
            size_t bytes_to_write = bytes;
            // Write the value
            std::visit([this, &file, &bytes_to_write](const auto& value) {
                using V = std::decay_t<decltype(value)>;
                if constexpr (
                    std::is_same_v<V, NDArray<char>> || std::is_same_v<V, NDArray<int>> ||
                    std::is_same_v<V, NDArray<long>> || std::is_same_v<V, NDArray<long long>> ||
                    std::is_same_v<V, NDArray<float>> || std::is_same_v<V, NDArray<double>> ||
                    std::is_same_v<V, NDArray<std::string>>
                ) {
                    if (m_compression == MAGIC_GZIPPED) {
                    #ifdef ENABLE_ZLIB
                        LOG_TRACE("Compressing NDArray data using zlib and writing");
                        // Compress the NDArray data using zlib and write to file
                        std::ostringstream oss;
                        oss << value;
                        LOG_TRACE("Wrote uncompressed data to temp buffer");
                        std::string uncompressed_data = oss.str();

                        uLongf uncompressed_size = static_cast<uLongf>(uncompressed_data.size());
                        uLongf compressed_bound = compressBound(uncompressed_size);
                        std::vector<Bytef> compressed_data(compressed_bound);

                        int z_result = compress2(
                            compressed_data.data(),
                            &compressed_bound,
                            reinterpret_cast<const Bytef*>(uncompressed_data.data()),
                            uncompressed_size,
                            Z_BEST_COMPRESSION
                        );

                        if (z_result != Z_OK) {
                            throw std::runtime_error("zlib compression failed");
                        }

                        // Write the size of the compressed data
                        size_t compressed_size = static_cast<size_t>(compressed_bound);
                        file.write(reinterpret_cast<const char*>(&compressed_size), sizeof(compressed_size));

                        // Write the size of the uncompressed data
                        size_t uncompressed_size_write = static_cast<size_t>(uncompressed_size);
                        file.write(reinterpret_cast<const char*>(&uncompressed_size_write), sizeof(uncompressed_size_write));

                        // Write the compressed data itself
                        file.write(reinterpret_cast<const char*>(compressed_data.data()), compressed_size);
                        
                        // Pad the rest with zero bytes if compressed data is smaller than expected
                        size_t bytes_written = compressed_size + sizeof(size_t) + sizeof(size_t);
                        LOG_TRACE("Wrote compressed data to file with size ", bytes_written);

                        if (bytes_written < bytes_to_write) {
                            std::vector<char> padding(bytes_to_write - bytes_written, 0);
                            file.write(padding.data(), padding.size());
                            LOG_TRACE("Padded compressed data with ", padding.size(), " zero bytes");
                        }
                    #else
                        throw std::runtime_error("deflate compression requested but zlib is not enabled (ENABLE_ZLIB not defined)");
                    #endif
                    } else {
                        // Uncompressed NDArray write (as before)
                        file << value;
                    }
                } else {
                    // Scalar write (as before)
                    file << value;
                }
            }, *value_ptr);
        }
        
        file.flush();
        file.close();
        LOG_TRACE("Flushed and closed file");
    }
    

    /**
     * @brief Constructor with compression options
     * @param fname Filename to use for storage
     * @param compression Compression type ("CLD0" or "CLDG")
     * @param block_size Block size for compression
     */
    SCStore(const std::string& fname, const std::string& compression = MAGIC_UNCOMPRESSED, size_t block_size = DEFAULT_BLOCK_SIZE)
        : m_filename(fname), m_header_dirty(true), m_block_size(block_size) {
        
        set_compression(compression);
        loadIndex();
    }

    /**
     * @brief Destructor - writes all pending changes to disk
     */
    ~SCStore() {
        // flush();
    }

    /**
     * @brief Sets the compression type and block size
     * @param compression Compression type ("CLD0" or "CLDG")
     * @param block_size Block size for compression
     */
    void set_compression(const std::string& compression, size_t block_size = DEFAULT_BLOCK_SIZE) {
        if(compression != MAGIC_UNCOMPRESSED && compression != MAGIC_GZIPPED) {
            LOG_ERROR("Invalid compression type: ", compression);
            throw std::runtime_error("Invalid compression type");
        }

        m_compression = compression;
        m_block_size = block_size;
    }

    /**
     * @brief Stores a value with the given key
     * @param key Key to store
     * @param value Value to store
     */
    template<typename V>
    void put(const std::string& key, const V& value) {
        std::unique_lock<std::shared_mutex> lock(m_mutex);
        m_cache[key] = std::make_shared<ValueVariant>(value);
        size_t total_bytes = value.totalBytes();
        if(m_compression == MAGIC_GZIPPED) {
            #ifdef ENABLE_ZLIB
            LOG_TRACE("Getting total size with compression");

            // Estimate compressed size using zlib's compressBound
            uLongf comp_bound = compressBound(static_cast<uLongf>(value.totalBytes()));
            LOG_TRACE("Compressed size: ", static_cast<size_t>(comp_bound));
            total_bytes = static_cast<size_t>(comp_bound);
            total_bytes += sizeof(size_t) + sizeof(size_t); // size of compressed data and uncompressed data
            #else
            throw std::runtime_error("deflate compression requested but zlib is not enabled (ENABLE_ZLIB not defined)");
            #endif
        } 
        m_index[key] = std::make_tuple(0, total_bytes, true);
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
        
            // Seek to index, past magic string and compression flag
            file->seekg(MAGIC_STRING_LENGTH+4, std::ios::beg);
            auto [position, size, dirty] = m_index[key];
            if (position > 0) {
                file->seekg(position, std::ios::beg);
                LOG_TRACE("Position after seek: ", file->tellg());
                std::shared_ptr<V> value_ptr = std::make_shared<V>();
                if constexpr (
                    std::is_same_v<V, NDArray<char>> || std::is_same_v<V, NDArray<int>> ||
                    std::is_same_v<V, NDArray<long>> || std::is_same_v<V, NDArray<long long>> ||
                    std::is_same_v<V, NDArray<float>> || std::is_same_v<V, NDArray<double>> ||
                    std::is_same_v<V, NDArray<std::string>>
                ) {
                    if (m_compression == MAGIC_GZIPPED) {
                        #ifdef ENABLE_ZLIB
                        // Read the compressed data size
                        LOG_TRACE("Reading compressed data size");
                        size_t compressed_size = 0;
                        file->read(reinterpret_cast<char*>(&compressed_size), sizeof(size_t));
                        if (file->gcount() != sizeof(size_t) || compressed_size == 0) {
                            throw std::runtime_error("Failed to read compressed NDArray size");
                        }

                        size_t uncompressed_size = 0;
                        file->read(reinterpret_cast<char*>(&uncompressed_size), sizeof(size_t));
                        if (file->gcount() != sizeof(size_t) || uncompressed_size == 0) {
                            throw std::runtime_error("Failed to read compressed NDArray size");
                        }

                        LOG_TRACE("Read compressed data size: ", compressed_size);
                        LOG_TRACE("Read uncompressed data size: ", uncompressed_size);
                        
                        // Read the compressed data
                        std::vector<char> compressed_data(compressed_size);
                        file->read(compressed_data.data(), compressed_size);
                        LOG_TRACE("Returned buffer size: ", compressed_data.size());

                        if (static_cast<size_t>(compressed_data.size()) != compressed_size) {
                            throw std::runtime_error("Failed to read compressed NDArray data. Expected " + std::to_string(compressed_size) + " bytes, got " + std::to_string(file->gcount()));
                        }

                        // Decompress using zlib
                        std::vector<char> uncompressed_data(uncompressed_size);

                        uLongf dest_len = static_cast<uLongf>(uncompressed_size);
                        int z_result = uncompress(
                            reinterpret_cast<Bytef*>(uncompressed_data.data()), &dest_len,
                            reinterpret_cast<const Bytef*>(compressed_data.data()), static_cast<uLongf>(compressed_size)
                        );

                        if (z_result == Z_MEM_ERROR) {
                            throw std::runtime_error("zlib decompression error: Z_MEM_ERROR (insufficient memory)");
                        } else if (z_result == Z_BUF_ERROR) {
                            throw std::runtime_error("zlib decompression error: Z_BUF_ERROR (output buffer too small)");
                        } else if (z_result == Z_DATA_ERROR) {
                            throw std::runtime_error("zlib decompression error: Z_DATA_ERROR (input data corrupted or incomplete)");
                        } else if (z_result != Z_OK) {
                            throw std::runtime_error("zlib decompression error: Unknown zlib error code " + std::to_string(z_result));
                        }
                        
                        if (dest_len != uncompressed_size) {
                            throw std::runtime_error("Decompressed NDArray data size mismatch: got " + std::to_string(dest_len) + ", expected " + std::to_string(uncompressed_size));
                        }

                        // Now, create a stream from the uncompressed data and deserialize
                        std::istringstream iss(std::string(uncompressed_data.data(), uncompressed_data.size()));
                        iss >> *value_ptr;
                        #else
                        throw std::runtime_error("Zlib support not enabled, cannot decompress gzipped NDArray");
                        #endif
                    } else {
                        // Uncompressed NDArray read (as before)
                        *file >> *value_ptr;
                    }
                } else {
                    // Scalar read (as before)
                    *file >> *value_ptr;
                }
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
