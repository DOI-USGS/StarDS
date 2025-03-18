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

if(CAMERASTATEFILE_BUILD_LIB)

  set(JSON_BuildTests OFF CACHE INTERNAL "")

  add_subdirectory("submodules/gularkfilesystem")
  add_subdirectory("submodules/json")
  
  find_package(CSpice REQUIRED)
  find_package(fmt REQUIRED)
  find_package(cereal REQUIRED)
  find_package(spdlog REQUIRED)
  find_package(HighFive REQUIRED)

  set(CAMERASTATEFILE_INSTALL_INCLUDE_DIR "include/CameraStateFile")
  set(CAMERASTATEFILE_SRC_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/camerastatefile.cpp 
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/utils.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/io.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/query.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/spice_types.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/memoized_functions.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/config.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/inventory.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/inventoryimpl.cpp
                          ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/src/api.cpp)


  set(CAMERASTATEFILE_HEADER_FILES ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/camerastatefile.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/utils.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/memoized_functions.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/io.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/spice_types.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/query.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/config.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/inventory.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/inventoryimpl.h
                           ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/api.h)

  set(CAMERASTATEFILE_PRIVATE_HEADER_FILES ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/memo.h
                                   ${CAMERASTATEFILE_BUILD_INCLUDE_DIR}/restincurl.h)

  set(CAMERASTATEFILE_CONFIG_FILES ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/apollo15.json 
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/apollo16.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/apollo17.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/base.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/cassini.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/chandrayaan1.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/clem1.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/galileo.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/hayabusa.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/hayabusa2.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/juno.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/kaguya.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/lro.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/lo.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/mer1.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/mer2.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/mess.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/mex.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/mgs.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/mro.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/msl.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/newhorizons.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/odyssey.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/smart1.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/tgo.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/viking1.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/viking2.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/voyager1.json
                           ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/voyager2.json
                           )

  set(CAMERASTATEFILE_KERNELS ${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/db/kernels/naif0011.tls)

  add_library(CameraStateFile SHARED ${CAMERASTATEFILE_SRC_FILES})

  set_target_properties(CameraStateFile PROPERTIES
                                  VERSION ${PROJECT_VERSION}
                                  SOVERSION 0)

  target_compile_definitions(CameraStateFile PRIVATE SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE 
                                     PUBLIC -D_SOURCE_PREFIX="${CMAKE_CURRENT_SOURCE_DIR}")
  
target_include_directories(CameraStateFile
                            PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/CameraStateFile/include>
                            $<INSTALL_INTERFACE:include>
                            PRIVATE
                            ${CMAKE_CURRENT_SOURCE_DIR}/submodules/BTree/include/
                            cereal
                            )

  target_link_libraries(CameraStateFile
                        PUBLIC
                        fmt::fmt-header-only
                        nlohmann_json::nlohmann_json
                        PRIVATE
                        cspice
                        spdlog::spdlog_header_only
                        HighFive
                        )
   
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

  # Install the json db files
  install(FILES ${CAMERASTATEFILE_CONFIG_FILES} DESTINATION "etc/CameraStateFile/db")

  # Install the shipped kernels
  install(FILES ${CAMERASTATEFILE_KERNELS} DESTINATION "etc/CameraStateFile/db/kernels")

  # Install the library
  install(TARGETS CameraStateFile nlohmann_json ghc_filesystem
          EXPORT cameraStateFileTargets
          LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
          INCLUDES DESTINATION ${CAMERASTATEFILE_INSTALL_INCLUDE_DIR})

  # Install the target
  install(EXPORT cameraStateFileTargets
          NAMESPACE spice::
          DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

else()
  message(STATUS "Skipping Library")
endif()

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

