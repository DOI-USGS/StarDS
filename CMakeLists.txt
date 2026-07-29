include(CMakeDependentOption)
cmake_minimum_required(VERSION 3.10)

# Single source of truth for the project version: the repo-root STARDS_VERSION file.
# CMake stamps this onto the library target and the Python package (via
# @PROJECT_VERSION@ in setup.py.in / _version.py.in). The header StarDS/include/stards.h
# hardcodes the same numbers so it still works when copy-pasted standalone — keep
# STAR_VERSION_* in stards.h in sync with STARDS_VERSION.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/STARDS_VERSION" STARDS_VERSION_RAW LIMIT_COUNT 1)
string(STRIP "${STARDS_VERSION_RAW}" STARDS_VERSION)
project(STARDS VERSION ${STARDS_VERSION} DESCRIPTION "Simple Tensors Arrays and Rasters storage format")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Defines CMAKE_INSTALL_LIBDIR etc.; without it the package config installs to an
# absolute "/cmake/STARDS" outside the install prefix.
include(GNUInstallDirs)

message(STATUS "CMake Module Path: " ${CMAKE_MODULE_PATH})

# A lot of linux OSes do not support C++20
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

# to avoid complaints in conda-forge, see https://conda-forge.org/docs/maintainer/knowledge_base/#newer-c-features-with-old-sdk
# (only affects libc++; harmless elsewhere)
add_compile_definitions(_LIBCPP_DISABLE_AVAILABILITY)

# On Windows with a MinGW/GCC toolchain, statically link the GCC runtime into every
# target defined below (tools, tests, Python extension). Otherwise the produced
# .exe/.pyd depend on libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll at
# load time, and windows-latest has several conflicting copies of those on PATH
# (Strawberry Perl, Rtools, git, mingw) — loading a mismatched one yields
# STATUS_ENTRYPOINT_NOT_FOUND (0xc0000139) when the exe runs (e.g. during
# gtest_discover_tests, which executes the freshly built test binary). Static
# linking makes the binaries self-contained.
#
# -static (in addition to the explicit -static-libgcc/-static-libstdc++) also pulls
# libwinpthread in statically, so none of the three GCC-runtime DLLs is a load-time
# dependency. This is safe for the third-party deps: find_package(CURL/ZLIB/OpenSSL)
# and the LZ4 find_library give CMake FULL library paths (not -lNAME), and -static
# only affects -lNAME resolution — the only such libs here (ws2_32, and the
# built-from-source gtest/gmock) have static archives available in MinGW.
# Directory-scoped and guarded to MinGW only: no effect on MSVC, Linux, or macOS.
if(MINGW)
  add_link_options(-static-libgcc -static-libstdc++ -static)
endif()

# Set up Anaconda prefix in the case with a non-default conda env is activated
if(EXISTS $ENV{CONDA_PREFIX})
  message("CONDA PREFIX: $ENV{CONDA_PREFIX}")
  list(APPEND CMAKE_FIND_ROOT_PATH $ENV{CONDA_PREFIX}
                                   $ENV{CONDA_PREFIX}/lib/cmake/Qt5)
  # On Windows conda, headers/libs live under $CONDA_PREFIX/Library/{include,lib,bin},
  # not directly under $CONDA_PREFIX. Append that prefix so find_package(CURL/ZLIB/
  # OpenSSL) and the LZ4 find_library resolve there too. Purely additive — these
  # paths don't exist on Linux/macOS and are simply ignored, so no behavior change
  # off Windows.
  list(APPEND CMAKE_PREFIX_PATH    $ENV{CONDA_PREFIX} $ENV{CONDA_PREFIX}/Library)
  list(APPEND CMAKE_FIND_ROOT_PATH $ENV{CONDA_PREFIX}/Library)
endif()

list(APPEND CMAKE_FIND_ROOT_PATH /var/lang/)

message(STATUS "find path: " ${CMAKE_FIND_ROOT_PATH})

