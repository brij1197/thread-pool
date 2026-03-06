include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.2
)
FetchContent_MakeAvailable(Catch2)

#   Test Executable
add_executable(tests
    test_thread_pool.cpp
)

target_link_libraries(tests
    PRIVATE
        threadpool
        Catch2::Catch2WithMain
)

target_compile_options(tests PRIVATE
    $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:-Wall -Wextra -Wpedantic>
)

#   Register with CTest
include(Catch)
catch_discover_tests(tests)