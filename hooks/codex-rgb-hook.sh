#!/usr/bin/env bash
set -euo pipefail

HOOK_LED_URL="${HOOK_LED_URL:-http://127.0.0.1:7317/event}"
payload="$(cat)"

normalized_payload="$(PAYLOAD="$payload" node <<'NODE'
const payload = process.env.PAYLOAD || "";
let data;

try {
  data = payload.trim() ? JSON.parse(payload) : {};
} catch {
  data = { raw: payload };
}

const text = JSON.stringify(data).toLowerCase();
const reconnecting =
  text.includes("reconnecting...") ||
  text.includes("stream disconnected before completion") ||
  text.includes("upstream request failed") ||
  text.includes("responsestreamdisconnected") ||
  text.includes("upstream failure");

if (reconnecting) {
  data.hook_event_name = "StopFailure";
  data.event_name = "StopFailure";
  data.tool_name = data.tool_name || "codex_reconnect";
  data.vibe_rgb_reason = "codex_reconnecting_or_upstream_failure";
}

process.stdout.write(JSON.stringify(data));
NODE
)"

curl -sS \
  --max-time 0.8 \
  -X POST \
  -H "Content-Type: application/json" \
  --data "$normalized_payload" \
  "$HOOK_LED_URL" >/dev/null || true

# Codex hooks may interpret stdout as hook output. An empty JSON object is
# intentionally inert across all event types, including Stop.
printf '{}\n'

exit 0
