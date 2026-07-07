// Zero-copy NumPy buffer protocol for efficient NDArray ↔ NumPy conversion
// Eliminates 2-3 intermediate copies by using direct buffer access

%{
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>
#include <algorithm>
#include <numeric>

// Initialize NumPy C API (must be called once)
static bool numpy_initialized = false;

static int ensure_numpy_initialized() {
    if (!numpy_initialized) {
        import_array();  // This macro may return a value
        numpy_initialized = true;
    }
    return 0;
}

// Map C++ types to NumPy type numbers
template<typename T> int get_numpy_typenum();

template<> int get_numpy_typenum<int8_t>() { return NPY_INT8; }
template<> int get_numpy_typenum<int16_t>() { return NPY_INT16; }
template<> int get_numpy_typenum<int32_t>() { return NPY_INT32; }
template<> int get_numpy_typenum<int64_t>() { return NPY_INT64; }
template<> int get_numpy_typenum<uint8_t>() { return NPY_UINT8; }
template<> int get_numpy_typenum<uint16_t>() { return NPY_UINT16; }
template<> int get_numpy_typenum<uint32_t>() { return NPY_UINT32; }
template<> int get_numpy_typenum<uint64_t>() { return NPY_UINT64; }
template<> int get_numpy_typenum<float>() { return NPY_FLOAT32; }
template<> int get_numpy_typenum<double>() { return NPY_FLOAT64; }

// Convert NumPy array to NDArray using direct buffer access
template<typename T>
star::NDArray<T> ndarray_from_numpy_buffer(PyObject* numpy_array) {
    ensure_numpy_initialized();

    if (!PyArray_Check(numpy_array)) {
        throw std::runtime_error("Input is not a NumPy array");
    }

    PyArrayObject* arr = reinterpret_cast<PyArrayObject*>(numpy_array);

    // Verify type matches
    int expected_typenum = get_numpy_typenum<T>();
    if (PyArray_TYPE(arr) != expected_typenum) {
        throw std::runtime_error("NumPy array type mismatch");
    }

    // Get dimensions
    int ndim = PyArray_NDIM(arr);
    npy_intp* dims = PyArray_DIMS(arr);

    std::vector<size_t> shape(dims, dims + ndim);

    // Calculate total size
    size_t size = shape.empty() ? 1 : std::accumulate(shape.begin(), shape.end(), size_t(1), std::multiplies<size_t>());

    // Ensure contiguous (may require copy if not already contiguous)
    PyArrayObject* contiguous_arr = arr;
    bool needs_cleanup = false;
    if (!PyArray_ISCONTIGUOUS(arr)) {
        contiguous_arr = reinterpret_cast<PyArrayObject*>(PyArray_ContiguousFromAny(
            reinterpret_cast<PyObject*>(arr), PyArray_TYPE(arr), 0, 0));
        needs_cleanup = true;
    }

    // Get direct pointer to data
    T* data_ptr = reinterpret_cast<T*>(PyArray_DATA(contiguous_arr));

    // Single memcpy into vector
    std::vector<T> data(data_ptr, data_ptr + size);

    // Clean up temporary contiguous array if created
    if (needs_cleanup) {
        Py_DECREF(contiguous_arr);
    }

    return star::NDArray<T>(std::move(data), shape);
}

// Convert NDArray to NumPy array using direct buffer access
template<typename T>
PyObject* ndarray_to_numpy_buffer(const star::NDArray<T>& arr) {
    ensure_numpy_initialized();

    // Get shape
    size_t ndim = arr.dimension();
    npy_intp dims[NPY_MAXDIMS];
    for (size_t i = 0; i < ndim; i++) {
        dims[i] = arr.shape(i);
    }

    // Create NumPy array
    int typenum = get_numpy_typenum<T>();
    PyObject* result = PyArray_SimpleNew(ndim, dims, typenum);

    if (result == NULL) {
        throw std::runtime_error("Failed to create NumPy array");
    }

    // Single memcpy instead of element-by-element loop
    void* dst = PyArray_DATA((PyArrayObject*)result);
    const T* src = arr.data_ptr();  // Get pointer, not vector reference
    size_t num_bytes = arr.size() * sizeof(T);
    std::memcpy(dst, src, num_bytes);

    return result;
}

%}

// Expose conversion functions for all numeric types
%inline %{
    star::NDArray<int8_t> ndarray_from_numpy_int8(PyObject* arr) {
        return ndarray_from_numpy_buffer<int8_t>(arr);
    }
    star::NDArray<int16_t> ndarray_from_numpy_int16(PyObject* arr) {
        return ndarray_from_numpy_buffer<int16_t>(arr);
    }
    star::NDArray<int32_t> ndarray_from_numpy_int32(PyObject* arr) {
        return ndarray_from_numpy_buffer<int32_t>(arr);
    }
    star::NDArray<int64_t> ndarray_from_numpy_int64(PyObject* arr) {
        return ndarray_from_numpy_buffer<int64_t>(arr);
    }
    star::NDArray<uint8_t> ndarray_from_numpy_uint8(PyObject* arr) {
        return ndarray_from_numpy_buffer<uint8_t>(arr);
    }
    star::NDArray<uint16_t> ndarray_from_numpy_uint16(PyObject* arr) {
        return ndarray_from_numpy_buffer<uint16_t>(arr);
    }
    star::NDArray<uint32_t> ndarray_from_numpy_uint32(PyObject* arr) {
        return ndarray_from_numpy_buffer<uint32_t>(arr);
    }
    star::NDArray<uint64_t> ndarray_from_numpy_uint64(PyObject* arr) {
        return ndarray_from_numpy_buffer<uint64_t>(arr);
    }
    star::NDArray<float> ndarray_from_numpy_float32(PyObject* arr) {
        return ndarray_from_numpy_buffer<float>(arr);
    }
    star::NDArray<double> ndarray_from_numpy_float64(PyObject* arr) {
        return ndarray_from_numpy_buffer<double>(arr);
    }

    PyObject* ndarray_to_numpy_int8(const star::NDArray<int8_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_int16(const star::NDArray<int16_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_int32(const star::NDArray<int32_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_int64(const star::NDArray<int64_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_uint8(const star::NDArray<uint8_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_uint16(const star::NDArray<uint16_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_uint32(const star::NDArray<uint32_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_uint64(const star::NDArray<uint64_t>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_float32(const star::NDArray<float>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
    PyObject* ndarray_to_numpy_float64(const star::NDArray<double>& arr) {
        return ndarray_to_numpy_buffer(arr);
    }
%}
