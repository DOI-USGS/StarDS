#include <iostream>
#include <fstream>
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <variant>
#include <vector>
#include <numeric>

template<typename T, typename = void>
struct is_serializable : std::false_type {};

template<typename T>
struct is_serializable<T, 
    std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>()),
                decltype(std::declval<std::istream&>() >> std::declval<T>())>> 
    : std::true_type {};

template<typename T>
struct is_vector : std::false_type {};

template<typename T>
struct is_vector<std::vector<T>> : std::true_type {};

struct VariantHash {
    template<typename... Ts>
    std::size_t operator()(const std::variant<Ts...>& v) const {
        return std::visit([](const auto& x) { return std::hash<std::decay_t<decltype(x)>>{}(x); }, v);
    }
};

struct VariantEqual {
    template<typename... Ts>
    bool operator()(const std::variant<Ts...>& a, const std::variant<Ts...>& b) const {
        return a == b;
    }
};

// Helper to get dimensions of n-dimensional array
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

/**
 * @brief A binary key-value store for serializable data types
 * 
 * @tparam V The value type to be stored, must be serializable
 * @tparam KeyTypes Variadic template parameter pack for key types, all must be serializable
 * 
 * This class implements a binary key-value store that can persist data to disk.
 * It supports multiple key types through variadic templates and maintains an in-memory
 * cache along with disk storage. Large arrays are handled efficiently by streaming
 * directly to/from disk.
 */
template<typename V, typename... KeyTypes>
class BinaryKVStore {
private:
    using KeyVariant = std::variant<KeyTypes...>;
    static constexpr size_t LARGE_ARRAY_THRESHOLD = 1024;
    std::string filename;
    std::unordered_map<KeyVariant, std::pair<std::streampos, bool>, 
                      VariantHash, VariantEqual> index;
    std::unordered_map<KeyVariant, std::shared_ptr<V>, 
                      VariantHash, VariantEqual> cache;

    static_assert((is_serializable<KeyTypes>::value && ...), "All key types must be serializable");
    static_assert(is_serializable<V>::value, "Value type must be serializable");

    void serializeKey(std::ostream& os, const KeyVariant& key) {
        os << key.index() << " ";
        std::visit([&os](const auto& k) { os << k; }, key);
    }

    KeyVariant deserializeKey(std::istream& is) {
        size_t type_index;
        is >> type_index;
        
        KeyVariant key;
        std::visit([&is, type_index](auto& x) {
            using T = std::decay_t<decltype(x)>;
            if (type_index == KeyVariant(T{}).index()) {
                is >> x;
            }
        }, key);
        return key;
    }

    // Get dimensions and sizes
    template<typename T>
    void getArrayInfo(const T& arr, std::vector<size_t>& sizes) {
        if constexpr (is_vector<T>::value) {
            sizes.push_back(arr.size());
            if (!arr.empty()) {
                getArrayInfo(arr[0], sizes);
            }
        }
    }

    // Flatten array
    template<typename T>
    void flattenArray(const T& arr, std::vector<typename ArrayTraits<T>::BaseType>& flat) {
        if constexpr (is_vector<T>::value) {
            for (const auto& item : arr) {
                flattenArray(item, flat);
            }
        } else {
            flat.push_back(arr);
        }
    }

    // Reconstruct array from flat data
    template<typename T>
    void reconstructArray(T& arr, const std::vector<typename ArrayTraits<T>::BaseType>& flat, 
                         const std::vector<size_t>& sizes, size_t& index) {
        if constexpr (is_vector<T>::value) {
            arr.resize(sizes[0]);
            for (size_t i = 0; i < arr.size(); ++i) {
                reconstructArray(arr[i], flat, std::vector<size_t>(sizes.begin() + 1, sizes.end()), index);
            }
        } else {
            arr = flat[index++];
        }
    }

    template<typename T>
    void writeArray(std::ostream& os, const T& arr) {
        std::vector<size_t> sizes;
        getArrayInfo(arr, sizes);
        
        size_t dim_count = sizes.size();
        os.write(reinterpret_cast<const char*>(&dim_count), sizeof(dim_count));
        os.write(reinterpret_cast<const char*>(sizes.data()), sizeof(size_t) * dim_count);

        std::vector<typename ArrayTraits<T>::BaseType> flat;
        flattenArray(arr, flat);
        size_t total_size = flat.size() * sizeof(typename ArrayTraits<T>::BaseType);

        if (total_size > LARGE_ARRAY_THRESHOLD) {
            os.write(reinterpret_cast<const char*>(flat.data()), total_size);
        } else {
            for (const auto& item : flat) {
                os << item;
            }
        }
    }

