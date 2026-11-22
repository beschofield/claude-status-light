# claude-status-light

Little desk lights that show what Claude Code is doing: **blue** while it works, **amber**
when it needs you, **green** when it's done. Each light is a Seeed XIAO ESP32-C3 and a
12-LED NeoPixel ring (RGB or RGBW). The first one lives in a 3D-printed mushroom cluster.

Every Claude Code session, on any machine, reports to a small hub; each light shows the
combined state of the sessions on its channel (e.g. `home` or `work`).

| Folder | What |
|---|---|
| `hub/` | the status hub (Python, no dependencies) + systemd unit + tests |
| `client/` | `claude-light`, called by Claude Code hooks, + the hook config |
| `firmware/` | XIAO firmware (PlatformIO), ready-to-flash images in `firmware/images/` |
| `docs/` | `setup.md` (hub, hooks, a new light), `hardware.md` (parts, wiring) |
| `model/` | the enclosure: modeled in Onshape, progress in `model/ONSHAPE.md` |

Start with `docs/setup.md`.
