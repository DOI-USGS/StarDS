cmake_minimum_required(VERSION 3.14)

# Setup for SWIG
set(CMAKE_SWIG_FLAGS)
find_package(SWIG REQUIRED COMPONENTS python)
include(UseSWIG)
list(APPEND CMAKE_SWIG_FLAGS "-c++" "-py3" "-I${CMAKE_SOURCE_DIR}/Star/include")

# Setup for Python linking
find_package(Python3 REQUIRED COMPONENTS Interpreter Development NumPy)

message(STATUS "Found SWIG: ${SWIG_VERSION}")
message(STATUS "Found Python: ${Python3_VERSION}")
message(STATUS "Python include: ${Python3_INCLUDE_DIRS}")
message(STATUS "NumPy include: ${Python3_NumPy_INCLUDE_DIRS}")

# Setup for wrapper library
set(PYSTAR_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/pystar")

set_source_files_properties(swig/star.i PROPERTIES CPLUSPLUS ON)

swig_add_library(pystar
                 LANGUAGE python
                 SOURCES swig/star.i
                 OUTPUT_DIR ${PYSTAR_OUTPUT_DIR})

swig_link_libraries(pystar Star Python3::Module)

set_target_properties(pystar PROPERTIES
    SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE
    LIBRARY_OUTPUT_DIRECTORY ${PYSTAR_OUTPUT_DIR}
)

# Include directories
target_include_directories(pystar PRIVATE
    ${CMAKE_SOURCE_DIR}/Star/include
    ${Python3_INCLUDE_DIRS}
    ${Python3_NumPy_INCLUDE_DIRS}
)

# Optional dependencies
if(STAR_ENABLE_ZLIB)
    target_compile_definitions(pystar PRIVATE ENABLE_ZLIB)
    swig_link_libraries(pystar ZLIB::ZLIB)
endif()

if(STAR_ENABLE_CURL)
    target_compile_definitions(pystar PRIVATE ENABLE_CURL)
endif()

if(STAR_ENABLE_S3)
    target_compile_definitions(pystar PRIVATE ENABLE_S3)
endif()

# Set C++20 standard
set_property(TARGET pystar PROPERTY CXX_STANDARD 20)
set_property(TARGET pystar PROPERTY CXX_STANDARD_REQUIRED ON)

# Copy Python wrapper files to output directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/pystar/__init__.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystar/dataset.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystar/ndarray.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystar/metadata.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystar/enums.py
          ${CMAKE_CURRENT_SOURCE_DIR}/pystar/logger.py
     DESTINATION ${PYSTAR_OUTPUT_DIR})

# Generate version file from CMake version
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/pystar/_version.py.in
    ${PYSTAR_OUTPUT_DIR}/_version.py
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
        DEPENDS pystar
        WORKING_DIRECTORY ${PYSTAR_OUTPUT_DIR}
    )
endif()

# Setup to run pip install on install
install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} -m pip install . WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})")
