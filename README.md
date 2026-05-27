# Claude Hook ESP32-S3 USB RGB

Chinese version: [README.zh-CN.md](README.zh-CN.md)

Use Claude Code hooks to trigger the onboard ESP32-S3 RGB LED over USB serial.

No Wi-Fi is used.

## Files

- `firmware/esp32_usb_rgb_hooks/esp32_usb_rgb_hooks.ino`
  - Arduino sketch for ESP32-S3.
  - Reads compact JSON lines from USB Serial.
  - Drives onboard RGB LED on GPIO48.
- `host/esp32-usb-led-bridge.mjs`
  - Local Node.js bridge.
  - Receives Claude hook events at `http://127.0.0.1:7317/event`.
  - Writes compact events to `/dev/cu.usbmodem11101`.
- `hooks/esp32-rgb-hook.sh`
  - Claude Code hook command.
  - Sends hook JSON to the local bridge.
- `.claude/settings.example.json`
  - Example Claude Code hook config.

## Flash Firmware

Use Arduino IDE:

- Board: `ESP32S3 Dev Module`
- Port: `/dev/cu.usbmodem11101`
- USB CDC On Boot: `Enabled`
- Flash Size: `16MB`
- PSRAM: `OPI PSRAM`

Or use the Arduino CLI bundled with Arduino IDE:

```bash
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --upload \
  -p /dev/cu.usbmodem11101 \
  --fqbn 'esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB' \
  firmware/esp32_usb_rgb_hooks
```

## Run The USB Bridge

The bridge is installed as a macOS LaunchAgent and starts automatically when you log in.

```bash
./start-bridge.sh
```

Optional environment variables:

```bash
ESP32_PORT=/dev/cu.usbmodem11101
HOOK_LED_PORT=7317
ESP32_BAUD=115200
```

Stop it with:

```bash
./stop-bridge.sh
```

Disable auto-start by unloading the LaunchAgent and removing the plist:

```bash
./stop-bridge.sh
rm ~/Library/LaunchAgents/com.retsubasa.vibe-rgb-reminder.bridge.plist
```

## Test Manually

With the bridge running:

```bash
curl -X POST http://127.0.0.1:7317/event \
  -H 'Content-Type: application/json' \
  -d '{"hook_event_name":"PostToolUse","tool_name":"Bash"}'
```

The RGB LED should flash green three times, then return to the normal stepped gradient.

## Enable Claude Code Hooks

Copy `.claude/settings.example.json` to `.claude/settings.json` in the project where you want the LED hooks enabled.

Make the hook executable:

```bash
chmod +x hooks/esp32-rgb-hook.sh
```

Then run `/hooks` inside Claude Code to verify that the hooks loaded.

## Event RGB Rules

Each event switches the RGB LED into a persistent visual mode. The current mode keeps running until a newer hook event arrives.

Color encodes importance:

- Blue/cyan: normal or idle.
- Yellow: normal tool activity.
- Orange: active execution or file mutation.
- Magenta/white: human intervention required.
- Red/white: failure or high urgency.
- Green: completed successfully.
- White/rose violet: context compaction.

Blink frequency encodes event type:

- Stepped gradient: normal running.
- Single slow pulse: generic tool or completion.
- Double pulse: user intent or file mutation.
- Triple spark: shell execution.
- Three green flashes: successful tool completion.
- Rapid strobe: permission or failure.
- Solid: completed response state.

| Hook event | Extra match | RGB color | Pattern | Meaning |
| --- | --- | --- | --- | --- |
| Idle / normal running | no current event | Deep blue -> bright cyan | Stepped color-level breathing, 10 visible steps | System heartbeat, Claude is alive |
| `SessionStart` | any | Deep blue -> bright cyan | Persistent stepped gradient | Session alive / normal running |
| `UserPromptSubmit` | any | Amber | Persistent double-pulse loop | User intent entered |
| `PreToolUse` | `tool_name=Bash` | Orange | Persistent triple-spark loop | Shell execution active or pending |
| `PreToolUse` | `tool_name=Edit` or `Write` | Orange + white | Persistent double-pulse mutation loop | File mutation active or pending |
| `PreToolUse` | other tools | Yellow | Persistent single slow probe | Tool active or pending |
| `PostToolUse` | any | Green | Three flashes, then return to normal stepped gradient | Last tool completed successfully |
| `PostToolUseFailure` | any | Red + white + black | Persistent high-contrast alarm loop | Last tool failed |
| `StopFailure` | any | Red + white + black | Persistent high-contrast alarm loop | Claude failed to stop cleanly |
| `PermissionRequest` | any | Magenta + white + black | Persistent rapid strobe | Human intervention required |
| `Stop` | any | Green | Solid on | Claude finished responding |
| `PreCompact` | any | White + rose violet | Persistent context-fold loop | Context compaction is starting |
| `PostCompact` | any | White + rose violet | Persistent context-fold loop | Context compaction completed |
| `SessionEnd` | any | Off | Stays off | Session ended |
| `Notification` | any | Violet | Persistent quick ping loop | Claude Code notification |
| Other / unknown | any | Dim violet | Persistent tiny glint loop | Unmapped event |
