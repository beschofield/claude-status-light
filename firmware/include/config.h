#pragma once
// Hardware and timing settings shared by every light. Per-light settings (name, hub,
// token, channel) are entered on the setup screen and saved on the board instead.

#ifndef RGBW
#define RGBW 0  // 0 = RGB ring, 1 = RGBW ring. Set per build env in platformio.ini.
#endif

constexpr int LED_PIN = 4;          // XIAO pin D2 (GPIO4) -> 470 ohm -> ring "IN"
constexpr int LED_COUNT = 12;
constexpr int BRIGHTNESS = 60;      // 0-255; keeps USB current low and the light easy on the eyes
constexpr int PIXEL_OFFSET = 0;     // rotate animations so pixel 0 sits where you want it
constexpr int BOOT_BUTTON = 9;      // the XIAO's BOOT button; hold 2 s (after start-up) for setup

constexpr uint32_t READY_HOLD_MS = 60000;   // "ready" fades out after this
constexpr int HUB_WAIT_S = 25;              // long-poll length; the hub answers sooner on a change
constexpr int HUB_FAILS_BEFORE_WARNING = 3; // failed polls in a row before the light shows it

constexpr const char* SETUP_NETWORK = "StatusLight-Setup";
constexpr const char* DEFAULT_NAME = "status-light";
constexpr const char* DEFAULT_CHANNEL = "home";
