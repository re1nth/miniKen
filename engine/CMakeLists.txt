cmake_minimum_required(VERSION 3.16)
project(miniKen_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Source files
# ---------------------------------------------------------------------------
add_library(miniKen_engine STATIC
    sessionStore/sessionMapObject.cpp
    compressor/commentOut.cpp
    compressor/compactBlock.cpp
    decompressor/relaxCommentOut.cpp
    decompressor/relaxCompactBlock.cpp
    decompressor/relaxTextual.cpp
    parser/cppProfile.cpp
    parser/querySquasher.cpp
    parser/responseBuilder.cpp
    orchestrator.cpp
)

target_include_directories(miniKen_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# tree-sitter dependencies
#
# Expects tree-sitter core and the C++ grammar to be installed or available
# via pkg-config / manual paths.  Adjust TREE_SITTER_ROOT if needed.
# ---------------------------------------------------------------------------
find_library(TREE_SITTER_LIB
    NAMES tree-sitter
    HINTS /usr/local/lib /usr/lib ${TREE_SITTER_ROOT}/lib
)

find_library(TREE_SITTER_CPP_LIB
    NAMES tree-sitter-cpp
    HINTS /usr/local/lib /usr/lib ${TREE_SITTER_ROOT}/lib
)

find_path(TREE_SITTER_INCLUDE
    NAMES tree_sitter/api.h
    HINTS /usr/local/include /usr/include ${TREE_SITTER_ROOT}/include
)

if(TREE_SITTER_LIB AND TREE_SITTER_CPP_LIB AND TREE_SITTER_INCLUDE)
    message(STATUS "tree-sitter found: ${TREE_SITTER_LIB}")
    message(STATUS "tree-sitter-cpp found: ${TREE_SITTER_CPP_LIB}")

    target_include_directories(miniKen_engine PRIVATE ${TREE_SITTER_INCLUDE})
    target_link_libraries(miniKen_engine PUBLIC
        ${TREE_SITTER_LIB}
        ${TREE_SITTER_CPP_LIB}
    )
    target_include_directories(miniKen_engine PUBLIC ${TREE_SITTER_INCLUDE})
else()
    message(WARNING
        "tree-sitter or tree-sitter-cpp not found. "
        "Set TREE_SITTER_ROOT to the installation prefix, e.g.:\n"
        "  cmake -DTREE_SITTER_ROOT=/opt/homebrew .. \n"
        "The library will not link until these are resolved.")
endif()
