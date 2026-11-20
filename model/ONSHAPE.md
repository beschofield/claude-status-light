# Onshape modeling log

The owner is learning Onshape (first time, started 2026-11-20) and builds the model
herself. Teach one lesson at a time, click by click, and review her screenshots; don't
generate the CAD. The Python draft in this folder is only a dimension reference.

Onshape document: "claude status light" (free plan, so it is public). One Part Studio, one part so far.

## Done

**Lesson 1: setup and blank.** Units mm. Variables (all Length, kept at the top of the feature list):

| variable | value | | variable | value |
|---|---|---|---|---|
| flora_d | 45 mm | | fit | 0.4 mm |
| flora_t | 7 mm | | base_d | 56 mm |
| ring_od | 36.8 mm | | base_h | 22 mm |
| ring_id | 23.3 mm | | flora_cav_h | 10 mm |
| ring_t | 3.6 mm | | lid_d | 44 mm |
| | | | lid_depth | 3.6 mm |

"base extrude": circle `#base_d` on Top plane, extruded `#base_h`.

**Lesson 2: pockets.** Each is a sketch plus an Extrude-Remove; sketches are hidden after use.
Feature list, in order: base extrude, lid pocket/lid extrude, ring pocket/ring extrude,
flora pocket/flora extrude, wire pocket/wire extrude.

| cut | sketch on | diameter | depth |
|---|---|---|---|
| lid pocket | top face | `#lid_d` | `#lid_depth` |
| ring seat | lid pocket floor | `#ring_od + 2 * #fit` | `#ring_t` |
| Flora pocket | bottom face | `#flora_d + 2 * #fit` | `#flora_cav_h` |
| wire hole | Flora pocket ceiling | 14 mm | Up to next |

Section view on the Front plane confirmed the stepped profile.

## Next

1. **Test print** (owner doing this next): right-click Part 1, Export, STL, mm. Print upside down
   (open Flora pocket up, no supports). Check that the Flora and the ring drop in. Adjust `fit` if needed.
2. **Lesson 3: bottom plate (decided: option A, press-fit).** Second part in the same Part Studio:
   a disc that fills the Flora pocket opening with a press-fit rim. Plus the USB notch in the wall,
   about 11 mm wide and 7 mm tall, running down to the bottom edge. Its position must match where the
   Flora's USB port faces when the board is in the pocket.
3. Later: translucent lid with three stem holes on the 15 mm LED circle (90/210/330 deg),
   then stems and caps (owner wants gills/ridges or Voronoi caps).

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
