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

## Quick start

```bash
export ANTHROPIC_API_KEY="sk-ant-..."
./start.sh
```

That single command handles everything:

1. Verifies system prerequisites (cmake, C++ compiler, Node.js ≥ 18, npm, git)
2. Detects your tree-sitter installation
3. Clones and builds the `tree-sitter-cpp` grammar (cached in `/tmp` after the first run)
4. Builds the `miniken` binary via cmake (incremental — fast on repeat runs)
5. Runs `npm install` + `tsc` inside `mcp-server/`
6. Starts the server

Once running, open **http://localhost:4040** for the web interface.

**System requirements**

| Dependency | Install |
|---|---|
| CMake ≥ 3.16 | `brew install cmake` / `sudo apt install cmake` |
| C++17 compiler | `xcode-select --install` (macOS) / `sudo apt install build-essential` (Ubuntu) |
| tree-sitter | `brew install tree-sitter` / `sudo apt install libtree-sitter-dev` |
| Node.js v18+ | https://nodejs.org |
| git | `brew install git` / `sudo apt install git` |

**Options**

```bash
# Custom port (default 4040)
./start.sh --port 8080

# API key inline (if not already exported)
ANTHROPIC_API_KEY=sk-ant-... ./start.sh

# Override the model (default: claude-sonnet-4-6)
MINIKEN_MODEL=claude-haiku-4-5-20251001 ./start.sh
```

---

## Web interface

The web interface launches alongside the server at **http://localhost:4040**.

- Type a question in the input bar
- Click **＋ Files** to attach source files (`.cpp`, `.py`, `.java`, `.js`, and more)
- Press **Send** or `Cmd+Enter`
- Both responses appear side by side:
  - **Left** — miniKen compressed path (compress → model → decompress)
  - **Right** — Raw uncompressed path (original files sent directly)
- A metrics bar shows the exact tokens saved (or added) for that query

---

## CLI

The `miniken` binary is also available directly after running `./start.sh` at least once:

```bash
export ANTHROPIC_API_KEY="sk-ant-..."

# Ask a question about a file
./testbed_build/miniken "What does each function do?" src/order.cpp

# Request a code change — modified files are decompressed before you see them
./testbed_build/miniken \
  "Add an applyFlatShippingFee function that adds a fee to totalAmountAfterDiscount" \
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
# Run all suites via the helper script (builds first if needed):
bash testbed/tests/run_all.sh

# Or run individual suites directly from the build directory:
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

mcp-server/                    MCP server + web interface backend
  src/index.ts                 entry point: starts MCP (stdio) + web server (HTTP)
  src/webServer.ts             Express server — serves UI and POST /api/query
  src/queryHandler.ts          dual-path handler (miniKen binary + raw Anthropic SDK)
  public/index.html            web UI (chat, file attach, side-by-side comparison)
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

start.sh                       one-shot setup + launch (prerequisites → build → run)
install.sh                     registers the MCP server with Claude Code
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

### Install

Run `./start.sh` once (see [Quick start](#quick-start)) — it builds everything. Then register the MCP server with Claude Code:

```bash
./install.sh
```

`install.sh` patches `~/.claude/settings.json` to register the server. Then **restart Claude Code** (or run `/mcp` in the CLI to reload servers).

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
