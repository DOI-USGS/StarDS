#include "stards.h"

#include <string>
#include <vector>
#include <utility>
#include <limits>
#include <map>
#include <memory>
#include <algorithm>
#include <type_traits>

#include <emscripten/bind.h>
#include <emscripten/emscripten.h>
#include <emscripten/em_js.h>
#include <emscripten/val.h>

using namespace emscripten;
using star::StarDataset;
using star::DataType;
using star::MetadataValue;
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

// The single place the DataType -> (C++ type, JS TypedArray ctor) mapping lives.
// Resolves `dt` to its element type and invokes fn(T{}, ctor); callers recover the
// type via decltype(tag). This lets get()/getSlice()/metaGet() share one table
// instead of each repeating a 10-case switch that must be kept in lockstep.
// `ctx` names the caller for the error message on an unsupported dtype.
template <typename F>
val dispatch_numeric(DataType dt, const char* ctx, F&& fn) {
    switch (dt) {
        case DataType::INT8:    return fn(int8_t{},   "Int8Array");
        case DataType::INT16:   return fn(int16_t{},  "Int16Array");
        case DataType::INT32:   return fn(int32_t{},  "Int32Array");
        case DataType::UINT8:   return fn(uint8_t{},  "Uint8Array");
        case DataType::UINT16:  return fn(uint16_t{}, "Uint16Array");
        case DataType::UINT32:  return fn(uint32_t{}, "Uint32Array");
        case DataType::FLOAT32: return fn(float{},    "Float32Array");
        case DataType::FLOAT64: return fn(double{},   "Float64Array");
        // 64-bit ints exceed JS Number safe range; expose via BigInt arrays.
        case DataType::INT64:   return fn(int64_t{},  "BigInt64Array");
        case DataType::UINT64:  return fn(uint64_t{}, "BigUint64Array");
        default:
            throw std::runtime_error(std::string(ctx) + "(): unsupported dtype");
    }
}

// Reverse of datatype_to_string: the dtype name the JS side passes to metaPut()
// (matches what dtype()/metaDtype() report). Throws on an unknown name.
DataType datatype_from_string(const std::string& s) {
    if (s == "int8")    return DataType::INT8;
    if (s == "int16")   return DataType::INT16;
    if (s == "int32")   return DataType::INT32;
    if (s == "int64")   return DataType::INT64;
    if (s == "uint8")   return DataType::UINT8;
    if (s == "uint16")  return DataType::UINT16;
    if (s == "uint32")  return DataType::UINT32;
    if (s == "uint64")  return DataType::UINT64;
    if (s == "float32") return DataType::FLOAT32;
    if (s == "float64") return DataType::FLOAT64;
    if (s == "string")  return DataType::STRING;
    throw std::runtime_error("unknown dtype: " + s);
}

// True for a JS Array or TypedArray (anything with a numeric `.length`), false for
// a bare number/string/null/undefined. Lets the put path accept a scalar or a 1-D
// sequence from the same argument.
bool is_array_like(const val& v) {
    if (v.isString() || v.isNull() || v.isUndefined()) return false;
    return v["length"].isNumber();
}

// Read one JS value as C++ T. 64-bit ints arrive as BigInt (needs -sWASM_BIGINT at
// link time); every other type round-trips through double, exact for all values a
// JS Number can represent.
template <typename T>
T js_to_scalar(const val& v) {
    if constexpr (std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
        return v.as<T>();
    } else {
        return static_cast<T>(v.as<double>());
    }
}

// Build an NDArray<T> from a JS scalar (-> empty shape, a scalar entry) or an
// array-like (-> 1-D). The write counterpart of to_typed_array().
template <typename T>
NDArray<T> js_to_ndarray(const val& v) {
    if (is_array_like(v)) {
        const unsigned n = v["length"].as<unsigned>();
        std::vector<T> data;
        data.reserve(n);
        for (unsigned i = 0; i < n; ++i) data.push_back(js_to_scalar<T>(v[i]));
        return NDArray<T>(std::move(data), std::vector<size_t>{n});
    }
    return NDArray<T>(std::vector<T>{js_to_scalar<T>(v)}, std::vector<size_t>{});
}

// String counterpart: a JS string -> scalar entry, a JS array of strings -> 1-D.
NDArray<std::string> js_to_string_ndarray(const val& v) {
    if (is_array_like(v)) {
        const unsigned n = v["length"].as<unsigned>();
        std::vector<std::string> data;
        data.reserve(n);
        for (unsigned i = 0; i < n; ++i) data.push_back(v[i].as<std::string>());
        return NDArray<std::string>(std::move(data), std::vector<size_t>{n});
    }
    return NDArray<std::string>(std::vector<std::string>{v.as<std::string>()},
                                std::vector<size_t>{});
}

// Convert a decoded metadata value to its natural JS type: a scalar string/number
// for scalars, a typed array (numeric) or Array<string> for arrays. 64-bit ints
// stay BigInt arrays even when scalar (a bare Number would silently lose
// precision). Shared by metaGet() and metaGetAll().
val meta_value_to_js(const MetadataValue& mv) {
    if (mv.dtype == DataType::STRING) {
        NDArray<std::string> a = mv.as<std::string>();
        if (mv.is_scalar()) return a.size() ? val(a.flat(0)) : val(std::string());
        val out = val::array();
        for (size_t i = 0; i < a.size(); ++i) out.set(i, val(a.flat(i)));
        return out;
    }
    const bool scalar = mv.is_scalar();
    return dispatch_numeric(mv.dtype, "metaGet", [&](auto tag, const char* ctor) {
        using T = decltype(tag);
        NDArray<T> a = mv.as<T>();
        if constexpr (!std::is_same_v<T, int64_t> && !std::is_same_v<T, uint64_t>) {
            if (scalar) return val(a.flat(0));
        }
        return to_typed_array(a, ctor);
    });
}

