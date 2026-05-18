cmake_minimum_required(VERSION 3.14)
project(coroutine_sim)

set(CMAKE_CXX_STANDARD 20)
include(FetchContent)

FetchContent_Declare(fschuetz04_simcpp20
        GIT_REPOSITORY https://github.com/fschuetz04/simcpp20
        GIT_TAG        v0.3.1)

FetchContent_MakeAvailable(fschuetz04_simcpp20)

FetchContent_Declare(magic_enum
        GIT_REPOSITORY https://github.com/Neargye/magic_enum
        GIT_TAG v0.9.7)
FetchContent_MakeAvailable(magic_enum)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2  # or latest stable
)
FetchContent_MakeAvailable(googletest)

enable_testing()
include(GoogleTest)

find_program(IWYU_PATH NAMES include-what-you-use iwyu)
if(IWYU_PATH)
    set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE
            "${IWYU_PATH};-Xiwyu;--no_fwd_decls"
    )
    message(STATUS "include-what-you-use enabled: ${IWYU_PATH}")
else()
    message(STATUS "include-what-you-use not found, skipping")
endif()

add_subdirectory(src)
add_subdirectory(test)