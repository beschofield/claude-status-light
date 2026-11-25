# Work status light: security and implementation brief

**Last updated:** 2026-11-24 · **Applies to:** `main` after PR #8 · **Owner:** the person whose desk it sits on

> This brief is kept current: every change to this project that could affect the work light
> (firmware, the `claude-light` script, hook configuration, build or flashing process) updates
> this file in the same pull request, including the TL;DR and the change log at the bottom.

---

## TL;DR

- **What it is:** a small USB desk light that changes color to show whether Claude Code on this
  laptop is **working** (blue), **waiting for me** (amber), **done** (green) or **idle** (off).
- **No network, at all.** The light's firmware is a build with Wi-Fi, Bluetooth and all networking
  code **left out at compile time** (verified: none of those functions exist in the binary). The
  laptop-side script makes **no network connections** and doesn't even load Python's networking
  modules (verified). Nothing goes to the internet, the cloud, a personal server, or GitHub.
- **Only one byte per event crosses the USB cable:** the letter `w`, `n`, `r` or `i`. No prompts,
  code, file names, paths, usernames or any other data ever leave the laptop or reach the light.
- **What runs on the laptop:** one ~180-line Python script using only the standard library,
  started by Claude Code's own hook feature for each event and exiting within milliseconds. No
  installer, no admin rights, no drivers, no background service, no auto-update.
- **What it stores:** one small owner-only file (permissions 600) listing active Claude Code
  session IDs (random identifiers) with their one-letter state and a timestamp. Entries expire.
- **The device can't act as a keyboard or USB drive.** Its chip (ESP32-C3) only has a fixed-
  function USB serial/debug port in hardware; it physically cannot present itself as anything
  else. It is flashed at home and holds no data.
- **Honest caveats:** Claude Code passes each hook a JSON description of the event on stdin, which
  can include prompt text or tool output. The script reads it into memory, keeps only the session
  ID and discards the rest; it never writes, logs or sends it. The device firmware is not signed,
  and this project has not had an independent audit. Details below.

---

## 1. Purpose

