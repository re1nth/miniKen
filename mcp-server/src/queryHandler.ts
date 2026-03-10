import { execFile } from "child_process";
import { promisify } from "util";
import { writeFileSync, unlinkSync } from "fs";
import { tmpdir } from "os";
import { join, resolve } from "path";
import Anthropic from "@anthropic-ai/sdk";

const execFileAsync = promisify(execFile);

// Path to the compiled miniKen binary (same as the MCP server uses).
const BINARY = resolve(__dirname, "../../testbed_build/miniken");

// ── Public types ──────────────────────────────────────────────────────────────

export interface QueryFile {
  name: string;
  content: string;
}

export interface MiniKenMetrics {
  withoutCompression: number;
  withCompression: number;
  tokensSaved: number;
  savingsPct: number;
  outputTokens: number;
}

export interface QueryResult {
  miniKen: {
    response: string;
    metrics: Partial<MiniKenMetrics>;
  };
  raw: {
    response: string;
    inputTokens: number;
    outputTokens: number;
  };
}

// ── Helpers ───────────────────────────────────────────────────────────────────

function parseMinikentStdout(stdout: string): {
  metrics: Partial<MiniKenMetrics>;
  response: string;
} {
  const metrics: Partial<MiniKenMetrics> = {};

  const m1 = stdout.match(/Without compression:\s*(\d+)/);
  if (m1) metrics.withoutCompression = parseInt(m1[1]);

  const m2 = stdout.match(/With compression:\s*(\d+)/);
  if (m2) metrics.withCompression = parseInt(m2[1]);

  const m3 = stdout.match(/Tokens saved:\s*(-?\d+)\s*\(\s*(-?\d+)%\)/);
  if (m3) {
    metrics.tokensSaved = parseInt(m3[1]);
    metrics.savingsPct  = parseInt(m3[2]);
  }

  const m4 = stdout.match(/Output tokens:\s*(\d+)/);
  if (m4) metrics.outputTokens = parseInt(m4[1]);

  const respMatch = stdout.match(/=== miniKen Response ===\n([\s\S]*)/);
  const response = respMatch ? respMatch[1].trim() : stdout.trim();

  return { metrics, response };
}

function buildRawPrompt(query: string, files: QueryFile[]): string {
  let prompt = "USER QUERY:\n" + query + "\n\n";
  for (const f of files) {
    prompt += `[FILE: ${f.name}]\n${f.content}\n`;
  }
  return prompt;
}

// ── miniKen path ──────────────────────────────────────────────────────────────

async function runMiniKen(
  query: string,
  tempPaths: string[]
): Promise<{ response: string; metrics: Partial<MiniKenMetrics> }> {
  try {
    const { stdout } = await execFileAsync(BINARY, [query, ...tempPaths], {
      env: process.env,
      maxBuffer: 10 * 1024 * 1024,
    });
    return parseMinikentStdout(stdout);
  } catch (err: unknown) {
    const e = err as { message: string; stdout?: string };
    const parsed = parseMinikentStdout(e.stdout ?? "");
    if (parsed.response) return parsed;
    return { response: `miniKen error: ${e.message}`, metrics: {} };
  }
}

// ── Raw (uncompressed) path ───────────────────────────────────────────────────

async function runRaw(
  query: string,
  files: QueryFile[]
): Promise<{ response: string; inputTokens: number; outputTokens: number }> {
  const apiKey = process.env.ANTHROPIC_API_KEY;
  if (!apiKey) {
    return {
      response: "ANTHROPIC_API_KEY is not set — cannot make a raw API call.",
      inputTokens: 0,
      outputTokens: 0,
    };
  }

  const client  = new Anthropic({ apiKey });
  const model   = process.env.MINIKEN_MODEL ?? "claude-sonnet-4-6";
  const prompt  = buildRawPrompt(query, files);

  try {
    const msg = await client.messages.create({
      model,
      max_tokens: 8192,
      messages: [{ role: "user", content: prompt }],
    });

    const response = msg.content
      .filter((b) => b.type === "text")
      .map((b) => (b as Anthropic.TextBlock).text)
      .join("\n");

    return {
      response,
      inputTokens:  msg.usage.input_tokens,
      outputTokens: msg.usage.output_tokens,
    };
  } catch (err: unknown) {
    const e = err as { message: string };
    return {
      response: `Raw API error: ${e.message}`,
      inputTokens: 0,
      outputTokens: 0,
    };
  }
}

// ── Public entry point ────────────────────────────────────────────────────────

export async function processQuery(
  query: string,
  files: QueryFile[]
): Promise<QueryResult> {
  // Write files to named temp paths so the miniKen binary can read them.
  const tempPaths: string[] = [];
  const cleanups:  (() => void)[] = [];

  try {
    for (const f of files) {
      const safeName = f.name.replace(/[^a-zA-Z0-9._-]/g, "_");
      const tmpPath  = join(tmpdir(), `mk_web_${Date.now()}_${safeName}`);
      writeFileSync(tmpPath, f.content, "utf-8");
      tempPaths.push(tmpPath);
      cleanups.push(() => { try { unlinkSync(tmpPath); } catch { /* ignore */ } });
    }

    const [miniKenResult, rawResult] = await Promise.all([
      runMiniKen(query, tempPaths),
      runRaw(query, files),
    ]);

    return { miniKen: miniKenResult, raw: rawResult };
  } finally {
    for (const cleanup of cleanups) cleanup();
  }
}
