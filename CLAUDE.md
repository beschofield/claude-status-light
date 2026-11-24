# CLAUDE.md

Project context for Claude Code. Read this first.

## Goal
Cute desk status lights that show what Claude Code is doing, at home and at work (work
uses Claude Code on a different device). A kit, so more lights are easy: each light is a
Seeed XIAO ESP32-C3 + a 12-LED NeoPixel ring. The first one lives in a 3D-printed cluster
of translucent mushrooms; later ones may just sit in a jar or frosted cup.
The owner designs the enclosure herself in Onshape, so teach and review step by step
rather than generating CAD (see model/ONSHAPE.md).

## Per-light choices (like RGB vs RGBW)
Each light picks a **ring** (RGB / RGBW) and a **connection**, one firmware build per pair:
`xiao_{rgb,rgbw}_{wifi,usb}`.
- **Wi-Fi + hub**: for sessions running on another machine (charon). Used at **home**.
- **USB only**: radio off (Wi-Fi code not even compiled in); the computer running Claude
  Code drives it over USB. Used at **work**.
Decision (2026-11-24) and reasoning: docs/setup.md, "Choosing how a light connects".

**Work rule (owner's requirement):** the work setup must be as conservative as possible with
work IT. Everything lives on the work laptop and the light: nothing on the work laptop may
contact the hub, charon, any public URL or the owner's GitHub. Flash the work light at home;
copy only `client/claude-light` to the laptop by hand; `client.conf` has only `serial=`.
`claude-light` in serial mode makes no network calls even if a hub is configured (tested by
blocking sockets). Keep it that way. Guide: docs/work-usb.md.

## Architecture
- **hub/**: `status_hub.py`, Python stdlib, runs on charon as the systemd user service
  `status-hub` (127.0.0.1:8787), published through Tailscale Funnel at
  `https://charon.taildc6780.ts.net/status-light` (charon's `/` is the ghost figure; to
  unpublish only this path: `tailscale funnel --https=443 --set-path /status-light off`).
  Combines sessions per channel (default `home`): n > w > r > i; ready lasts 60 s; stale
  sessions expire. Token in ~/.config/claude-status-light/token. Long-poll for lights.
- **client/**: `claude-light`, Python stdlib, called by hooks (`client/hooks-settings.json`).
  Sends session id + state only. Hub route or `serial=` USB route (combines that machine's
  sessions locally).
- **firmware/**: PlatformIO. Wi-Fi builds: WiFiManager setup screen (StatusLight-Setup)
  stores Wi-Fi + name/hub/token/channel; hub long-poll in a FreeRTOS task; local page at
  NAME.local. USB builds: serial only, stale-state timeout. Both: USB serial commands
  w/n/r/i/?. Built on charon, flashed in the browser from committed images (firmware/images/).

## Hardware (decided)
- XIAO ESP32-C3. Ring data on D2 (GPIO4) through 470 ohm; ring power from 5V; capacitor
  across ring power. 3.3 V data usually fine; level shifter if not.
- Rings: all 36.8 mm OD, 23.3 mm ID, 3.6 mm thick (owner measured; ignore the 6.7 mm figure
  on DigiKey's forum). Owner has an RGB ring (1643, using this one first) and an RGBW
  natural white (2852).
- Details and part numbers: docs/hardware.md.

## Behavior (decided)
w = working: blue comet; n = needs you: amber breathing; r = ready: green fill then glow,
fades after 60 s; i = idle: off. System looks: purple = setup screen, white dot = joining
Wi-Fi, red blink = hub unreachable, magenta double blink = bad token.
**All looks live in `firmware/include/looks_table.h`** (one line each: {r,g,b,w}, effect,
period, low/high %, show ms), modeled on the ghost figure's glow_looks.h. The owner will keep
editing looks after the lights are built and wants pastels and varied colors: keep looks as
data in that file, add new effects in src/looks.cpp only when a look needs one, and keep the
`p` preview listing every look.

## Status / open items
- 2026-11-24: XIAOs arrived. One flashed with the RGB Wi-Fi image and joined home Wi-Fi.
  Hub running and published (tested through the public URL). Ring not yet soldered.
- Hooks on charon not installed yet: wait until the wired light passes `claude-light --test`.
  On charon use `hub=http://127.0.0.1:8787`.
- Security: owner is cautious about the public hub. Offered: systemd sandboxing of the hub
  (no home-dir access except its token), connection caps. Not done yet; ask.
- Work laptop OS unknown; the USB client needs Python 3 there. If Python isn't allowed, write
  a PowerShell/zsh-only client (still no network).
- Enclosure: lesson 3 swaps the Flora pocket for a XIAO pocket (model/ONSHAPE.md).
- Test whether T3 Code triggers the Claude Code hooks. ChatGPT has no hooks (browser
  extension idea). Codex has its own hook/notify config: could call claude-light too.
- Order (2026-11-24): the first physical prototype is the **USB-only work light** (RGB ring
  1643 on a XIAO). Software work goes home Wi-Fi path first (hooks on charon, hub hardening),
  then the work USB client (needs the work laptop's OS / Python answer).
- Later: updates over Wi-Fi for sealed Wi-Fi lights, like the ghost figure's OTA (partition
  layout already leaves room). USB lights are always plugged in, so they reflash over USB.
