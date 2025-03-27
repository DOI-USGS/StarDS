include(CMakeDependentOption)
cmake_minimum_required(VERSION 3.10)
project(CameraStateFile VERSION 1.0.0 DESCRIPTION "File format for CSM States")

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
set(CAMERASTATEFILE_BUILD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/include/")

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

option (CAMERASTATEFILE_BUILD_LIB "Build the CameraStateFile Library" ON)

# Option to enable curl support
option (CAMERASTATEFILE_ENABLE_CURL "Enable curl support for HTTP streaming" ON)
if(CAMERASTATEFILE_ENABLE_CURL)
  add_definitions(-DENABLE_CURL)
  # Find curl package if needed
  find_package(CURL REQUIRED)
endif()

set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory("submodules/gularkfilesystem")

set(CAMERASTATEFILE_INSTALL_INCLUDE_DIR "include/CameraStateFile")


set(CAMERASTATEFILE_HEADER_FILES ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/scs.h
                                 ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/ezgz.hpp
                                 ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/deflate.hpp)
add_library(CameraStateFile INTERFACE)
target_include_directories(CameraStateFile INTERFACE ${CAMERASTATEFILE_BUILD_INCLUDE_DIR})

# set(CAMERASTATEFILE_PRIVATE_HEADER_FILES ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/memo.h
#                                  ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/restincurl.h)


set_target_properties(CameraStateFile PROPERTIES
                                VERSION ${PROJECT_VERSION}
                                SOVERSION 0)

target_include_directories(CameraStateFile
                          INTERFACE
                          $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/include>
                          $<INSTALL_INTERFACE:include>
                          )

if(CAMERASTATEFILE_ENABLE_CURL)
  target_link_libraries(CameraStateFile INTERFACE ${CURL_LIBRARIES})
  target_include_directories(CameraStateFile INTERFACE ${CURL_INCLUDE_DIRS})
endif()
 
install(TARGETS CameraStateFile LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY ${CAMERASTATEFILE_INCLUDE_DIR} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Generate the package config
configure_file(cmake/config.cmake.in
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              @ONLY)

# Install the package config
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
              DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Install the headers
install(FILES ${CAMERASTATEFILE_HEADER_FILES} DESTINATION ${CAMERASTATEFILE_INSTALL_INCLUDE_DIR})

# # Install the target
# install(EXPORT CameraStateFile
#         NAMESPACE usgs::
#         DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})


###############
# Tests Build #
###############

cmake_dependent_option (CAMERASTATEFILE_BUILD_TESTS "Build the CameraStateFile Tests" ON CAMERASTATEFILE_BUILD_LIB OFF)

if(CAMERASTATEFILE_BUILD_TESTS)
  include(GoogleTest)
  include(cmake/gtest.cmake)
  include(CTest)

  find_package (Threads)
  enable_testing()
  add_subdirectory(CameraStateFile/tests)
else()
  message(STATUS "Skipping Tests")
endif()

# ##################
# # Bindings Build #
# ##################

# cmake_dependent_option (CAMERASTATEFILE_BUILD_BINDINGS "Build the CameraStateFile Bindings in Other Languages" ON CAMERASTATEFILE_BUILD_LIB OFF)

# if(CAMERASTATEFILE_BUILD_BINDINGS)
#   add_subdirectory(bindings)
# else()
#   message(STATUS "Skipping Bindings")
# endif()
