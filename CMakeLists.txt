cmake_minimum_required(VERSION 3.20)
project(ChessTutor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Portable chess code. Must never include Emscripten or browser headers.
add_library(core
    core/version.cpp
)
target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR})
target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic)

# WebAssembly module for the browser. Output goes straight into the web app's source tree.
if(EMSCRIPTEN)
    add_executable(chess_wasm bindings/bindings.cpp)
    target_link_libraries(chess_wasm PRIVATE core)
    target_compile_options(chess_wasm PRIVATE -Wall -Wextra -Wpedantic)
    target_link_options(chess_wasm PRIVATE
        -lembind
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sENVIRONMENT=web,worker
        -sALLOW_MEMORY_GROWTH=1
        --emit-tsd chess.d.ts
    )
    set_target_properties(chess_wasm PROPERTIES
        OUTPUT_NAME chess
        SUFFIX .js
        RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/web/src/wasm
    )
endif()

# Tests and benchmarks run natively only.
if(NOT EMSCRIPTEN)
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG        v3.9.1
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(Catch2)

    enable_testing()
    add_executable(tests
        tests/test_version.cpp
    )
    target_link_libraries(tests PRIVATE core Catch2::Catch2WithMain)
    target_compile_options(tests PRIVATE -Wall -Wextra -Wpedantic)

    include(Catch)
    catch_discover_tests(tests)
endif()
