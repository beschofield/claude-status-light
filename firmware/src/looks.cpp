// Draws the looks defined in include/looks_table.h. To change colors or effects, edit that
// file, not this one.
#include "looks.h"

#include <Adafruit_NeoPixel.h>

#include "config.h"
#include "looks_table.h"

#if RGBW
static Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);
#else
static Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
#endif

static char mode = 'i';
static uint32_t modeStart = 0;
static SystemLook sys = SYS_NONE;
static uint32_t sysStart = 0;
static int previewIndex = -1;  // >= 0 while `p` is previewing
static uint32_t previewStart = 0;
constexpr uint32_t PREVIEW_STEP_MS = 3000;
constexpr uint32_t FADE_OUT_MS = 3000;

static void put(int i, float v, Rgbw c) {
  v = constrain(v, 0.0f, 1.0f);
  int px = ((i + PIXEL_OFFSET) % LED_COUNT + LED_COUNT) % LED_COUNT;
#if RGBW
  ring.setPixelColor(px, ring.Color(c.r * v, c.g * v, c.b * v, c.w * v));
#else
  ring.setPixelColor(px, ring.Color(min(255, c.r + c.w) * v, min(255, c.g + c.w) * v,
                                    min(255, c.b + c.w) * v));
#endif
}

static float wave(uint32_t t, uint32_t periodMs) {  // 0..1..0 over one period, starts at 0
  if (!periodMs) return 1;
  return (1 - cosf(2 * PI * (t % periodMs) / (float)periodMs)) / 2;
}

// Draws one look `t` ms after it started. Returns false once a timed look (show ms) is over.
static bool draw(const Look& L, uint32_t t) {
  if (L.showMs && t >= L.showMs) return false;
  float lo = L.low / 100.0f, hi = L.high / 100.0f;
  float fade = 1;
  if (L.showMs && t + FADE_OUT_MS > L.showMs) fade = (L.showMs - t) / (float)FADE_OUT_MS;
  uint32_t p = L.periodMs ? L.periodMs : 1;

  switch (L.effect) {
    case Effect::Steady:
      for (int i = 0; i < LED_COUNT; i++) put(i, hi * fade, L.color);
      break;
    case Effect::Breathe:
      for (int i = 0; i < LED_COUNT; i++) put(i, (lo + (hi - lo) * wave(t, p)) * fade, L.color);
      break;
    case Effect::Blink: {
      float v = (t % p) < p / 2 ? hi : lo;
      for (int i = 0; i < LED_COUNT; i++) put(i, v * fade, L.color);
      break;
    }
    case Effect::Spin: {
      int head = (int)((t % p) * LED_COUNT / p);
      for (int i = 0; i < LED_COUNT; i++) put(i, (i == head ? hi : lo) * fade, L.color);
      break;
    }
    case Effect::Comet: {
      float head = (t % p) * (float)LED_COUNT / p;
      for (int i = 0; i < LED_COUNT; i++) {
        float d = head - i;
        if (d < 0) d += LED_COUNT;
        float tail = d < 5 ? 1 - d / 5 : 0;
        put(i, (lo + (hi - lo) * tail * tail) * fade, L.color);
      }
      break;
    }
    case Effect::Fill: {
      float filled = t * (float)LED_COUNT / p;
      float glow = lo + (hi - lo) * wave(t, 4000);
      for (int i = 0; i < LED_COUNT; i++) {
        float v = constrain(filled - i, 0.0f, 1.0f);
        put(i, (v * glow + (v < 1 ? v * 0.4f : 0)) * fade, L.color);  // bright leading edge
      }
      break;
    }
    case Effect::Pip:
      if (t % p < 150) put(0, hi, L.color);
      break;
    case Effect::DoublePip: {
      uint32_t s = t % p;
      if (s < 120 || (s > 300 && s < 420)) put(0, hi, L.color);
      break;
    }
  }
  return true;
}

static const Look& statusLook(char m) {
  switch (m) {
    case 'w': return looks::Working;
    case 'n': return looks::Needs;
    case 'r': return looks::Ready;
    default: return looks::Idle;
  }
}

static const Look& systemLook(SystemLook s) {
  switch (s) {
    case SYS_SETUP: return looks::Setup;
    case SYS_CONNECTING: return looks::Connecting;
    case SYS_OFFLINE: return looks::Offline;
    default: return looks::BadToken;
  }
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

void looksPreview() {
  previewIndex = 0;
  previewStart = millis();
  Serial.printf("preview: %s\n", looks::All[0]->name);
}

uint8_t looksBrightnessStep(int delta) {
  int b = constrain((int)ring.getBrightness() + delta, 5, 255);
  ring.setBrightness(b);
  return b;
}

void looksFrame() {
  uint32_t now = millis();
  ring.clear();
  if (previewIndex >= 0) {
    uint32_t t = now - previewStart;
    if (t >= PREVIEW_STEP_MS) {
      constexpr int count = sizeof(looks::All) / sizeof(looks::All[0]);
      if (++previewIndex >= count) {
        previewIndex = -1;
        Serial.println("preview: done");
      } else {
        previewStart = now;
        t = 0;
        Serial.printf("preview: %s\n", looks::All[previewIndex]->name);
      }
    }
    if (previewIndex >= 0) {
      draw(*looks::All[previewIndex], t);  // the first 3 s of each look
    }
  } else if (sys != SYS_NONE) {
    draw(systemLook(sys), now - sysStart);
  } else if (!draw(statusLook(mode), now - modeStart)) {
    mode = 'i';  // a timed look (ready) is over
  }
  ring.show();
}
