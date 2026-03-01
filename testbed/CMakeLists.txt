cmake_minimum_required(VERSION 3.16)
project(miniKen_testbed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Inherit TREE_SITTER_ROOT from command line or environment.
set(TREE_SITTER_ROOT "${TREE_SITTER_ROOT}" CACHE PATH "tree-sitter installation root")

# Pull in the engine (and its tree-sitter dependency transitively).
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../engine
                 ${CMAKE_CURRENT_BINARY_DIR}/engine_build)

# ─── Helper: add one lifecycle test executable ────────────────────────────────
function(add_lc_test target src)
    add_executable(${target} tests/${src})
    target_link_libraries(${target} PRIVATE miniKen_engine)
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}           # testRunner.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../engine # engine headers
    )
endfunction()

# ─── Lifecycle test executables ───────────────────────────────────────────────
# (tests/ directory, one binary per module group)
add_lc_test(test_sessionMapObject  test_sessionMapObject.cpp)
add_lc_test(test_compressor        test_compressor.cpp)
add_lc_test(test_decompressor      test_decompressor.cpp)
add_lc_test(test_parser            test_parser.cpp)
add_lc_test(test_orchestrator      test_orchestrator.cpp)

# ─── Stages presentation generator ──────────────────────────────────────────
add_executable(generate_stages stages/cpp/generate.cpp)
target_link_libraries(generate_stages PRIVATE miniKen_engine)
target_include_directories(generate_stages PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
)

# ─── Token-performance benchmark ─────────────────────────────────────────────
add_executable(benchmark performance/benchmark.cpp)
target_link_libraries(benchmark PRIVATE miniKen_engine)
target_include_directories(benchmark PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
)

# ─── CLI driver ───────────────────────────────────────────────────────────────
add_executable(miniken cli/miniken.cpp)
target_link_libraries(miniken PRIVATE miniKen_engine)
target_include_directories(miniken PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../engine
)
