# Claude / Codex Hook ESP32-S3 USB RGB

用 Claude Code / Codex 的 hook 事件，通过 USB 串口触发 ESP32-S3 N16R8 板载 RGB LED。

不使用 Wi-Fi。链路是：

```text
Claude Code / Codex hook
  -> 本机 hook 脚本
  -> 127.0.0.1:7317 本机桥接服务
  -> USB Serial /dev/cu.usbmodem11101
  -> ESP32-S3 GPIO48 板载 RGB
```

## 文件结构

- `firmware/esp32_usb_rgb_hooks/esp32_usb_rgb_hooks.ino`
  - ESP32-S3 Arduino 固件。
  - 从 USB Serial 读取 JSON 行。
  - 控制 GPIO48 上的板载 RGB LED。
- `host/esp32-usb-led-bridge.mjs`
  - 本机 Node.js USB 桥接服务。
  - 监听 `http://127.0.0.1:7317/event`。
  - 把 hook 事件压缩后写入 `/dev/cu.usbmodem11101`。
- `hooks/esp32-rgb-hook.sh`
  - Claude Code 专用 hook 脚本。
- `hooks/codex-rgb-hook.sh`
  - Codex 专用 hook 脚本。
  - 会输出 `{}`，避免干扰 Codex 的 hook 协议。
- `.claude/settings.json`
  - 当前项目的 Claude Code hook 配置。
- `.claude/settings.example.json`
  - 可复制到其它项目的 Claude Code hook 配置示例。
- `start-bridge.sh`
  - 启动 macOS LaunchAgent 桥接服务。
- `stop-bridge.sh`
  - 停止桥接服务。

## ESP32 固件烧录

Arduino IDE 里选择：

- Board: `ESP32S3 Dev Module`
- Port: `/dev/cu.usbmodem11101`
- USB CDC On Boot: `Enabled`
- Flash Size: `16MB`
- PSRAM: `OPI PSRAM`

也可以用 Arduino IDE 自带的 `arduino-cli`：

```bash
"/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli" compile --upload \
  -p /dev/cu.usbmodem11101 \
  --fqbn 'esp32:esp32:esp32s3:CDCOnBoot=cdc,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB' \
  firmware/esp32_usb_rgb_hooks
```

## USB 桥接服务

桥接服务已经安装成 macOS LaunchAgent，登录 macOS 后会自动启动。

手动启动或重启：

```bash
./start-bridge.sh
```

停止：

```bash
./stop-bridge.sh
```

查看健康状态：

```bash
curl http://127.0.0.1:7317/health
```

日志：

```text
/tmp/esp32-usb-led-bridge.log
/tmp/esp32-usb-led-bridge.err.log
```

如果要禁用开机自启：

```bash
./stop-bridge.sh
rm ~/Library/LaunchAgents/com.retsubasa.vibe-rgb-reminder.bridge.plist
```

## 手动测试

桥接服务运行时，可以直接发事件：

```bash
curl -X POST http://127.0.0.1:7317/event \
  -H 'Content-Type: application/json' \
  -d '{"hook_event_name":"PostToolUse","tool_name":"Bash"}'
```

也可以测试 compact：

```bash
curl -X POST http://127.0.0.1:7317/event \
  -H 'Content-Type: application/json' \
  -d '{"hook_event_name":"PreCompact"}'
```

## Claude Code 全局配置

Claude Code 全局配置已经写入：

```text
~/.claude/settings.json
```

已启用的事件：

```text
SessionStart
UserPromptSubmit
PreToolUse
PostToolUse
PostToolUseFailure
PermissionRequest
Stop
StopFailure
Notification
PreCompact
PostCompact
SessionEnd
```

在 Claude Code 里运行：

```text
/hooks
```

可以确认 hooks 是否加载。

## Codex 全局配置

Codex 全局配置已经写入：

```text
~/.codex/config.toml
```

已启用：

```toml
[features]
hooks = true
```

已配置的 Codex hook 事件：

```text
SessionStart
UserPromptSubmit
PreToolUse
PermissionRequest
PostToolUse
PreCompact
PostCompact
Stop
```

