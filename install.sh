#!/usr/bin/env bash
# install.sh — one-shot setup for the miniKen MCP server in Claude Code
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
MCP_DIR="$SCRIPT_DIR/mcp-server"
SETTINGS="$HOME/.claude/settings.json"

echo "=== miniKen MCP Setup ==="
echo ""

# ── 1. Check prerequisites ───────────────────────────────────────────────────
if ! command -v node &>/dev/null; then
  echo "Error: Node.js not found. Install from https://nodejs.org (v18+)"
  exit 1
fi

NODE_MAJOR=$(node -e "process.stdout.write(process.version.split('.')[0].slice(1))")
if [ "$NODE_MAJOR" -lt 18 ]; then
  echo "Error: Node.js v18+ required (found $(node --version))"
  exit 1
fi

if ! command -v npm &>/dev/null; then
  echo "Error: npm not found. It ships with Node.js — reinstall from https://nodejs.org"
  exit 1
fi

# ── 2. Build the MCP server ──────────────────────────────────────────────────
echo "Installing npm dependencies..."
cd "$MCP_DIR"
npm install --silent

echo "Compiling TypeScript..."
npm run build

echo ""

# ── 3. Patch ~/.claude/settings.json ─────────────────────────────────────────
mkdir -p "$(dirname "$SETTINGS")"
[ -f "$SETTINGS" ] || echo '{}' > "$SETTINGS"

node - "$SETTINGS" "$MCP_DIR" <<'NODE'
const fs   = require("fs");
const path = require("path");

const [,, settingsPath, mcpDir] = process.argv;
const settings = JSON.parse(fs.readFileSync(settingsPath, "utf-8"));

settings.mcpServers            = settings.mcpServers || {};
settings.mcpServers["miniken"] = {
  command: "node",
  args:    [path.join(mcpDir, "dist", "index.js")]
};

fs.writeFileSync(settingsPath, JSON.stringify(settings, null, 2) + "\n");
console.log("Patched " + settingsPath);
NODE

# ── 4. Done ───────────────────────────────────────────────────────────────────
echo ""
echo "=== Done! ==="
echo ""
echo "Restart Claude Code (or run /mcp in the CLI) to activate the server."
echo ""
echo "Available tools:"
echo "  eco_status        — check whether eco mode is on"
echo "  eco_mode_toggle   — turn eco mode on / off"
echo "  eco_query         — token-efficient code query (compress → model → decompress)"
echo ""
echo "Try it:"
echo "  > Turn on eco mode"
echo "  > eco_query: what does this function do? [drag a file in]"
