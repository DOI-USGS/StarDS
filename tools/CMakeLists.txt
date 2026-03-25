cmake_minimum_required(VERSION 3.10)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build scs_inspect tool
add_executable(scs_inspect ${CMAKE_SOURCE_DIR}/scs_inspect.cpp)

target_link_libraries(scs_inspect
                      PRIVATE
                      CameraStateFile
                      )

target_include_directories(scs_inspect PRIVATE ${CMAKE_SOURCE_DIR}/CameraStateFile/include/)

# Optional: Install the tool
install(TARGETS scs_inspect
        RUNTIME DESTINATION bin)

message(STATUS "Building scs_inspect tool")
