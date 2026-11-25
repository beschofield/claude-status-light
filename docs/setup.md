# Setup

## Choosing how a light connects

Each light is built one of two ways, like choosing RGB or RGBW. The rule: **the light has to
hear from the machine where Claude Code actually runs** (that's where hooks run), which isn't
always the computer in front of you.

| | **Wi-Fi + hub** (`_wifi` firmware) | **USB only** (`_usb` firmware) |
|---|---|---|
| Use when | Claude Code runs on another machine (e.g. charon, reached over SSH or remotely), or several machines should drive one light | Claude Code runs on the very computer the light plugs into |
| How | hooks → hub on charon (HTTPS) ← light polls it over Wi-Fi | hooks → USB port → light |
| Network | light and hooks talk to the hub's public HTTPS address (token-protected) | none at all; the light's radio is off |
| Needs | the hub running, Wi-Fi with internet | that computer on and awake |
| Our lights | **home** (sessions run on charon) | **work** (sessions run only on the work laptop) |

Decided 2026-11-24. Why: home sessions run on charon, which has no USB port at the desk, so
only the hub route can reach them. At work, Claude Code only runs on the work laptop and the
light sits next to it, so USB covers everything while staying off the network entirely,
the most conservative option for work IT (see `docs/work-usb.md`).

The rest of this page is the **Wi-Fi + hub** route. For USB only, follow `docs/work-usb.md`.

```
 Claude Code session ──hook──> claude-light ──HTTPS──> status hub (charon) <──long-poll── light (XIAO)
 (charon, home PC, ...)                                 combines sessions per channel
```

1. **The hub** runs on charon and is reachable over HTTPS through Tailscale Funnel.
2. **The client** (`client/claude-light`) is installed on every machine whose sessions should
   show, and Claude Code hooks call it.
3. **Each light** joins Wi-Fi and is told the hub address, the token and its channel.

Channels keep groups of sessions apart (default `home`); each light listens to one channel.
Only a session id and a one-letter state are ever sent.

## 1. Hub (charon) — running since 2026-11-24

```bash
cd ~/Claude/claude-status-light
python3 hub/status_hub.py --init-token     # once: creates ~/.config/claude-status-light/token
cp hub/status-hub.service ~/.config/systemd/user/
systemctl --user daemon-reload && systemctl --user enable --now status-hub
curl -s localhost:8787/healthz       # -> ok
tailscale funnel --bg --set-path /status-light http://127.0.0.1:8787
curl -s https://charon.<tailnet>.ts.net/status-light/healthz    # -> ok
```

The hub address is `https://charon.<tailnet>.ts.net/status-light`. Every request except
`/healthz` needs the token from `~/.config/claude-status-light/token`, sent as an
`Authorization: Bearer` header (never in the URL).

Because it's public, the hub is locked down (hardened 2026-11-24):
- **Sandboxed service** (`hub/status-hub.service`): it sees an empty home folder except its own
  code and token (read-only), can't write outside a private /tmp, can't gain privileges, and has
  memory, thread and system-call limits. Tested: from inside, `~/.ssh` and other repos don't exist.
- **In the code**: the token is checked before any request body is read (header only,
  never in the URL); refused requests get their connection closed; a body must arrive within
  10 s and be under 4 KB; at most 32 connections at once (extra ones are closed); idle
  connections dropped after 120 s (longer than the Funnel proxy's own 90 s, so no lost events).
- Listens on 127.0.0.1 only; Tailscale Funnel handles HTTPS in front of it.

To unpublish only this path (not the ghost figure's `/`):
`tailscale funnel --https=443 --set-path /status-light off`. The plain `--https=443 off` that
Tailscale suggests would take down the ghost figure too.

Tests: `python3 -m unittest discover -s hub`

## 2. Client + hooks (each machine whose sessions should show)

Needs Python 3 (no packages).

```bash
mkdir -p ~/.local/bin ~/.config/claude-status-light
cp client/claude-light ~/.local/bin/ && chmod +x ~/.local/bin/claude-light
cat > ~/.config/claude-status-light/client.conf <<'EOF'
hub=https://charon.<tailnet>.ts.net/status-light
token=<the hub token>
channel=home
EOF
claude-light --test        # the light cycles blue, amber, green, off
```
On charon itself, use `hub=http://127.0.0.1:8787` so its sessions never leave the machine.
Then merge `client/hooks-settings.json` into `~/.claude/settings.json` (the `"hooks"` block).

Debug: `CLAUDE_LIGHT_DEBUG=1 claude-light w` prints errors; `claude-light --status` shows
what the hub sees.

## 3. A Wi-Fi light

1. Flash `status-light-<ring>_wifi-full.bin`: see `firmware/README.md`.
2. On first start it opens a Wi-Fi network called **StatusLight-Setup** and breathes purple.
   Join it from a phone; the setup page opens (or browse to 192.168.4.1).
3. Pick the Wi-Fi, and fill in **name** (e.g. `desk-light`), **hub address**, **token** and
   **channel**. Save. The light restarts and joins.
4. Check: `http://<name>.local` shows what it's displaying and lets you test each mode.

To change settings later: hold **BOOT** for 2 s (or type `s` on USB serial) to reopen the
setup page. Leaving the token blank keeps the saved one.

## Adding another light

Pick the ring (RGB/RGBW) and the connection (Wi-Fi/USB) with the table above, build the
hardware (`docs/hardware.md`), flash the matching image, then do step 3 (Wi-Fi) or
`docs/work-usb.md` (USB).
