#pragma once
#include <Arduino.h>

// Status modes (from the hub, USB serial or the local web page):
//   'w' working, 'n' needs you, 'r' ready, 'i' idle
// System looks shown on top of the status while something needs attention:
//   SYS_SETUP       setup screen open (join the StatusLight-Setup Wi-Fi from a phone)
//   SYS_CONNECTING  joining Wi-Fi
//   SYS_OFFLINE     can't reach the hub
//   SYS_BAD_TOKEN   the hub rejected the token
enum SystemLook : uint8_t { SYS_NONE, SYS_SETUP, SYS_CONNECTING, SYS_OFFLINE, SYS_BAD_TOKEN };

void looksBegin();
bool isStatusMode(char m);
void looksSetMode(char m);  // restarts the animation only when the mode changes
char looksMode();
void looksSetSystem(SystemLook s);
SystemLook looksSystem();
void looksFrame();  // draw one frame; call every ~10 ms
void looksPreview();  // show every look in include/looks_table.h for 3 s each
uint8_t looksBrightnessStep(int delta);  // live brightness change (not saved); returns the new level
