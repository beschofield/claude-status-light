# Firmware

Arduino (PlatformIO) firmware for a **Seeed XIAO ESP32-C3** driving a 12-LED NeoPixel ring,
RGB or RGBW. One build per ring type: `xiao_rgb`, `xiao_rgbw`.

## Build (charon or any machine with PlatformIO)

```bash
cd firmware
pio run                          # both ring types
python3 tools/make_image.py      # -> images/status-light-{rgb,rgbw}-{full,update}.bin
```

The images are committed, so any computer can download them from GitHub.

## Flash (Chrome or Edge, any computer)

1. Plug the XIAO in with a USB-C **data** cable.
2. First flash of a new board only: hold **BOOT**, tap **RESET**, release BOOT (flashing mode).
3. Open https://espressif.github.io/esptool-js/ , click **Connect**, pick the XIAO's port.
4. Pick the image:
   - **first time / recovery**: `status-light-<ring>-full.bin` at address **0x0**
     (wipes saved Wi-Fi and settings)
   - **update**: `status-light-<ring>-update.bin` at address **0xe000** (keeps them)
5. **Program**, then press **RESET** (or unplug and replug).

With PlatformIO on the computer the XIAO is plugged into: `pio run -e xiao_rgbw -t upload`.

## Using it

- First start: the ring breathes **purple** and a Wi-Fi called **StatusLight-Setup** appears.
  Join it from a phone and fill in Wi-Fi, name, hub address, token and channel (see
  `docs/setup.md`).
- `http://<name>.local`: what it's showing, and buttons to test each mode.
- USB serial (115200): send `w` `n` `r` `i` to set a mode, `s` for the setup screen, `?` for
  status. `pio device monitor` on a machine it's plugged into.
- Hold **BOOT** 2 s after start-up: setup screen.

## What the light shows

| Look | Meaning |
|---|---|
| blue comet | working |
| amber breathing | needs you (permission prompt or question) |
| green fill, then glow, fades after 60 s | ready (a session finished) |
| off | idle |
| purple breathing | setup screen open |
| dim white dot circling | joining Wi-Fi |
| one red blink every 2 s | hub unreachable |
| two magenta blinks every 2 s | hub rejected the token |

Colors live in `src/looks.cpp` as {r, g, b, w}; pin, LED count, brightness and timings in
`include/config.h`. HTTPS hubs must use a Let's Encrypt certificate (Tailscale Funnel does);
`tools/make_certs.py` regenerates `include/certs.h`.
