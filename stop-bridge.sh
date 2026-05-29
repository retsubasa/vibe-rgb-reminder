#!/usr/bin/env bash
set -euo pipefail

label="${VIBE_RGB_LAUNCH_LABEL:-com.retsubasa.vibe-rgb-reminder.bridge}"
launch_agents_dir="${HOME}/Library/LaunchAgents"
uid="$(id -u)"
hook_led_port="${HOOK_LED_PORT:-7317}"

launchctl bootout "gui/${uid}" "${launch_agents_dir}/${label}.plist" 2>/dev/null || true

while IFS= read -r pid; do
  [ -n "$pid" ] || continue
  command="$(ps -p "$pid" -o command= 2>/dev/null || true)"
  case "$command" in
    *"esp32-usb-led-bridge.mjs"*)
      kill "$pid" 2>/dev/null || true
      ;;
  esac
done < <(lsof -tiTCP:"${hook_led_port}" -sTCP:LISTEN 2>/dev/null || true)

echo "ESP32 USB LED bridge stopped."
