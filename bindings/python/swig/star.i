%module(directors="1") pystards

%{
#define SWIG_FILE_WITH_INIT
#include "stards.h"
using namespace star;
%}

// Feature macros (ENABLE_CURL / ENABLE_ZLIB / ENABLE_LZ4 / ENABLE_S3) are always
// defined when building the bindings (the bindings CMake requires all four), and
// the C++ compiler sees them via the %{ %} block above. But directory-scoped
// add_definitions() also feed them to SWIG's PARSER, where they'd (a) be wrapped
// as broken valueless module constants and (b) pull the internal CURL/S3 stream
// classes (HttpStreamBuf, S3Stream, ...) into the wrapper half-formed, breaking
// `import pystards`. Those classes derive from std::streambuf/std::istream and are
// pure implementation detail — Python never touches them; the compiled extension
// provides the real HTTP/S3 behaviour. So undefine the macros for the PARSER
// ONLY (harmless to the already-compiled %{ %} block, which keeps them), which
// cleanly excludes those #ifdef'd internal blocks from wrapping, then re-expose
// the feature flags as genuine integer constants for introspection.
#undef ENABLE_CURL
#undef ENABLE_ZLIB
#undef ENABLE_LZ4
#undef ENABLE_S3
%constant int ENABLE_CURL = 1;
%constant int ENABLE_ZLIB = 1;
%constant int ENABLE_LZ4  = 1;
%constant int ENABLE_S3   = 1;

// Work around SWIG's inability to parse std::enable_if SFINAE
// Tell SWIG to see a simplified version
namespace std {
    template<bool B, typename T = void>
    struct enable_if {
        typedef T type;
    };

    template<typename T>
    struct is_arithmetic {
        static const bool value = true;
    };

    template<typename T, typename U>
    struct is_same {
        static const bool value = false;
    };
}

// Standard library support
%include <std_string.i>
%include <std_map.i>
%include <std_shared_ptr.i>
%include <std_unique_ptr.i>

// Include type conversion typemaps BEFORE std_vector.i and template instantiations
%include "exceptions.i"
%include "numpy_typemaps.i"
%include "numpy_buffer.i"

// Now include std_vector.i (after typemaps are defined)
%include <std_vector.i>

// Template instantiations for std::vector (after typemaps and std_vector.i)
%template(VectorSize) std::vector<size_t>;
%template(VectorString) std::vector<std::string>;
%template(VectorInt8) std::vector<int8_t>;
%template(VectorInt16) std::vector<int16_t>;
%template(VectorInt32) std::vector<int32_t>;
%template(VectorInt64) std::vector<int64_t>;
%template(VectorUInt8) std::vector<uint8_t>;
%template(VectorUInt16) std::vector<uint16_t>;
%template(VectorUInt32) std::vector<uint32_t>;
#ifdef STAR_SIZE_T_IS_UINT64
// size_t and uint64_t are the same C++ type on this platform (LP64): reuse the
// VectorSize instantiation instead of emitting a duplicate std::vector<uint64_t>
// (which would produce a redefined swig::traits<>). Expose the VectorUInt64 name
// as a Python alias so callers (e.g. pystards.ndarray) still find it.
%pythoncode %{
VectorUInt64 = VectorSize
%}
#else
%template(VectorUInt64) std::vector<uint64_t>;
#endif
%template(VectorFloat32) std::vector<float>;
%template(VectorFloat64) std::vector<double>;

// Ignore problematic methods BEFORE parsing header
// Rename 'print' which is a Python keyword
%rename(print_info) print;

// --- Silence benign SWIG generation warnings -----------------------------
// These are cosmetic (SWIG limitations / intentional wrapping choices), not
// bugs. Filtering them keeps a clean, warning-free binding build.

// W325: ExtractionPlan::ElementRange is a nested struct SWIG can't wrap. It's
// an internal extraction detail with no Python use; we also %ignore it below,
// but the warning fires at parse time so filter it too.
%warnfilter(325) star::ExtractionPlan::ElementRange;

