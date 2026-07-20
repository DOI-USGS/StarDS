cmake_minimum_required(VERSION 3.14)

# Feature policy for the bindings.
#
# By default the bindings wrap the FULL API (compression + cloud HTTP/S3), so a
# native build with the default options gets every feature. But the features are
# no longer force-forced: a caller may disable CURL/S3 (e.g. a build for a
# sandboxed environment with no sockets) and still get a working extension — the
# SWIG interface %ignore's the HTTP/S3 classes, so the wrapper compiles the same
# with those off; only the runtime remote paths become throws.
#
# ZLIB and LZ4 are the compression codecs. They stay ON by default so the wrapper
# can read/write GZIP and LZ4 files, but a build MAY turn them off (StarConfig's
# default codec is chosen from whatever is compiled in — see stards.h). We only
# warn on a disabled feature; nothing is forced.
foreach(_feat ZLIB LZ4 CURL S3)
    if(NOT STARDS_ENABLE_${_feat})
        message(STATUS
            "pystards: building without STARDS_ENABLE_${_feat}. The extension "
            "will still import; the corresponding runtime paths are unavailable.")
    endif()
endforeach()

# Setup for SWIG
set(CMAKE_SWIG_FLAGS)
find_package(SWIG REQUIRED COMPONENTS python)
include(UseSWIG)
list(APPEND CMAKE_SWIG_FLAGS "-c++" "-py3" "-I${CMAKE_SOURCE_DIR}/StarDS/include")

