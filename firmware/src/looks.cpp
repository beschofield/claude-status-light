#include "looks.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"

// Colors are {r, g, b, w}. w is the white LED on RGBW rings (clean whites and pastels);
// on RGB rings it is blended from r, g and b instead, so one palette works on both.
// Tweak freely: e.g. {255, 120, 0, 0} -> {255, 120, 0, 60} makes "needs you" a softer peach.
struct Color {
  uint8_t r, g, b, w;
};
namespace palette {
constexpr Color Working = {20, 90, 255, 0};
constexpr Color Needs = {255, 120, 0, 0};
constexpr Color Ready = {40, 255, 90, 0};
constexpr Color Setup = {150, 40, 255, 0};
constexpr Color Connecting = {0, 0, 0, 90};
constexpr Color Offline = {255, 0, 0, 0};
constexpr Color BadToken = {255, 0, 160, 0};
}  // namespace palette

#if RGBW
static Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
#else
static Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

static char mode = 'i';
static uint32_t modeStart = 0;
static SystemLook sys = SYS_NONE;
static uint32_t sysStart = 0;

static void put(int i, float v, Color c) {
  v = constrain(v, 0.0f, 1.0f);
  int px = ((i + PIXEL_OFFSET) % LED_COUNT + LED_COUNT) % LED_COUNT;
#if RGBW
  ring.setPixelColor(px, ring.Color(c.r * v, c.g * v, c.b * v, c.w * v));
#else
  ring.setPixelColor(px, ring.Color(min(255, c.r + c.w) * v, min(255, c.g + c.w) * v,
                                    min(255, c.b + c.w) * v));
#endif
}

static float breathe(uint32_t t, float periodS) {
  return (sinf(t / 1000.0f * 2 * PI / periodS - PI / 2) + 1) / 2;  // 0..1, starts dark
}

void looksBegin() {
  ring.begin();
  ring.setBrightness(BRIGHTNESS);
  ring.clear();
  ring.show();
}

bool isStatusMode(char m) { return m == 'w' || m == 'n' || m == 'r' || m == 'i'; }

void looksSetMode(char m) {
  if (!isStatusMode(m) || m == mode) return;
  mode = m;
  modeStart = millis();
}

char looksMode() { return mode; }

void looksSetSystem(SystemLook s) {
  if (s == sys) return;
  sys = s;
  sysStart = millis();
}

SystemLook looksSystem() { return sys; }

static void drawSystem(uint32_t t) {
  switch (sys) {
    case SYS_SETUP:  // slow purple breathing
      for (int i = 0; i < LED_COUNT; i++) put(i, 0.1f + 0.6f * breathe(t, 3), palette::Setup);
      break;
    case SYS_CONNECTING: {  // one dim white dot going round
      int head = (t / 120) % LED_COUNT;
      put(head, 0.6f, palette::Connecting);
      break;
    }
    case SYS_OFFLINE:  // one red pixel blinking every 2 s
      if (t % 2000 < 150) put(0, 0.7f, palette::Offline);
      break;
    case SYS_BAD_TOKEN:  // two magenta blinks every 2 s
      if (t % 2000 < 120 || (t % 2000 > 300 && t % 2000 < 420)) put(0, 0.7f, palette::BadToken);
      break;
    default:
      break;
  }
}

static void drawMode(uint32_t t) {
  switch (mode) {
    case 'w': {  // comet: bright head, 5-pixel fading tail
      float head = fmodf(t / 90.0f, (float)LED_COUNT);
      for (int i = 0; i < LED_COUNT; i++) {
        float d = head - i;
        if (d < 0) d += LED_COUNT;
        float v = d < 5 ? 1 - d / 5 : 0;
        put(i, v * v, palette::Working);
      }
      break;
    }
    case 'n':  // breathing, 2 s
      for (int i = 0; i < LED_COUNT; i++) put(i, 0.15f + 0.85f * breathe(t, 2), palette::Needs);
      break;
    case 'r': {  // fill around the ring in 1 s, then a soft glow; fades out after READY_HOLD_MS
      if (t > READY_HOLD_MS) {
        mode = 'i';
        break;
      }
      float fill = t / 1000.0f * LED_COUNT;
      float glow = 0.45f + 0.15f * sinf(t / 2000.0f * PI);
      float fade = t > READY_HOLD_MS - 3000 ? (READY_HOLD_MS - t) / 3000.0f : 1;
      for (int i = 0; i < LED_COUNT; i++) {
        float v = constrain(fill - i, 0.0f, 1.0f);
        put(i, v * glow * fade + (v < 1 ? v * 0.4f : 0), palette::Ready);
      }
      break;
    }
    default:
      break;  // idle: off
  }
}

void looksFrame() {
  uint32_t now = millis();
  ring.clear();
  if (sys != SYS_NONE) {
    drawSystem(now - sysStart);
  } else {
    drawMode(now - modeStart);
  }
  ring.show();
}