    template<typename T>
    void readArray(std::istream& is, T& arr) {
        size_t dim_count;
        is.read(reinterpret_cast<char*>(&dim_count), sizeof(dim_count));
        
        std::vector<size_t> sizes(dim_count);
        is.read(reinterpret_cast<char*>(sizes.data()), sizeof(size_t) * dim_count);

        size_t total_elements = std::accumulate(sizes.begin(), sizes.end(), size_t(1), std::multiplies<>());
        std::vector<typename ArrayTraits<T>::BaseType> flat(total_elements);
        size_t total_size = total_elements * sizeof(typename ArrayTraits<T>::BaseType);

        if (total_size > LARGE_ARRAY_THRESHOLD) {
            is.read(reinterpret_cast<char*>(flat.data()), total_size);
        } else {
            for (auto& item : flat) {
                is >> item;
            }
        }

        size_t index = 0;
        reconstructArray(arr, flat, sizes, index);
    }

    template<typename T>
    void serializeValue(std::ostream& os, const T& value) {
        if constexpr (is_vector<T>::value) {
            writeArray(os, value);
        } else {
            os << value;
        }
    }

    template<typename T>
    void deserializeValue(std::istream& is, T& value) {
        if constexpr (is_vector<T>::value) {
            readArray(is, value);
        } else {
            is >> value;
        }
    }

    void loadIndex() {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return;

        while (file.good()) {
            std::streampos pos = file.tellg();
            KeyVariant key = deserializeKey(file);
            if (!file.good()) break;

            V dummy;
            deserializeValue(file, dummy);
            index[key] = {pos, false};
        }
        file.close();
    }

    void writeAll() {
        std::ofstream file(filename, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing");
        }

        for (const auto& [key, value_ptr] : cache) {
            serializeKey(file, key);
            serializeValue(file, *value_ptr);
            index[key] = {file.tellp(), true};
        }
        file.close();
    }

    std::shared_ptr<V> loadValue(const KeyVariant& key) {
        if (cache.count(key)) {
            return cache[key];
        }

        auto it = index.find(key);
        if (it == index.end()) {
            return nullptr;
        }

        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for reading");
        }

        file.seekg(it->second.first);
        deserializeKey(file);
        V value;
        deserializeValue(file, value);
        file.close();

        auto ptr = std::make_shared<V>(value);
        cache[key] = ptr;
        it->second.second = true;
        return ptr;
    }

public:
    explicit BinaryKVStore(const std::string& fname) : filename(fname) {
        loadIndex();
    }

    ~BinaryKVStore() {
        writeAll();
    }

    template<typename K>
    void put(const K& key, const V& value) {
        static_assert((std::is_same_v<K, KeyTypes> || ...), "Key type must be one of the specified types");
        KeyVariant k = key;
        cache[k] = std::make_shared<V>(value);
        index[k] = {0, true};
    }

    template<typename K>
    std::shared_ptr<V> get(const K& key) {
        static_assert((std::is_same_v<K, KeyTypes> || ...), "Key type must be one of the specified types");
        KeyVariant k = key;
        auto cached = cache.find(k);
        if (cached != cache.end()) {
            return cached->second;
        }
        return loadValue(k);
    }

    template<typename K>
    bool contains(const K& key) const {
        static_assert((std::is_same_v<K, KeyTypes> || ...), "Key type must be one of the specified types");
        return index.count(KeyVariant(key)) > 0;
    }

    template<typename K>
    void remove(const K& key) {
        static_assert((std::is_same_v<K, KeyTypes> || ...), "Key type must be one of the specified types");
        KeyVariant k = key;
        cache.erase(k);
        index.erase(k);
        writeAll();
    }

    size_t size() const {
        return index.size();
    }
};

// Example usage
int main() {
    try {
        // Store with 2D array values
        BinaryKVStore<std::vector<std::vector<int>>, std::string, int> store("data.bin");

        // 2D array
        std::vector<std::vector<int>> array_2d = {
            {1, 2, 3, 4, 5, 6, 7, 8, 9},
            {10, 11, 12, 13, 14, 15, 16, 17, 18},
            {19, 20, 21, 22, 23, 24, 25, 26, 27},
            {28, 29, 30, 31, 32, 33, 34, 35, 36},
            {37, 38, 39, 40, 41, 42, 43, 44, 45},
            {46, 47, 48, 49, 50, 51, 52, 53, 54}
        };
        
        store.put(std::string("matrix"), array_2d);

        // Large 3D array
        std::vector<std::vector<std::vector<int>>> array_3d(10, 
            std::vector<std::vector<int>>(10, 
            std::vector<int>(10, 42)));
        store.put(1, array_3d);

        // Retrieve and verify
        auto val_2d = store.get(std::string("matrix"));
        if (val_2d) {
            std::cout << "2D array dimensions: " 
                      << val_2d->size() << "x" << (*val_2d)[0].size() << "\n";
            std::cout << "Element [1][1]: " << (*val_2d)[1][1] << "\n";
        }

        auto val_3d = store.get(1);
        if (val_3d) {
            std::cout << "3D array dimensions: " 
                      << val_3d->size() << "x" 
                      << (*val_3d)[0].size() << "x" 
                      << (*val_3d)[0][0].size() << "\n";
            std::cout << "Element [0][0][0]: " << (*val_3d)[0][0][0] << "\n";
        }

        std::cout << "Size: " << store.size() << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}