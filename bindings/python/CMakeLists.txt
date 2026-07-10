cmake_minimum_required(VERSION 3.14)

# The Python bindings wrap the full StarDataset API, including compression and
# cloud (HTTP/S3) storage. Those features are therefore REQUIRED here (not
# optional): the built extension always exposes the same, complete surface, and
# the ENABLE_* macros are guaranteed defined so the wrapper is deterministic.
# Fail fast with a clear message rather than silently shipping a crippled module.
foreach(_feat ZLIB LZ4 CURL S3)
    if(NOT STAR_ENABLE_${_feat})
        message(FATAL_ERROR
            "STAR_ENABLE_${_feat}=OFF, but the Python bindings require it. "
            "Building pystards needs ZLIB, LZ4, CURL and S3 enabled "
            "(-DSTAR_ENABLE_${_feat}=ON), or disable the bindings with "
            "-DSTAR_BUILD_PYTHON_BINDINGS=OFF.")
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
# so probe it here and tell star.i via -DSTAR_SIZE_T_IS_UINT64.
# A static_assert compiles only when the two are the SAME type, so the probe's
# success is exactly the condition we want to detect.
include(CheckCXXSourceCompiles)
check_cxx_source_compiles("
  #include <cstddef>
  #include <cstdint>
  #include <type_traits>
  static_assert(std::is_same<std::size_t, std::uint64_t>::value, \"\");
  int main() { return 0; }
  " STAR_SIZE_T_IS_UINT64)
if(STAR_SIZE_T_IS_UINT64)
  message(STATUS "size_t is the same type as uint64_t; aliasing VectorUInt64 -> VectorSize in SWIG")
  list(APPEND CMAKE_SWIG_FLAGS "-DSTAR_SIZE_T_IS_UINT64")
else()
  message(STATUS "size_t and uint64_t are distinct types; instantiating both SWIG vectors")
endif()

# Setup for Python linking
find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)

message(STATUS "Found SWIG: ${SWIG_VERSION}")
message(STATUS "Found Python: ${Python3_VERSION}")
message(STATUS "Python include: ${Python3_INCLUDE_DIRS}")
message(STATUS "NumPy include: ${Python3_NumPy_INCLUDE_DIRS}")

# Setup for wrapper library
set(PYSTARDS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/pystards")

set_source_files_properties(swig/star.i PROPERTIES CPLUSPLUS ON)

swig_add_library(pystards
                 LANGUAGE python
                 SOURCES swig/star.i
                 OUTPUT_DIR ${PYSTARDS_OUTPUT_DIR})

swig_link_libraries(pystards Star Python3::Module)

set_target_properties(pystards PROPERTIES
    SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
    LIBRARY_OUTPUT_DIRECTORY ${PYSTARDS_OUTPUT_DIR}
)

# Include directories
target_include_directories(pystards PRIVATE
    ${CMAKE_SOURCE_DIR}/StarDS/include
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
)

# Optional dependencies
if(STAR_ENABLE_ZLIB)
    target_compile_definitions(pystards PRIVATE ENABLE_ZLIB)
    swig_link_libraries(pystards ZLIB::ZLIB)
endif()

if(STAR_ENABLE_CURL)
    target_compile_definitions(pystards PRIVATE ENABLE_CURL)
endif()

if(STAR_ENABLE_S3)
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
if(STAR_BUILD_TESTS)
    add_custom_target(pytest
        COMMAND ${Python3_EXECUTABLE} -m pytest ${CMAKE_CURRENT_SOURCE_DIR}/tests -v
        DEPENDS pystards
        WORKING_DIRECTORY ${PYSTARDS_OUTPUT_DIR}
    )
endif()

# Setup to run pip install on install
install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install . WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
