#!/usr/bin/env bash
# run_all.sh — Build (if needed) and run all miniKen test suites.
#
# ── Prerequisites ─────────────────────────────────────────────────────────────
# 1. Build tree-sitter-cpp (not on Homebrew; must be compiled from source):
#
#      git clone --depth 1 https://github.com/tree-sitter/tree-sitter-cpp /tmp/tree-sitter-cpp
#      cc -O2 -c /tmp/tree-sitter-cpp/src/parser.c  -o /tmp/ts_cpp_parser.o
#      cc -O2 -c /tmp/tree-sitter-cpp/src/scanner.c -o /tmp/ts_cpp_scanner.o
#      ar rcs /tmp/libtree-sitter-cpp.a /tmp/ts_cpp_parser.o /tmp/ts_cpp_scanner.o
#
# 2. Configure the testbed (only needed once):
#
#      cmake -S <repo>/testbed -B <repo>/testbed/build \
#            -DTREE_SITTER_ROOT=/opt/homebrew \
#            -DTREE_SITTER_CPP_LIB=/tmp/libtree-sitter-cpp.a
#
#    Replace /opt/homebrew with your tree-sitter install prefix if different.
#    On Linux with system packages the flag may not be needed at all.
#
# ── Running the tests ─────────────────────────────────────────────────────────
#      # From the repo root:
#      bash testbed/tests/run_all.sh
#
#      # Or from this directory:
#      ./run_all.sh
#
#      # Point at a custom build directory:
#      ./run_all.sh /path/to/build
# ─────────────────────────────────────────────────────────────────────────────

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
TESTBED_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

# ── Resolve build dir ─────────────────────────────────────────────────────────
BUILD_DIR="${1:-$TESTBED_DIR/build}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found: $BUILD_DIR"
    echo "Run cmake first, e.g.:"
    echo "  cmake -S $TESTBED_DIR -B $BUILD_DIR -DTREE_SITTER_ROOT=/opt/homebrew"
    exit 1
fi

# ── Build ─────────────────────────────────────────────────────────────────────
echo "Building..."
cmake --build "$BUILD_DIR"
echo ""

# ── Run suites ────────────────────────────────────────────────────────────────
SUITES=(
    test_sessionMapObject
    test_compressor
    test_decompressor
    test_parser
    test_orchestrator
)

total_pass=0
total_fail=0
failed_suites=()

for suite in "${SUITES[@]}"; do
    binary="$BUILD_DIR/$suite"
    if [ ! -x "$binary" ]; then
        echo "  SKIP  $suite  (binary not found: $binary)"
        continue
    fi

    echo "══════ $suite ══════"
    set +e
    "$binary"
    exit_code=$?
    set -e

    if [ $exit_code -ne 0 ]; then
        failed_suites+=("$suite")
        total_fail=$((total_fail + 1))
    else
        total_pass=$((total_pass + 1))
    fi
    echo ""
done

# ── Summary ───────────────────────────────────────────────────────────────────
echo "────────────────────────────────────────────────────────"
echo "Suites passed : $total_pass / $((total_pass + total_fail))"

if [ ${#failed_suites[@]} -gt 0 ]; then
    echo "Suites FAILED : ${failed_suites[*]}"
    exit 1
else
    echo "All suites passed."
fi
