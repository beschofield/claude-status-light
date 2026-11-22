#pragma once
#include <Arduino.h>

// Per-light settings, entered on the setup screen and kept in the board's flash.
struct Settings {
  String name;     // also the network name: http://NAME.local
  String hub;      // e.g. https://charon.example.ts.net/status-light ; empty = no hub
  String token;    // the hub's token
  String channel;  // which sessions to show, e.g. "home" or "work"
};

extern Settings settings;
void settingsLoad();
void settingsSave();
