# Onshape modeling log

The owner is learning Onshape (first time, started 2026-11-20) and builds the model
herself. Teach one lesson at a time, click by click, and review her screenshots; don't
generate the CAD.

Onshape document: "claude status light" (free plan, so it is public). One Part Studio.

## Direction change (2026-11-21): Flora -> XIAO ESP32-C3

The Flora is gone. The board is now a Seeed XIAO ESP32-C3 (see docs/hardware.md), so the
base's round 45.8 mm Flora pocket is replaced by a small rectangular pocket for the XIAO,
with its USB-C port at the outside wall. The ring, lid pocket and wire hole stay as they are.

XIAO ESP32-C3 (measure yours with calipers before modeling; these are nominal):
- board 21 x 17.8 mm; about 3.5-4 mm thick including the metal shield can
- USB-C socket on one short end, flush with or slightly past the board edge
- a flex antenna on a thin lead (measure it); it sticks flat to an inside wall
- the USB-C **plug** on a typical cable is about 12 x 6.5 mm, so the wall opening must clear
  that, and the wall at the opening must be thin enough (under ~5 mm) for the plug to seat

## Done (still valid)

**Lesson 1: setup and blank.** Units mm. Variables at the top of the feature list.
"base extrude": circle `#base_d` (56 mm) on Top plane, extruded `#base_h` (22 mm).

**Lesson 2: pockets.** Each is a sketch plus an Extrude-Remove; sketches hidden after use.

| cut | sketch on | diameter | depth | status |
|---|---|---|---|---|
| lid pocket | top face | `#lid_d` (44) | `#lid_depth` (3.6) | keep |
| ring seat | lid pocket floor | `#ring_od + 2 * #fit` | `#ring_t` | keep |
| Flora pocket | bottom face | `#flora_d + 2 * #fit` | `#flora_cav_h` | **replace** (lesson 3) |
| wire hole | Flora pocket ceiling | 14 mm | Up to next | keep (re-check after lesson 3) |

**Test print 1** (2026-11-21): printed upside down by mistake (bad advice), and the bare
ring rocked on the saggy bridged seat floor. Always print the base **upright** (open
bottom on the bed): the ring seat floor is then a flat top surface. The slicer's "floating
cantilever" warning about the pocket ceiling around the wire hole is harmless. The
dimensions themselves were never shown to be wrong.

## Next

**Lesson 3: swap the Flora pocket for a XIAO pocket.** Teaches editing an existing model.
1. Measure the XIAO (length, width, thickness with the shield, how far the USB-C sticks out).
2. Variables: add `xiao_l`, `xiao_w`, `xiao_t`, `usb_w` (~13), `usb_h` (~7.5); delete the
   Flora ones once nothing uses them (Onshape shows errors on anything that still does).
3. Delete (or suppress) the Flora pocket sketch and cut. The wire hole's "Up to next" now
   needs a new start face, so expect it to go red; that's the lesson in reading errors.
4. New cavity from the bottom: a rectangle `#xiao_w + 2*#fit` wide that runs from the
   outside wall inward `#xiao_l + #fit`, `#xiao_t` plus wiring room deep. Plus a roomier
   area under the wire hole for the three wires and the capacitor.
5. USB-C opening: a `#usb_w` x `#usb_h` rectangle on the wall, centered on the socket.
6. Optional: shrink `base_d` now that a 45 mm board doesn't set the size (the 44 mm lid
   pocket and ~37.6 mm ring seat do). Change one number and look at what follows.

**Lesson 4: press-fit bottom plate** (decided: option A). Second part in the same Part
Studio: a disc that fills the opening, with a rim that presses in, holding the XIAO up
against its pocket. Leave a flat spot for the antenna.

**Later:** translucent lid with three stem holes on the 15 mm LED circle (90/210/330 deg),
then stems and caps (owner likes gills/ridges or Voronoi caps). Set `PIXEL_OFFSET` in
`firmware/include/config.h` once the stems are on.

## Onshape gotchas she has hit (warn early)

- Keyboard shortcuts only work inside a sketch, with focus on the canvas. Toolbar icons work too.
- Blue sketch = underdefined. Start circles on the origin (or add Coincident) and dimension them; all black before ✓.
- Extrude: pick the sketch by clicking its name in the feature list. Clicking in the viewport often grabs the whole face.
- Extrude Remove: tick **Merge with all**, otherwise "Boolean resulted in no geometry change".
- Use the Opposite direction (two-arrow) icon when the preview points into empty air.
- After moving a sketch to a new face, re-check the extrude direction.
- Hide finished sketches; they float in the model and block clicks.
- Section view is in the dropdown under the view cube (top right). Use the Front plane, and hide the plane's big square.
- She sends screenshots by scp to charon (~/Claude/) when working with Claude Code.