原来的 `notify = [...]` 没有被覆盖，Computer Use 的 turn-ended 通知仍然保留。

## RGB 规则库

每个事件会切换到一个持续灯效。当前灯效会一直保持，直到新的 hook 事件到来。

颜色表示重要性：

- 蓝 / 青：正常空闲状态。
- 黄色：普通工具活动。
- 橙色：命令执行、文件修改、较高注意。
- 品红 / 白：需要人为介入。
- 红 / 白：失败或高紧急。
- 绿色：完成成功。
- 白 / 玫紫：上下文压缩 compact。

闪动频率表示事件类型：

- 色阶渐变：正常运行。
- 慢单脉冲：普通工具状态。
- 双脉冲：用户输入或文件修改。
- 三连火花：shell 执行。
- 绿色三闪：工具成功完成。
- 快速频闪：权限请求或失败。
- 常亮：整轮回复完成状态。

| Hook 事件 | 额外条件 | RGB 颜色 | 灯效规则 | 含义 |
| --- | --- | --- | --- | --- |
| 空闲 / 正常运行 | 无事件 | 深蓝 -> 亮青 | 10 阶色阶渐变 | 系统心跳，表示会话活着 |
| `SessionStart` | 任意 | 深蓝 -> 亮青 | 持续色阶渐变 | 会话开始 / 正常运行 |
| `UserPromptSubmit` | 任意 | 琥珀色 | 持续双脉冲 | 用户输入进入系统 |
| `PreToolUse` | `Bash` / `exec_command` | 橙色 | 持续三连火花 | shell 命令即将执行或正在执行 |
| `PreToolUse` | `Edit` / `Write` / `apply_patch` | 橙色 + 白色 | 持续双脉冲 | 文件修改即将发生或正在发生 |
| `PreToolUse` | 其它工具 | 黄色 | 持续慢单脉冲 | 普通工具活动 |
| `PostToolUse` | 任意 | 绿色 | 闪三下，然后回到正常紫罗兰色阶 | 上一个工具成功完成 |
| `PostToolUseFailure` | 任意 | 红 + 白 + 黑 | 持续高对比警报 | 工具失败 |
| `StopFailure` | 任意 | 红 + 白 + 黑 | 持续高对比警报 | Claude 停止失败 |
| `PermissionRequest` | 任意 | 品红 + 白 + 黑 | 持续快速频闪 | 需要人为介入 / 授权 |
| `Stop` | 任意 | 绿色 | 常亮 | 回复完成 |
| `PreCompact` | 任意 | 白 + 玫紫 | 持续上下文折叠循环 | 即将压缩上下文 |
| `PostCompact` | 任意 | 白 + 玫紫 | 持续上下文折叠循环 | 上下文压缩完成 |
| `SessionEnd` | 任意 | 关闭 | 保持熄灭 | 会话结束 |
| `Notification` | 任意 | 紫罗兰 | 持续快速提示 | Claude Code 通知 |
| 未映射事件 | 任意 | 暗紫 | 持续微弱闪光 | 未分类事件 |

## 常见问题

### ESP32 有两个 USB 口，应该插哪个？

当前使用的是 ESP32-S3 原生 USB/JTAG 口，macOS 显示为：

```text
/dev/cu.usbmodem11101
```

这一路已经可以上传、串口通信和触发 RGB，不需要走 Wi-Fi。

### 如果端口变了怎么办？

先查看当前端口：

```bash
ls /dev/cu.usbmodem*
```

如果不是 `/dev/cu.usbmodem11101`，修改 LaunchAgent 里的 `ESP32_PORT`：

```text
~/Library/LaunchAgents/com.retsubasa.vibe-rgb-reminder.bridge.plist
```

然后重启服务：

```bash
./start-bridge.sh
```

### 如果灯不动怎么办？

按顺序检查：

```bash
curl http://127.0.0.1:7317/health
lsof /dev/cu.usbmodem11101
tail -40 /tmp/esp32-usb-led-bridge.log
```

如果 Arduino IDE 的串口监视器开着，可能会占用串口，需要先关掉。
