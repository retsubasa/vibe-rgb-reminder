#include <Arduino.h>

const int RGB_PIN = 48;
const int BRIGHTNESS = 48;

String lineBuffer;

enum VisualMode {
  VIS_IDLE,
  VIS_PROMPT,
  VIS_BASH,
  VIS_EDIT,
  VIS_TOOL,
  VIS_SUCCESS,
  VIS_FAILURE,
  VIS_PERMISSION,
  VIS_STOP,
  VIS_NOTIFICATION,
  VIS_COMPACT,
  VIS_UNKNOWN,
  VIS_OFF
};

VisualMode visualMode = VIS_IDLE;
unsigned long modeStartedAt = 0;
unsigned long lastFrameAt = 0;

void setRgb(uint8_t r, uint8_t g, uint8_t b) {
  rgbLedWrite(RGB_PIN, r, g, b);
}

void setMode(VisualMode mode) {
  visualMode = mode;
  modeStartedAt = millis();
  lastFrameAt = 0;
}

void colorStep(uint8_t r, uint8_t g, uint8_t b) {
  setRgb(r, g, b);
}

void steppedBlueBreath(unsigned long t) {
  const uint8_t steps[] = {2, 6, 13, 24, 38, 48, 38, 24, 13, 6};
  int phase = (t / 170) % 10;
  uint8_t v = steps[phase];

  // Deliberately stepped, not smooth: more visible on tiny onboard RGB LEDs.
  colorStep(0, (v * 2) / 3, 10 + v);
}

void startupConstellation(unsigned long t) {
  steppedBlueBreath(t);
}

void promptPulse(unsigned long t) {
  int phase = (t / 180) % 8;

  if (phase == 0 || phase == 2) colorStep(BRIGHTNESS, 24, 0);
  else if (phase == 1 || phase == 3) colorStep(8, 4, 0);
  else colorStep(18, 9, 0);
}

void bashSpark(unsigned long t) {
  int phase = (t / 90) % 10;

  if (phase == 0 || phase == 2 || phase == 4) colorStep(BRIGHTNESS, 16, 0);
  else if (phase == 1 || phase == 3 || phase == 5) colorStep(0, 0, 0);
  else colorStep(10, 4, 0);
}