Claude Code (Anthropic's coding assistant) runs in a terminal on this laptop. When it is busy for
a while, or stops to ask for permission, that's easy to miss. The light gives an at-a-glance signal
without having to watch the terminal. It is purely an indicator: it has no inputs, controls
nothing, and Claude Code works identically with it unplugged.

## 2. Overview

```
 ┌───────────────────────── work laptop ─────────────────────────┐        ┌──── desk light ────┐
 │                                                               │        │                    │
 │  Claude Code ──(hook event, JSON on stdin)──> claude-light ───┼─ USB ──┼─> XIAO ESP32-C3 ──> 12 LEDs
 │                                               (Python script) │ 1 byte │   (radio off)      │
 │                                     writes 1 letter: w/n/r/i  │        │                    │
 └───────────────────────────────────────────────────────────────┘        └────────────────────┘
      no network connections anywhere in this picture
```

1. Claude Code has a built-in, user-configured **hooks** feature: on certain events it runs a
   command. Six events are configured (section 5).
2. The command is `claude-light <letter>`. The script works out the combined state of all
   Claude Code sessions on this laptop and writes **one letter** to the light's USB serial port.
3. The light's firmware reads that letter and animates the LEDs. That's the whole system.

## 3. Hardware

| Part | Details |
|---|---|
| Microcontroller | Seeed Studio XIAO ESP32-C3 (Espressif ESP32-C3 chip, 21 x 17.8 mm). |
| LEDs | 12-LED "NeoPixel" ring (Adafruit 1643 RGB, or 2852 RGBW), wired to one data pin. |
| Other | One 470 ohm resistor, an optional capacitor, three wires, a USB-C cable to the laptop. |
| Power | From the laptop's USB port. LED brightness is capped in firmware at 60/255; worst-case draw is well under the 500 mA a USB port supplies. |
| Radio hardware | The chip has Wi-Fi and Bluetooth hardware, but the work firmware contains **no code** to start either (section 4.2). A small external antenna may be attached but is unused. |

## 4. The light's firmware

### 4.1 Build and provenance
- Source: this repository, `firmware/` (C++, Arduino framework, built with PlatformIO). The build
  used at work is **`xiao_rgb_usb`** (or `xiao_rgbw_usb` for an RGBW ring).
- It is compiled with `USE_WIFI=0`, which excludes all networking source code from the build.
- Toolchain and libraries at the time of this brief: PlatformIO platform `espressif32` 8.0.0,
  Arduino-ESP32 core 3.3.x (package 4.30312.261024), Adafruit NeoPixel 1.15.5 (the only library
  the USB build uses). WiFiManager 2.0.17 is listed for the Wi-Fi builds but none of it ends up in
  the USB binary (verified, see 4.2).
- Built on the owner's personal machine, never on the work laptop. Flashed at home with
  Espressif's browser flasher (https://espressif.github.io/esptool-js/) on a personal computer.
- Committed images and their SHA-256 checksums (to confirm a flashed image matches the source):

| File | SHA-256 |
|---|---|
| `firmware/images/status-light-rgb_usb-full.bin` | `c462986a434eed04c0dcd2e12184e20b84578ba4aee5c74ca0630c9e2d7788db` |
| `firmware/images/status-light-rgb_usb-update.bin` | `7bf2b41c94c0c7a88c138bdff667911ed5cf9cdc9b687bd1ed4ef56afc1944e3` |
| `firmware/images/status-light-rgbw_usb-full.bin` | `0c76b6fc8647c7e92ceca4641dcbfe7d14a88a8b74c386792dc502013e40a196` |
| `firmware/images/status-light-rgbw_usb-update.bin` | `0100837934629f741d3d3e7159336bde3113e9e5596c0f9cd2334eb5d146b912` |

### 4.2 What is (and isn't) in the work firmware
Checked by listing the symbols in the compiled firmware (`nm` on `firmware.elf`) for both USB
builds. Present / absent:

| Capability | In the USB build? |
|---|---|
| Wi-Fi driver (`esp_wifi_init`, `esp_wifi_start`, `esp_wifi_connect`) | **absent** |
| TCP/IP network stack (`lwip_init`, `esp_netif_init`) | **absent** |
| Bluetooth (`esp_bt_controller_init`, `btStart`, Bluedroid, NimBLE) | **absent** |
| TLS / HTTPS (`mbedtls_ssl_setup`, `mbedtls_ssl_handshake`) | **absent** |
| HTTP client or web server, Wi-Fi setup portal (WiFiManager) | **absent** |
| Writing settings to the chip's storage (`Preferences::put*`) | **absent** |
| USB serial input/output | present (the only interface) |
| NeoPixel LED driver | present |

For comparison, the Wi-Fi build used at home does contain the Wi-Fi functions; the difference
comes from the compile-time switch, not from a runtime setting that could be turned on.
The USB binary is about 323 KB versus about 1.4 MB for the Wi-Fi build.

### 4.3 Behavior
- Reads single characters from USB serial. It acts on: `w` `n` `r` `i` (set the look), `?` (print
  a one-line status), `p` (preview all looks), `[` `]` (brightness, until restart). All other
  bytes are ignored.
- Writes to USB serial only: a one-line start-up message, the reply to `?`, and preview/brightness
  messages. The laptop script never reads anything back (it opens the port write-only).
- Stale-state safety: if no letter arrives for 20 minutes while showing "working", or 3 hours
  while showing "needs you" (e.g. the laptop slept mid-session), it turns itself off.
- "Ready" fades out by itself after 60 seconds.
- Keeps no data: it doesn't record what it was told, has no clock, and writes nothing to its
  flash storage. Unplugging it resets it to "off".

### 4.4 USB identity and what the device can't do
- The ESP32-C3's only USB hardware is a fixed-function **USB Serial/JTAG controller**. The laptop
  sees a standard serial port (CDC-ACM; e.g. "USB Serial Device (COMx)" on Windows,
  `/dev/cu.usbmodem…` on macOS) and a JTAG debug interface, with Espressif's vendor ID **0x303A**
  (product ID 0x1001, "USB JTAG/serial debug unit"). Both use drivers built into Windows 10+ and macOS.
- This controller is not programmable as other USB device types: the ESP32-C3 has no general USB
  (OTG) controller, so the device **cannot** pretend to be a keyboard, mouse, network adapter or
  storage drive, whatever firmware is on it.
- The JTAG interface lets a computer debug the *light's* chip. It gives the light no access to the
  laptop.

## 5. The laptop side

### 5.1 What gets installed
Exactly three things, all by hand, in the user's own account:
1. **`claude-light`**: one Python file (~180 lines), copied over by USB stick or typed in. The
   repository is not cloned and GitHub is not accessed from the work laptop.
2. **`~/.config/claude-status-light/client.conf`**: one line, e.g. `serial=COM5` (Windows) or
   `serial=/dev/cu.usbmodem1101` (macOS). No server address, no token, no credentials.
3. **Six hook entries** in Claude Code's user settings (`~/.claude/settings.json`), shown below.

Nothing else: no installer, no admin or root rights, no drivers, no system services or startup
items, no scheduled tasks, no browser extensions, no third-party Python packages, no auto-update.
Requires an existing Python 3 (standard library only).

### 5.2 Hook configuration
| Claude Code event | Command run | Meaning |
|---|---|---|
| SessionStart | `claude-light i` | a session started (idle) |
| UserPromptSubmit | `claude-light w` | you sent a prompt (working) |
| PostToolUse | `claude-light w` | Claude finished a tool step (still working) |
| Notification, only `permission_prompt` or `elicitation_dialog` | `claude-light n` | Claude is asking you something |
| Stop | `claude-light r` | Claude finished its reply (ready) |
| SessionEnd | `claude-light end` | the session closed |

Each hook has a 5-second limit set in the configuration, and the script exits in well under a
second. Claude Code runs the hook as the logged-in user, with that user's normal permissions.

### 5.3 What the script does, step by step
Source: `client/claude-light`.
1. **Reads its settings** from `client.conf` (and `CLAUDE_LIGHT_*` environment variables, which
   override the file).
2. **Reads the hook's input.** Claude Code writes a JSON description of the event to the hook's
   stdin. Depending on the event this can include the session ID, the working directory, the
   path of the session transcript and, for some events, **the prompt text, tool inputs/outputs or
   the notification message**. The script reads stdin for at most 0.5 seconds, parses the JSON
   in memory, and **uses only the `session_id` field**. Everything else is discarded when the
   process exits a moment later. It is never written to disk, logged, printed or transmitted.
3. **Combines sessions:** loads `serial-sessions.json`, records this session's state (or removes
   it on `i`/`end`), drops entries past their expiry (ready 60 s, working 20 min, needs-you 3 h),
   and picks the most urgent state: needs-you > working > ready > idle.
