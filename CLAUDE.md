# CLAUDE.md

Project context for Claude Code. Read this first.

## Goal
A cute desk status light for Claude Code (and ideally T3 Code, Codex, ChatGPT): an Adafruit Flora driving a 12-LED NeoPixel ring, housed in a 3D-printed cluster of translucent mushrooms. The owner is learning Blender and wants to design the model herself, so explain rather than just generate when she asks about modeling.

## Hardware (decided)
- Adafruit Flora v3 (45 mm round, 7 mm thick), 12-LED NeoPixel ring (36.8 mm OD, 23.3 mm ID, 3.6 mm thick).
- Wiring: ring PWR -> Flora VBATT, GND -> GND, IN -> D6 (never OUT). Add a 300-500 ohm resistor on the data wire next to the ring and a 1000 uF capacitor across ring power if possible.
- Soldering equipment not available yet; prototyping with alligator clips. Linux host.

## Behavior (decided)
All mushrooms show ONE shared status so animations can chase across the cluster:
- w = working: blue comet; n = needs input: amber breathing; r = ready: green fill then glow, fades after 60 s; i = idle: off.
- Claude Code hooks (hooks/hooks-settings.json) call hooks/light.sh, which writes one character to the Flora's serial port.

## Layout
- firmware/flora_status_light/: Arduino sketch (Adafruit NeoPixel library, board "Adafruit Flora").
- hooks/: light.sh and the hook config to merge into ~/.claude/settings.json (copy folder to ~/claude-light or update paths).
- model/: build_shroom.py, a parametric draft generator (trimesh + manifold3d) that writes STLs to model/out/. A starting point only; the owner is rebuilding it by hand in Blender.

## Model design (draft v1)
Vertical stack in the 56 mm base (z from the print bed): Flora cavity z0-10, wire hole z10-14.8, ring seat z14.8-18.4, lid pocket z18.4-22. A translucent lid (2.4 mm) sits on the ring with three 10.4 mm holes on the 15 mm LED circle at 90/210/330 degrees. Three hollow stems (10 mm OD, 7 mm bore) at heights 62/46/35 with hollow dome caps stand on those LEDs.

## Known issues / open items
- The Flora cavity is open at the bottom; the board would fall out. Needs a bottom plate or side slot (owner wants to decide this herself).
- Fit untested: print the base and one stem first.
- Caps are plain domes; owner likes ShroomWhisper-style gills/ridges and Voronoi lattice caps (MakerWorld). Check licenses before remixing.
- LED-to-stem alignment: find which pixel indices sit under the stems (likely 0, 4, 8) and add an offset in the sketch.
- Test whether T3 Code triggers the Claude Code hooks.
- Notification hook also fires on the idle prompt, so the light may turn amber after sitting ready.
- ChatGPT has no hooks; idea is a small browser extension that pings light.sh.
