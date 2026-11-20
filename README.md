# claude-status-light

A Flora + NeoPixel ring desk light that shows what Claude Code is doing, built into a 3D-printed mushroom cluster.

- `firmware/`: Flora sketch (flash with the Arduino IDE).
- `hooks/`: `light.sh` and the Claude Code hook config. Copy `hooks/` to `~/claude-light` and merge `hooks-settings.json` into `~/.claude/settings.json`.
- `model/`: the enclosure. The real model is in Onshape; `model/ONSHAPE.md` tracks progress. The Python script is an older parametric draft: `pip install -r requirements.txt && python3 -I build_shroom.py out`.

See `CLAUDE.md` for wiring, decisions and open items.

Quick test: `hooks/light.sh w` (working), `n` (needs you), `r` (ready), `i` (off). Needs access to the serial port (`dialout` group on Linux).
