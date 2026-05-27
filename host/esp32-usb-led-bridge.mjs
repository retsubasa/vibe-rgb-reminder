#!/usr/bin/env node
import fs from "node:fs";
import http from "node:http";
import { execFileSync } from "node:child_process";

const serialPath = process.env.ESP32_PORT || "/dev/cu.usbmodem11101";
const httpPort = Number(process.env.HOOK_LED_PORT || 7317);
const baudRate = process.env.ESP32_BAUD || "115200";

function setupSerial() {
  execFileSync("stty", ["-f", serialPath, baudRate, "raw", "-echo", "-hupcl"]);
  return fs.createWriteStream(serialPath, { flags: "a" });
}

let serial = setupSerial();

function reopenSerial() {
  try {
    serial.destroy();
  } catch {
    // Best effort only.
  }
  serial = setupSerial();
}

function eventFromHookPayload(payload) {
  const tool =
    payload.tool_name ||
    payload.toolName ||
    payload.tool?.name ||
    payload.tool?.tool_name ||
    "";

  return {
    event: String(payload.hook_event_name || payload.event_name || payload.event || payload.name || "Unknown"),
    tool: String(tool),
    cwd: String(payload.cwd || ""),
    session: String(payload.session_id || ""),
    ts: Date.now(),
  };
}

function writeEvent(event) {
  const line = `${JSON.stringify(event)}\n`;

  if (!serial.write(line)) {
    return new Promise((resolve, reject) => {
      serial.once("drain", resolve);
      serial.once("error", reject);
    });
  }

  return Promise.resolve();
}

async function collectBody(req) {
  const chunks = [];
  let size = 0;

  for await (const chunk of req) {
    size += chunk.length;
    if (size > 1024 * 1024) {
      throw new Error("request body too large");
    }
    chunks.push(chunk);
  }

  return Buffer.concat(chunks).toString("utf8");
}

const server = http.createServer(async (req, res) => {
  if (req.method === "GET" && req.url === "/health") {
    res.writeHead(200, { "content-type": "application/json" });
    res.end(JSON.stringify({ ok: true, serialPath, httpPort }));
    return;
  }

  if (req.method !== "POST" || req.url !== "/event") {
    res.writeHead(404, { "content-type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: "not found" }));
    return;
  }

  try {
    const body = await collectBody(req);
    const payload = body.trim() ? JSON.parse(body) : {};
    const event = eventFromHookPayload(payload);

    await writeEvent(event);

    res.writeHead(200, { "content-type": "application/json" });
    res.end(JSON.stringify({ ok: true, sent: event }));
  } catch (error) {
    console.error("[bridge] write failed:", error.message);

    try {
      reopenSerial();
    } catch (reopenError) {
      console.error("[bridge] reopen failed:", reopenError.message);
    }

    res.writeHead(500, { "content-type": "application/json" });
    res.end(JSON.stringify({ ok: false, error: error.message }));
  }
});

server.listen(httpPort, "127.0.0.1", () => {
  console.log(`[bridge] listening on http://127.0.0.1:${httpPort}`);
  console.log(`[bridge] writing events to ${serialPath} at ${baudRate} baud`);
});

for (const signal of ["SIGINT", "SIGTERM"]) {
  process.on(signal, () => {
    server.close();
    serial.end();
    process.exit(0);
  });
}
