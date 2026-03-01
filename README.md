# miniKen

**Coding tasks in Eco mode** — compress your source code before it reaches the model, decompress the response before it reaches you. Same results, fewer tokens.

---

## How it works

```
Your Code
    │
    ▼
[Compressor]  two-pass tree-sitter parse
    │           • strips comments (stores them for later)
    │           • replaces every identifier with a compact symbol
    │             getUserBalance → f1   CustomerOrder → C1
    │             accountId → v3        #include <map> → p2
    ▼
Compressed Context  (~47% fewer tokens, ~65% fewer characters)
    │
    ▼
[ Model ]  works in compact-symbol space
    │
    ▼
Compressed Response
    │
    ▼
[Decompressor]  reverse-maps symbols, restores comments
    ▼
Patched Code  — your original identifiers, fully restored
```

All compression and decompression runs locally on your machine. The model never sees verbose identifiers; you never see compact symbols.

---

## Build

**Prerequisites:** CMake ≥ 3.16, a C++17 compiler, tree-sitter, tree-sitter-cpp.

```bash
cmake -S engine -B build
cmake --build build

# If tree-sitter is not on the default path:
cmake -S engine -B build -DTREE_SITTER_ROOT=/opt/homebrew
cmake --build build

# Build the full testbed (tests + CLI + benchmark):
cmake -S testbed -B testbed_build -DTREE_SITTER_ROOT=/opt/homebrew
cmake --build testbed_build
```

---

## Usage

### CLI

```bash
export ANTHROPIC_API_KEY="sk-ant-..."

# Ask a question about a file
./testbed_build/miniken "What does each function do?" src/order.cpp

# Request a code change — modified files are decompressed before you see them
./testbed_build/miniken \
  "Add a applyFlatShippingFee function that adds a fee to totalAmountAfterDiscount" \
  src/order.cpp src/types.h

# Use a cheaper/faster model for quick queries
MINIKEN_MODEL=claude-haiku-4-5-20251001 \
  ./testbed_build/miniken "Summarise this file" src/order.cpp
```

`MINIKEN_MODEL` defaults to `claude-sonnet-4-6`. Any Anthropic model ID is accepted.

### Stage generator

See exactly what the model receives and what comes back:

```bash
./testbed_build/generate_stages testbed/stages/cpp/original.cpp /tmp/out
# writes /tmp/out/compressed.cpp and /tmp/out/decompressed.cpp
```

### Benchmark

```bash
./testbed_build/benchmark
```

---

## Performance

Results across five representative C++ samples:

| Sample | Original tokens | Compressed tokens | Reduction |
|---|---|---|---|
| Single utility function | 37 | 27 | 27% |
| Small class with methods | 71 | 28 | 61% |
| Two collaborating classes | 114 | 53 | 54% |
| Authentication module | 164 | 99 | 40% |
| Payment processing pipeline | 141 | 64 | 55% |
| **Average** | | | **47% tokens / 65% chars** |

---

## Tests

```bash
cd testbed_build
./test_sessionMapObject   # 27 tests
./test_compressor         # 16 tests
./test_decompressor       # 23 tests
./test_parser             # 18 tests  (requires tree-sitter)
./test_orchestrator       # 8 tests
# 92 total
```

---

## Project layout

```
engine/
  orchestrator.h/.cpp          entry point: Orchestrator::run()
  parser/
    languageProfile.h          LanguageProfile struct (language-neutral)
    cppProfile.h/.cpp          C++ grammar + AST node-type knowledge
    querySquasher.h/.cpp       two-pass compressor
    responseBuilder.h/.cpp     decompressor
  compressor/
    commentOut.h/.cpp          strips and stores comment blocks
    compactBlock.h/.cpp        maps long names → compact symbols
  decompressor/
    relaxCommentOut.h/.cpp     restores comment blocks
    relaxCompactBlock.h/.cpp   restores identifiers in source
    relaxTextual.h/.cpp        expands {{symbol}} markers in LLM text
  sessionStore/
    sessionMapObject.h/.cpp    in-memory symbol maps + counters
  static/
    prompt_format              system prompt sent to the model

testbed/
  cli/miniken.cpp              command-line driver
  stages/
    cpp/                       C++ original / compressed / decompressed
    python/                    Python  ─┐  stage trios showing each
    java/                      Java    ─┤  pipeline stage for each
    javascript/                JS      ─┘  language
  tests/                       92 unit + integration tests
  performance/benchmark.cpp    token-reduction benchmark
```

---

## Language support

The parser layer is language-agnostic. All language knowledge lives in a `LanguageProfile` struct — which AST node types are definitions, which are identifiers, which are declarator wrappers to descend into. Adding a new language means writing one profile file; nothing else changes.

| Language | Status |
|---|---|
| C++ | Implemented (`cppProfile.cpp`) |
| Python | Stage files ready; profile pending |
| Java | Stage files ready; profile pending |
| JavaScript | Stage files ready; profile pending |

---

## Integration

**Claude Code (recommended)** — build a small MCP server that wraps the `miniken` binary and exposes a `miniken_query(question, files)` tool. Claude Code calls it automatically when working on C++ files.

**VSCode** — a sidebar extension that lets you pick files, type a query, and see the decompressed response alongside compression statistics.

**Cursor** — not recommended; Cursor has no API for intercepting its own LLM requests cleanly.

---

## Architecture notes

- **Session isolation** — every call gets a fresh `SessionMapObject`; no global state, safe for concurrent use.
- **Two-pass compression** — Pass 1 registers all definitions with the correct type prefix; Pass 2 replaces every occurrence (definitions and call sites alike).
- **Lossless round-trip** — comments are keyed to their compact symbol and re-attached during decompression; identifier mapping is bijective within a session.
- **No comments in model output** — the system prompt instructs the model not to add comments to returned files, keeping the decompressor's job clean.
