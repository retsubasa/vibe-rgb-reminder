#!/usr/bin/env bash
set -euo pipefail

label="${VIBE_RGB_LAUNCH_LABEL:-com.retsubasa.vibe-rgb-reminder.bridge}"
launch_agents_dir="${HOME}/Library/LaunchAgents"
uid="$(id -u)"

launchctl bootout "gui/${uid}" "${launch_agents_dir}/${label}.plist" 2>/dev/null || true

echo "ESP32 USB LED bridge stopped."
