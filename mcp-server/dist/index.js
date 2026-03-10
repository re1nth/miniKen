"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const index_js_1 = require("@modelcontextprotocol/sdk/server/index.js");
const stdio_js_1 = require("@modelcontextprotocol/sdk/server/stdio.js");
const webServer_1 = require("./webServer");
const types_js_1 = require("@modelcontextprotocol/sdk/types.js");
const child_process_1 = require("child_process");
const util_1 = require("util");
const fs_1 = require("fs");
const os_1 = require("os");
const path_1 = require("path");
const execFileAsync = (0, util_1.promisify)(child_process_1.execFile);
// ── State ────────────────────────────────────────────────────────────────────
const STATE_DIR = (0, path_1.join)((0, os_1.homedir)(), ".miniken");
const STATE_FILE = (0, path_1.join)(STATE_DIR, "state.json");
function loadState() {
    if (!(0, fs_1.existsSync)(STATE_FILE))
        return { ecoMode: false };
    return JSON.parse((0, fs_1.readFileSync)(STATE_FILE, "utf-8"));
}
function saveState(state) {
    (0, fs_1.mkdirSync)(STATE_DIR, { recursive: true });
    (0, fs_1.writeFileSync)(STATE_FILE, JSON.stringify(state, null, 2));
}
// ── Binary path ──────────────────────────────────────────────────────────────
// mcp-server/dist/index.js  →  ../../testbed_build/miniken
const BINARY = (0, path_1.resolve)(__dirname, "../../testbed_build/miniken");
// ── MCP server ───────────────────────────────────────────────────────────────
const server = new index_js_1.Server({ name: "miniken", version: "1.0.0" }, { capabilities: { tools: {} } });
server.setRequestHandler(types_js_1.ListToolsRequestSchema, async () => ({
    tools: [
        {
            name: "eco_query",
            description: "Send a token-efficient query about your code. miniKen compresses the " +
                "source files (replacing long identifiers with short symbols), forwards " +
                "the query to Claude, then decompresses the response before returning it. " +
                "Use this when working with large codebases to save context-window tokens.",
            inputSchema: {
                type: "object",
                properties: {
                    query: {
                        type: "string",
                        description: "The question or instruction about the code.",
                    },
                    file_paths: {
                        type: "array",
                        items: { type: "string" },
                        description: "Absolute paths to source files to include in the query.",
                    },
                },
                required: ["query", "file_paths"],
            },
        },
        {
            name: "eco_mode_toggle",
            description: "Enable or disable eco mode. When enabled, prefer eco_query for any " +
                "task that reads source files — it uses ~47% fewer tokens.",
            inputSchema: {
                type: "object",
                properties: {
                    enabled: {
                        type: "boolean",
                        description: "true to enable eco mode, false to disable.",
                    },
                },
                required: ["enabled"],
            },
        },
        {
            name: "eco_status",
            description: "Report whether eco mode is currently enabled.",
            inputSchema: {
                type: "object",
                properties: {},
            },
        },
    ],
}));
server.setRequestHandler(types_js_1.CallToolRequestSchema, async (request) => {
    const { name, arguments: args } = request.params;
    // ── eco_status ─────────────────────────────────────────────────────────────
    if (name === "eco_status") {
        const { ecoMode } = loadState();
        return {
            content: [
                {
                    type: "text",
                    text: ecoMode
                        ? "Eco mode is ENABLED. Use eco_query for code analysis."
                        : "Eco mode is DISABLED. Standard mode active.",
                },
            ],
        };
    }
    // ── eco_mode_toggle ────────────────────────────────────────────────────────
    if (name === "eco_mode_toggle") {
        const enabled = args.enabled;
        saveState({ ecoMode: enabled });
        return {
            content: [
                {
                    type: "text",
                    text: enabled
                        ? "Eco mode ENABLED — use eco_query to analyse code with fewer tokens."
                        : "Eco mode DISABLED — back to standard mode.",
                },
            ],
        };
    }
    // ── eco_query ──────────────────────────────────────────────────────────────
    if (name === "eco_query") {
        const { query, file_paths } = args;
        if (!(0, fs_1.existsSync)(BINARY)) {
            return {
                content: [
                    {
                        type: "text",
                        text: `miniKen binary not found at: ${BINARY}\n\n` +
                            "Build it first:\n" +
                            "  cd testbed && cmake -B ../testbed_build . && cmake --build ../testbed_build",
                    },
                ],
                isError: true,
            };
        }
        if (file_paths.length === 0) {
            return {
                content: [{ type: "text", text: "Error: file_paths must not be empty." }],
                isError: true,
            };
        }
        try {
            const { stdout } = await execFileAsync(BINARY, [query, ...file_paths], {
                env: process.env,
                maxBuffer: 10 * 1024 * 1024, // 10 MB
            });
            return { content: [{ type: "text", text: stdout }] };
        }
        catch (err) {
            const e = err;
            return {
                content: [
                    {
                        type: "text",
                        text: `eco_query failed: ${e.message}\n${e.stderr ?? ""}`,
                    },
                ],
                isError: true,
            };
        }
    }
    throw new Error(`Unknown tool: ${name}`);
});
// ── Start ─────────────────────────────────────────────────────────────────────
async function main() {
    (0, webServer_1.startWebServer)(4040);
    const transport = new stdio_js_1.StdioServerTransport();
    await server.connect(transport);
}
main().catch((err) => {
    process.stderr.write(`miniKen MCP server error: ${err}\n`);
    process.exit(1);
});
