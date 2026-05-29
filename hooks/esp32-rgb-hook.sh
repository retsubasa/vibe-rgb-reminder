#!/usr/bin/env bash
set -u

HOOK_LED_URL="${HOOK_LED_URL:-http://127.0.0.1:7317/event}"
HOOK_LED_LOG="${HOOK_LED_LOG:-/tmp/vibe-rgb-claude-hook.log}"
payload="$(cat 2>/dev/null || true)"

curl_output="$(
  curl -sS \
  --max-time 0.8 \
  -X POST \
  -H "Content-Type: application/json" \
  --data "$payload" \
  "$HOOK_LED_URL" 2>&1
)"
curl_status=$?

{
  printf '%s event=%s curl_status=%s url=%s\n' \
    "$(date '+%Y-%m-%dT%H:%M:%S%z')" \
    "$(printf '%s' "$payload" | sed -n 's/.*"hook_event_name"[[:space:]]*:[[:space:]]*"\([^"]*\)".*/\1/p')" \
    "$curl_status" \
    "$HOOK_LED_URL"
  if [ "$curl_status" -ne 0 ] && [ -n "$curl_output" ]; then
    printf '  curl: %s\n' "$curl_output"
  fi
} >>"$HOOK_LED_LOG" 2>/dev/null || true

exit 0
