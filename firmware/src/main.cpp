// Claude Code status light: Seeed XIAO ESP32-C3 + 12-LED NeoPixel ring (RGB or RGBW).
//
// The light long-polls a status hub over Wi-Fi (see hub/) and shows the combined state of
// every Claude Code session on its channel. It also takes single characters over USB
// serial (115200, any baud works), which is the fallback when it can't join the Wi-Fi:
//   w working   n needs you   r ready   i idle
//   s open the setup screen   ? print status
// Hold the BOOT button for 2 s (any time after start-up) to open the setup screen too.

#include <Arduino.h>

#include "config.h"
#include "looks.h"
#include "net.h"
#include "settings.h"

static uint32_t bootPressedAt = 0;

static void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (isStatusMode(c)) {
      looksSetMode(c);
    } else if (c == 's') {
      netOpenSetup();
    } else if (c == '?') {
      Serial.println(netStatus());
    }
  }
}

static void handleBootButton() {
  static bool fired = false;  // once per press
  if (digitalRead(BOOT_BUTTON) == LOW) {
    if (!bootPressedAt) bootPressedAt = millis();
    if (!fired && millis() - bootPressedAt > 2000) {
      fired = true;
      netOpenSetup();
    }
  } else {
    bootPressedAt = 0;
    fired = false;
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  looksBegin();
  settingsLoad();
  looksSetSystem(SYS_CONNECTING);
  looksFrame();
  Serial.printf("status light \"%s\" starting (%s ring)\n", settings.name.c_str(), RGBW ? "RGBW" : "RGB");
  netBegin();
}

void loop() {
  static uint32_t lastFrame = 0;
  handleSerial();
  handleBootButton();
  if (char m = pendingMode) {
    pendingMode = 0;
    looksSetMode(m);
  }
  netLoop();
  if (millis() - lastFrame >= 10) {
    lastFrame = millis();
    looksFrame();
  }
  delay(1);
}