4. **Saves** that small file back, created owner-only (permissions 600) in a folder created 700.
5. **Writes one byte** (the combined letter) to the configured serial port, opened write-only,
   then closes it. On Windows the port is opened as `\\.\COMn`.
6. **Exits with success, always.** Any error (light unplugged, wrong port) is silently ignored so
   Claude Code is never affected. Errors are printed only when run by hand in a terminal or with
   `CLAUDE_LIGHT_DEBUG=1`.

### 5.4 Network isolation of the script
- With `serial=` set, the script never calls any network function, **even if a server address
  were also configured by mistake**: the USB path returns before the network path is reached.
- Python's networking module (`urllib`) is imported **only inside** the two functions used for the
  home server route, so in USB mode no networking code is even loaded. Verified by loading the
  script and listing loaded modules after a USB write: none of `socket`, `ssl`, `http`, `urllib`,
  `email`.
- Verified a second way by replacing Python's socket functions with ones that log and refuse any
  connection, then running the script in USB mode: **0 connection attempts**. (The same harness
  catches the attempt when the script is pointed at a server, which shows the test works.)

### 5.5 Data on the laptop
| What | Where | Contents | Lifetime |
|---|---|---|---|
| Settings | `~/.config/claude-status-light/client.conf` | the serial port name | until deleted |
| Session state | `~/.config/claude-status-light/serial-sessions.json` (600) | e.g. `{"6f1c…": ["w", 1795565311.05]}`: session ID, letter, Unix time | entries expire (60 s to 3 h) or are removed at session end |
| The script | wherever it was copied | source code | until deleted |

The session IDs are random identifiers Claude Code generates; they contain no names or content.

## 6. Exactly what data goes where

