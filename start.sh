#!/usr/bin/env bash
# start.sh — one-shot setup and launch for miniKen
#
# Runs the first time OR on any subsequent run:
#   1. Checks system prerequisites
#   2. Detects tree-sitter install root
#   3. Builds tree-sitter-c and tree-sitter-cpp grammars (if missing)
#   4. Builds the miniKen binary (cmake / make, incremental)
#   5. Installs + builds the mcp-server (npm install, tsc)
#   6. Launches the server  →  http://localhost:4040
#
# Usage:
#   ./start.sh
#   ANTHROPIC_API_KEY=sk-ant-... ./start.sh
#   ./start.sh --port 4040

set -euo pipefail

REPO="$(cd "$(dirname "$0")" && pwd)"

# ── Colour helpers ─────────────────────────────────────────────────────────────
GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; BOLD='\033[1m'; NC='\033[0m'
ok()   { echo -e "${GREEN}  ✓  $*${NC}"; }
info() { echo -e "${BOLD}  →  $*${NC}"; }
warn() { echo -e "${YELLOW}  ⚠  $*${NC}"; }
die()  { echo -e "${RED}  ✗  $*${NC}"; exit 1; }

# ── CLI options ────────────────────────────────────────────────────────────────
PORT=4040
while [[ $# -gt 0 ]]; do
  case "$1" in
    --port) PORT="$2"; shift 2 ;;
    *)      die "Unknown argument: $1" ;;
  esac
done

echo ""
echo -e "${BOLD}╔══════════════════════════════╗${NC}"
echo -e "${BOLD}║       miniKen  start.sh      ║${NC}"
echo -e "${BOLD}╚══════════════════════════════╝${NC}"
echo ""

# ── 1. System prerequisites ────────────────────────────────────────────────────
info "Checking prerequisites…"

# CMake
if ! command -v cmake &>/dev/null; then
  die "cmake not found. Install it:
       macOS:   brew install cmake
       Ubuntu:  sudo apt install cmake"
fi
CMAKE_VER=$(cmake --version | head -1 | awk '{print $3}')
ok "cmake $CMAKE_VER"

# C++ compiler
if ! command -v cc &>/dev/null && ! command -v clang++ &>/dev/null && ! command -v g++ &>/dev/null; then
  die "No C++ compiler found. Install Xcode Command Line Tools (macOS: xcode-select --install) or build-essential (Ubuntu)."
fi
ok "C++ compiler"

# Node.js
if ! command -v node &>/dev/null; then
  die "Node.js not found. Install v18+ from https://nodejs.org"
fi
NODE_MAJOR=$(node -e "process.stdout.write(process.version.split('.')[0].slice(1))")
if [ "$NODE_MAJOR" -lt 18 ]; then
  die "Node.js v18+ required (found $(node --version))"
fi
ok "Node.js $(node --version)"

# npm
if ! command -v npm &>/dev/null; then
  die "npm not found. It ships with Node.js — reinstall from https://nodejs.org"
fi
ok "npm $(npm --version)"

# git (needed to clone tree-sitter-cpp grammar)
if ! command -v git &>/dev/null; then
  die "git not found. Install git."
fi
ok "git"

# ANTHROPIC_API_KEY
if [ -z "${ANTHROPIC_API_KEY:-}" ]; then
  warn "ANTHROPIC_API_KEY is not set."
  echo "       The web interface will launch but API calls will fail."
  echo "       Export it before running, or set it now:"
  echo -n "       Enter API key (or press Enter to skip): "
  read -r KEY_INPUT
  if [ -n "$KEY_INPUT" ]; then
    export ANTHROPIC_API_KEY="$KEY_INPUT"
    ok "ANTHROPIC_API_KEY set for this session"
  fi
else
  ok "ANTHROPIC_API_KEY is set"
fi

echo ""

# ── 2. Detect tree-sitter install root ────────────────────────────────────────
info "Locating tree-sitter…"

TS_ROOT=""
for candidate in /opt/homebrew /usr/local /usr; do
  if [ -f "$candidate/include/tree_sitter/api.h" ] && \
     ( [ -f "$candidate/lib/libtree-sitter.a" ] || [ -f "$candidate/lib/libtree-sitter.dylib" ] || [ -f "$candidate/lib/libtree-sitter.so" ] ); then
    TS_ROOT="$candidate"
    break
  fi
done

