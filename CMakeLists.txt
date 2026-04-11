include(CMakeDependentOption)
cmake_minimum_required(VERSION 3.10)
project(STAR VERSION 1.0.0 DESCRIPTION "Simple Tensors Arrays and Rasters storage format")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

message(STATUS "CMake Module Path: " ${CMAKE_MODULE_PATH})

# A lot of linux OSes do not support C++20
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# to avoid complaints in conda-forge, see https://conda-forge.org/docs/maintainer/knowledge_base/#newer-c-features-with-old-sdk 
set(CXXFLAGS, "${CXXFLAGS} -D_LIBCPP_DISABLE_AVAILABILITY")

# Set up Anaconda prefix in the case with a non-default conda env is activated
if(EXISTS $ENV{CONDA_PREFIX})
  message("CONDA PREFIX: $ENV{CONDA_PREFIX}")
  list(APPEND CMAKE_FIND_ROOT_PATH $ENV{CONDA_PREFIX}
                                   $ENV{CONDA_PREFIX}/lib/cmake/Qt5)
endif()

list(APPEND CMAKE_FIND_ROOT_PATH /var/lang/)

message(STATUS "find path: " ${CMAKE_FIND_ROOT_PATH})

# Variables required by multiple build options
set(STAR_BUILD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/Star/include/")

# Generate version header from CMake version
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/Star/include/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/Star/include/version.h
    @ONLY
)

set(default_build_type "Release")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE "${default_build_type}" CACHE
      STRING "Choose the type of build." FORCE)
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "$ENV{CONDA_PREFIX}")
endif()
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")

#################
# Library Build #
#################

option (STAR_BUILD_LIB "Build the Star Library" ON)



set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory("submodules/gularkfilesystem")

set(STAR_INSTALL_INCLUDE_DIR "include/Star")


set(STAR_HEADER_FILES ${STAR_BUILD_INCLUDE_DIR}/star.h
                                 )
add_library(Star INTERFACE)
target_include_directories(Star INTERFACE
    $<BUILD_INTERFACE:${STAR_BUILD_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/Star/include>
    $<INSTALL_INTERFACE:include>
)

# set(STAR_PRIVATE_HEADER_FILES ${STAR_BUILD_INCLUDE_DIR}/memo.h
#                                  ${STAR_BUILD_INCLUDE_DIR}/restincurl.h)


set_target_properties(Star PROPERTIES
                                VERSION ${PROJECT_VERSION}
                                SOVERSION 0)

target_include_directories(Star
                          INTERFACE
                          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Star/include>
                          $<INSTALL_INTERFACE:include>)


# Option to enable curl support
option (STAR_ENABLE_CURL "Enable curl support for HTTP streaming" ON)
if(STAR_ENABLE_CURL)
  add_definitions(-DENABLE_CURL)
  # Find curl package if needed
  find_package(CURL REQUIRED)
  target_link_libraries(Star INTERFACE ${CURL_LIBRARIES})
  target_include_directories(Star INTERFACE ${CURL_INCLUDE_DIRS})
endif()
 
# Option to enable zlib support
option (STAR_ENABLE_ZLIB "Enable zlib support for compression" ON)
if(STAR_ENABLE_ZLIB)
  add_definitions(-DENABLE_ZLIB)
  find_package(ZLIB REQUIRED)
  target_include_directories(Star INTERFACE ${ZLIB_INCLUDE_DIRS})
  target_link_libraries(Star INTERFACE ${ZLIB_LIBRARIES})
endif()

# Option to enable LZ4 support
option(STAR_ENABLE_LZ4 "Enable LZ4 support for fast compression" ON)
if(STAR_ENABLE_LZ4)
  add_definitions(-DENABLE_LZ4)

  # Try pkg-config first, then manual search
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(LZ4 QUIET liblz4)
  endif()

  # Fallback to manual search if pkg-config didn't find it
  if(NOT LZ4_FOUND)
    find_path(LZ4_INCLUDE_DIR lz4.h)
    find_library(LZ4_LIBRARY NAMES lz4 liblz4)

    if(LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
      set(LZ4_FOUND TRUE)
      set(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})
      set(LZ4_LIBRARIES ${LZ4_LIBRARY})
    endif()
  endif()

  if(LZ4_FOUND)
    message(STATUS "Found LZ4: ${LZ4_LIBRARIES}")
    target_include_directories(Star INTERFACE ${LZ4_INCLUDE_DIRS})
    target_link_libraries(Star INTERFACE ${LZ4_LIBRARIES})
  else()
    message(WARNING "LZ4 library not found. Disabling LZ4 support.")
    remove_definitions(-DENABLE_LZ4)
  endif()
endif()

# Option to enable S3 support
option(STAR_ENABLE_S3 "Enable S3 support for cloud storage" ON)
if(STAR_ENABLE_S3)
  # S3 requires CURL for HTTP and OpenSSL for signing
  if(NOT STAR_ENABLE_CURL)
    message(FATAL_ERROR "S3 support requires CURL. Enable STAR_ENABLE_CURL.")
  endif()

  # Find OpenSSL for SHA256 and HMAC-SHA256
  find_package(OpenSSL REQUIRED)

  # Add preprocessor definition
  add_definitions(-DENABLE_S3)

  # Link OpenSSL
  target_link_libraries(Star INTERFACE ${OPENSSL_LIBRARIES})
  target_include_directories(Star INTERFACE ${OPENSSL_INCLUDE_DIR})

  message(STATUS "S3 support: enabled (requires CURL and OpenSSL)")
else()
  message(STATUS "S3 support: disabled")
endif()


install(TARGETS Star LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Generate the package config
configure_file(cmake/config.cmake.in
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              @ONLY)

# Install the package config
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Install the headers
install(FILES ${STAR_HEADER_FILES} DESTINATION ${STAR_INSTALL_INCLUDE_DIR})

# # Install the target
# install(EXPORT Star
#         NAMESPACE usgs::
#         DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})


###############
# Tests Build #
###############

cmake_dependent_option (STAR_BUILD_TESTS "Build the Star Tests" ON STAR_BUILD_LIB OFF)

if(STAR_BUILD_TESTS)
  include(GoogleTest)
  include(cmake/gtest.cmake)
  include(CTest)

  find_package (Threads)
  enable_testing()
  add_subdirectory(Star/tests)
else()
  message(STATUS "Skipping Tests")
endif()

###############
# Tools Build #
###############

cmake_dependent_option (STAR_BUILD_TOOLS "Build the Star Command-Line Tools" ON STAR_BUILD_LIB OFF)

if(STAR_BUILD_TOOLS)
  add_subdirectory(bin)
else()
  message(STATUS "Skipping Tools")
endif()

##################
# Bindings Build #
##################

cmake_dependent_option(STAR_BUILD_PYTHON_BINDINGS
    "Build Python bindings using SWIG" OFF
    "STAR_BUILD_LIB" OFF)

if(STAR_BUILD_PYTHON_BINDINGS)
  add_subdirectory(bindings/python)
else()
  message(STATUS "Skipping Python bindings")
endif()