# On LP64 platforms (e.g. Linux) size_t and uint64_t are the SAME C++ type
# (unsigned long), so instantiating std::vector<size_t> and std::vector<uint64_t>
# as separate SWIG templates yields duplicate swig::traits<> specializations and
# fails to compile. On macOS they are distinct types (unsigned long vs unsigned
# long long) and both are needed. The C preprocessor cannot detect type identity,
# so probe it here and tell star.i via -DSTARDS_SIZE_T_IS_UINT64.
# A static_assert compiles only when the two are the SAME type, so the probe's
# success is exactly the condition we want to detect.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
  #include <cstddef>
  #include <cstdint>
  #include <type_traits>
  static_assert(std::is_same<std::size_t, std::uint64_t>::value, \"\");
  int main() { return 0; }
  " STARDS_SIZE_T_IS_UINT64)
if(STARDS_SIZE_T_IS_UINT64)
  message(STATUS "size_t is the same type as uint64_t; aliasing VectorUInt64 -> VectorSize in SWIG")
  list(APPEND CMAKE_SWIG_FLAGS "-DSTARDS_SIZE_T_IS_UINT64")
else()
  message(STATUS "size_t and uint64_t are distinct types; instantiating both SWIG vectors")
endif()

# Setup for Python linking.
#
# Pin the interpreter to the active conda/virtual env FIRST. Without this,
# CMake's FindPython prefers a "newest version" heuristic and can pick the system
# Python (e.g. /usr/bin/python3) over the env's — which then lacks numpy and the
# bindings' runtime deps. We set Python3_EXECUTABLE DIRECTLY from CONDA_PREFIX
# (rather than via ROOT_DIR + a search strategy): the location-based search still
# runs CMake's virtual-env ABI cross-check, which spuriously rejects conda
# interpreters ("Wrong ABI for the interpreter") on some runners. Setting the
# executable outright skips that filtered search. If CONDA_PREFIX is unset we
# fall back to CMake's default discovery.
if(NOT DEFINED Python3_EXECUTABLE AND DEFINED ENV{CONDA_PREFIX})
    if(EXISTS "$ENV{CONDA_PREFIX}/bin/python")
        set(Python3_EXECUTABLE "$ENV{CONDA_PREFIX}/bin/python")
    elseif(EXISTS "$ENV{CONDA_PREFIX}/python.exe")   # Windows conda layout
        set(Python3_EXECUTABLE "$ENV{CONDA_PREFIX}/python.exe")
    endif()
endif()

# We build a Python EXTENSION MODULE (loaded by the interpreter at import time),
# not an embedded interpreter, so we only need Development.Module — NOT the full
# Development component (which also requires libpython / Development.Embed and
# triggers the libpython ABI check that fails on some conda/CI setups).
#
# NumPy is requested as an OPTIONAL component rather than REQUIRED: CMake's
# FindPython NumPy probe is fragile across CMake/NumPy versions (e.g. it fails on
# some CI runners with NumPy 2.x even though numpy is installed and importable),
# and a hard REQUIRED there aborts configure. We instead fall back to asking the
# found interpreter for its header dir via `numpy.get_include()`, which is what
# NumPy itself documents as the source of truth.
find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
find_package(Python3 QUIET COMPONENTS NumPy)

# Resolve the NumPy include directory: prefer CMake's, else query the interpreter.
set(STARDS_NUMPY_INCLUDE_DIR "${Python3_NumPy_INCLUDE_DIRS}")
if(NOT STARDS_NUMPY_INCLUDE_DIR)
    execute_process(
        COMMAND "${Python3_EXECUTABLE}" -c "import numpy; print(numpy.get_include())"
        OUTPUT_VARIABLE STARDS_NUMPY_INCLUDE_DIR
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE _numpy_rc)
    if(NOT _numpy_rc EQUAL 0 OR NOT STARDS_NUMPY_INCLUDE_DIR)
        message(FATAL_ERROR
            "NumPy headers not found. Install numpy in the build environment "
            "(the Python bindings need numpy.get_include()).")
    endif()
    message(STATUS "NumPy include resolved via numpy.get_include()")
endif()

message(STATUS "Found SWIG: ${SWIG_VERSION}")
message(STATUS "Found Python: ${Python3_VERSION}")
message(STATUS "Python include: ${Python3_INCLUDE_DIRS}")
message(STATUS "NumPy include: ${STARDS_NUMPY_INCLUDE_DIR}")

# Setup for wrapper library
set(PYSTARDS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/pystards")

set_source_files_properties(swig/star.i PROPERTIES CPLUSPLUS ON)

swig_add_library(pystards
                 LANGUAGE python
                 SOURCES swig/star.i
                 OUTPUT_DIR ${PYSTARDS_OUTPUT_DIR})

swig_link_libraries(pystards StarDS Python3::Module)

set_target_properties(pystards PROPERTIES
    SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
    LIBRARY_OUTPUT_DIRECTORY ${PYSTARDS_OUTPUT_DIR}
)

# Include directories
target_include_directories(pystards PRIVATE
    ${CMAKE_SOURCE_DIR}/StarDS/include
    ${Python3_INCLUDE_DIRS}
    ${STARDS_NUMPY_INCLUDE_DIR}
)

# Optional dependencies
if(STARDS_ENABLE_ZLIB)
    target_compile_definitions(pystards PRIVATE ENABLE_ZLIB)
    swig_link_libraries(pystards ZLIB::ZLIB)
endif()

if(STARDS_ENABLE_CURL)
    target_compile_definitions(pystards PRIVATE ENABLE_CURL)
endif()

if(STARDS_ENABLE_S3)
    target_compile_definitions(pystards PRIVATE ENABLE_S3)
endif()

# Set C++20 standard
set_property(TARGET pystards PROPERTY CXX_STANDARD 20)
set_property(TARGET pystards PROPERTY CXX_STANDARD_REQUIRED ON)

# Copy Python wrapper files to output directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pystards/__init__.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystards/dataset.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystards/ndarray.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystards/metadata.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystards/enums.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystards/logger.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystards/_dtypes.py
     DESTINATION ${PYSTARDS_OUTPUT_DIR})

# Generate version file from CMake version
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pystards/_version.py.in
    ${PYSTARDS_OUTPUT_DIR}/_version.py
    @ONLY
)

# Create the setup.py from template
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in
               ${CMAKE_CURRENT_BINARY_DIR}/setup.py
               @ONLY)

# Test target
if(STARDS_BUILD_TESTS)
    add_custom_target(pytest
        COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests -v
        DEPENDS pystards
        WORKING_DIRECTORY ${PYSTARDS_OUTPUT_DIR}
    )
endif()

# Setup to run pip install on install
install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install . WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
