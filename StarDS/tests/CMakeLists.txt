cmake_minimum_required(VERSION 3.10)

# A lot of linux OSes do not support C++20
set(CMAKE_CXX_STANDARD 20)

get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "dir='${dir}'")
endforeach()
set(STARDS_TEST_DIRECTORY ${CMAKE_SOURCE_DIR}/StarDS/tests/)

# collect all of the test sources (UnitTestSCS.cpp removed - used old API)
set (STARDS_TEST_SOURCE    ${STARDS_TEST_DIRECTORY}/UnitTestBlockCompression.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestNdarray.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestMetadataBlock.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestMetadataAPI.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestIntegration.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestSlicing.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestFileHeader.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestPerformance.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestThreading.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestCloseReopenAPI.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestConcurrentWrites.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestLayers.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestReadmeExamples.cpp
                                    ${STARDS_TEST_DIRECTORY}/UnitTestBytesAPI.cpp
                                    ${STARDS_TEST_DIRECTORY}/Fixtures.cpp
                            )

# /vsicurl (HTTP) mock-server tests need CURL support.
if(STARDS_ENABLE_CURL)
    list(APPEND STARDS_TEST_SOURCE ${STARDS_TEST_DIRECTORY}/UnitTestVsiCurl.cpp)
    message(STATUS "Including /vsicurl mock-server tests")
endif()

# Add S3 tests if S3 support is enabled
if(STARDS_ENABLE_S3)
    list(APPEND STARDS_TEST_SOURCE ${STARDS_TEST_DIRECTORY}/UnitTestS3.cpp
                                 ${STARDS_TEST_DIRECTORY}/UnitTestVsiS3Mock.cpp)
    message(STATUS "Including S3 unit tests (real + mock-server)")
endif()

# Conversion parity test: exercises stards_translate across every compression codec
# and checks the decoded data matches. Only meaningful when the tool is built and
# the example JSON exists (the test self-skips otherwise).
set(STARDS_PARITY_JSON "${CMAKE_SOURCE_DIR}/StarDS/examples/mroctx_isd.json")
if(STARDS_BUILD_TOOLS AND STARDS_BUILD_TRANSLATE AND EXISTS "${STARDS_PARITY_JSON}")
    list(APPEND STARDS_TEST_SOURCE ${STARDS_TEST_DIRECTORY}/UnitTestConversionParity.cpp)
    message(STATUS "Including conversion-parity test")
endif()

# setup test executable
add_executable(runStarDSTests TestMain.cpp ${STARDS_TEST_SOURCE})

target_compile_definitions(runStarDSTests PUBLIC SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE)

# Point the parity test at the built stards_translate binary and the example JSON.
if(STARDS_BUILD_TOOLS AND STARDS_BUILD_TRANSLATE AND EXISTS "${STARDS_PARITY_JSON}")
    target_compile_definitions(runStarDSTests PRIVATE
        STARDS_TRANSLATE_EXE="$<TARGET_FILE:stards_translate>"
        STARDS_EXAMPLE_JSON="${STARDS_PARITY_JSON}")
    add_dependencies(runStarDSTests stards_translate)
endif()

target_link_libraries(runStarDSTests
                      PRIVATE
                      StarDS
                      gtest
                      gmock
                      Threads::Threads
                      )

# Note: ghc::filesystem (<ghc/fs_std.hpp>) is provided by the toolchain/conda
# include path (the cpp-filesystem package), not a vendored submodule.

gtest_discover_tests(runStarDSTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/StarDS/tests/)