if [ -z "$TS_ROOT" ]; then
  die "tree-sitter not found. Install it:
       macOS:   brew install tree-sitter
       Ubuntu:  sudo apt install libtree-sitter-dev"
fi
ok "tree-sitter at $TS_ROOT"

# ── 3. Build tree-sitter-c and tree-sitter-cpp grammars ───────────────────────

# Helper: resolve a grammar library, using an installed copy if present,
# otherwise cloning + compiling from the official tree-sitter GitHub repo.
#
# Usage: build_ts_grammar <name> <out_lib_var> [scanner]
#   <name>        e.g. "tree-sitter-c" or "tree-sitter-cpp"
#   <out_lib_var> shell variable name to set with the resolved library path
#   [scanner]     pass "scanner" if the grammar has a scanner.c
build_ts_grammar() {
  local name="$1"
  local out_lib_var="$2"
  local has_scanner="${3:-}"
  
  # Use eval to set the variable passed by name
  eval "$out_lib_var=\"\""

  # Prefer a copy already installed under the tree-sitter root or Homebrew.
  for candidate in \
      "$TS_ROOT/lib/lib${name}.a" \
      "$TS_ROOT/lib/lib${name}.dylib" \
      "/opt/homebrew/lib/lib${name}.a" \
      "/opt/homebrew/lib/lib${name}.dylib"; do
    if [ -f "$candidate" ]; then
      out_lib="$candidate"
      ok "${name} found at $candidate"
      return
    fi
  done

  # Not installed — build from source into /tmp.
  local lib_path="/tmp/lib${name}.a"
  if [ -f "$lib_path" ]; then
    out_lib="$lib_path"
    ok "${name} already built ($lib_path)"
    return
  fi

  info "Building ${name} from source…"
  local src="/tmp/${name}"
  if [ ! -f "$src/src/parser.c" ]; then
    rm -rf "$src"
    git clone --quiet --depth 1 \
      "https://github.com/tree-sitter/${name}" "$src"
  fi

  cc -O2 -c "$src/src/parser.c" -o "/tmp/${name}_parser.o"
  local objs="/tmp/${name}_parser.o"

  if [ -n "$has_scanner" ]; then
    cc -O2 -c "$src/src/scanner.c" -o "/tmp/${name}_scanner.o"
    objs="$objs /tmp/${name}_scanner.o"
  fi

  ar rcs "$lib_path" $objs
  rm -f /tmp/${name}_parser.o /tmp/${name}_scanner.o 2>/dev/null || true

  out_lib="$lib_path"
  ok "${name} built ($lib_path)"
}

TS_C_LIB=""
TS_CPP_LIB=""

build_ts_grammar "tree-sitter-c"   TS_C_LIB
build_ts_grammar "tree-sitter-cpp" TS_CPP_LIB scanner

echo ""

# ── 4. Build the miniKen binary ───────────────────────────────────────────────
info "Building miniKen binary…"

BUILD_DIR="$REPO/testbed_build"

cmake -S "$REPO/testbed" -B "$BUILD_DIR" \
      -DTREE_SITTER_ROOT="$TS_ROOT" \
      -DTREE_SITTER_C_LIB="$TS_C_LIB" \
      -DTREE_SITTER_CPP_LIB="$TS_CPP_LIB" \
      2>&1 | grep -E "^(--|CMake Warning|CMake Error)" || true

cmake --build "$BUILD_DIR" --target miniken 2>&1 | \
  grep -E "(Building|Linking|Error|error:)" || true

BINARY="$BUILD_DIR/miniken"
if [ ! -x "$BINARY" ]; then
  die "miniKen binary not produced at $BINARY — check build output above."
fi
ok "miniKen binary ready  ($BINARY)"

echo ""

# ── 5. Install + build the mcp-server ────────────────────────────────────────
info "Installing mcp-server dependencies…"
cd "$REPO/mcp-server"
npm install --silent
ok "npm dependencies installed"

info "Compiling TypeScript…"
npm run build 2>&1 | grep -v "^$" || true
ok "TypeScript compiled"

echo ""

# ── 6. Launch ─────────────────────────────────────────────────────────────────
info "Starting miniKen server on port ${PORT}..."
echo ""
echo -e "${BOLD}  Web interface -> http://localhost:${PORT}${NC}"
echo ""
echo "  Press Ctrl+C to stop."
echo ""

cd "$REPO"
exec node mcp-server/dist/index.js
