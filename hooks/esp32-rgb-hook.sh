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

exit 0
