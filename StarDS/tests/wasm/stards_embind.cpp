// Minimal embind wrapper exposing StarDS to JavaScript/WebAssembly.
//
// This is a proof-of-concept surface, not the full API — enough to open a
// (local or remote) .stards dataset from JS, list its keys, and pull an array
// back as a typed value. Everything the C++ header already does (ranged reads
// over fetch() under ASYNCIFY, decompression, etc.) works unchanged behind it.
//
// Build: see run_embind_example.sh (compiles with --bind and links the module
// as a factory so the async runtime is ready before use).
#include "stards.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>

#include <memory>
#include <string>
#include <vector>

using namespace emscripten;
using star::StarDataset;
using star::DataType;
using star::NDArray;

namespace {

// Copy an NDArray<T> into a JS typed array (Int32Array, Float64Array, ...).
// typed_memory_view gives JS a zero-copy view over the Wasm heap; we then hand
// that to `new TypedArray(view)`, which copies the bytes in one bulk operation
// (millions of per-element val.set() calls would be pathologically slow). The
// returned typed array owns its own buffer, so it stays valid after this returns.
template <typename T>
val to_typed_array(const NDArray<T>& arr, const char* js_ctor) {
    const std::vector<T>& d = arr.data();
    val view(typed_memory_view(d.size(), d.data()));
    return val::global(js_ctor).new_(view);
}

// A thin JS-facing handle around a StarDataset shared_ptr.
class JsDataset {
public:
    // Opens read-only. Under WASM a URL routes through the fetch() backend; a
    // bare path is a (virtual) local file. Throws on failure -> JS exception.
    explicit JsDataset(const std::string& path)
        : m_ds(StarDataset::open(path, "r")) {}

    // Array keys present in the dataset (returned to JS as an Array<string>).
    val keys() const {
        std::vector<std::string> k = m_ds->get_all_keys();
        val arr = val::array();
        for (size_t i = 0; i < k.size(); ++i) arr.set(i, val(k[i]));
        return arr;
    }

    std::string dtype(const std::string& key) const {
        return std::string(star::datatype_to_string(m_ds->dtype_of(key)));
    }

    val shape(const std::string& key) const {
        // Metadata-only: shape_of() reads dims from the index, no data download.
        std::vector<size_t> s = m_ds->shape_of(key);
        val out = val::array();
        for (size_t i = 0; i < s.size(); ++i) out.set(i, val(static_cast<double>(s[i])));
        return out;
    }

    // Return the whole array for `key` as a JS typed array of the right kind.
    val get(const std::string& key) const {
        switch (m_ds->dtype_of(key)) {
            case DataType::INT8:    return to_typed_array(m_ds->get<int8_t>(key),   "Int8Array");
            case DataType::INT16:   return to_typed_array(m_ds->get<int16_t>(key),  "Int16Array");
            case DataType::INT32:   return to_typed_array(m_ds->get<int32_t>(key),  "Int32Array");
            case DataType::UINT8:   return to_typed_array(m_ds->get<uint8_t>(key),  "Uint8Array");
            case DataType::UINT16:  return to_typed_array(m_ds->get<uint16_t>(key), "Uint16Array");
            case DataType::UINT32:  return to_typed_array(m_ds->get<uint32_t>(key), "Uint32Array");
            case DataType::FLOAT32: return to_typed_array(m_ds->get<float>(key),    "Float32Array");
            case DataType::FLOAT64: return to_typed_array(m_ds->get<double>(key),   "Float64Array");
            // 64-bit ints exceed JS Number safe range; expose via BigInt arrays.
            case DataType::INT64:   return to_typed_array(m_ds->get<int64_t>(key),  "BigInt64Array");
            case DataType::UINT64:  return to_typed_array(m_ds->get<uint64_t>(key), "BigUint64Array");
            default:
                throw std::runtime_error("get(): unsupported dtype for key " + key);
        }
    }

    // Requests issued so far (for demos/tests) — proves reads hit the network.
    double network_requests() const {
        return static_cast<double>(star::g_network_request_count.load());
    }

private:
    std::shared_ptr<StarDataset> m_ds;
};

}  // namespace

EMSCRIPTEN_BINDINGS(stards) {
    class_<JsDataset>("Dataset")
        .constructor<std::string>()
        .function("keys", &JsDataset::keys)
        .function("dtype", &JsDataset::dtype)
        .function("shape", &JsDataset::shape)
        .function("get", &JsDataset::get)
        .function("networkRequests", &JsDataset::network_requests);
}