# Variables required by multiple build options
set(STARDS_BUILD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/StarDS/include/")

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

option (STARDS_BUILD_LIB "Build the StarDS Library" ON)

set(JSON_BuildTests OFF CACHE INTERNAL "")

# add_subdirectory("submodules/gularkfilesystem")

# The public header installs to $PREFIX/include/stards/stards.h. Point the
# install interface at that same directory so downstream find_package(STARDS)
# consumers keep using the bare `#include "stards.h"` (matching in-tree usage).
set(STARDS_INSTALL_INCLUDE_DIR "${CMAKE_INSTALL_INCLUDEDIR}/stards")


set(STARDS_HEADER_FILES ${STARDS_BUILD_INCLUDE_DIR}/stards.h
                                 )
add_library(StarDS INTERFACE)
target_include_directories(StarDS INTERFACE
    $<BUILD_INTERFACE:${STARDS_BUILD_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/StarDS/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/stards>
)

# set(STARDS_PRIVATE_HEADER_FILES ${STARDS_BUILD_INCLUDE_DIR}/memo.h
#                                  ${STARDS_BUILD_INCLUDE_DIR}/restincurl.h)


set_target_properties(StarDS PROPERTIES
                                VERSION ${PROJECT_VERSION}
                                SOVERSION 0)

target_include_directories(StarDS
                          INTERFACE
                          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/StarDS/include>
                          $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/stards>)


# Feature options (shared by the native and WebAssembly branches below).
option (STARDS_ENABLE_CURL "Enable curl support for HTTP streaming" ON)
option (STARDS_ENABLE_ZLIB "Enable zlib support for compression" ON)
option(STARDS_ENABLE_LZ4 "Enable LZ4 support for fast compression" ON)
option(STARDS_ENABLE_S3 "Enable S3 support for cloud storage" ON)

if(EMSCRIPTEN)
  # ===========================================================================
  # WebAssembly (emscripten) build.
  #
  # The emscripten sysroot ships none of the native network/crypto deps — no
  # libcurl, no OpenSSL, no LZ4 — so the feature wiring differs from native:
  #   * HTTP (/vsicurl) + S3 (/vsis3) stay ENABLED (ENABLE_CURL/ENABLE_S3), but
  #     the header uses the JS global fetch() (via EM_ASYNC_JS) instead of
  #     libcurl (guarded on __EMSCRIPTEN__). read_at() stays blocking, so the
  #     build needs -sASYNCIFY to suspend/rewind the Wasm stack across the
  #     awaited fetch Promise. (We do NOT use -sFETCH/emscripten_fetch: its
  #     synchronous mode returns NULL off a Web Worker — i.e. on the browser
  #     main thread and under node — which is exactly the case we need.)
  #     Callers must supply S3 credentials explicitly (the SSO / ~/.aws
  #     discovery paths are compiled out under __EMSCRIPTEN__).
  #   * S3 SigV4 signing needs SHA-256; with no OpenSSL we compile just that one
  #     primitive from vendored mbedTLS (cmake/stards_mbedtls_config.h trims it
  #     to SHA-256 only; HMAC-SHA256 is hand-rolled over it in the header).
  #   * zlib comes from emscripten's own port (-sUSE_ZLIB=1).
  #   * LZ4 has no emscripten port, so it is left off; the header's compression
  #     path degrades exactly as in any no-LZ4 native build.
  # ===========================================================================
  message(STATUS "Configuring StarDS for WebAssembly (emscripten)")

  if(STARDS_ENABLE_CURL)
    add_definitions(-DENABLE_CURL)
    # The header's WASM network backend is EM_ASYNC_JS + await fetch() (the JS
    # global fetch() needs no extra port). -sASYNCIFY lets those awaited fetches
    # look blocking to read_at() by suspending/rewinding the Wasm stack; it is a
    # link-time flag. No -sFETCH: emscripten_fetch's synchronous mode is unusable
    # off a Web Worker (see the header note near <emscripten/em_js.h>).
    target_link_options(StarDS INTERFACE "-sASYNCIFY")
  endif()

  if(STARDS_ENABLE_ZLIB)
    add_definitions(-DENABLE_ZLIB)
    target_compile_options(StarDS INTERFACE "-sUSE_ZLIB=1")
    target_link_options(StarDS INTERFACE "-sUSE_ZLIB=1")
  endif()

  if(STARDS_ENABLE_LZ4)
    message(STATUS "LZ4 has no emscripten port; building WASM without LZ4.")
    # Intentionally do NOT define ENABLE_LZ4 — the header falls back to zlib/none.
  endif()

  if(STARDS_ENABLE_S3)
    if(NOT STARDS_ENABLE_CURL)
      message(FATAL_ERROR "S3 support requires CURL. Enable STARDS_ENABLE_CURL.")
    endif()
    add_definitions(-DENABLE_S3)

    # SHA-256 for SigV4, from vendored mbedTLS (no OpenSSL in the sysroot). Only
    # two self-contained source files are needed.
    set(STARDS_MBEDTLS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/submodules/mbedtls")
    if(NOT EXISTS "${STARDS_MBEDTLS_DIR}/library/sha256.c")
      message(FATAL_ERROR
        "mbedTLS submodule not found at ${STARDS_MBEDTLS_DIR}. "
        "Run: git submodule update --init submodules/mbedtls")
    endif()
    add_library(stards_mbedtls STATIC
      "${STARDS_MBEDTLS_DIR}/library/sha256.c"
      "${STARDS_MBEDTLS_DIR}/library/platform_util.c")
    # PUBLIC so linking StarDS propagates the mbedtls include dir + config to the
    # header's <mbedtls/sha256.h> include and to all downstream consumers.
    target_include_directories(stards_mbedtls PUBLIC
      "${STARDS_MBEDTLS_DIR}/include"
      "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
    target_compile_definitions(stards_mbedtls PUBLIC
      MBEDTLS_CONFIG_FILE="stards_mbedtls_config.h")

    target_link_libraries(StarDS INTERFACE stards_mbedtls)
    message(STATUS "S3 support: enabled (fetch() via EM_ASYNC_JS + vendored mbedTLS SHA-256)")
  else()
    message(STATUS "S3 support: disabled")
  endif()

else()  # ---- native (non-emscripten) build ----

if(STARDS_ENABLE_CURL)
  add_definitions(-DENABLE_CURL)
  # Find curl package if needed
  find_package(CURL REQUIRED)
  target_link_libraries(StarDS INTERFACE ${CURL_LIBRARIES})
  target_include_directories(StarDS INTERFACE ${CURL_INCLUDE_DIRS})
endif()

# Option to enable zlib support
if(STARDS_ENABLE_ZLIB)
  add_definitions(-DENABLE_ZLIB)
  find_package(ZLIB REQUIRED)
  target_include_directories(StarDS INTERFACE ${ZLIB_INCLUDE_DIRS})
  target_link_libraries(StarDS INTERFACE ${ZLIB_LIBRARIES})
endif()

# Option to enable LZ4 support
if(STARDS_ENABLE_LZ4)
  add_definitions(-DENABLE_LZ4)

  # Try pkg-config first, then manual search
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(LZ4 QUIET liblz4)
  endif()

  # Fallback to manual search if pkg-config didn't find it. NAMES is a superset of
  # the historical list — it can only find MORE than before (adds the conda-forge /
  # MSVC import- and static-lib spellings so Windows resolves lz4-c reliably).
  if(NOT LZ4_FOUND)
    find_path(LZ4_INCLUDE_DIR lz4.h)
    find_library(LZ4_LIBRARY NAMES lz4 liblz4 lz4_static liblz4_static)

    if(LZ4_INCLUDE_DIR AND LZ4_LIBRARY)
      set(LZ4_FOUND TRUE)
      set(LZ4_INCLUDE_DIRS ${LZ4_INCLUDE_DIR})
      set(LZ4_LIBRARIES ${LZ4_LIBRARY})
    endif()
  endif()

  if(LZ4_FOUND)
    message(STATUS "Found LZ4: ${LZ4_LIBRARIES}")
    target_include_directories(StarDS INTERFACE ${LZ4_INCLUDE_DIRS})
    target_link_libraries(StarDS INTERFACE ${LZ4_LIBRARIES})
  else()
    message(WARNING "LZ4 library not found. Disabling LZ4 support.")
    remove_definitions(-DENABLE_LZ4)
  endif()
endif()

# Option to enable S3 support
if(STARDS_ENABLE_S3)
  # S3 requires CURL for HTTP and OpenSSL for signing
  if(NOT STARDS_ENABLE_CURL)
    message(FATAL_ERROR "S3 support requires CURL. Enable STARDS_ENABLE_CURL.")
  endif()

  # Find OpenSSL for SHA256 and HMAC-SHA256
  find_package(OpenSSL REQUIRED)

  # Add preprocessor definition
  add_definitions(-DENABLE_S3)

  # Link OpenSSL
  target_link_libraries(StarDS INTERFACE ${OPENSSL_LIBRARIES})
  target_include_directories(StarDS INTERFACE ${OPENSSL_INCLUDE_DIR})

  message(STATUS "S3 support: enabled (requires CURL and OpenSSL)")
else()
  message(STATUS "S3 support: disabled")
endif()

endif()  # EMSCRIPTEN / native feature-detection branch


# Install the (header-only INTERFACE) target as an export set so a matching
# STARDSTargets.cmake is generated for find_package(STARDS).
install(TARGETS StarDS EXPORT StarDSTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Generate/install STARDSTargets.cmake with the STARDS:: namespace that
# config.cmake.in expects (STARDS::StarDS).
install(EXPORT StarDSTargets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Generate the package config
configure_file(cmake/config.cmake.in
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              @ONLY)

# Install the package config
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Install the headers
install(FILES ${STARDS_HEADER_FILES} DESTINATION ${STARDS_INSTALL_INCLUDE_DIR})


###############
# Tools Build #
###############

cmake_dependent_option (STARDS_BUILD_TOOLS "Build the StarDS Command-Line Tools" ON STARDS_BUILD_LIB OFF)

# Built before the tests so the conversion-parity test can reference the
# stards_translate target (via $<TARGET_FILE:...> and add_dependencies).
if(STARDS_BUILD_TOOLS)
  add_subdirectory(bin)
else()
  message(STATUS "Skipping Tools")
endif()

###############
# Tests Build #
###############

cmake_dependent_option (STARDS_BUILD_TESTS "Build the StarDS Tests" ON STARDS_BUILD_LIB OFF)

if(STARDS_BUILD_TESTS)
  include(GoogleTest)
  include(cmake/gtest.cmake)
  include(CTest)

  find_package (Threads)
  enable_testing()
  add_subdirectory(StarDS/tests)
else()
  message(STATUS "Skipping Tests")
endif()

##################
# Bindings Build #
##################

cmake_dependent_option(STARDS_BUILD_PYTHON_BINDINGS
    "Build Python bindings using SWIG" ON
    "STARDS_BUILD_LIB" OFF)

if(STARDS_BUILD_PYTHON_BINDINGS)
  add_subdirectory(bindings/python)
else()
  message(STATUS "Skipping Python bindings")
endif()
