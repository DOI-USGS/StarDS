// =============================================================================
// Generic type-dispatching helpers (C++ side of the Python bindings).
//
// SWIG exposes every C++ template instantiation under a distinct name
// (put_array_int8, get_float64, ...). Rather than re-implement the 11-way type
// switch in Python for every operation, these helpers do the switch ONCE, in
// C++, where the element type is actually known:
//
//   * from a NumPy array  -> PyArray_TYPE(...) selects the C++ element type
//   * to a NumPy array    -> the stored DataType selects the C++ element type
//
// The Python layer then calls a single function (star_put / star_get / ...) with
// no per-type branching. Numeric types round-trip through the zero-copy buffer
// helpers in numpy_buffer.i; strings (NumPy object arrays) are handled
// element-by-element since they can't use the buffer protocol.
// =============================================================================

%{
#include <string>
#include <vector>

// ---- NumPy object-array (string) helpers -----------------------------------

// Build a star::NDArray<std::string> from a NumPy object array (or 0-d object).
static star::NDArray<std::string> star_string_ndarray_from_numpy(PyObject* obj) {
    ensure_numpy_initialized();
    if (!PyArray_Check(obj)) {
        throw std::runtime_error("Input is not a NumPy array");
    }
    PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(obj);

    int ndim = PyArray_NDIM(arr);
    npy_intp* dims = PyArray_DIMS(arr);
    std::vector<size_t> shape(dims, dims + ndim);

    size_t count = shape.empty()
        ? 1
        : std::accumulate(shape.begin(), shape.end(), size_t(1), std::multiplies<size_t>());

    star::NDArray<std::string> result(shape);
    std::vector<std::string>& data = result.data();
    data.resize(count);

    // Iterate the object array in C order via a flat iterator (works regardless
    // of contiguity/strides, and reads the original array's elements directly).
    PyObject* it = PyArray_IterNew(reinterpret_cast<PyObject*>(arr));
    if (!it) throw std::runtime_error("Failed to iterate string array");
    for (size_t i = 0; i < count; ++i) {
        PyObject* item = *reinterpret_cast<PyObject**>(PyArray_ITER_DATA(it));
        if (item && PyUnicode_Check(item)) {
            const char* s = PyUnicode_AsUTF8(item);
            data[i] = s ? s : "";
        } else if (item) {
            PyObject* str = PyObject_Str(item);
            const char* s = str ? PyUnicode_AsUTF8(str) : "";
            data[i] = s ? s : "";
            Py_XDECREF(str);
        }
        PyArray_ITER_NEXT(it);
    }
    Py_DECREF(it);
    return result;
}

// Convert a star::NDArray<std::string> to a NumPy object array.
static PyObject* star_string_ndarray_to_numpy(const star::NDArray<std::string>& arr) {
    ensure_numpy_initialized();
    size_t ndim = arr.dimension();
    npy_intp dims[NPY_MAXDIMS];
    for (size_t i = 0; i < ndim; ++i) dims[i] = (npy_intp)arr.shape(i);

    PyObject* result = PyArray_SimpleNew((int)ndim, dims, NPY_OBJECT);
    if (!result) throw std::runtime_error("Failed to create object array");

    PyArrayObject* out = reinterpret_cast<PyArrayObject*>(result);
    const std::vector<std::string>& data = arr.data();
    // Write elements directly into `out` in C order via a flat iterator (no copy).
    PyObject* it = PyArray_IterNew(result);
    if (!it) throw std::runtime_error("Failed to iterate output object array");
    for (size_t i = 0; i < data.size(); ++i) {
        // Non-UTF-8 bytes make PyUnicode_FromStringAndSize return NULL (and set an
        // exception), which would leave a NULL slot in the object array. Decode with
        // "surrogateescape" so arbitrary byte content always yields a valid str.
        PyObject* s = PyUnicode_DecodeUTF8(data[i].data(), (Py_ssize_t)data[i].size(),
                                           "surrogateescape");
        // The slot holds a borrowed PyObject*; store our new reference directly.
        PyObject** slot = reinterpret_cast<PyObject**>(PyArray_ITER_DATA(it));
        *slot = s;  // steals our reference (freshly created object array is all-NULL)
        PyArray_ITER_NEXT(it);
    }
    Py_DECREF(it);
    (void)out;
    return result;
}

// ---- put: NumPy array -> target.put<T>(key, ...) ---------------------------
// `Target` is StarDataset, LayerView, MetadataAccessor or LayerMetadataAccessor;
// they all expose a put(key, NDArray<T>) (or put<NDArray<T>> for the accessors),
// which template argument deduction resolves from the NDArray we pass.
template<typename Target>
static void star_put_dispatch(Target& target, const std::string& key, PyObject* obj) {
    ensure_numpy_initialized();  // must run before any PyArray_* macro (inits PyArray_API)
    if (!PyArray_Check(obj)) {
        throw std::runtime_error("Value is not a NumPy array");
    }
    int t = PyArray_TYPE(reinterpret_cast<PyArrayObject*>(obj));
    switch (t) {
        case NPY_INT8:    target.put(key, ndarray_from_numpy_buffer<int8_t>(obj)); break;
        case NPY_INT16:   target.put(key, ndarray_from_numpy_buffer<int16_t>(obj)); break;
        case NPY_INT32:   target.put(key, ndarray_from_numpy_buffer<int32_t>(obj)); break;
        case NPY_INT64:   target.put(key, ndarray_from_numpy_buffer<int64_t>(obj)); break;
        case NPY_UINT8:   target.put(key, ndarray_from_numpy_buffer<uint8_t>(obj)); break;
        case NPY_UINT16:  target.put(key, ndarray_from_numpy_buffer<uint16_t>(obj)); break;
        case NPY_UINT32:  target.put(key, ndarray_from_numpy_buffer<uint32_t>(obj)); break;
        case NPY_UINT64:  target.put(key, ndarray_from_numpy_buffer<uint64_t>(obj)); break;
        case NPY_FLOAT32: target.put(key, ndarray_from_numpy_buffer<float>(obj)); break;
        case NPY_FLOAT64: target.put(key, ndarray_from_numpy_buffer<double>(obj)); break;
        case NPY_OBJECT:  target.put(key, star_string_ndarray_from_numpy(obj)); break;
        default:
            throw std::runtime_error("Unsupported NumPy dtype for put (typenum=" + std::to_string(t) + ")");
    }
}

// ---- get: target.get<T>(key) -> NumPy array, dispatched on `dt` ------------
template<typename Target>
static PyObject* star_get_dispatch(Target& target, const std::string& key, star::DataType dt) {
    switch (dt) {
        case star::DataType::INT8:    return ndarray_to_numpy_buffer(target.template get<int8_t>(key));
        case star::DataType::INT16:   return ndarray_to_numpy_buffer(target.template get<int16_t>(key));
        case star::DataType::INT32:   return ndarray_to_numpy_buffer(target.template get<int32_t>(key));
        case star::DataType::INT64:   return ndarray_to_numpy_buffer(target.template get<int64_t>(key));
        case star::DataType::UINT8:   return ndarray_to_numpy_buffer(target.template get<uint8_t>(key));
        case star::DataType::UINT16:  return ndarray_to_numpy_buffer(target.template get<uint16_t>(key));
        case star::DataType::UINT32:  return ndarray_to_numpy_buffer(target.template get<uint32_t>(key));
        case star::DataType::UINT64:  return ndarray_to_numpy_buffer(target.template get<uint64_t>(key));
        case star::DataType::FLOAT32: return ndarray_to_numpy_buffer(target.template get<float>(key));
        case star::DataType::FLOAT64: return ndarray_to_numpy_buffer(target.template get<double>(key));
        case star::DataType::STRING:  return star_string_ndarray_to_numpy(target.template get<std::string>(key));
        default:
            throw std::runtime_error("Unsupported DataType for get");
    }
}
%}

