#pragma once
#include <Arduino.h>

#include "looks.h"

// Wi-Fi (with the phone setup screen), the hub long-poll, and the local web page.
void netBegin();
void netLoop();       // call from loop()
void netOpenSetup();  // open the setup screen (BOOT button, serial 's')
String netStatus();   // one line for the serial '?' command

// Mode requested by the hub-poll task; loop() applies it. 0 = nothing pending.
extern volatile char pendingMode;
