// Every look the light can show, in one place. Edit this file to change how the light looks,
// then rebuild and flash. Type `p` on USB serial to preview every look in turn, and `[` / `]`
// to try other brightness levels live.
//
// Each look is:  { "name", {red, green, blue, white}, effect, period ms, low %, high %, show ms }
//
//   color     0-255 each (any online color picker shows red/green/blue). White drives the
//             white LED on RGBW rings: add some for pastels, or use only white for a clean
//             white. On RGB rings the white amount is mixed from red+green+blue instead, so
//             the same line works on both (a bit bluer on RGB).
//   effect    Steady   one brightness (high), no movement
//             Breathe  slow fade low -> high -> low; one breath = period
//             Blink    whole ring on (high) / off (low); one on + off = period
//             Spin     one bright pixel (high) circles the ring, the rest sit at low; one lap = period
//             Comet    like Spin with a fading 5-pixel tail
//             Fill     lights up pixel by pixel over period, then glows gently between low and high
//             Pip      pixel 0 flashes briefly (high) once per period; the rest stay off
//             DoublePip  same, two quick flashes per period
//   low/high  brightness, 0-100 %, on top of BRIGHTNESS below
//   show ms   0 = shown as long as the state lasts. Otherwise the look fades out over its last
//             3 s and the light goes idle (used for "ready").
//
// What picks the look: a system look (setup screen, joining Wi-Fi, hub problems) wins over the
// status from Claude Code; `p` preview wins over both. Status names: w = Working, n = Needs,
// r = Ready, i = Idle.
#pragma once

#include <stdint.h>

// Overall brightness, 0-255. 60 is soft on a desk and well inside what USB can power
// (12 LEDs at full white would be ~0.7 A at 255). Try levels live with [ and ].
constexpr uint8_t BRIGHTNESS = 60;

struct Rgbw {
  uint8_t r, g, b, w;
};

enum class Effect : uint8_t { Steady, Breathe, Blink, Spin, Comet, Fill, Pip, DoublePip };

struct Look {
  const char* name;
  Rgbw color;
  Effect effect;
  uint16_t periodMs;
  uint8_t low;
  uint8_t high;
  uint32_t showMs;
};

namespace looks {

// clang-format off
// ---- Status from Claude Code ------------------------------------------------------------------
//                          name                  color                  effect             period low high  show
inline constexpr Look Working    {"working",           { 20,  90, 255,   0}, Effect::Comet,     1080,   0, 100,     0};
inline constexpr Look Needs      {"needs you",         {255, 120,   0,   0}, Effect::Breathe,   2000,  15, 100,     0};
inline constexpr Look Ready      {"ready",             { 40, 255,  90,   0}, Effect::Fill,      1000,  45,  60, 60000};
inline constexpr Look Idle       {"idle",              {  0,   0,   0,   0}, Effect::Steady,       0,   0,   0,     0};

// ---- System (Wi-Fi lights) --------------------------------------------------------------------
inline constexpr Look Setup      {"setup screen open", {150,  40, 255,   0}, Effect::Breathe,   3000,  10,  70,     0};
inline constexpr Look Connecting {"joining Wi-Fi",     {  0,   0,   0,  90}, Effect::Spin,      1440,   0,  60,     0};
inline constexpr Look Offline    {"hub unreachable",   {255,   0,   0,   0}, Effect::Pip,       2000,   0,  70,     0};
inline constexpr Look BadToken   {"hub token wrong",   {255,   0, 160,   0}, Effect::DoublePip, 2000,   0,  70,     0};
// clang-format on

// The order `p` previews them in.
inline constexpr const Look* All[] = {&Working, &Needs, &Ready, &Idle, &Setup, &Connecting, &Offline, &BadToken};

}  // namespace looks
