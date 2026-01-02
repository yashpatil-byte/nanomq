cmake_minimum_required(VERSION 3.15)
project(NanoMQ VERSION 1.0.0 LANGUAGES CXX)

# C++17 required for memory model and atomic operations
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler flags for performance
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3 -march=native -mtune=native")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -fsanitize=address -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -flto")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Core library sources
set(CORE_SOURCES
    src/core/ring_buffer.cpp
    src/core/atomic_ops.cpp
    src/core/memory.cpp
    src/storage/mmap_file.cpp
    src/storage/wal.cpp
    src/storage/segment.cpp
    src/network/tcp_server.cpp
    src/network/tcp_client.cpp
    src/network/protocol.cpp
    src/broker/broker.cpp
    src/broker/topic.cpp
    src/broker/subscription.cpp
    src/api/publisher_impl.cpp
    src/api/subscriber_impl.cpp
    src/api/admin_server.cpp
)

# NanoMQ core library
add_library(nanomq STATIC ${CORE_SOURCES})
target_include_directories(nanomq PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(nanomq PUBLIC pthread)

# CLI tool
add_executable(nanomq-cli cli/main.cpp)
target_link_libraries(nanomq-cli PRIVATE nanomq)

# Broker server
add_executable(nanomq-broker src/broker/main.cpp)
target_link_libraries(nanomq-broker PRIVATE nanomq)

# Examples
add_executable(simple_pubsub examples/simple_pubsub.cpp)
target_link_libraries(simple_pubsub PRIVATE nanomq)

add_executable(batch_publish examples/batch_publish.cpp)
target_link_libraries(batch_publish PRIVATE nanomq)

add_executable(consumer_group examples/consumer_group.cpp)
target_link_libraries(consumer_group PRIVATE nanomq)

# Testing with Google Test
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    
    # Download and configure Google Test
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Test executables
    add_executable(test_ring_buffer tests/test_ring_buffer.cpp)
    target_link_libraries(test_ring_buffer PRIVATE nanomq gtest gtest_main)
    add_test(NAME test_ring_buffer COMMAND test_ring_buffer)
    
    add_executable(test_persistence tests/test_persistence.cpp)
    target_link_libraries(test_persistence PRIVATE nanomq gtest gtest_main)
    add_test(NAME test_persistence COMMAND test_persistence)
    
    add_executable(test_latency tests/test_latency.cpp)
    target_link_libraries(test_latency PRIVATE nanomq gtest gtest_main)
    add_test(NAME test_latency COMMAND test_latency)
    
    add_executable(test_throughput tests/test_throughput.cpp)
    target_link_libraries(test_throughput PRIVATE nanomq gtest gtest_main)
    add_test(NAME test_throughput COMMAND test_throughput)
endif()

# Benchmarks with Google Benchmark
option(BUILD_BENCHMARKS "Build benchmarks" ON)
if(BUILD_BENCHMARKS)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    FetchContent_MakeAvailable(googlebenchmark)
    
    add_executable(bench_latency benchmarks/bench_latency.cpp)
    target_link_libraries(bench_latency PRIVATE nanomq benchmark::benchmark)
    
    add_executable(bench_throughput benchmarks/bench_throughput.cpp)
    target_link_libraries(bench_throughput PRIVATE nanomq benchmark::benchmark)
    
    add_executable(bench_cpu benchmarks/bench_cpu.cpp)
    target_link_libraries(bench_cpu PRIVATE nanomq benchmark::benchmark)
endif()

# Installation
install(TARGETS nanomq nanomq-cli nanomq-broker
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)
install(DIRECTORY include/ DESTINATION include)

