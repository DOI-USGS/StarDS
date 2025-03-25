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


const std::string PROJECT_NAME = "SCS";


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
          ss << "[" << PROJECT_NAME << ":" << LOG_LEVEL_STRINGS[level] << "]"
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

// Logging macros for different severity levels
#define LOG_TRACE(...) logger::log_internal(logger::TRACE, __LINE__, __func__, __VA_ARGS__)
#define LOG_DEBUG(...) logger::log_internal(logger::DEBUG, __LINE__, __func__, __VA_ARGS__)
#define LOG_INFO(...)  logger::log_internal(logger::INFO,  __LINE__, __func__, __VA_ARGS__)
#define LOG_WARN(...)  logger::log_internal(logger::WARN,  __LINE__, __func__, __VA_ARGS__)
#define LOG_ERROR(...) logger::log_internal(logger::ERROR, __LINE__, __func__, __VA_ARGS__)



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
     * @brief Calculate total bytes needed to store the array
     * @return Total bytes
     */
    size_t totalBytes() const {
        return sizeof(size_t) +                      // Number of dimensions
               sizeof(size_t) * shape.size() +       // Shape dimensions
               sizeof(size_t) * strides.size() +     // Strides
               sizeof(T) * data.size();              // Actual data
    }

    /**
     * @brief Write array to output stream
     * @param os Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const NDArray<T>& arr) {
        LOG_TRACE("Writing NDArray<", typeid(T).name(), "> to output stream");
        
        // Write number of dimensions
        size_t num_dims = arr.shape.size();
        os.write(reinterpret_cast<const char*>(&num_dims), sizeof(num_dims));
        LOG_TRACE("Wrote number of dimensions: ", num_dims, " of size ", sizeof(num_dims));
        
        // Write shape
        os.write(reinterpret_cast<const char*>(arr.shape.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Wrote shape of size ", sizeof(size_t) * num_dims);
        
        // Write strides
        os.write(reinterpret_cast<const char*>(arr.strides.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Wrote strides of size ", sizeof(size_t) * num_dims);
        
        // Write data
        os.write(reinterpret_cast<const char*>(arr.data.data()), sizeof(T) * arr.data.size());
        LOG_TRACE("Wrote data of size ", sizeof(T) * arr.data.size());
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
        arr.data.resize(total_size);
        is.read(reinterpret_cast<char*>(arr.data.data()), sizeof(T) * total_size);
        LOG_TRACE("Read data array of size ", sizeof(T) * total_size);
        return is;
    }

private:
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


// Specialization for std::string to handle binary serialization properly
template<>
class NDArray<std::string> {
public:
    // Public attributes
    std::vector<std::string> data;  // Flat 1D storage for string elements
    std::vector<size_t> shape;      // Dimensions of the array
    std::vector<size_t> strides;    // Number of elements to step in each dimension

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
    NDArray(const std::vector<size_t>& shape_in, const std::string& initial_value) : shape(shape_in) {
        calculateStrides();
        size_t total_size = computeTotalSize();
        data.resize(total_size, initial_value);
    }

    /**
     * @brief Constructor with data and shape
     * @param data_in Flat data to use
     * @param shape_in Dimensions of the array
     */
    NDArray(const std::vector<std::string>& data_in, const std::vector<size_t>& shape_in) : data(data_in), shape(shape_in) {
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
    std::string& at(const std::vector<size_t>& indices) {
        return data[flattenIndex(indices)];
    }

    /**
     * @brief Access element using multi-dimensional index (const version)
     * @param indices Indices for each dimension
     * @return Const reference to the element
     */
    const std::string& at(const std::vector<size_t>& indices) const {
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
     * @brief Calculate total bytes needed to store the array
     * @return Total bytes
     */
    size_t totalBytes() const {
        size_t string_bytes = 0;
        for (const auto& str : data) {
            string_bytes += str.size() + sizeof(size_t); // String length + string data
        }
        
        return sizeof(size_t) +                      // Number of dimensions
               sizeof(size_t) * shape.size() +       // Shape dimensions
               sizeof(size_t) * strides.size() +     // Strides
               string_bytes;                         // Actual string data
    }

    /**
     * @brief Write array to output stream
     * @param os Output stream
     */
    friend std::ostream& operator<<(std::ostream& os, const NDArray<std::string>& arr) {
        // Write number of dimensions
        size_t num_dims = arr.shape.size();
        os.write(reinterpret_cast<const char*>(&num_dims), sizeof(num_dims));
        
        // Write shape
        os.write(reinterpret_cast<const char*>(arr.shape.data()), sizeof(size_t) * num_dims);
        
        // Write strides
        os.write(reinterpret_cast<const char*>(arr.strides.data()), sizeof(size_t) * num_dims);
        
        // Write data (strings need special handling)
        size_t num_strings = arr.data.size();
        // os.write(reinterpret_cast<const char*>(&num_strings), sizeof(num_strings));
        
        for (const auto& str : arr.data) {
            size_t str_len = str.size();
            os.write(reinterpret_cast<const char*>(&str_len), sizeof(str_len));
            os.write(str.data(), str_len);
        }
        
        return os;
    }

    /**
     * @brief Read array from input stream
     * @param is Input stream
     */
    friend std::istream& operator>>(std::istream& is, NDArray<std::string>& arr) {
        LOG_TRACE("Reading NDArray<std::string> from input stream");
        
        // Read number of dimensions
        size_t num_dims;
        is.read(reinterpret_cast<char*>(&num_dims), sizeof(num_dims));
        LOG_TRACE("Read number of dimensions: ", num_dims);
        
        // Read shape
        arr.shape.resize(num_dims);
        is.read(reinterpret_cast<char*>(arr.shape.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Read shape with ", num_dims, " dimensions");
        
        // Read strides
        arr.strides.resize(num_dims);
        is.read(reinterpret_cast<char*>(arr.strides.data()), sizeof(size_t) * num_dims);
        LOG_TRACE("Read strides");
        
        // Read data (strings need special handling)
        size_t num_strings = arr.computeTotalSize();
        // is.read(reinterpret_cast<char*>(&num_strings), sizeof(num_strings));
        // LOG_TRACE("Reading ", num_strings, " strings");
        
        arr.data.resize(num_strings);
        for (size_t i = 0; i < num_strings; ++i) {
            size_t str_len;
            is.read(reinterpret_cast<char*>(&str_len), sizeof(str_len));
            
            std::string str(str_len, '\0');
            is.read(&str[0], str_len);
            arr.data[i] = std::move(str);
            LOG_TRACE("Read string ", i, " with length ", str_len);
        }
        
        LOG_TRACE("Finished reading NDArray<std::string>");
        return is;
    }

private:
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
// Type Traits and Helpers
//==============================================================================

//==============================================================================
// Type Definitions
//==============================================================================

using ValueVariant = std::variant<
    NDArray<char>, NDArray<int>, NDArray<long>, NDArray<long long>,
    NDArray<float>, NDArray<double>, NDArray<std::string>
>;



/**
 * @brief Type trait to check if a type is serializable
 */
template<typename T, typename = void>
struct is_serializable : std::false_type {};

template<typename T>
struct is_serializable<T, 
    std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>()),
                decltype(std::declval<std::istream&>() >> std::declval<T>())>> 
    : std::true_type {};

/**
 * @brief Type trait to check if a type is a vector
 */
template<typename T>
struct is_vector : std::false_type {};

template<typename T>
struct is_vector<std::vector<T>> : std::true_type {};

/**
 * @brief Hash function for variants
 */
struct VariantHash {
    template<typename... Ts>
    std::size_t operator()(const std::variant<Ts...>& v) const {
        return std::visit([](const auto& x) { return std::hash<std::decay_t<decltype(x)>>{}(x); }, v);
    }
};

/**
 * @brief Equality comparison for variants
 */
struct VariantEqual {
    template<typename... Ts>
    bool operator()(const std::variant<Ts...>& a, const std::variant<Ts...>& b) const {
        return a == b;
    }
};

/**
 * @brief Helper to get dimensions of n-dimensional array
 */
template<typename T>
struct ArrayTraits {
    static constexpr size_t dimensions = 0;
    using BaseType = T;
};

template<typename T>
struct ArrayTraits<std::vector<T>> {
    static constexpr size_t dimensions = ArrayTraits<T>::dimensions + 1;
    using BaseType = typename ArrayTraits<T>::BaseType;
};

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
    static constexpr size_t BLOCK_SIZE = 64 * 1024; // 64KB blocks
    static constexpr size_t LARGE_ARRAY_THRESHOLD = 1024;
public: 
    std::string m_filename;
    std::map<std::string, std::tuple<size_t, size_t, bool>> m_index; // key -> {position, bytes, dirty flag}
    std::map<std::string, std::shared_ptr<ValueVariant>> m_cache;
    size_t m_header_size = 0; // Size of the header section in bytes
    bool m_header_dirty = false; // Flag to indicate if header size needs recalculation

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
     * @brief Gets dimensions and sizes of an array
     * @param arr Array to analyze
     * @param sizes Vector to store sizes in
     */
    template<typename T>
    void getArrayInfo(const T& arr, std::vector<size_t>& sizes) {
        LOG_TRACE("Getting array info for type ", typeid(T).name());
        if constexpr (is_vector<T>::value) {
            sizes.push_back(arr.size());
            if (!arr.empty()) {
                getArrayInfo(arr[0], sizes);
            }
        }
    }


    /**
     * @brief Calculates the size of the header based on current index
     * @return Size of the header in bytes
     */
    size_t calculateHeaderSize() {
        LOG_TRACE("Calculating header size");
        size_t size = sizeof(m_header_size);  // Size of header size field
        size += sizeof(uint64_t);             // Size of count field
        
        for (const auto& [key, _] : m_index) {
            size += key.length();             // Key data
            size += sizeof(size_t);           // key size
            size += sizeof(size_t);           // position
            size += sizeof(size_t);           // bytes
        }
        
        LOG_TRACE("Header size calculated to ", size);
        return size;
    }

    /**
     * @brief Loads the index from the file
     */
    void loadIndex() {
        LOG_TRACE("Loading index for file ", m_filename);
        std::fstream file(m_filename, std::ios::binary | std::ios::in);
        if (!file.is_open()) return;

        // Read header size first
        file.read(reinterpret_cast<char*>(&m_header_size), sizeof(m_header_size));
        
        // Read number of entries
        uint64_t count = 0;
        file.read(reinterpret_cast<char*>(&count), sizeof(count));
        
        LOG_TRACE("Found ", count, " entries in index");
        for (size_t i = 0; i < count; i++) {
            std::string key = deserializeKey(file);
            size_t position;
            size_t bytes;

            file.read(reinterpret_cast<char*>(&position), sizeof(position));
            file.read(reinterpret_cast<char*>(&bytes), sizeof(bytes));
            m_index[key] = std::make_tuple(position, bytes, false);
            LOG_TRACE("Loaded key ", key, " with position ", position, " and bytes ", bytes);
        }
        
        file.close();
        m_header_dirty = false;
        LOG_TRACE("Index loaded successfully");
    }

    /**
     * @brief Writes all data to the file
     */
    void flush() {
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

        // Write header size
        file.seekp(0);
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

            LOG_TRACE("Writing index entry for key=", key, ", position=", pos, ", bytes=", bytes);
            file.write(reinterpret_cast<const char*>(&pos), sizeof(pos));
            file.write(reinterpret_cast<const char*>(&bytes), sizeof(bytes));
            pos += bytes;
        }
        
        // sanity check the file position
        if(file.tellp() != m_header_size) {
            LOG_ERROR("File position mismatch after writing index, expected ", m_header_size, " but got ", file.tellp());
            throw std::runtime_error("File position mismatch after writing index");
        }

        // Write data after the header
        for (const auto& [key, value_ptr] : m_cache) {
            size_t current_pos = file.tellp();
            
            LOG_TRACE("Writing value for key ", key, " at position ", current_pos);
            // Update index with new position
            auto [indexpos, bytes, dirty] = m_index[key];
            if(indexpos != current_pos) {
                LOG_ERROR("Index position mismatch for key ", key, " expected ", indexpos, " but got ", current_pos);
                throw std::runtime_error("Index position mismatch");
            }

            LOG_TRACE("Writing value for key=", key, ", position=", indexpos, ", bytes=", bytes);
            // Write the value
            std::visit([this, &file](const auto& value) {
                file << value;
            }, *value_ptr);
        }
        
        file.flush();
        file.close();
        LOG_TRACE("Flushed and closed file");
    }


    /**
     * @brief Constructor
     * @param fname Filename to use for storage
     */
    explicit SCStore(const std::string& fname) 
        : m_filename(fname), m_header_dirty(true) {
        loadIndex();
    }

    /**
     * @brief Destructor - writes all pending changes to disk
     */
    ~SCStore() {
        // flush();
    }

    /**
     * @brief Stores a value with the given key
     * @param key Key to store
     * @param value Value to store
     */
    template<typename V>
    void put(const std::string& key, const V& value) {
        m_cache[key] = std::make_shared<ValueVariant>(value);
        m_index[key] = std::make_tuple(0, value.totalBytes(), true);
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
        if (!(m_index.count(key) > 0)) return nullptr;
        // Check if the value is already in the cache
        auto cache_it = m_cache.find(key);
        if (cache_it == m_cache.end()) {
            // Not in cache, load from file
            LOG_TRACE("Value not in cache, loading from file for key ", key);
            std::ifstream file(m_filename, std::ios::binary);
            if (!file) {
                LOG_TRACE("Failed to open file for reading");
                return nullptr;
            }
            
            auto [position, size, dirty] = m_index[key];
            LOG_TRACE("Reading key=", key, ", position=", position, ", size=", size, ", dirty=", dirty);
            if (position > 0) {  // Position 0 means not yet written to file
                file.seekg(position);
                // Deserialize the value from file
                    
                std::shared_ptr<V> value_ptr = std::make_shared<V>();
                file >> *value_ptr;
                m_cache[key] = std::make_shared<ValueVariant>(*value_ptr);
                LOG_TRACE("Deserialized value of type ", typeid(V).name(), " with size ", value_ptr->totalBytes());
                return value_ptr;
            } else {
                LOG_TRACE("Value not yet written to file for key ", key);
                return nullptr;
            }
        }
        
        // Now try to get the value from cache
        if (auto val = std::get_if<V>(m_cache[key].get())) {
            return std::make_shared<V>(*val);
        }
        LOG_TRACE("No value found for key ", key);
        return nullptr;
    }

    /**
     * @brief Checks if a key exists in the store
     * @param key Key to check
     * @return True if the key exists, false otherwise
     */
    bool contains(const std::string& key) const {
        return m_index.count(key) > 0;
    }

    /**
     * @brief Removes a key-value pair from the store
     * @param key Key to remove
     */
    void remove(const std::string& key) {
        if (m_index.count(key)) {
            m_cache.erase(key);
            m_index.erase(key);
            m_header_dirty = true; // Header size will change with removed key
            flush();
        }
    }

    /**
     * @brief Returns the number of key-value pairs in the store
     * @return Number of key-value pairs
     */
    size_t size() const {
        return m_index.size();
    }
};
