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
blocking sockets), and doesn't import urllib at all (only inside the hub-route functions).
Keep it that way. Guide: docs/work-usb.md.

**Work light brief (standing rule, owner's request 2026-11-24):** `docs/WORK-LIGHT-BRIEF.md`
is a detailed security/implementation brief the owner can hand to anyone at work (assume a
security reviewer who wants every detail). **Update it in the same PR as any change** to the
firmware, `client/claude-light`, `client/hooks-settings.json`, the build/flash process or
images: the TL;DR, the affected sections, "Last updated / Applies to", the image SHA-256 table
(`sha256sum firmware/images/*_usb-*.bin`) and a change-log row. Re-verify claims you touch
(symbol checks with nm on the USB ELF, the no-network/no-urllib tests) rather than assuming.
Be quick and thorough; never let it go stale.

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

## Resume here (paused 2026-11-24, waiting on soldering access)

**Where things stand**
| Piece | State |
|---|---|
| Hub on charon | **Live.** `status-hub` user service, enabled at boot, sandboxed/hardened; public at `https://charon.taildc6780.ts.net/status-light`; token in `~/.config/claude-status-light/token`. Home channel empty (no sessions reporting yet). |
| Home light (XIAO #1) | Flashed with the **RGB Wi-Fi** image from PR #5 (before the looks table, so no `p`/`[`/`]` yet) and joined home Wi-Fi via the setup screen. **Unconfirmed:** whether the owner entered the hub address/token/channel on the setup screen afterwards. **Ring not soldered.** |
| Work light (XIAO #2) | Untouched. Will get `status-light-rgb_usb-full.bin` (or `rgbw_usb`) flashed at home. |
| Rings | RGB 1643 (for the home light first) and RGBW 2852 natural, both on hand. |
| Hooks / client on charon | **Not installed** (on purpose until the wired light works). |
| Work laptop | Nothing installed. OS and Python availability unknown. |
| Enclosure (Onshape) | Lessons 1-2 done; lesson 3 (XIAO pocket) needs XIAO measurements. See model/ONSHAPE.md. |
| Repo | Everything merged to `main` (PRs #1-#8). No open PRs, no other branches. |

**Next steps, in order**
1. *Owner:* solder the RGB ring to XIAO #1 at the makerspace: 5V -> PWR, GND -> GND,
   D2 -> 470 ohm (near the ring) -> IN. (docs/hardware.md)
2. *Owner (optional, recommended):* flash `firmware/images/status-light-rgb_wifi-update.bin`
   at **0xe000** to get the looks table, `p` preview and `[`/`]` (keeps Wi-Fi and settings).
3. *Owner:* if not done, hold BOOT 2 s, join StatusLight-Setup, re-pick Wi-Fi, enter hub
   `https://charon.taildc6780.ts.net/status-light`, the token (`cat ~/.config/claude-status-light/token`
   on charon), channel `home`. `http://<name>.local` should say "Hub: connected".
4. *Owner:* test from `http://<name>.local` (each mode) and serial `p`.
5. *Claude, on charon:* install the client (`cp client/claude-light ~/.local/bin/`), write
   `~/.config/claude-status-light/client.conf` with `hub=http://127.0.0.1:8787` and the token,
   run `claude-light --test` while the owner watches the light, then (with the owner's OK, since
   hooks affect every session) merge `client/hooks-settings.json` into `~/.claude/settings.json`.
6. Then the work light (docs/work-usb.md): flash XIAO #2 with the `_usb` image at home, solder,
   test in the flasher's Console tab; ask the owner for the work laptop's OS and whether Python 3
   is allowed (if not, write a PowerShell/zsh client). **Update docs/WORK-LIGHT-BRIEF.md** with
   anything that changes.
7. Onshape lesson 3 once the owner has measured the XIAO (length, width, thickness with shield,
   USB-C overhang, antenna size).

## Open items / notes
- Security: owner is cautious about the public hub. Hardened 2026-11-24: sandboxed systemd
  unit (sees only hub/ and its token, read-only), token checked before reading bodies (header-only),
  refused requests closed, 10 s body deadline, 4 KB body limit, 32-connection cap, 120 s idle
  timeout (must stay above the Funnel proxy's 90 s). Keep it that way when changing the hub.
  Deploy hub changes with: pull main, `cp hub/status-hub.service ~/.config/systemd/user/`,
  `systemctl --user daemon-reload && systemctl --user restart status-hub`, then check
  `/healthz` locally and through the public URL.
- Funnel: to unpublish only the hub, `tailscale funnel --https=443 --set-path /status-light off`
  (the plain `--https=443 off` would also take down the ghost figure at `/`).
- Test whether T3 Code triggers the Claude Code hooks. ChatGPT has no hooks (browser
  extension idea). Codex has its own hook/notify config: could call claude-light too.
- Later: updates over Wi-Fi for sealed Wi-Fi lights, like the ghost figure's OTA (partition
  layout already leaves room). USB lights are always plugged in, so they reflash over USB.
- Git workflow used so far: work on a branch (worktree in background sessions), PR, and merge
  only when the owner says so (owner usually asks for a code review first).
