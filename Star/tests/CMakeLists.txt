cmake_minimum_required(VERSION 3.10)

# A lot of linux OSes do not support C++20
set(CMAKE_CXX_STANDARD 20)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
set(STAR_TEST_DIRECTORY ${CMAKE_SOURCE_DIR}/Star/tests/)

# collect all of the test sources (UnitTestSCS.cpp removed - used old API)
set (STAR_TEST_SOURCE    ${STAR_TEST_DIRECTORY}/UnitTestBlockCompression.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestNdarray.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestMetadataBlock.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestMetadataAPI.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestIntegration.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestSlicing.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestFileHeader.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestPerformance.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestThreading.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestCloseReopenAPI.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestConcurrentWrites.cpp
                                    ${STAR_TEST_DIRECTORY}/Fixtures.cpp
                            )

# Add S3 tests if S3 support is enabled
if(STAR_ENABLE_S3)
    list(APPEND STAR_TEST_SOURCE ${STAR_TEST_DIRECTORY}/UnitTestS3.cpp)
    message(STATUS "Including S3 unit tests")
endif()

# setup test executable
add_executable(runStarTests TestMain.cpp ${STAR_TEST_SOURCE})

target_compile_definitions(runStarTests PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

target_link_libraries(runStarTests
                      PRIVATE
                      Star
                      gtest
                      gmock
                      Threads::Threads
                      )


target_include_directories(runStarTests PRIVATE ${CMAKE_SOURCE_DIR}/submodules/gularkfilesystem/include/)

gtest_discover_tests(runStarTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/Star/tests/)
