// Claude Code status light: Seeed XIAO ESP32-C3 + 12-LED NeoPixel ring (RGB or RGBW).
//
// Two connection types, chosen per build (see platformio.ini):
//   Wi-Fi (USE_WIFI=1): long-polls the status hub (see hub/) and shows the combined state of
//     every Claude Code session on its channel. Hold BOOT 2 s for the setup screen.
//   USB (USE_WIFI=0): the radio stays off; the computer running Claude Code sends the state
//     over USB with `claude-light` (serial= in client.conf).
// Both take single characters over USB serial (115200; any baud works):
//   w working   n needs you   r ready   i idle   ? print status
//   s open the setup screen (Wi-Fi builds)

#include <Arduino.h>

#include "config.h"
#include "looks.h"
#include "settings.h"
#if USE_WIFI
#include "net.h"
#endif

static uint32_t lastCommandAt = 0;

static void statusLine() {
#if USE_WIFI
  Serial.println(netStatus());
#else
  Serial.printf("name %s, ring %s, USB only (radio off), showing %c\n", settings.name.c_str(),
                RGBW ? "RGBW" : "RGB", looksMode());
#endif
}

static void handleSerial() {
  while (Serial.available()) {
    char c = Serial.read();
    if (isStatusMode(c)) {
      looksSetMode(c);
      lastCommandAt = millis();
    } else if (c == '?') {
      statusLine();
#if USE_WIFI
    } else if (c == 's') {
      netOpenSetup();
#endif
    }
  }
}

#if USE_WIFI
static void handleBootButton() {
  static uint32_t pressedAt = 0;
  static bool fired = false;  // once per press
  if (digitalRead(BOOT_BUTTON) == LOW) {
    if (!pressedAt) pressedAt = millis();
    if (!fired && millis() - pressedAt > 2000) {
      fired = true;
      netOpenSetup();
    }
  } else {
    pressedAt = 0;
    fired = false;
  }
}
#else
// Nothing heard for a long time (e.g. the laptop went to sleep mid-session): go idle,
// like the hub does for silent sessions.
static void clearStaleUsbState() {
  uint32_t quiet = millis() - lastCommandAt;
  char m = looksMode();
  if ((m == 'w' && quiet > USB_WORKING_STALE_MS) || (m == 'n' && quiet > USB_NEEDS_STALE_MS)) {
    looksSetMode('i');
  }
}
#endif

void setup() {
  Serial.begin(115200);
  looksBegin();
  settingsLoad();
#if USE_WIFI
  pinMode(BOOT_BUTTON, INPUT_PULLUP);
  looksSetSystem(SYS_CONNECTING);
  looksFrame();
  Serial.printf("status light \"%s\" starting (%s ring, Wi-Fi)\n", settings.name.c_str(),
                RGBW ? "RGBW" : "RGB");
  netBegin();
#else
  Serial.printf("status light \"%s\" starting (%s ring, USB only)\n", settings.name.c_str(),
                RGBW ? "RGBW" : "RGB");
#endif
}

void loop() {
  static uint32_t lastFrame = 0;
  handleSerial();
#if USE_WIFI
  handleBootButton();
  if (char m = pendingMode) {
    pendingMode = 0;
    looksSetMode(m);
  }
  netLoop();
#else
  clearStaleUsbState();
#endif
  if (millis() - lastFrame >= 10) {
    lastFrame = millis();
    looksFrame();
  }
  delay(1);
}
