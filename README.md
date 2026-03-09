# miniKen

**Coding tasks in Eco mode** — compress your source code before it reaches the model, decompress the response before it reaches you. Same results, fewer tokens.

---

## Results

Measured live against the Anthropic API (`claude-sonnet-4-6`), same query ("Summarise each function in one sentence") across all trials. Token counts are exact: `/v1/messages/count_tokens` for the uncompressed prompt, `usage.input_tokens` from the actual API response for the compressed prompt — no estimation.

All test files are the same verbose order-processing domain: classes named `CustomerPurchaseOrder`, `OrderLineItem`; functions named `calculateOrderTotalBeforeDiscount`, `applyPromotionalDiscountToOrder`, `validatePurchaseOrderFields`, `confirmAndFinaliseCustomerOrder`, `cancelCustomerOrderWithReason`.

### C++ (parser implemented)

| Files | Without compression | With compression | Saved |
|---|---|---|---|
| `original.cpp` — with doc comments (1 file) | 1,129 | 795 | **334 (29%)** |
| `decompressed.cpp` — same code, no comments (1 file) | 938 | 795 | **143 (15%)** |
| `original.cpp` + `decompressed.cpp` (2 files) | 1,778 | 1,301 | **477 (26%)** |

The gap between `original.cpp` (29%) and `decompressed.cpp` (15%) isolates the two sources of savings: comment stripping accounts for ~14 percentage points on this file; identifier compression accounts for the remaining 15 pp and applies to all files regardless of comments.

### Other languages (parsers pending)

These files contain identically verbose code in Java, Python, and JavaScript. With no language profile implemented yet they pass through unmodified — 0% savings — establishing the baseline that the C++ results are measured against.

| Files | Without compression | With compression | Saved |
|---|---|---|---|
| `original.java` | 1,314 | 1,314 | 0% |
| `original.py` | 1,133 | 1,133 | 0% |
| `original.js` | 1,242 | 1,242 | 0% |

Adding a language profile for any of these is a one-file change (see [Language support](#language-support)).

### Per-call metrics

Every call emits an exact metrics block, visible in the CLI and inline in Claude Code conversations:

```
--- miniKen Token Metrics ---
Without compression: 1129 tokens
With compression:    795 tokens
Tokens saved:        334  (29%)
Output tokens:       349
-----------------------------
```

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

mcp-server/                    Claude Code MCP integration
  src/index.ts                 MCP server (eco_query, eco_mode_toggle, eco_status)
  package.json
  tsconfig.json

testbed/
  cli/miniken.cpp              command-line driver
  stages/
    cpp/                       C++ original / compressed / decompressed
    python/                    Python  ─┐  stage trios showing each
    java/                      Java    ─┤  pipeline stage for each
    javascript/                JS      ─┘  language
  tests/                       92 unit + integration tests
  performance/benchmark.cpp    token-reduction benchmark

install.sh                     one-shot Claude Code MCP setup
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

## Claude Code integration (MCP)

miniKen ships an MCP server so Claude Code can call it as a native tool.

**Prerequisites:** Node.js v18+, the `testbed_build/miniken` binary (see [Build](#build) above).

### Install

```bash
./install.sh
```

That's it. The script:
1. Installs npm dependencies inside `mcp-server/`
2. Compiles the TypeScript server
3. Patches `~/.claude/settings.json` to register the server

Then **restart Claude Code** (or run `/mcp` in the CLI to reload servers).

### Tools

| Tool | Description |
|---|---|
| `eco_status` | Check whether eco mode is currently enabled |
| `eco_mode_toggle` | Turn eco mode on or off (persists across sessions) |
| `eco_query` | Compress files → query model → decompress response |

### Usage inside Claude Code

```
> Turn on eco mode
> eco_query: what does calculateDiscount do? [attach src/order.cpp]
```

With eco mode on you can instruct Claude to use `eco_query` instead of reading files directly, keeping its context window leaner.

### Manual registration (alternative to install.sh)

Add this to `~/.claude/settings.json` under `mcpServers`:

```json
{
  "mcpServers": {
    "miniken": {
      "command": "node",
      "args": ["/absolute/path/to/miniKen/mcp-server/dist/index.js"]
    }
  }
}
```

---

## Other integrations

**VSCode** — a sidebar extension that lets you pick files, type a query, and see the decompressed response alongside compression statistics (not yet implemented).

**Cursor** — not recommended; Cursor has no public API for intercepting its own LLM requests cleanly.

---

## Architecture notes

- **Session isolation** — every call gets a fresh `SessionMapObject`; no global state, safe for concurrent use.
- **Two-pass compression** — Pass 1 registers all definitions with the correct type prefix; Pass 2 replaces every occurrence (definitions and call sites alike).
- **Lossless round-trip** — comments are keyed to their compact symbol and re-attached during decompression; identifier mapping is bijective within a session.
- **No comments in model output** — the system prompt instructs the model not to add comments to returned files, keeping the decompressor's job clean.
