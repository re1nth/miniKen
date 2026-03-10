import express, { Request, Response } from "express";
import { join } from "path";
import { processQuery, QueryFile } from "./queryHandler";

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

  app.listen(port, () => {
    process.stderr.write(
      `[miniKen] Web interface running → http://localhost:${port}\n`
    );
  });
}
