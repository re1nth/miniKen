# miniKen

**Coding tasks in Eco mode** — compress your source code before it reaches the model, decompress the response before it reaches you. Same results, fewer tokens.

<img width="2533" height="1219" alt="Screenshot 2026-03-10 at 9 28 34 PM" src="https://github.com/user-attachments/assets/1a664386-31e2-4b37-bbcb-958202506901" />

---

## Results

Measured live against the Anthropic API (`claude-sonnet-4-6`), same query ("Summarise each function in one sentence") across all trials. Token counts are exact: `/v1/messages/count_tokens` for the uncompressed prompt, `usage.input_tokens` from the actual API response for the compressed prompt — no estimation.

All test files are the same verbose order-processing domain: classes named `CustomerPurchaseOrder`, `OrderLineItem`; functions named `calculateOrderTotalBeforeDiscount`, `applyPromotionalDiscountToOrder`, `validatePurchaseOrderFields`, `confirmAndFinaliseCustomerOrder`, `cancelCustomerOrderWithReason`.

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

## Languages Supported

- C
- CPP

**System requirements**

| Dependency | Install |
|---|---|
| CMake ≥ 3.16 | `brew install cmake` / `sudo apt install cmake` |
| C++17 compiler | `xcode-select --install` (macOS) / `sudo apt install build-essential` (Ubuntu) |
| tree-sitter | `brew install tree-sitter` / `sudo apt install libtree-sitter-dev` |
| Node.js v18+ | https://nodejs.org |
| git | `brew install git` / `sudo apt install git` |
