import { Server } from "@modelcontextprotocol/sdk/server/index.js";
import { StdioServerTransport } from "@modelcontextprotocol/sdk/server/stdio.js";
import {
  CallToolRequestSchema,
  ListToolsRequestSchema,
} from "@modelcontextprotocol/sdk/types.js";
import { execFile } from "child_process";
import { promisify } from "util";
import { readFileSync, writeFileSync, mkdirSync, existsSync } from "fs";
import { homedir } from "os";
import { join, resolve } from "path";

const execFileAsync = promisify(execFile);

// ── State ────────────────────────────────────────────────────────────────────

const STATE_DIR = join(homedir(), ".miniken");
const STATE_FILE = join(STATE_DIR, "state.json");

interface State {
  ecoMode: boolean;
}

function loadState(): State {
  if (!existsSync(STATE_FILE)) return { ecoMode: false };
  return JSON.parse(readFileSync(STATE_FILE, "utf-8")) as State;
}

function saveState(state: State): void {
  mkdirSync(STATE_DIR, { recursive: true });
  writeFileSync(STATE_FILE, JSON.stringify(state, null, 2));
}

// ── Binary path ──────────────────────────────────────────────────────────────
// mcp-server/dist/index.js  →  ../../testbed_build/miniken

const BINARY = resolve(__dirname, "../../testbed_build/miniken");

// ── MCP server ───────────────────────────────────────────────────────────────

const server = new Server(
  { name: "miniken", version: "1.0.0" },
  { capabilities: { tools: {} } }
);

server.setRequestHandler(ListToolsRequestSchema, async () => ({
  tools: [
    {
      name: "eco_query",
      description:
        "Send a token-efficient query about your code. miniKen compresses the " +
        "source files (replacing long identifiers with short symbols), forwards " +
        "the query to Claude, then decompresses the response before returning it. " +
        "Use this when working with large codebases to save context-window tokens.",
      inputSchema: {
        type: "object" as const,
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
      description:
        "Enable or disable eco mode. When enabled, prefer eco_query for any " +
        "task that reads source files — it uses ~47% fewer tokens.",
      inputSchema: {
        type: "object" as const,
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
        type: "object" as const,
        properties: {},
      },
    },
  ],
}));

server.setRequestHandler(CallToolRequestSchema, async (request) => {
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
    const enabled = (args as { enabled: boolean }).enabled;
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
    const { query, file_paths } = args as {
      query: string;
      file_paths: string[];
    };

    if (!existsSync(BINARY)) {
      return {
        content: [
          {
            type: "text",
            text:
              `miniKen binary not found at: ${BINARY}\n\n` +
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
    } catch (err: unknown) {
      const e = err as { message: string; stderr?: string };
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
  const transport = new StdioServerTransport();
  await server.connect(transport);
}

main().catch((err) => {
  process.stderr.write(`miniKen MCP server error: ${err}\n`);
  process.exit(1);
});