| From → to | Channel | Data | Never included |
|---|---|---|---|
| Claude Code → `claude-light` | stdin of a local process | Claude Code's hook JSON (may contain prompt/tool text, paths) | (stays in memory; only `session_id` is used) |
| `claude-light` → disk | local file, owner-only | session IDs, letters, timestamps | prompts, code, paths, names |
| `claude-light` → light | USB serial, 1 byte | `w`, `n`, `r` or `i` | everything else |
| Light → laptop | USB serial | start-up line / replies to `?` (not read by the script) | any laptop data (the light never has any) |
| Anything → network | none | nothing | everything |

## 7. Threat model

| Concern | Assessment |
|---|---|
| Data leaving the laptop over the network | Not possible from this system: no network code in the firmware, none used or loaded by the script in USB mode. |
| Data leaving via the device | The device receives one letter per event and stores nothing. There is no data on it to leak. |
| Device acting as a keyboard/USB drive ("BadUSB") | The ESP32-C3's USB is fixed-function serial/JTAG hardware; it cannot enumerate as HID or storage regardless of firmware. |
| Tampered firmware on the device | Flashed at home from source in this repo; checksums above. Even a malicious firmware could only send serial text, which the script never reads, and the chip still couldn't act as another USB device type. It could, in principle, turn on its radio, which the official build can't; see "Limitations". |
| Tampered script | It's one short, readable standard-library Python file; review it before copying, and compare against the repo. |
| Another program on the laptop writing to the light | Any local program can write to a serial port; the worst outcome is a wrong color. |
| Effect on Claude Code | Hooks always exit successfully within a 5 s limit; failure only means the light doesn't update. |
| Hook input containing sensitive text | Present in memory for milliseconds and discarded; not persisted or sent (section 5.3). |
| Persistence / auto-update / remote control | None: no service, no update mechanism, no inbound or outbound connection. |

## 8. How a reviewer can check this themselves
1. **Read the script** (`client/claude-light`): confirm imports at the top are `json, os, sys,
   threading, time, pathlib` and that `urllib` appears only inside `send_hub` and the `--status`
   hub branch.
2. **Turn off Wi-Fi / unplug the network**, then use Claude Code: the light still works, because
   nothing needs the network.
3. **Watch network activity** (e.g. Little Snitch / `nettop` on macOS, Resource Monitor or
   `netstat -b` on Windows) while Claude Code runs: no connections from `python`.
4. **Check the USB device**: Windows Device Manager or macOS System Information → USB shows vendor
   0x303A, "USB JTAG/serial debug unit": a serial port, no keyboard or storage.
5. **Check that the device has no radio code**: from a checkout of this repo on a personal machine,
   `cd firmware && pio run -e xiao_rgb_usb`, then list symbols in
   `.pio/build/xiao_rgb_usb/firmware.elf` with the toolchain's `nm` and search for `esp_wifi_`,
   `esp_bt_`, `lwip_`: no matches.
6. **Compare the flashed image** with `sha256sum` (macOS: `shasum -a 256`) against the table in 4.1.

## 9. Limitations and things not done
- **No independent security review.** This is a personal project, reviewed by its owner with AI
  assistance (Claude Code) and automated code review.
- **Firmware isn't signed and secure boot isn't enabled.** Anyone with physical access could
  reflash the light with different firmware, including one that turns on its Wi-Fi radio; it still
  couldn't present as a keyboard or drive, and it receives nothing but single letters.
- **The script needs Python 3 on the laptop.** If installing or using Python isn't allowed, an
  equivalent PowerShell (Windows) or zsh (macOS) script can be written with the same behavior.
- **The device is visible** to endpoint-management tools as an Espressif USB serial device.
- **Brightness and colors** are cosmetic settings in firmware; they don't affect any of the above.

## 10. Relationship to the home setup (not used at work)
The same project also supports a **Wi-Fi** version of the light for home use, where Claude Code
runs on a remote server and lights fetch status from a small, token-protected status service over
HTTPS. **None of that is used at work:** the work light runs the USB-only firmware without
networking code, and the work laptop's configuration contains only a serial port, no server
address or token. The two setups share source code but no data, accounts or connections.

## 11. Change log
| Date | PR | Change affecting the work light |
|---|---|---|
| 2026-11-24 | #6 | USB-only firmware builds (`_usb`) with networking compiled out; script's serial mode documented as network-free; work setup guide. |
| 2026-11-24 | #8 | This brief. Script: networking module now imported only on the home route (never loaded in USB mode); session-state file created owner-only (600). |
