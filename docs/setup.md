# Setup

Three pieces, set up once each:

```
 Claude Code session ──hook──> claude-light ──HTTPS──> status hub (charon) <──long-poll── light (XIAO)
 (charon, home PC, work laptop)                         combines sessions per channel
```

1. **The hub** runs on charon and is reachable over HTTPS through Tailscale Funnel.
2. **The client** (`client/claude-light`) is installed on every machine that runs Claude Code,
   and Claude Code hooks call it.
3. **Each light** joins Wi-Fi and is told the hub address, the token and its channel.

Channels keep places apart: home sessions post to `home`, the work laptop posts to `work`,
and each light listens to one channel. Only a session id and a one-letter state are ever sent.

## 1. Hub (charon)

```bash
cd ~/Claude/claude-status-light
python3 hub/status_hub.py            # first run creates ~/.config/claude-status-light/token; Ctrl-C
cp hub/status-hub.service ~/.config/systemd/user/
systemctl --user daemon-reload && systemctl --user enable --now status-hub
curl -s localhost:8787/healthz       # -> ok
```

Publish it under a path on charon's existing Funnel (the ghost figure keeps `/`):

```bash
sudo tailscale funnel --bg --set-path /status-light http://127.0.0.1:8787
curl -s https://charon.<tailnet>.ts.net/status-light/healthz    # -> ok
```

The hub address is then `https://charon.<tailnet>.ts.net/status-light`. Every request except
`/healthz` needs the token from `~/.config/claude-status-light/token`.

Tests: `python3 -m unittest discover -s hub`

## 2. Client + hooks (each machine that runs Claude Code)

Needs Python 3 (no packages).

**Linux / macOS (charon, home PC):**
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
Then merge `client/hooks-settings.json` into `~/.claude/settings.json` (the `"hooks"` block).

**Work laptop:** the same, with `channel=work`. Check that your employer is fine with a
hook sending "working/idle" pings to a personal server; nothing else is sent.
- Windows: put `claude-light` anywhere, create `%USERPROFILE%\.config\claude-status-light\client.conf`,
  and use `python C:/path/to/claude-light w` (etc.) as the hook commands.
- If the light can't join the office Wi-Fi (employee-login networks or sign-in pages don't
  work with it), plug it into the laptop by USB and add `serial=COM5` (Windows) or
  `serial=/dev/cu.usbmodem...` (macOS) to `client.conf`. The client then writes to the light
  directly and combines the laptop's sessions itself; no hub or Wi-Fi involved.

Debug: `CLAUDE_LIGHT_DEBUG=1 claude-light w` prints errors; `claude-light --status` shows
what the hub sees.

## 3. A light

1. Flash it: see `firmware/README.md`.
2. On first start it opens a Wi-Fi network called **StatusLight-Setup** and breathes purple.
   Join it from a phone; the setup page opens (or browse to 192.168.4.1).
3. Pick the Wi-Fi, and fill in **name** (e.g. `desk-light`), **hub address**, **token** and
   **channel** (`home` or `work`). Save. The light restarts and joins.
4. Check: `http://<name>.local` shows what it's displaying and lets you test each mode.

To change settings later: hold **BOOT** for 2 s (or type `s` on USB serial) to reopen the
setup page. Leaving the token blank keeps the saved one.

## Adding another light

Build or buy the hardware (`docs/hardware.md`), flash the right image (RGB or RGBW), and do
step 3 with a new name. Same channel = shows the same sessions; new channel = point some
machine's `client.conf` at it.