void editAurora(unsigned long t) {
  int phase = (t / 150) % 8;

  if (phase == 0 || phase == 3) colorStep(BRIGHTNESS, 8, 0);
  else if (phase == 1 || phase == 4) colorStep(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
  else if (phase == 2 || phase == 5) colorStep(0, 0, 0);
  else colorStep(18, 4, 0);
}

void toolProbe(unsigned long t) {
  int phase = (t / 260) % 6;

  if (phase == 0) colorStep(BRIGHTNESS, BRIGHTNESS, 0);
  else if (phase == 1) colorStep(12, 10, 0);
  else colorStep(6, 5, 0);
}

void successOrbit(unsigned long t) {
  if (t > 1200) {
    setMode(VIS_IDLE);
    return;
  }

  int phase = (t / 130) % 10;
  if (phase == 0 || phase == 2 || phase == 4) colorStep(0, BRIGHTNESS, 0);
  else colorStep(0, 0, 0);
}

void failureAlarm(unsigned long t) {
  int phase = (t / 120) % 8;

  if (phase == 0 || phase == 2 || phase == 4) colorStep(BRIGHTNESS, 0, 0);
  else if (phase == 1 || phase == 3 || phase == 5) colorStep(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
  else colorStep(0, 0, 0);
}

void permissionStrobe(unsigned long t) {
  int phase = (t / 95) % 6;

  if (phase == 0 || phase == 2) colorStep(BRIGHTNESS, 0, BRIGHTNESS / 3);
  else if (phase == 4) colorStep(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
  else colorStep(0, 0, 0);
}

void stopDusk(unsigned long t) {
  colorStep(0, BRIGHTNESS, 0);
}

void notificationPing(unsigned long t) {
  int phase = (t / 120) % 8;

  if (phase == 0 || phase == 3) colorStep(BRIGHTNESS, 0, BRIGHTNESS);
  else if (phase == 1 || phase == 4) colorStep(12, 0, 18);
  else colorStep(3, 0, 6);
}

void compactFold(unsigned long t) {
  int phase = (t / 180) % 8;

  if (phase == 0 || phase == 4) colorStep(BRIGHTNESS, BRIGHTNESS, BRIGHTNESS);
  else if (phase == 1 || phase == 5) colorStep(BRIGHTNESS, 0, BRIGHTNESS / 2);
  else if (phase == 2 || phase == 6) colorStep(12, 0, 20);
  else colorStep(2, 0, 5);
}

void unknownGlint(unsigned long t) {
  int phase = (t / 240) % 4;

  if (phase == 0) colorStep(18, 0, 18);
  else colorStep(3, 0, 5);
}

void updateVisual() {
  unsigned long now = millis();
  if (now - lastFrameAt < 24) {
    return;
  }
  lastFrameAt = now;

  unsigned long t = now - modeStartedAt;

  if (visualMode == VIS_IDLE) startupConstellation(t);
  else if (visualMode == VIS_PROMPT) promptPulse(t);
  else if (visualMode == VIS_BASH) bashSpark(t);
  else if (visualMode == VIS_EDIT) editAurora(t);
  else if (visualMode == VIS_TOOL) toolProbe(t);
  else if (visualMode == VIS_SUCCESS) successOrbit(t);
  else if (visualMode == VIS_FAILURE) failureAlarm(t);
  else if (visualMode == VIS_PERMISSION) permissionStrobe(t);
  else if (visualMode == VIS_STOP) stopDusk(t);
  else if (visualMode == VIS_NOTIFICATION) notificationPing(t);
  else if (visualMode == VIS_COMPACT) compactFold(t);
  else if (visualMode == VIS_UNKNOWN) unknownGlint(t);
  else colorStep(0, 0, 0);
}

String extractJsonString(const String &json, const char *key) {
  String pattern = String("\"") + key + "\":\"";
  int start = json.indexOf(pattern);
  if (start < 0) {
    return "";
  }

  start += pattern.length();
  int end = json.indexOf('"', start);
  if (end < 0) {
    return "";
  }

  return json.substring(start, end);
}

void showEvent(const String &event, const String &tool) {
  if (event == "SessionStart") {
    setMode(VIS_IDLE);
  } else if (event == "UserPromptSubmit") {
    setMode(VIS_PROMPT);
  } else if (event == "PreToolUse") {
    if (tool == "Bash" || tool == "shell" || tool == "exec" || tool == "exec_command" || tool == "functions.exec_command" || tool == "unified_exec") {
      setMode(VIS_BASH);
    } else if (tool == "Edit" || tool == "Write" || tool == "apply_patch" || tool == "functions.apply_patch") {
      setMode(VIS_EDIT);
    } else {
      setMode(VIS_TOOL);
    }
  } else if (event == "PostToolUse") {
    setMode(VIS_SUCCESS);
  } else if (event == "PostToolUseFailure" || event == "StopFailure") {
    setMode(VIS_FAILURE);
  } else if (event == "PermissionRequest") {
    setMode(VIS_PERMISSION);
  } else if (event == "Stop") {
    setMode(VIS_STOP);
  } else if (event == "SessionEnd") {
    setMode(VIS_OFF);
  } else if (event == "Notification") {
    setMode(VIS_NOTIFICATION);
  } else if (event == "PreCompact" || event == "PostCompact") {
    setMode(VIS_COMPACT);
  } else {
    setMode(VIS_UNKNOWN);
  }
}

void handleLine(const String &line) {
  String event = extractJsonString(line, "event");
  String tool = extractJsonString(line, "tool");

  if (event.length() == 0) {
    Serial.println("{\"ok\":false,\"error\":\"missing event\"}");
    return;
  }

  showEvent(event, tool);

  Serial.print("{\"ok\":true,\"event\":\"");
  Serial.print(event);
  Serial.print("\",\"tool\":\"");
  Serial.print(tool);
  Serial.println("\"}");
}

void setup() {
  pinMode(RGB_PIN, OUTPUT);
  setRgb(0, 0, 0);

  Serial.begin(115200);
  delay(1200);

  Serial.println("{\"ready\":true,\"device\":\"esp32-s3\",\"rgbPin\":48}");
  setMode(VIS_IDLE);
}

void loop() {
  updateVisual();

  while (Serial.available() > 0) {
    char c = Serial.read();

    if (c == '\n') {
      lineBuffer.trim();
      if (lineBuffer.length() > 0) {
        handleLine(lineBuffer);
      }
      lineBuffer = "";
    } else if (c != '\r') {
      lineBuffer += c;
      if (lineBuffer.length() > 512) {
        lineBuffer = "";
        Serial.println("{\"ok\":false,\"error\":\"line too long\"}");
      }
    }
  }
}
