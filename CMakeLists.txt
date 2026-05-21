cmake_minimum_required(VERSION 3.16)
project(DSE CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BUILD_TYPE Release)

find_package(OpenSSL REQUIRED)

add_compile_options( -O3  -g -march=native -flto -Wall -Wextra)

add_subdirectory(third_party/quill)

add_executable(dse
    src/main.cpp
    src/TcpHandler.cpp
    src/logging_object.cpp
    src/matching_engine.cpp
    src/spsc_queue.cpp
    src/tls_server.cpp
)

target_include_directories(dse PRIVATE include)
target_link_libraries(dse PRIVATE quill::quill OpenSSL::SSL OpenSSL::Crypto)


