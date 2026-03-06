cmake_minimum_required(VERSION 3.16)
project(ThreadPool
    VERSION 1.0.0
    DESCRIPTION "A simple thread pool implementation in C++"
    LANGUAGES CXX
)

#   Global Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#   Library Target
add_library(threadpool
    src/thread_pool.cpp
)

target_include_directories(threadpool
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

#   Link pthreads
find_package(Threads REQUIRED)
target_link_libraries(threadpool PUBLIC Threads::Threads)

#   Compiler Warnings
target_compile_options(threadpool PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang>:-Wall -Wextra -Wpedantic -Wshadow>
    $<$<CXX_COMPILER_ID:MSVC>:/W4>
)

#   Examples
option(THREADPOOL_BUILD_EXAMPLES "Build example programs" ON)
if(THREADPOOL_BUILD_EXAMPLES)
    add_executable(basic_usage examples/basic_usage.cpp)
    target_link_libraries(example PRIVATE threadpool)

    add_executable(parallel_transform examples/parallel_transform.cpp)
    target_link_libraries(parallel_transform PRIVATE threadpool)
endif()