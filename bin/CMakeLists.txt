cmake_minimum_required(VERSION 3.10)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build scsls tool
add_executable(scsls ${CMAKE_CURRENT_SOURCE_DIR}/scsls.cpp)

target_link_libraries(scsls
                      PRIVATE
                      CameraStateFile
                      )

target_include_directories(scsls PRIVATE ${CMAKE_SOURCE_DIR}/CameraStateFile/include/)

# Optional: Install the tool
install(TARGETS scsls
        RUNTIME DESTINATION bin)

message(STATUS "Building scsls tool")

###########################
# scs_translate tool      #
###########################

option(CAMERASTATEFILE_BUILD_TRANSLATE "Build the scs_translate format conversion tool" ON)

if(CAMERASTATEFILE_BUILD_TRANSLATE)
    message(STATUS "Building scs_translate tool")

    # nlohmann/json is required and available in submodules
    add_subdirectory(${CMAKE_SOURCE_DIR}/submodules/json ${CMAKE_BINARY_DIR}/json EXCLUDE_FROM_ALL)

    # Check for optional msgpack support
    find_package(msgpack QUIET)

    add_executable(scs_translate ${CMAKE_CURRENT_SOURCE_DIR}/scs_translate.cpp)

    target_link_libraries(scs_translate
                          PRIVATE
                          CameraStateFile
                          nlohmann_json::nlohmann_json
                          )

    target_include_directories(scs_translate PRIVATE
                               ${CMAKE_SOURCE_DIR}/CameraStateFile/include/)

    # Add msgpack support if available
    if(msgpack_FOUND)
        message(STATUS "  MessagePack support: enabled")
        target_link_libraries(scs_translate PRIVATE msgpackc-cxx)
        target_compile_definitions(scs_translate PRIVATE ENABLE_MSGPACK)
    else()
        message(STATUS "  MessagePack support: disabled (msgpack not found)")
    endif()

    # Install the tool
    install(TARGETS scs_translate
            RUNTIME DESTINATION bin)

    # Optional: Build round-trip test
    if(CAMERASTATEFILE_BUILD_TESTS)
        add_executable(test_translate_roundtrip ${CMAKE_CURRENT_SOURCE_DIR}/test_translate_roundtrip.cpp)
        target_link_libraries(test_translate_roundtrip
                              PRIVATE
                              CameraStateFile)
        target_include_directories(test_translate_roundtrip PRIVATE
                                   ${CMAKE_SOURCE_DIR}/CameraStateFile/include/)
        message(STATUS "  Building translate round-trip test")
    endif()

    # Optional: Build JSON conversion example
    message(STATUS "  Building JSON conversion example")

    # Add validation test to CTest
    if(CAMERASTATEFILE_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/validate_scs_translate.sh")
        add_test(NAME scs_translate_validation
                 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/validate_scs_translate.sh
                 WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
        set_tests_properties(scs_translate_validation PROPERTIES
                             LABELS "integration"
                             TIMEOUT 300)
        message(STATUS "  Added scs_translate validation test to CTest")
    endif()
else()
    message(STATUS "Skipping scs_translate tool")
endif()
