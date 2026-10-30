# Cut a fitted-systems sheet (a ChatGPT painting on black: three rows of
# three, each part seen from above, centred in its cell) into
# assets/parts/<icon>_<empire>.png, the pictures ShipArt draws for an
# empire's reactor, shield emitters, sensor dish and so on.
#   python3 tools/art/cut_systems.py <sheet.png> <empire> [longest_px]
# Order on the sheet: reactor, shield dome, sensor dish / jammer spine,
# repair hatch, regenerator / ram prow (pointing right), booster pod
# (exhaust left), quantum drive (bell left).
import os
import sys
import numpy as np
from PIL import Image
from scipy import ndimage

ICONS = ["reactor", "shield", "sensor", "cloak", "repair", "regen", "ram", "booster", "qdrv"]
src, empire = sys.argv[1], sys.argv[2]
px = int(sys.argv[3]) if len(sys.argv) > 3 else 192
root = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "parts")
a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)
H, W, _ = a.shape


def splits(profile: np.ndarray, n: int) -> list:
	# where to cut a strip into n: the middle of the emptiest run near each
	# even split (a part that pokes over the even line is not cut in half)
	size = profile.size
	cuts = [0]
	for i in range(1, n):
		lo, hi = size * i // n - size // 8, size * i // n + size // 8
		empty = profile[lo:hi] < 10.0
		best, run, start = (0, size * i // n), 0, 0
		for j, e in enumerate(empty):
			if e:
				if run == 0:
					start = j
				run += 1
				if run > best[0]:
					best = (run, lo + start + run // 2)
			else:
				run = 0
		cuts.append(best[1])
	return cuts + [size]


xs = splits(a.max(axis=2).max(axis=0), 3)
for k, icon in enumerate(ICONS):
	r, col = k // 3, k % 3
	strip = a[:, xs[col]:xs[col + 1]]
	ys = splits(strip.max(axis=2).max(axis=1), 3)
	c = strip[ys[r]:ys[r + 1]]
	m = c.max(axis=2)
	alpha = np.clip((m - 14.0) / 40.0, 0.0, 1.0)
	# tendrils and fins a hair apart from the body still belong to it
	lab, n = ndimage.label(ndimage.binary_dilation(alpha > 0.3, iterations=4))
	sizes = ndimage.sum(np.ones_like(lab), lab, index=range(1, n + 1))
	own = lab == (int(np.argmax(sizes)) + 1)
	alpha = alpha * own
	# dark gaps inside the part are metal in shadow, not space
	body = ndimage.binary_fill_holes(ndimage.binary_closing(alpha > 0.3, iterations=3))
	hole = body & (alpha < 0.95)
	alpha = np.where(hole, 1.0, alpha)
	rgb = np.where(alpha[..., None] > 0.01, c / np.maximum(alpha[..., None], 0.01), 0)
	rgb = np.where(hole[..., None], np.maximum(c, 6.0), rgb)
	rgb = np.clip(rgb, 0, 255)
	rows = np.where(body.any(axis=1))[0]
	cols = np.where(body.any(axis=0))[0]
	y0, y1, x0, x1 = rows[0], rows[-1] + 1, cols[0], cols[-1] + 1
	img = Image.fromarray(np.dstack([rgb, alpha * 255]).astype(np.uint8)[y0:y1, x0:x1], "RGBA")
	s = px / max(img.width, img.height)
	img = img.resize((max(1, round(img.width * s)), max(1, round(img.height * s))), Image.LANCZOS)
	name = icon if empire == "terran" else icon + "_" + empire
	img.save(os.path.join(root, name + ".png"))
	print(name, img.size, "aspect %.2f" % (img.width / img.height))
