# CLAUDE.md

Project context for Claude Code. Read this first.

## Goal
A cute desk status light for Claude Code (and ideally T3 Code, Codex, ChatGPT): an Adafruit Flora driving a 12-LED NeoPixel ring, housed in a 3D-printed cluster of translucent mushrooms. The owner designs the model herself in Onshape (switched from Blender, 2026-11-20), so teach and review step by step rather than generating CAD.

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
- model/: build_shroom.py, a parametric draft generator (trimesh + manifold3d) that writes STLs to model/out/. A reference for dimensions only; the owner is building the real model in Onshape.

## Modeling progress
The real model lives in Onshape (document "claude status light"); progress, variables and lesson plan are in model/ONSHAPE.md. Continue teaching from there.

## Model design (draft v1, Python reference)
Vertical stack in the 56 mm base (z from the print bed): Flora cavity z0-10, wire hole z10-14.8, ring seat z14.8-18.4, lid pocket z18.4-22. A translucent lid (2.4 mm) sits on the ring with three 10.4 mm holes on the 15 mm LED circle at 90/210/330 degrees. Three hollow stems (10 mm OD, 7 mm bore) at heights 62/46/35 with hollow dome caps stand on those LEDs.

## Known issues / open items
- Bottom closure DECIDED (2026-11-20): option A, a separate press-fit bottom plate; USB notch runs to the bottom edge. Not modeled yet (lesson 3).
- Fit untested: owner is test-printing the Onshape base before lesson 3. See model/ONSHAPE.md.
- Caps are plain domes; owner likes ShroomWhisper-style gills/ridges and Voronoi lattice caps (MakerWorld). Check licenses before remixing.
- LED-to-stem alignment: find which pixel index sits under the tallest stem and set PIXEL_OFFSET in the sketch.
- Test whether T3 Code triggers the Claude Code hooks.
- Hooks run where Claude Code runs (often the remote box charon), but the Flora is on the desk computer's USB. Needs a bridge (e.g. ssh reverse tunnel or a tiny listener on the desk machine) before charon sessions can drive it.
- ChatGPT has no hooks; idea is a small browser extension that pings light.sh.
