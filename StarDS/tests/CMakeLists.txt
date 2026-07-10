cmake_minimum_required(VERSION 3.10)

# A lot of linux OSes do not support C++20
set(CMAKE_CXX_STANDARD 20)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
set(STAR_TEST_DIRECTORY ${CMAKE_SOURCE_DIR}/StarDS/tests/)

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
                                    ${STAR_TEST_DIRECTORY}/UnitTestLayers.cpp
                                    ${STAR_TEST_DIRECTORY}/UnitTestReadmeExamples.cpp
                                    ${STAR_TEST_DIRECTORY}/Fixtures.cpp
                            )

# /vsicurl (HTTP) mock-server tests need CURL support.
if(STAR_ENABLE_CURL)
    list(APPEND STAR_TEST_SOURCE ${STAR_TEST_DIRECTORY}/UnitTestVsiCurl.cpp)
    message(STATUS "Including /vsicurl mock-server tests")
endif()

# Add S3 tests if S3 support is enabled
if(STAR_ENABLE_S3)
    list(APPEND STAR_TEST_SOURCE ${STAR_TEST_DIRECTORY}/UnitTestS3.cpp
                                 ${STAR_TEST_DIRECTORY}/UnitTestVsiS3Mock.cpp)
    message(STATUS "Including S3 unit tests (real + mock-server)")
endif()

# Conversion parity test: exercises star_translate across every compression codec
# and checks the decoded data matches. Only meaningful when the tool is built and
# the example JSON exists (the test self-skips otherwise).
set(STAR_PARITY_JSON "${CMAKE_SOURCE_DIR}/StarDS/examples/mroctx_isd.json")
if(STAR_BUILD_TOOLS AND STAR_BUILD_TRANSLATE AND EXISTS "${STAR_PARITY_JSON}")
    list(APPEND STAR_TEST_SOURCE ${STAR_TEST_DIRECTORY}/UnitTestConversionParity.cpp)
    message(STATUS "Including conversion-parity test")
endif()

# setup test executable
add_executable(runStarTests TestMain.cpp ${STAR_TEST_SOURCE})

target_compile_definitions(runStarTests PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# Point the parity test at the built star_translate binary and the example JSON.
if(STAR_BUILD_TOOLS AND STAR_BUILD_TRANSLATE AND EXISTS "${STAR_PARITY_JSON}")
    target_compile_definitions(runStarTests PRIVATE
        STAR_TRANSLATE_EXE="$<TARGET_FILE:star_translate>"
        STAR_EXAMPLE_JSON="${STAR_PARITY_JSON}")
    add_dependencies(runStarTests star_translate)
endif()

target_link_libraries(runStarTests
                      PRIVATE
                      Star
                      gtest
                      gmock
                      Threads::Threads
                      )

# Note: ghc::filesystem (<ghc/fs_std.hpp>) is provided by the toolchain/conda
# include path (the cpp-filesystem package), not a vendored submodule.

gtest_discover_tests(runStarTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/StarDS/tests/)
