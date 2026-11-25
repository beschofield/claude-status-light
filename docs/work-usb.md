# Work light: USB only, nothing on the network

> For a detailed security and implementation write-up to share with someone at work, see
> **`docs/WORK-LIGHT-BRIEF.md`**.

For a computer where Claude Code runs on that machine itself and the light sits on the
same desk, e.g. a work laptop. Goal: be as conservative as possible with work IT.

**What stays true:**
- The light's Wi-Fi radio is never turned on (the `_usb` firmware doesn't even contain
  the Wi-Fi code). It never broadcasts a setup network.
- The laptop makes **no network connections** for the light: no hub, no charon, no public
  URL, no GitHub. `claude-light` in USB mode only writes one letter to the USB port; this
  is tested (it stays offline even if a hub address were configured by mistake).
- Nothing from this project is cloned, downloaded or signed into on the work laptop.
- What the laptop does see: a USB serial device (Espressif "USB JTAG/serial debug unit",
  vendor id 303A) and a small Python script run by Claude Code hooks. No admin rights,
  no drivers, no background service.

Still worth a quick check against your employer's policy on personal USB devices and on
Claude Code hooks.

## 1. Flash the light at home (your own computer)

1. Download `firmware/images/status-light-rgb_usb-full.bin` (or `rgbw_usb` for an RGBW
   ring) from the repo.
2. Flash it at **0x0** with https://espressif.github.io/esptool-js/ (steps in
   `firmware/README.md`), then press RESET.
3. Test it still at home: in the same page's **Console** tab (115200), type `w`, `n`, `r`,
   `i`. The ring should go blue comet, amber, green, off. `?` prints its status.

## 2. Put one file on the work laptop

Copy **only** `client/claude-light` (a single Python file, no packages) to the laptop by
USB stick, or open it and retype/paste it. Don't clone the repo or log into GitHub there.
Needs Python 3 on the laptop; if it isn't there and you can't install it, a version in the
laptop's own shell (PowerShell or zsh) can be written instead.

Suggested place: `~/claude-light/claude-light` (macOS) or
`C:\Users\<you>\claude-light\claude-light` (Windows).

## 3. Tell it which port the light is on

Plug the light into the laptop, then find its port:
- **Windows:** Device Manager → Ports (COM & LPT) → "USB Serial Device (COM5)" or similar.
- **macOS:** in Terminal, `ls /dev/cu.usbmodem*`.

Create the config file (folder `.config/claude-status-light` in your home folder) named
`client.conf`, with exactly one line:

```
serial=COM5
```
(or `serial=/dev/cu.usbmodem1101` on macOS). No hub, no token.

Test: `python3 ~/claude-light/claude-light --test` (Windows: `python C:\Users\<you>\claude-light\claude-light --test`).
The light cycles blue, amber, green, off.

## 4. Add the hooks

Merge the `"hooks"` block from `client/hooks-settings.json` into the laptop's
`~/.claude/settings.json`, changing each command to the file's path, e.g.
- macOS: `python3 ~/claude-light/claude-light w`
- Windows: `python C:/Users/<you>/claude-light/claude-light w`

(retype the small block rather than downloading it). Start a Claude Code session: the light
goes blue while it works, amber when it asks you something, green when it's done.

## Notes

- Several sessions at once are combined on the laptop (amber beats blue beats green).
- If the laptop sleeps mid-session, the light clears a stuck blue after 20 minutes and a
  stuck amber after 3 hours, the same limits the hub uses.
- Updates: reflash at home with `status-light-<ring>_usb-update.bin` at **0xe000**.
