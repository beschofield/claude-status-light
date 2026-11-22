// Claude Code status light: Adafruit Flora + 12-LED NeoPixel ring.
// Serial commands (single characters, 9600 baud):
//   w = working   (blue comet chasing around the ring)
//   n = needs you (amber breathing)
//   r = ready     (green fill, then soft glow; fades out after 60 s)
//   i = idle      (off)
#include <Adafruit_NeoPixel.h>

#define PIN 6            // ring data-in -> Flora D6
#define N 12             // LEDs in the ring
#define BRIGHT 60        // 0-255; keeps current well under USB limits
#define READY_HOLD_MS 60000UL
// Rotates every animation so pixel 0 of the pattern lands under a chosen LED.
// Once the stems are on, find which ring index sits under the tallest mushroom
// and put it here.
#define PIXEL_OFFSET 0

// Ring type: 0 = RGB ring (e.g. Adafruit 1643), 1 = RGBW ring (Adafruit 2851/2852/2853).
// PlatformIO sets this per build env; in the Arduino IDE, edit the default here.
#ifndef RGBW
#define RGBW 0
#endif

#if RGBW
Adafruit_NeoPixel ring(N, PIN, NEO_GRBW + NEO_KHZ800);
#else
Adafruit_NeoPixel ring(N, PIN, NEO_GRB + NEO_KHZ800);
#endif
char mode = 'i';
uint32_t t0 = 0;

void setMode(char m) {
  mode = m;
  t0 = millis();
}

// Colors take an optional white amount w (0-255) for pastels and true whites.
// On an RGBW ring it drives the white LED; on an RGB ring it is mixed into
// r, g and b, so the same color reads similarly (a bit bluer) on both.
void setRGB(int i, float v, uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
  if (v < 0) v = 0;
  if (v > 1) v = 1;
#if RGBW
  uint32_t c = ring.Color(r * v, g * v, b * v, w * v);
#else
  uint32_t c = ring.Color(min(255, r + w) * v, min(255, g + w) * v, min(255, b + w) * v);
#endif
  ring.setPixelColor((i + PIXEL_OFFSET) % N, c);
}

void setup() {
  ring.begin();
  ring.setBrightness(BRIGHT);
  ring.clear();
  ring.show();
  Serial.begin(9600);
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == 'w' || c == 'n' || c == 'r' || c == 'i') setMode(c);
  }

  uint32_t t = millis() - t0;
  ring.clear();

  switch (mode) {
    case 'w': {
      // Comet: bright head, fading tail of 5 pixels, ~90 ms per step.
      float head = fmod(t / 90.0, (float)N);
      for (int i = 0; i < N; i++) {
        float d = head - i;
        if (d < 0) d += N;
        float v = (d < 5) ? 1.0 - d / 5.0 : 0;
        setRGB(i, v * v, 20, 90, 255);
      }
      break;
    }
    case 'n': {
      // Amber breathing, 2 s period.
      float b = (sin(t / 1000.0 * PI) + 1.0) / 2.0;
      for (int i = 0; i < N; i++) setRGB(i, 0.15 + 0.85 * b, 255, 120, 0);
      break;
    }
    case 'r': {
      if (t > READY_HOLD_MS) { setMode('i'); break; }
      float fill = t / 1000.0 * N;  // sweep fills the ring in 1 s
      float breathe = 0.45 + 0.15 * sin(t / 2000.0 * PI);
      for (int i = 0; i < N; i++) {
        float v = fill - i;
        if (v > 1) v = 1;
        setRGB(i, v * breathe + (v < 1 ? v * 0.4 : 0), 40, 255, 90);
      }
      break;
    }
    default:
      break;  // idle: off
  }

  ring.show();
  delay(10);
}
