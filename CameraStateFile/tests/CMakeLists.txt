cmake_minimum_required(VERSION 3.10)

# A lot of linux OSes do not support C++20
set(CMAKE_CXX_STANDARD 20)

find_package(CSpice REQUIRED)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
set(CAMERASTATEFILE_TEST_DIRECTORY ${CMAKE_SOURCE_DIR}/CameraStateFile/tests/)

# collect all of the test sources
set (CAMERASTATEFILE_TEST_SOURCE    ${CAMERASTATEFILE_TEST_DIRECTORY}/UnitTestSCS.cpp
                                    ${CAMERASTATEFILE_TEST_DIRECTORY}/Fixtures.cpp
                            )

# setup test executable
add_executable(runCameraStateFileTests TestMain.cpp ${CAMERASTATEFILE_TEST_SOURCE})

target_compile_definitions(runCameraStateFileTests PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

target_link_libraries(runCameraStateFileTests
                      PRIVATE
                      CameraStateFile
                      gtest
                      gmock
                      Threads::Threads
                      )


target_include_directories(runCameraStateFileTests PRIVATE ${CMAKE_SOURCE_DIR}/submodules/gularkfilesystem/include/)

gtest_discover_tests(runCameraStateFileTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/CameraStateFile/tests/)