%inline %{
    // --- Array namespace (StarDataset) -------------------------------------
    void star_put(star::StarDataset& ds, const std::string& key, PyObject* numpy_array) {
        star_put_dispatch(ds, key, numpy_array);
    }
    PyObject* star_get(star::StarDataset& ds, const std::string& key) {
        return star_get_dispatch(ds, key, ds.dtype_of(key));
    }
    PyObject* star_get_slice(star::StarDataset& ds, const std::string& key,
                             const std::vector<star::Slice>& slices) {
        switch (ds.dtype_of(key)) {
            case star::DataType::INT8:    return ndarray_to_numpy_buffer(ds.get_slice<int8_t>(key, slices));
            case star::DataType::INT16:   return ndarray_to_numpy_buffer(ds.get_slice<int16_t>(key, slices));
            case star::DataType::INT32:   return ndarray_to_numpy_buffer(ds.get_slice<int32_t>(key, slices));
            case star::DataType::INT64:   return ndarray_to_numpy_buffer(ds.get_slice<int64_t>(key, slices));
            case star::DataType::UINT8:   return ndarray_to_numpy_buffer(ds.get_slice<uint8_t>(key, slices));
            case star::DataType::UINT16:  return ndarray_to_numpy_buffer(ds.get_slice<uint16_t>(key, slices));
            case star::DataType::UINT32:  return ndarray_to_numpy_buffer(ds.get_slice<uint32_t>(key, slices));
            case star::DataType::UINT64:  return ndarray_to_numpy_buffer(ds.get_slice<uint64_t>(key, slices));
            case star::DataType::FLOAT32: return ndarray_to_numpy_buffer(ds.get_slice<float>(key, slices));
            case star::DataType::FLOAT64: return ndarray_to_numpy_buffer(ds.get_slice<double>(key, slices));
            default:
                throw std::runtime_error("Unsupported dtype for slicing");
        }
    }

    // --- Metadata namespace (StarDataset::meta) ----------------------------
    void star_put_meta(star::MetadataAccessor& meta, const std::string& key, PyObject* numpy_array) {
        star_put_dispatch(meta, key, numpy_array);
    }
    // Convert a MetadataValue (returned by meta.get) to a NumPy array.
    PyObject* star_meta_to_numpy(const star::MetadataValue& mv) {
        switch (mv.dtype) {
            case star::DataType::INT8:    return ndarray_to_numpy_buffer(mv.as<int8_t>());
            case star::DataType::INT16:   return ndarray_to_numpy_buffer(mv.as<int16_t>());
            case star::DataType::INT32:   return ndarray_to_numpy_buffer(mv.as<int32_t>());
            case star::DataType::INT64:   return ndarray_to_numpy_buffer(mv.as<int64_t>());
            case star::DataType::UINT8:   return ndarray_to_numpy_buffer(mv.as<uint8_t>());
            case star::DataType::UINT16:  return ndarray_to_numpy_buffer(mv.as<uint16_t>());
            case star::DataType::UINT32:  return ndarray_to_numpy_buffer(mv.as<uint32_t>());
            case star::DataType::UINT64:  return ndarray_to_numpy_buffer(mv.as<uint64_t>());
            case star::DataType::FLOAT32: return ndarray_to_numpy_buffer(mv.as<float>());
            case star::DataType::FLOAT64: return ndarray_to_numpy_buffer(mv.as<double>());
            case star::DataType::STRING:  return star_string_ndarray_to_numpy(mv.as<std::string>());
            default:
                throw std::runtime_error("Unsupported DataType in metadata value");
        }
    }

    // --- Layer namespace (LayerView + its metadata) ------------------------
    void star_layer_put(star::LayerView& layer, const std::string& key, PyObject* numpy_array) {
        star_put_dispatch(layer, key, numpy_array);
    }
    void star_layer_put_meta(star::LayerMetadataAccessor& meta, const std::string& key, PyObject* numpy_array) {
        star_put_dispatch(meta, key, numpy_array);
    }
    // --- In-memory byte APIs (open_bytes / write_bytes) ----------------------
    // Build a dataset from a Python bytes-like object holding a whole .stards
    // image. Returns a shared_ptr<StarDataset> (SWIG-wrapped like open()).
    std::shared_ptr<star::StarDataset> star_open_bytes(PyObject* data,
                                                       const star::OpenOptions& opts) {
        char* buf = nullptr;
        Py_ssize_t len = 0;
        if (PyBytes_Check(data)) {
            if (PyBytes_AsStringAndSize(data, &buf, &len) != 0) {
                throw std::runtime_error("open_bytes: could not read bytes object");
            }
        } else {
            // Accept any buffer-protocol object (bytearray, memoryview, numpy uint8, ...).
            Py_buffer view;
            if (PyObject_GetBuffer(data, &view, PyBUF_SIMPLE) != 0) {
                throw std::runtime_error("open_bytes: expected a bytes-like object");
            }
            std::vector<char> bytes(static_cast<const char*>(view.buf),
                                    static_cast<const char*>(view.buf) + view.len);
            PyBuffer_Release(&view);
            return star::StarDataset::open_bytes(std::move(bytes), opts);
        }
        return star::StarDataset::open_bytes(std::vector<char>(buf, buf + len), opts);
    }

    // Serialize a dataset to a Python bytes object (a complete .stards image).
    PyObject* star_write_bytes(star::StarDataset& ds) {
        std::vector<char> image = ds.write_bytes();
        return PyBytes_FromStringAndSize(image.data(),
                                         static_cast<Py_ssize_t>(image.size()));
    }

    // LayerView has no dtype_of(); resolve type by probing get<T> (inheritance-aware).
    PyObject* star_layer_get(star::LayerView& layer, const std::string& key) {
        // Determine dtype from the base dataset where the array physically lives.
        // The layer stores under a prefixed key; if absent, it inherits the base key.
        star::DataType dt;
        std::shared_ptr<star::StarDataset> base = layer.base();
        std::string prefixed = (layer.name() == "__base__")
            ? key : ("__layer_" + layer.name() + "__:" + key);
        try {
            dt = base->dtype_of(prefixed);
        } catch (const std::runtime_error&) {
            dt = base->dtype_of(key);  // inherited from base
        }
        return star_get_dispatch(layer, key, dt);
    }
%}
