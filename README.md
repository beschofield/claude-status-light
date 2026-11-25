# claude-status-light

Little desk lights that show what Claude Code is doing: **blue** while it works, **amber**
when it needs you, **green** when it's done. Each light is a Seeed XIAO ESP32-C3 and a
12-LED NeoPixel ring (RGB or RGBW). The first one lives in a 3D-printed mushroom cluster.

Each light connects one of two ways: **Wi-Fi + hub** (sessions on any machine report to a small
hub that the light polls; used at home) or **USB only** (radio off; the computer running Claude
Code drives it directly; used at work). See `docs/setup.md` to choose.

| Folder | What |
|---|---|
| `hub/` | the status hub (Python, no dependencies) + systemd unit + tests |
| `client/` | `claude-light`, called by Claude Code hooks, + the hook config |
| `firmware/` | XIAO firmware (PlatformIO), ready-to-flash images in `firmware/images/` |
| `docs/` | `setup.md` (choosing Wi-Fi vs USB, hub, hooks), `work-usb.md` (offline work light), `hardware.md` (parts, wiring) |
| `model/` | the enclosure: modeled in Onshape, progress in `model/ONSHAPE.md` |

Start with `docs/setup.md`. For the work light's security and implementation brief, see
`docs/WORK-LIGHT-BRIEF.md`.
