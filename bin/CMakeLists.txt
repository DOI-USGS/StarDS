cmake_minimum_required(VERSION 3.10)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build stardsls tool
add_executable(stardsls ${CMAKE_CURRENT_SOURCE_DIR}/stardsls.cpp)

target_link_libraries(stardsls
                      PRIVATE
                      StarDS
                      )

target_include_directories(stardsls PRIVATE ${CMAKE_SOURCE_DIR}/StarDS/include/)

# Optional: Install the tool
install(TARGETS stardsls
        RUNTIME DESTINATION bin)

message(STATUS "Building stardsls tool")

###########################
# stards_translate tool    #
###########################

option(STARDS_BUILD_TRANSLATE "Build the stards_translate format conversion tool" ON)

if(STARDS_BUILD_TRANSLATE)
    message(STATUS "Building stards_translate tool")

    # nlohmann/json is required and available in submodules
    add_subdirectory(${CMAKE_SOURCE_DIR}/submodules/json ${CMAKE_BINARY_DIR}/json EXCLUDE_FROM_ALL)

    # Check for optional msgpack support
    find_package(msgpack QUIET)

    add_executable(stards_translate ${CMAKE_CURRENT_SOURCE_DIR}/stards_translate.cpp)

    target_link_libraries(stards_translate
                          PRIVATE
                          StarDS
                          nlohmann_json::nlohmann_json
                          )

    target_include_directories(stards_translate PRIVATE
                               ${CMAKE_SOURCE_DIR}/StarDS/include/)

    # Add msgpack support if available
    if(msgpack_FOUND)
        message(STATUS "  MessagePack support: enabled")
        target_link_libraries(stards_translate PRIVATE msgpackc-cxx)
        target_compile_definitions(stards_translate PRIVATE ENABLE_MSGPACK)
    else()
        message(STATUS "  MessagePack support: disabled (msgpack not found)")
    endif()

    # Install the tool
    install(TARGETS stards_translate
            RUNTIME DESTINATION bin)

    # Optional: Build round-trip test
    if(STARDS_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_translate_roundtrip.cpp")
        add_executable(test_translate_roundtrip ${CMAKE_CURRENT_SOURCE_DIR}/test_translate_roundtrip.cpp)
        target_link_libraries(test_translate_roundtrip
                              PRIVATE
                              StarDS)
        target_include_directories(test_translate_roundtrip PRIVATE
                                   ${CMAKE_SOURCE_DIR}/StarDS/include/)
        message(STATUS "  Building translate round-trip test")
    endif()

    # Optional: Build JSON conversion example
    message(STATUS "  Building JSON conversion example")

    # Add validation test to CTest
    if(STARDS_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/validate_stards_translate.sh")
        add_test(NAME stards_translate_validation
                 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/validate_stards_translate.sh
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties(stards_translate_validation PROPERTIES
                             LABELS "integration"
                             TIMEOUT 300)
        message(STATUS "  Added stards_translate validation test to CTest")
    endif()
else()
    message(STATUS "Skipping stards_translate tool")
endif()
