import { execFile } from "child_process";
import { promisify } from "util";
import { writeFileSync, mkdirSync, rmSync } from "fs";
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
    rawRequest?: string;   // compressed prompt sent to the API
    rawResponse?: string;  // full Anthropic JSON response
  };
  raw: {
    response: string;
    inputTokens: number;
    outputTokens: number;
    rawRequest?: string;   // full uncompressed prompt sent to the API
    rawResponse?: string;  // full Anthropic JSON response
  };
}

// ── Helpers ───────────────────────────────────────────────────────────────────

function parseMinikentStdout(stdout: string): {
  metrics: Partial<MiniKenMetrics>;
  response: string;
  rawRequest?: string;
  rawResponse?: string;
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

  const promptMatch = stdout.match(
    /=== MINIKEN_COMPRESSED_PROMPT_BEGIN ===\n([\s\S]*?)\n=== MINIKEN_COMPRESSED_PROMPT_END ===/
  );
  const rawRequest = promptMatch ? promptMatch[1] : undefined;

  const jsonMatch = stdout.match(
    /=== MINIKEN_RAW_API_RESPONSE_BEGIN ===\n([\s\S]*?)\n=== MINIKEN_RAW_API_RESPONSE_END ===/
  );
  const rawResponse = jsonMatch ? jsonMatch[1] : undefined;

  const respMatch = stdout.match(/=== miniKen Response ===\n([\s\S]*)/);
  const response = respMatch ? respMatch[1].trim() : stdout.trim();

  return { metrics, response, rawRequest, rawResponse };
}

function buildRawPrompt(query: string, files: QueryFile[]): string {
  let prompt = "USER QUERY:\n" + query + "\n\n";
  for (const f of files) {
    prompt += `[FILE: ${f.name}]\n${f.content}\n`;
  }
  return prompt;
}

// ── miniKen path ──────────────────────────────────────────────────────────────

export async function runMiniKen(
  query: string,
  tempPaths: string[]
): Promise<{ response: string; metrics: Partial<MiniKenMetrics>; rawRequest?: string; rawResponse?: string }> {
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

export async function runRaw(
  query: string,
  files: QueryFile[]
): Promise<{ response: string; inputTokens: number; outputTokens: number; rawRequest?: string; rawResponse?: string }> {
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

  const requestBody: Anthropic.MessageCreateParamsNonStreaming = {
    model,
    max_tokens: 8192,
    messages: [{ role: "user", content: prompt }],
  };

  try {
    const msg = await client.messages.create(requestBody);

    const response = msg.content
      .filter((b) => b.type === "text")
      .map((b) => (b as Anthropic.TextBlock).text)
      .join("\n");

    return {
      response,
      inputTokens:  msg.usage.input_tokens,
      outputTokens: msg.usage.output_tokens,
      rawRequest:   JSON.stringify(requestBody, null, 2),
      rawResponse:  JSON.stringify(msg, null, 2),
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

// ── Temp file helper ──────────────────────────────────────────────────────────

export function writeTempFiles(
  files: QueryFile[]
): { tempPaths: string[]; cleanup: () => void } {
  // Use a unique temp directory so each file keeps its original name.
  // The binary strips the directory prefix, leaving only the bare filename
  // visible to the model (and in the decompressed output).
  const tmpDir = join(tmpdir(), `mk_web_${Date.now()}`);
  mkdirSync(tmpDir, { recursive: true });

  const tempPaths: string[] = [];
  for (const f of files) {
    const safeName = f.name.replace(/[^a-zA-Z0-9._-]/g, "_");
    const tmpPath  = join(tmpDir, safeName);
    writeFileSync(tmpPath, f.content, "utf-8");
    tempPaths.push(tmpPath);
  }

  return {
    tempPaths,
    cleanup: () => { try { rmSync(tmpDir, { recursive: true, force: true }); } catch { /* ignore */ } },
  };
}

// ── Public entry point ────────────────────────────────────────────────────────

export async function processQuery(
  query: string,
  files: QueryFile[]
): Promise<QueryResult> {
  const { tempPaths, cleanup } = writeTempFiles(files);
  try {
    const [miniKenResult, rawResult] = await Promise.all([
      runMiniKen(query, tempPaths),
      runRaw(query, files),
    ]);
    return { miniKen: miniKenResult, raw: rawResult };
  } finally {
    cleanup();
  }
}