// W401: SWIG doesn't wrap std::enable_shared_from_this (StarDataset's base).
// shared_ptr handling is provided explicitly via %shared_ptr below, so the base
// isn't needed on the Python side.
%warnfilter(401) star::StarDataset;

// W451: MAGIC_STRING is an internal `const char*` constant not needed in Python.
// Ignoring it avoids the "may leak memory" note (SWIG can't own the literal).
%ignore star::MAGIC_STRING;

// W509: StarDataset::put and NDArray's vector constructor each expose a move
// (&&) and a const-ref overload. SWIG can only surface one; the move overload
// wins (correct for zero-copy) and shadows the copy overload. Filter the
// "effectively ignored" notices for the affected templates.
%warnfilter(509) star::StarDataset::put;
%warnfilter(509) star::NDArray;

// Ignore internal HTTP/S3 streaming classes. These derive from std::streambuf /
// std::istream (which SWIG cannot wrap), are implementation details never used
// from Python, and wrapping them produces broken proxies (missing delete_*).
%ignore star::HttpStreamBuf;
%ignore star::HttpStream;
%ignore star::S3StreamBuf;
%ignore star::S3Stream;
%ignore star::S3Writer;
%ignore star::AWSV4Signer;
%ignore star::AWSConfigParser;
%ignore star::AWSTokenCache;

// Ignore internal serialization methods (not needed in Python)
%ignore star::StarDataset::serialize_layer_metadata_block;
%ignore star::StarDataset::load_layer_metadata;
%ignore star::StarDataset::parse_layer_metadata_block;

// Ignore nested structs (SWIG limitation)
%ignore ExtractionPlan::ElementRange;

// Ignore removed FileHeader fields (simplified versioning)
%ignore FileHeader::version_major;
%ignore FileHeader::version_minor;
%ignore FileHeader::version_patch;

// Ignore removed MetadataBlockConfig struct (no auto-routing)
%ignore MetadataBlockConfig;

// Ignore C++ iterators (we'll provide Python iteration later)
%ignore NDArray::begin;
%ignore NDArray::end;
%ignore NDArray::cbegin;
%ignore NDArray::cend;
%ignore NDArray::rbegin;
%ignore NDArray::rend;

// Ignore internal serialize methods with SFINAE
%ignore StarDataset::serialize_metadata_value;

// Ignore static factory methods with SFINAE (provide Python wrappers instead)
%ignore NDArray::zeros;
%ignore NDArray::ones;
%ignore NDArray::full;
%ignore NDArray::empty;
%ignore NDArray::arange;

// Ignore constructor (use static factory methods instead)
%ignore star::StarDataset::StarDataset(const std::string&, FileMode, const StarConfig*);

// Ignore methods with C++11 brace initialization in default args (SWIG can't parse)
%ignore NDArray::resize;

// Ignore non-copyable members (mutex, streams, etc.)
// Need to ignore both with and without namespace, and also ignore getters/setters
%ignore star::StarDataset::m_mutex;
%ignore star::StarDataset::m_thread_pool;
%ignore star::StarDataset::m_file;
%ignore StarDataset::m_mutex;
%ignore StarDataset::m_thread_pool;
%ignore StarDataset::m_file;
%ignore m_pending_metadata;
%ignore m_pending_arrays;

// Disable member variable wrapping for non-copyable types
%feature("immutable") star::StarDataset::m_mutex;
%feature("immutable") star::StarDataset::m_thread_pool;
%feature("immutable") StarDataset::m_mutex;
%feature("immutable") StarDataset::m_thread_pool;

// Define shared_ptr handling for MetadataValue before parsing header
// Define shared_ptr handling for StarDataset (returned by create/open)
// Define shared_ptr handling for LayerView (returned by get_layer/create_layer)
// Note: MetadataValue, LayerView, and StarDataset are in star namespace
%shared_ptr(star::MetadataValue)
%shared_ptr(star::LayerView)
%shared_ptr(star::StarDataset)

