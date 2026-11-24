#pragma once
#include <stdint.h>
// Hardware and timing settings shared by every light. Per-light settings (name, hub,
// token, channel) are entered on the setup screen and saved on the board instead.

#ifndef RGBW
#define RGBW 0  // 0 = RGB ring, 1 = RGBW ring. Set per build env in platformio.ini.
#endif

#ifndef USE_WIFI
#define USE_WIFI 1  // 1 = Wi-Fi + hub, 0 = USB only (radio never turned on). Set per build env.
#endif

// USB-only lights hear nothing when the computer sleeps mid-session, so a stuck state
// clears itself after the same limits the hub uses.
constexpr uint32_t USB_WORKING_STALE_MS = 20UL * 60 * 1000;
constexpr uint32_t USB_NEEDS_STALE_MS = 3UL * 60 * 60 * 1000;

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
