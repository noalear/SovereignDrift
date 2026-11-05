#!/usr/bin/env python3
"""Make the painted hulls solid again.

The ship sheets were cut from paintings on black, with alpha read from
brightness, so every dark panel, shadow and vent inside a hull came out
part transparent and the stars and nebula showed through the ship.  This
pass fills the body: inside the hull's silhouette every pixel becomes
opaque and gets its painted colour back (the cutter un-mixed black out of
the colours, so colour * alpha is the paint as drawn).  Real gaps between
parts (a catamaran's slot, the space between a station's spokes) are kept:
a hole whose core is truly black is space, a hole that is merely dark is
paint.

    python3 tools/art/fill_hulls.py <png> [<png> ...]      # rewrites in place
    python3 tools/art/fill_hulls.py --check <png> ...      # only report
"""
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

SPACE_CORE = 0.3      # a hole is space when this share of it is black 2 px in from its rim
MIN_GAP = 150         # smaller holes are always paint
EDGE = 2              # pixels of anti-aliased outline left alone


def fill(path: str, write: bool = True) -> tuple[int, int]:
	im = Image.open(path).convert("RGBA")
	px = np.asarray(im).astype(np.float32)
	a = px[..., 3] / 255.0
	body = a > 0.5
	filled = ndimage.binary_fill_holes(ndimage.binary_closing(body, iterations=3))
	holes = filled & ~body
	labels, n = ndimage.label(holes)
	space = np.zeros_like(body)
	if n:
		idx = range(1, n + 1)
		sizes = ndimage.sum(np.ones_like(labels), labels, index=idx)
		core = ndimage.binary_erosion(a < 0.03, iterations=2)
		cores = ndimage.sum(core.astype(np.float32), labels, index=idx)
		keep = [i + 1 for i in range(n) if sizes[i] >= MIN_GAP and cores[i] >= SPACE_CORE * sizes[i]]
		if keep:
			space = np.isin(labels, keep)
	inner = ndimage.binary_erosion(filled, iterations=EDGE) & ~ndimage.binary_dilation(space, iterations=EDGE)
	fix = inner & (a < 0.999)
	count = int(fix.sum())
	if write and count:
		rgb = px[..., :3] * a[..., None]       # back to the paint over black
		out = px.copy()
		out[..., :3] = np.where(fix[..., None], rgb, px[..., :3])
		out[..., 3] = np.where(fix, 255.0, px[..., 3])
		Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGBA").save(path, optimize=True)
	return count, int(space.sum())


if __name__ == "__main__":
	args = sys.argv[1:]
	check = "--check" in args
	for p in [x for x in args if x != "--check"]:
		fixed, kept = fill(p, write=not check)
		print(f"{p}: {fixed} px filled, {kept} px of gaps kept")
