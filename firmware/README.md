# Firmware

Arduino (PlatformIO) firmware for a **Seeed XIAO ESP32-C3** driving a 12-LED NeoPixel ring.
Two choices per light, one build for each combination:

| | Wi-Fi + hub | USB only (radio off) |
|---|---|---|
| **RGB ring** (Adafruit 1643) | `xiao_rgb_wifi` | `xiao_rgb_usb` |
| **RGBW ring** (2851/2852/2853) | `xiao_rgbw_wifi` | `xiao_rgbw_usb` |

Which connection to use: `docs/setup.md`, "Choosing how a light connects". The USB builds
leave out the Wi-Fi code entirely.

## Build (charon or any machine with PlatformIO)

```bash
cd firmware
pio run                          # all four
python3 tools/make_image.py      # -> images/status-light-<build>-{full,update}.bin
```

The images are committed, so any personal computer can download them from GitHub. (For the
work light, flash at home: `docs/work-usb.md`.)

## Flash (Chrome or Edge)

1. Plug the XIAO in with a USB-C **data** cable.
2. First flash of a new board only: hold **BOOT**, tap **RESET**, release BOOT (flashing mode).
3. Open https://espressif.github.io/esptool-js/ , click **Connect**, pick the XIAO's port.
4. Pick the image, e.g. `status-light-rgb_wifi-...`:
   - **first time / recovery / switching Wi-Fi⇄USB**: `...-full.bin` at address **0x0**
     (wipes saved Wi-Fi and settings)
   - **update**: `...-update.bin` at address **0xe000** (keeps them)
5. **Program**, then press **RESET** (or unplug and replug).

With PlatformIO on the computer the XIAO is plugged into: `pio run -e xiao_rgb_wifi -t upload`.

## Using it

- **Wi-Fi builds**, first start: the ring breathes **purple** and a Wi-Fi called
  **StatusLight-Setup** appears. Join it from a phone and fill in Wi-Fi, name, hub address,
  token and channel (`docs/setup.md`). `http://<name>.local` shows what it's displaying and
  has buttons to test each mode. Hold **BOOT** 2 s after start-up to reopen setup.
- **USB builds**: nothing to set up; `claude-light` with `serial=` drives it
  (`docs/work-usb.md`). A stuck blue clears after 20 min of silence, amber after 3 h.
- USB serial on both (115200): `w` `n` `r` `i` set a mode, `?` prints status, `p` previews
  every look (3 s each), `[` / `]` change brightness until the next restart, `s` opens the
  setup screen (Wi-Fi builds). The esptool-js page's Console tab, or `pio device monitor`.

## Changing how it looks

Everything visual is in **`include/looks_table.h`**: one line per look with its color
(red, green, blue, white), effect (Steady, Breathe, Blink, Spin, Comet, Fill, Pip,
DoublePip), speed, brightness range and how long it stays. The rules are at the top of that
file. Edit, rebuild (`pio run`, `python3 tools/make_image.py`), flash the `-update.bin`, then
type `p` to see them all.

## What the light shows

| Look | Meaning |
|---|---|
| blue comet | working |
| amber breathing | needs you (permission prompt or question) |
| green fill, then glow, fades after 60 s | ready (a session finished) |
| off | idle |
| purple breathing | setup screen open (Wi-Fi) |
| dim white dot circling | joining Wi-Fi |
| one red blink every 2 s | hub unreachable |
| two magenta blinks every 2 s | hub rejected the token |

Pin, LED count and timings are in `include/config.h`. HTTPS hubs must use a Let's Encrypt certificate (Tailscale Funnel does);
`tools/make_certs.py` regenerates `include/certs.h`.
