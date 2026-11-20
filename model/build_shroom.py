"""Parametric mushroom-cluster status light for Adafruit Flora + 12-LED NeoPixel ring.
Run: python3 -I build_shroom.py <outdir>
All units mm. Edit PARAMS and re-run to iterate.
"""
import sys, os
import numpy as np
import trimesh
from trimesh.creation import cylinder, box

P = dict(
    # electronics (from Adafruit specs)
    ring_od=36.8, ring_thick=3.6, flora_d=45.0, flora_t=7.0,
    clear=0.3,                 # fit clearance
    led_circle_r=15.0,         # radius of the LED circle on the ring (approx.)
    # base
    base_d=56.0, base_h=22.0,
    flora_cav_h=10.0, wire_hole_d=14.0,
    usb_w=11.0, usb_h=7.0,
    pocket_d=44.0,             # lid pocket
    # lid
    lid_t=2.4,
    # stems / caps
    stem_od=10.0, bore_d=7.0, hole_clear=0.2,
    mush=[  # stem height, cap radius, angle(deg) around ring
        dict(H=62, R=16, ang=90),
        dict(H=46, R=13, ang=210),
        dict(H=32, R=10, ang=330),
    ],
    cap_aspect=0.6, wall=0.8,
)


def cyl(r, h, z0=0.0, x=0.0, y=0.0, sections=96):
    c = cylinder(radius=r, height=h, sections=sections)
    c.apply_translation([x, y, z0 + h / 2])
    return c


def bool_(a, b, op):
    return trimesh.boolean.boolean_manifold([a, b], operation=op)


def dome(R, Hc, n=48):
    """Solid half-ellipsoid dome, flat bottom at z=0."""
    t = np.linspace(0, np.pi / 2, n)
    prof = [(R * np.cos(a), Hc * np.sin(a)) for a in t]  # (r,z) from rim up to apex
    pts = [(0, 0)] + prof  # axis->rim along bottom, then arc to apex
    pts.append((0, Hc))
    return revolve(pts)


def revolve(pts, sections=96):
    # pts: list of (r, z) forming a closed polygon in the r>=0 half plane
    from shapely.geometry import Polygon
    poly = Polygon(pts)
    return trimesh.creation.revolve(np.array(poly.exterior.coords)[:-1], angle=2 * np.pi, sections=sections)


def make_cap(R, p):
    Hc = R * p["cap_aspect"]
    w = p["wall"]
    outer = dome(R, Hc)
    inner = dome(R - w, Hc - w)
    inner.apply_translation([0, 0, w])  # leaves a flat floor w thick
    return bool_(outer, inner, "difference"), Hc


def make_mushroom(m, p):
    H, R = m["H"], m["R"]
    cap, Hc = make_cap(R, p)
    cap.apply_translation([0, 0, H])               # cap floor at z=H..H+w
    stem = cyl(p["stem_od"] / 2, H + p["wall"] + 0.4)
    mush = bool_(stem, cap, "union")
    bore = cyl(p["bore_d"] / 2, H + p["wall"] + 1.6, z0=-0.1)
    mush = bool_(mush, bore, "difference")
    return mush


def make_base(p):
    c = p["clear"]
    base = cyl(p["base_d"] / 2, p["base_h"])
    # Flora cavity from below
    base = bool_(base, cyl((p["flora_d"] + 2 * c) / 2, p["flora_cav_h"], z0=-0.1), "difference")
    # ring seat; ring top ends flush with the pocket ledge
    ring_floor = p["base_h"] - 7.2            # z of ring seat
    ring_top = ring_floor + p["ring_thick"]
    base = bool_(base, cyl((p["ring_od"] + 2 * c) / 2, p["base_h"] - ring_floor + 0.1, z0=ring_floor), "difference")
    # lid pocket above ring
    base = bool_(base, cyl(p["pocket_d"] / 2, p["base_h"] - ring_top + 0.1, z0=ring_top), "difference")
    # wire hole through the floor between ring seat and Flora cavity
    base = bool_(base, cyl(p["wire_hole_d"] / 2, ring_floor - p["flora_cav_h"] + 0.4, z0=p["flora_cav_h"] - 0.2), "difference")
    # USB notch at -Y
    notch = box(extents=[p["usb_w"], p["base_d"] / 2 + 2, p["usb_h"]])
    notch.apply_translation([0, -(p["base_d"] / 4 + 1), 1.0 + p["usb_h"] / 2])
    base = bool_(base, notch, "difference")
    return base, ring_top


def make_lid(p):
    lid = cyl((p["pocket_d"] - 0.4) / 2, p["lid_t"])
    for m in p["mush"]:
        a = np.radians(m["ang"])
        x, y = p["led_circle_r"] * np.cos(a), p["led_circle_r"] * np.sin(a)
        hole = cyl(p["stem_od"] / 2 + p["hole_clear"], p["lid_t"] + 0.2, z0=-0.1, x=x, y=y)
        lid = bool_(lid, hole, "difference")
    return lid


def main(out):
    os.makedirs(out, exist_ok=True)
    base, ring_top = make_base(P)
    lid = make_lid(P)
    base.export(os.path.join(out, "base.stl"))
    lid.export(os.path.join(out, "lid_translucent.stl"))
    parts = {"base": base, "lid": lid}
    for i, m in enumerate(P["mush"], 1):
        mu = make_mushroom(m, P)
        mu.export(os.path.join(out, f"mushroom_{i}_H{m['H']}.stl"))
        # assembled position: stem bottom rests on ring top, over its LED
        a = np.radians(m["ang"])
        mu_a = mu.copy()
        mu_a.apply_translation([P["led_circle_r"] * np.cos(a), P["led_circle_r"] * np.sin(a), ring_top])
        parts[f"m{i}"] = mu_a
    lid_a = lid.copy(); lid_a.apply_translation([0, 0, ring_top])
    parts["lid"] = lid_a
    asm = trimesh.util.concatenate(list(parts.values()))
    asm.export(os.path.join(out, "assembled_preview.stl"))
    for k, v in parts.items():
        print(k, "watertight" if v.is_watertight else "NOT watertight", np.round(v.bounds[1] - v.bounds[0], 1))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "out")
