import express, { Request, Response } from "express";
import { join } from "path";
import { processQuery, runMiniKen, runRaw, writeTempFiles, QueryFile } from "./queryHandler";

export function startWebServer(port = 4040): void {
  const app = express();

  app.use(express.json({ limit: "50mb" }));
  app.use(express.static(join(__dirname, "../public")));

  app.post(
    "/api/query",
    async (req: Request, res: Response): Promise<void> => {
      const { query, files } = req.body as {
        query: string;
        files: QueryFile[];
      };

      if (typeof query !== "string" || !Array.isArray(files)) {
        res.status(400).json({ error: "query (string) and files (array) are required." });
        return;
      }

      try {
        const result = await processQuery(query, files);
        res.json(result);
      } catch (err: unknown) {
        const e = err as { message: string };
        res.status(500).json({ error: e.message });
      }
    }
  );

  app.post(
    "/api/query/stream",
    async (req: Request, res: Response): Promise<void> => {
      const { query, files } = req.body as { query: string; files: QueryFile[] };

      if (typeof query !== "string" || !Array.isArray(files)) {
        res.status(400).json({ error: "query (string) and files (array) are required." });
        return;
      }

      res.setHeader("Content-Type", "text/event-stream");
      res.setHeader("Cache-Control", "no-cache");
      res.setHeader("Connection", "keep-alive");

      const send = (event: string, data: unknown) => {
        res.write(`event: ${event}\ndata: ${JSON.stringify(data)}\n\n`);
      };

      const DELAY_MS = 65_000;

      const { tempPaths, cleanup } = writeTempFiles(files);
      try {
        const miniKenResult = await runMiniKen(query, tempPaths);
        send("miniken", miniKenResult);

        send("countdown", { seconds: DELAY_MS / 1000 });
        await new Promise(r => setTimeout(r, DELAY_MS));

        const rawResult = await runRaw(query, files);
        send("raw", rawResult);

        send("done", {});
      } catch (err: unknown) {
        const e = err as { message: string };
        send("error", { message: e.message });
      } finally {
        cleanup();
        res.end();
      }
    }
  );

  app.listen(port, () => {
    process.stderr.write(
      `[miniKen] Web interface running → http://localhost:${port}\n`
    );
  });
}