// A thin JS-facing handle around a StarDataset shared_ptr.
class JsDataset {
public:
    // Opens read-only. Under WASM a URL routes through the fetch() backend; a
    // bare path is a (virtual) local file. Throws on failure -> JS exception.
    explicit JsDataset(const std::string& path)
        : m_ds(StarDataset::open(path, "r")) {}

    // Opens with an explicit mode ("r", "w"/"rw"/"a"). Writable modes are needed
    // for the metadata writers (metaPut/metaRemove/metaClear) and only make sense
    // on a local/virtual path — a remote URL is read-only over fetch().
    JsDataset(const std::string& path, const std::string& mode)
        : m_ds(StarDataset::open(path, mode)) {}

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
        return dispatch_numeric(m_ds->dtype_of(key), "get", [&](auto tag, const char* ctor) {
            return to_typed_array(m_ds->get<decltype(tag)>(key), ctor);
        });
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

    // Read a metadata-block entry `key` and return it as its natural JS type: a
    // number/string for scalars, a typed array for arrays, or null if the key is
    // absent (meta.get() yields nullptr for a miss, so we check before deref'ing).
    //
    // This is the general form of metaString(): it dispatches on dtype instead of
    // assuming string, so numeric attributes come back as numbers/typed arrays.
    val meta_get(const std::string& key) const {
        std::shared_ptr<MetadataValue> mv = m_ds->meta.get(key);
        if (!mv) return val::null();
        return meta_value_to_js(*mv);
    }

    // Names of all metadata-block entries (Array<string>). Cheap: reads the
    // registries, decodes no values.
    val meta_keys() const {
        std::vector<std::string> k = m_ds->get_metadata_keys();
        val out = val::array();
        for (size_t i = 0; i < k.size(); ++i) out.set(i, val(k[i]));
        return out;
    }

    bool meta_has(const std::string& key) const { return m_ds->meta.contains(key); }

    // dtype name of a metadata entry ("int32", "float64", "string", ...), or "" if
    // the key is absent.
    std::string meta_dtype(const std::string& key) const {
        std::shared_ptr<MetadataValue> mv = m_ds->meta.get(key);
        return mv ? mv->type_name() : std::string();
    }

    // Shape of a metadata entry as Array<number> (empty for a scalar, [] if absent).
    val meta_shape(const std::string& key) const {
        val out = val::array();
        std::shared_ptr<MetadataValue> mv = m_ds->meta.get(key);
        if (mv) {
            for (size_t i = 0; i < mv->shape.size(); ++i)
                out.set(i, val(static_cast<double>(mv->shape[i])));
        }
        return out;
    }

    // All metadata as a plain JS object { key: naturalValue }.
    val meta_get_all() const {
        std::map<std::string, MetadataValue> all = m_ds->meta.get_all();
        val obj = val::object();
        for (const auto& [k, mv] : all) obj.set(k, meta_value_to_js(mv));
        return obj;
    }

    // Write a metadata entry. `dtype` picks the on-disk element type (see
    // datatype_from_string); `value` may be a scalar (-> scalar entry) or an
    // array-like (-> 1-D). Requires the dataset opened writable, else meta.put
    // throws. Overwrites any existing entry for `key`.
    void meta_put(const std::string& key, val value, const std::string& dtype) {
        if (dtype == "string") {
            m_ds->meta.put(key, js_to_string_ndarray(value));
            return;
        }
        dispatch_numeric(datatype_from_string(dtype), "metaPut", [&](auto tag, const char*) {
            m_ds->meta.put(key, js_to_ndarray<decltype(tag)>(value));
            return val::undefined();
        });
    }

    void meta_remove(const std::string& key) { m_ds->meta.remove(key); }
    void meta_clear() { m_ds->meta.clear(); }

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
        return dispatch_numeric(m_ds->dtype_of(key), "getSlice", [&](auto tag, const char* ctor) {
            return to_typed_array(m_ds->get_slice<decltype(tag)>(key, s), ctor);
        });
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
        .constructor<std::string, std::string>()
        .function("keys", &JsDataset::keys)
        .function("dtype", &JsDataset::dtype)
        .function("shape", &JsDataset::shape)
        .function("get", &JsDataset::get)
        .function("metaString", &JsDataset::meta_string)
        .function("metaGet", &JsDataset::meta_get)
        .function("metaKeys", &JsDataset::meta_keys)
        .function("metaHas", &JsDataset::meta_has)
        .function("metaDtype", &JsDataset::meta_dtype)
        .function("metaShape", &JsDataset::meta_shape)
        .function("metaGetAll", &JsDataset::meta_get_all)
        .function("metaPut", &JsDataset::meta_put)
        .function("metaRemove", &JsDataset::meta_remove)
        .function("metaClear", &JsDataset::meta_clear)
        .function("isSliceable", &JsDataset::is_sliceable)
        .function("getSlice", &JsDataset::get_slice)
        .function("getSliceXYZ", &JsDataset::get_slice_xyz_f32)
        .function("networkRequests", &JsDataset::network_requests);
}
