# CLAUDE.md

Project context for Claude Code. Read this first.

## Goal
Cute desk status lights that show what Claude Code is doing, at home and at work (work
uses Claude Code on a different device). A kit, so more lights are easy: each light is a
Seeed XIAO ESP32-C3 + a 12-LED NeoPixel ring (RGB or RGBW). The first one lives in a
3D-printed cluster of translucent mushrooms; later ones may just sit in a jar or frosted cup.
The owner designs the enclosure herself in Onshape, so teach and review step by step
rather than generating CAD (see model/ONSHAPE.md).

## Architecture (decided 2026-11-21)
- **hub/**: `status_hub.py`, Python stdlib, runs on charon as a systemd user service,
  published through Tailscale Funnel at `/status-light` (charon's `/` is the ghost figure).
  Combines sessions per **channel** (`home`, `work`, ...): n > w > r > i; ready lasts 60 s;
  stale sessions expire. Token auth. Long-poll endpoint for lights.
- **client/**: `claude-light`, Python stdlib, installed on every machine running Claude
  Code and called by hooks (`client/hooks-settings.json`). Sends session id + state only.
  `serial=` mode writes straight to a USB-connected light (for office Wi-Fi the XIAO can't
  join), combining that machine's sessions locally.
- **firmware/**: PlatformIO, envs `xiao_rgb` / `xiao_rgbw`. WiFiManager setup screen
  (StatusLight-Setup) stores Wi-Fi + name/hub/token/channel; hub long-poll in a FreeRTOS
  task; local page at NAME.local; USB serial commands. Built on charon, flashed in the
  browser from committed images (firmware/images/).
- Lights only make outbound HTTPS calls, so any Wi-Fi with internet works.

## Hardware (decided)
- XIAO ESP32-C3. Ring data on D2 (GPIO4) through 470 ohm; ring power from 5V; capacitor
  across ring power. 3.3 V data usually fine; level shifter if not.
- Rings: all 36.8 mm OD, 23.3 mm ID, 3.6 mm thick (owner measured; ignore the 6.7 mm figure
  on DigiKey's forum). Owner has an RGB ring (1643); main mushroom ring will be RGBW
  natural white (2852). XIAOs and RGBW ring ordered 2026-11-21.
- Details and part numbers: docs/hardware.md.

## Behavior (decided)
w = working: blue comet; n = needs you: amber breathing; r = ready: green fill then glow,
fades after 60 s; i = idle: off. System looks: purple = setup screen, white dot = joining
Wi-Fi, red blink = hub unreachable, magenta double blink = bad token. Colors are {r,g,b,w}
in firmware/src/looks.cpp; the owner wants room for pastels and varied colors.

## Status / open items
- Nothing tested on hardware yet (parts on order). Hub and client are tested
  (`python3 -m unittest discover -s hub`, plus a client run against a local hub).
- Not deployed: hub service, Funnel path and hooks on charon need the owner's go-ahead
  (Funnel is public-facing; hooks affect every session).
- Work laptop: check employer policy on hooks pinging a personal server; office Wi-Fi may
  force the USB serial fallback.
- Enclosure: lesson 3 swaps the Flora pocket for a XIAO pocket (model/ONSHAPE.md).
- Test whether T3 Code triggers the Claude Code hooks. ChatGPT has no hooks (browser
  extension idea). Codex has its own hook/notify config: could call claude-light too.
- Later: updates over Wi-Fi (partition layout already leaves room).
