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
    parser/profiles/cProfile.cpp
    parser/profiles/cppProfile.cpp
    parser/profiles/goProfile.cpp
    parser/profiles/pythonProfile.cpp
    parser/languageRegistry.cpp
    parser/querySquasher.cpp
    parser/responseBuilder.cpp
    orchestrator.cpp
)

target_include_directories(miniKen_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# ---------------------------------------------------------------------------
# tree-sitter core + required grammars (C and C++)
#
# Set TREE_SITTER_ROOT to the install prefix if libraries are not in the
# default search paths, e.g.:
#   cmake -DTREE_SITTER_ROOT=/opt/homebrew ..
# ---------------------------------------------------------------------------
find_library(TREE_SITTER_LIB
    NAMES tree-sitter
    HINTS /usr/local/lib /usr/lib ${TREE_SITTER_ROOT}/lib
)

find_library(TREE_SITTER_C_LIB
    NAMES tree-sitter-c
    HINTS /usr/local/lib /usr/lib /opt/homebrew/lib ${TREE_SITTER_ROOT}/lib
)

find_library(TREE_SITTER_CPP_LIB
    NAMES tree-sitter-cpp
    HINTS /usr/local/lib /usr/lib /opt/homebrew/lib ${TREE_SITTER_ROOT}/lib
)

find_path(TREE_SITTER_INCLUDE
    NAMES tree_sitter/api.h
    HINTS /usr/local/include /usr/include /opt/homebrew/include ${TREE_SITTER_ROOT}/include
)

if(TREE_SITTER_LIB AND TREE_SITTER_C_LIB AND TREE_SITTER_CPP_LIB AND TREE_SITTER_INCLUDE)
    message(STATUS "tree-sitter found: ${TREE_SITTER_LIB}")
    message(STATUS "tree-sitter-c found: ${TREE_SITTER_C_LIB}")
    message(STATUS "tree-sitter-cpp found: ${TREE_SITTER_CPP_LIB}")

    target_include_directories(miniKen_engine PRIVATE ${TREE_SITTER_INCLUDE})
    target_include_directories(miniKen_engine PUBLIC  ${TREE_SITTER_INCLUDE})
    target_link_libraries(miniKen_engine PUBLIC
        ${TREE_SITTER_LIB}
        ${TREE_SITTER_C_LIB}
        ${TREE_SITTER_CPP_LIB}
    )
else()
    message(WARNING
        "tree-sitter libraries not found. "
        "Set TREE_SITTER_ROOT to the installation prefix, e.g.:\n"
        "  cmake -DTREE_SITTER_ROOT=/opt/homebrew ..\n"
        "The library will not link until these are resolved.")
endif()

# ---------------------------------------------------------------------------
# Optional grammars — enable additional language support when installed.
# Install via your package manager or build from source, then re-run cmake.
#
# Go:     brew install tree-sitter  (then build tree-sitter-go separately)
# Python: brew install tree-sitter  (then build tree-sitter-python separately)
# ---------------------------------------------------------------------------
find_library(TREE_SITTER_GO_LIB
    NAMES tree-sitter-go
    HINTS /usr/local/lib /usr/lib /opt/homebrew/lib ${TREE_SITTER_ROOT}/lib
)

if(TREE_SITTER_GO_LIB)
    message(STATUS "tree-sitter-go found: ${TREE_SITTER_GO_LIB} — Go support enabled")
    target_link_libraries(miniKen_engine PUBLIC ${TREE_SITTER_GO_LIB})
    target_compile_definitions(miniKen_engine PRIVATE HAVE_TREE_SITTER_GO)
else()
    message(STATUS "tree-sitter-go not found — Go support disabled (pass-through mode)")
endif()

find_library(TREE_SITTER_PYTHON_LIB
    NAMES tree-sitter-python
    HINTS /usr/local/lib /usr/lib /opt/homebrew/lib ${TREE_SITTER_ROOT}/lib
)

if(TREE_SITTER_PYTHON_LIB)
    message(STATUS "tree-sitter-python found: ${TREE_SITTER_PYTHON_LIB} — Python support enabled")
    target_link_libraries(miniKen_engine PUBLIC ${TREE_SITTER_PYTHON_LIB})
    target_compile_definitions(miniKen_engine PRIVATE HAVE_TREE_SITTER_PYTHON)
else()
    message(STATUS "tree-sitter-python not found — Python support disabled (pass-through mode)")
endif()
