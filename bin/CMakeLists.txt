cmake_minimum_required(VERSION 3.10)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build starls tool
add_executable(starls ${CMAKE_CURRENT_SOURCE_DIR}/starls.cpp)

target_link_libraries(starls
                      PRIVATE
                      Star
                      )

target_include_directories(starls PRIVATE ${CMAKE_SOURCE_DIR}/StarDS/include/)

# Optional: Install the tool
install(TARGETS starls
        RUNTIME DESTINATION bin)

message(STATUS "Building starls tool")

###########################
# star_translate tool      #
###########################

option(STAR_BUILD_TRANSLATE "Build the star_translate format conversion tool" ON)

if(STAR_BUILD_TRANSLATE)
    message(STATUS "Building star_translate tool")

    # nlohmann/json is required and available in submodules
    add_subdirectory(${CMAKE_SOURCE_DIR}/submodules/json ${CMAKE_BINARY_DIR}/json EXCLUDE_FROM_ALL)

    # Check for optional msgpack support
    find_package(msgpack QUIET)

    add_executable(star_translate ${CMAKE_CURRENT_SOURCE_DIR}/star_translate.cpp)

    target_link_libraries(star_translate
                          PRIVATE
                          Star
                          nlohmann_json::nlohmann_json
                          )

    target_include_directories(star_translate PRIVATE
                               ${CMAKE_SOURCE_DIR}/StarDS/include/)

    # Add msgpack support if available
    if(msgpack_FOUND)
        message(STATUS "  MessagePack support: enabled")
        target_link_libraries(star_translate PRIVATE msgpackc-cxx)
        target_compile_definitions(star_translate PRIVATE ENABLE_MSGPACK)
    else()
        message(STATUS "  MessagePack support: disabled (msgpack not found)")
    endif()

    # Install the tool
    install(TARGETS star_translate
            RUNTIME DESTINATION bin)

    # Optional: Build round-trip test
    if(STAR_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/test_translate_roundtrip.cpp")
        add_executable(test_translate_roundtrip ${CMAKE_CURRENT_SOURCE_DIR}/test_translate_roundtrip.cpp)
        target_link_libraries(test_translate_roundtrip
                              PRIVATE
                              Star)
        target_include_directories(test_translate_roundtrip PRIVATE
                                   ${CMAKE_SOURCE_DIR}/StarDS/include/)
        message(STATUS "  Building translate round-trip test")
    endif()

    # Optional: Build JSON conversion example
    message(STATUS "  Building JSON conversion example")

    # Add validation test to CTest
    if(STAR_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/validate_star_translate.sh")
        add_test(NAME star_translate_validation
                 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/validate_star_translate.sh
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties(star_translate_validation PROPERTIES
                             LABELS "integration"
                             TIMEOUT 300)
        message(STATUS "  Added star_translate validation test to CTest")
    endif()
else()
    message(STATUS "Skipping star_translate tool")
endif()