// Expose static factory methods with clear names
%rename(create) StarDataset::create;
%rename(open) StarDataset::open;

// Parse main header first (defines classes)
%include "stards.h"

// Import star namespace for SWIG (after header is parsed)
using namespace star;

// Template instantiations that depend on star namespace types
%template(VectorSlice) std::vector<Slice>;

// Template instantiations for LayerMetadataAccessor::put (expects NDArray<T>)
%template(put_int8) LayerMetadataAccessor::put<NDArray<int8_t>>;
%template(put_int16) LayerMetadataAccessor::put<NDArray<int16_t>>;
%template(put_int32) LayerMetadataAccessor::put<NDArray<int32_t>>;
%template(put_int64) LayerMetadataAccessor::put<NDArray<int64_t>>;
%template(put_uint8) LayerMetadataAccessor::put<NDArray<uint8_t>>;
%template(put_uint16) LayerMetadataAccessor::put<NDArray<uint16_t>>;
%template(put_uint32) LayerMetadataAccessor::put<NDArray<uint32_t>>;
%template(put_uint64) LayerMetadataAccessor::put<NDArray<uint64_t>>;
%template(put_float32) LayerMetadataAccessor::put<NDArray<float>>;
%template(put_float64) LayerMetadataAccessor::put<NDArray<double>>;
%template(put_string) LayerMetadataAccessor::put<NDArray<std::string>>;

// Extend FileHeader with Python properties for easier access
%extend star::FileHeader {
    %pythoncode %{
        @property
        def magic_string(self):
            """Get magic string as Python string"""
            # SWIG already converts char arrays to strings
            return self.magic
    %}
}

// Expose the C++ logger functions with prefixed names at module level
%inline %{
    void logger_set_log_level(int level) {
        logger::set_log_level(static_cast<logger::LogLevel>(level));
    }

    int logger_get_log_level() {
        return static_cast<int>(logger::get_log_level());
    }
%}

// Expose standalone getLibraryVersion() function
%inline %{
    std::string get_library_version() {
        return getLibraryVersion();
    }
%}

// Create a Python "logger" class/module within the generated module
%pythoncode %{
class logger:
    """Logger control for STARDS"""
    TRACE = 0
    DEBUG = 1
    INFO = 2
    WARN = 3
    ERROR = 4

    @staticmethod
    def set_log_level(level):
        """Set the logging level"""
        logger_set_log_level(level)

    @staticmethod
    def get_log_level():
        """Get the current logging level"""
        return logger_get_log_level()
%}

// Extend MetadataValue with helper methods for SWIG
%extend star::MetadataValue {
    star::NDArray<int8_t> as_int8() { return $self->as<int8_t>(); }
    star::NDArray<int16_t> as_int16() { return $self->as<int16_t>(); }
    star::NDArray<int32_t> as_int32() { return $self->as<int32_t>(); }
    star::NDArray<int64_t> as_int64() { return $self->as<int64_t>(); }
    star::NDArray<uint8_t> as_uint8() { return $self->as<uint8_t>(); }
    star::NDArray<uint16_t> as_uint16() { return $self->as<uint16_t>(); }
    star::NDArray<uint32_t> as_uint32() { return $self->as<uint32_t>(); }
    star::NDArray<uint64_t> as_uint64() { return $self->as<uint64_t>(); }
    star::NDArray<float> as_float32() { return $self->as<float>(); }
    star::NDArray<double> as_float64() { return $self->as<double>(); }
    star::NDArray<std::string> as_string() { return $self->as<std::string>(); }
}

// Now modify and instantiate templates (requires classes to be defined)
%include "rename_operators.i"
%include "iterators.i"
%include "ndarray.i"

// Generic type-dispatching helpers (star_put / star_get / ...). Must come after
// stards.h (needs the full class definitions + dtype_of) and after numpy_buffer.i
// (uses ndarray_from_numpy_buffer / ndarray_to_numpy_buffer).
%include "dispatch.i"
