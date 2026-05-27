#!/usr/bin/env bash
set -euo pipefail

HOOK_LED_URL="${HOOK_LED_URL:-http://127.0.0.1:7317/event}"
payload="$(cat)"

curl -sS \
  --max-time 0.8 \
  -X POST \
  -H "Content-Type: application/json" \
  --data "$payload" \
  "$HOOK_LED_URL" >/dev/null || true

# Codex hooks may interpret stdout as hook output. An empty JSON object is
# intentionally inert across all event types, including Stop.
printf '{}\n'

exit 0
