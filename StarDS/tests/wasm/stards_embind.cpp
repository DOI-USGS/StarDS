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

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

using namespace emscripten;
using star::StarDataset;
using star::DataType;
using star::NDArray;
using star::Slice;

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

    // Read a string-valued entry (header/attribute style) as a plain JS string,
    // from the metadata block or from a 1-element string column. Returns "" if the
    // key is absent, so callers can fall back to a default without a try/catch.
    std::string meta_string(const std::string& key) const {
        try {
            if (m_ds->meta.contains(key)) {
                NDArray<std::string> arr = m_ds->meta.get(key)->as<std::string>();
                return arr.size() ? arr.flat(0) : std::string();
            }
            if (m_ds->contains(key)) {
                NDArray<std::string> arr = m_ds->get<std::string>(key);
                return arr.size() ? arr.flat(0) : std::string();
            }
        } catch (const std::exception&) {
            // Wrong kind of entry for this key — treat as absent.
        }
        return std::string();
    }

    // True if `key` is stored as blocks and can be windowed with getSlice().
    // Metadata-block arrays are whole-array only (see StarDataset::is_sliceable).
    bool is_sliceable(const std::string& key) const { return m_ds->is_sliceable(key); }

    // Return elements [start, start+count) of the 1-D array `key` as a typed array.
    //
    // The point of this over get(): a slice reads only the compressed blocks that
    // cover the window, so a caller streaming a large column pays for the bytes it
    // actually wants instead of downloading the whole array up front. Streaming
    // consumers (the docs-site hero, for one) live on this.
    val get_slice(const std::string& key, double start, double count) const {
        const std::vector<Slice> s = {slice_1d(key, start, count)};
        switch (m_ds->dtype_of(key)) {
            case DataType::INT8:    return to_typed_array(m_ds->get_slice<int8_t>(key, s),   "Int8Array");
            case DataType::INT16:   return to_typed_array(m_ds->get_slice<int16_t>(key, s),  "Int16Array");
            case DataType::INT32:   return to_typed_array(m_ds->get_slice<int32_t>(key, s),  "Int32Array");
            case DataType::UINT8:   return to_typed_array(m_ds->get_slice<uint8_t>(key, s),  "Uint8Array");
            case DataType::UINT16:  return to_typed_array(m_ds->get_slice<uint16_t>(key, s), "Uint16Array");
            case DataType::UINT32:  return to_typed_array(m_ds->get_slice<uint32_t>(key, s), "Uint32Array");
            case DataType::FLOAT32: return to_typed_array(m_ds->get_slice<float>(key, s),    "Float32Array");
            case DataType::FLOAT64: return to_typed_array(m_ds->get_slice<double>(key, s),   "Float64Array");
            case DataType::INT64:   return to_typed_array(m_ds->get_slice<int64_t>(key, s),  "BigInt64Array");
            case DataType::UINT64:  return to_typed_array(m_ds->get_slice<uint64_t>(key, s), "BigUint64Array");
            default:
                throw std::runtime_error("getSlice(): unsupported dtype for key " + key);
        }
    }

    // Read the same window from three 1-D arrays and return it interleaved as one
    // Float32Array [x0,y0,z0, x1,y1,z1, ...].
    //
    // This is the shape GPU vertex buffers want, and doing the interleave here saves
    // the caller three separate heap->JS copies plus a JS-side transpose per batch —
    // which matters when a batch is hundreds of thousands of points. The arrays are
    // typically float64 on disk (full precision positions); float32 is what the
    // renderer uploads anyway.
    val get_slice_xyz_f32(const std::string& kx, const std::string& ky,
                          const std::string& kz, double start, double count) const {
        // Size from the CLAMPED window, so a request that runs past the end yields a
        // short array rather than one padded with zeros the caller would render.
        const size_t n = slice_1d(kx, start, count).length();
        std::vector<float> out(n * 3);
        const std::string* keys[3] = {&kx, &ky, &kz};
        for (int axis = 0; axis < 3; ++axis) {
            const std::string& key = *keys[axis];
            const std::vector<Slice> s = {slice_1d(key, start, count)};
            switch (m_ds->dtype_of(key)) {
                case DataType::FLOAT64: scatter(out, axis, m_ds->get_slice<double>(key, s)); break;
                case DataType::FLOAT32: scatter(out, axis, m_ds->get_slice<float>(key, s));  break;
                case DataType::INT32:   scatter(out, axis, m_ds->get_slice<int32_t>(key, s)); break;
                case DataType::INT16:   scatter(out, axis, m_ds->get_slice<int16_t>(key, s)); break;
                default:
                    throw std::runtime_error("getSliceXYZ(): unsupported dtype for key " + key);
            }
        }
        val view(typed_memory_view(out.size(), out.data()));
        return val::global("Float32Array").new_(view);
    }

    // Requests issued so far (for demos/tests) — proves reads hit the network.
    double network_requests() const {
        return static_cast<double>(star::g_network_request_count.load());
    }

private:
    // Clamp a [start, count) request to the array's actual length, so a caller that
    // asks for one batch past the end gets a short (or empty) result rather than an
    // out-of-range throw.
    Slice slice_1d(const std::string& key, double start, double count) const {
        const std::vector<size_t> shape = m_ds->shape_of(key);
        if (shape.size() != 1) {
            throw std::runtime_error("slice: key '" + key + "' is not 1-D");
        }
        const size_t n = shape[0];
        const size_t begin = start <= 0 ? 0 : std::min(static_cast<size_t>(start), n);
        const size_t want = count <= 0 ? 0 : static_cast<size_t>(count);
        return Slice{begin, std::min(begin + want, n), 1};
    }

    // Write arr[i] into out[i * 3 + axis], converting to float.
    template <typename T>
    static void scatter(std::vector<float>& out, int axis, const NDArray<T>& arr) {
        const std::vector<T>& d = arr.data();
        const size_t n = std::min(d.size(), out.size() / 3);
        for (size_t i = 0; i < n; ++i) out[i * 3 + axis] = static_cast<float>(d[i]);
    }

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
        .function("metaString", &JsDataset::meta_string)
        .function("isSliceable", &JsDataset::is_sliceable)
        .function("getSlice", &JsDataset::get_slice)
        .function("getSliceXYZ", &JsDataset::get_slice_xyz_f32)
        .function("networkRequests", &JsDataset::network_requests);
}
