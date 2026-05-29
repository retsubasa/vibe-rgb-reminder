#!/usr/bin/env bash
set -euo pipefail

project_dir="$(cd "$(dirname "$0")" && pwd)"
label="${VIBE_RGB_LAUNCH_LABEL:-com.retsubasa.vibe-rgb-reminder.bridge}"
launch_agents_dir="${HOME}/Library/LaunchAgents"
plist="${launch_agents_dir}/${label}.plist"
uid="$(id -u)"
node_bin="${NODE_BIN:-$(command -v node)}"
esp32_port="${ESP32_PORT:-/dev/cu.usbmodem11101}"
esp32_baud="${ESP32_BAUD:-115200}"
hook_led_port="${HOOK_LED_PORT:-7317}"
bridge_script="${project_dir}/host/esp32-usb-led-bridge.mjs"

stop_matching_bridge_listener() {
  local pid command

  while IFS= read -r pid; do
    [ -n "$pid" ] || continue
    command="$(ps -p "$pid" -o command= 2>/dev/null || true)"
    case "$command" in
      *"$bridge_script"*|*"esp32-usb-led-bridge.mjs"*)
        echo "Stopping old ESP32 USB LED bridge process on port ${hook_led_port}: ${pid}"
        kill "$pid" 2>/dev/null || true
        ;;
      *)
        echo "Port ${hook_led_port} is already used by another process, not killing it: ${command}" >&2
        ;;
    esac
  done < <(lsof -tiTCP:"${hook_led_port}" -sTCP:LISTEN 2>/dev/null || true)

  for _ in 1 2 3 4 5; do
    if ! lsof -tiTCP:"${hook_led_port}" -sTCP:LISTEN >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.2
  done

  echo "Port ${hook_led_port} is still occupied; bridge was not started." >&2
  return 1
}

mkdir -p "$launch_agents_dir"

launchctl bootout "gui/${uid}" "$plist" 2>/dev/null || true
stop_matching_bridge_listener

cat > "$plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN"
  "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
  <string>${label}</string>

  <key>ProgramArguments</key>
  <array>
    <string>${node_bin}</string>
    <string>${bridge_script}</string>
  </array>

  <key>WorkingDirectory</key>
  <string>${project_dir}</string>

  <key>EnvironmentVariables</key>
  <dict>
    <key>ESP32_PORT</key>
    <string>${esp32_port}</string>
    <key>ESP32_BAUD</key>
    <string>${esp32_baud}</string>
    <key>HOOK_LED_PORT</key>
    <string>${hook_led_port}</string>
    <key>PATH</key>
    <string>/usr/local/bin:/opt/homebrew/bin:/usr/bin:/bin:/usr/sbin:/sbin</string>
  </dict>

  <key>RunAtLoad</key>
  <true/>

  <key>KeepAlive</key>
  <dict>
    <key>SuccessfulExit</key>
    <false/>
  </dict>

  <key>ThrottleInterval</key>
  <integer>10</integer>

  <key>StandardOutPath</key>
  <string>/tmp/esp32-usb-led-bridge.log</string>

  <key>StandardErrorPath</key>
  <string>/tmp/esp32-usb-led-bridge.err.log</string>
</dict>
</plist>
PLIST

launchctl bootstrap "gui/${uid}" "$plist"
launchctl enable "gui/${uid}/${label}"

echo "ESP32 USB LED bridge started with LaunchAgent: ${label}"
echo "Health: http://127.0.0.1:${hook_led_port}/health"
echo "Log: /tmp/esp32-usb-led-bridge.log"
