# Cut a turret sheet (a ChatGPT painting on black: two rows of four, every
# turret seen from above, ring on the left, barrel pointing right) into
# assets/parts/<icon>_<empire>.png.  Terran turrets take the plain icon
# name, which is also the picture every other empire falls back to.
#   python3 tools/art/cut_turrets.py <sheet.png> <empire> [height_px]
# Order on the sheet: laser, rapid, cannon, torpedo tube (grenade) /
# missile rack, drone bay, disruptor, mine layer.  Prints each cut's size,
# its pivot (ShipArt.TURRET_PIVOT) and the length that keeps the ring the
# same size as the mass driver's (ShipArt.TURRET_LENGTH).
import os
import sys
import numpy as np
from PIL import Image
from scipy import ndimage

ICONS = ["laser", "rapid", "cannon", "grenade", "missile", "drone", "disruptor", "mine"]
src, empire = sys.argv[1], sys.argv[2]
px = int(sys.argv[3]) if len(sys.argv) > 3 else 140
root = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "parts")
a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)
H, W, _ = a.shape
for k, icon in enumerate(ICONS):
	r, col = k // 4, k % 4
	c = a[H * r // 2:H * (r + 1) // 2, W * col // 4:W * (col + 1) // 4]
	m = c.max(axis=2)
	alpha = np.clip((m - 14.0) / 40.0, 0.0, 1.0)
	# barrels and prongs a hair apart from the ring still belong to it
	lab, n = ndimage.label(ndimage.binary_dilation(alpha > 0.3, iterations=4))
	sizes = ndimage.sum(np.ones_like(lab), lab, index=range(1, n + 1))
	own = lab == (int(np.argmax(sizes)) + 1)
	alpha = alpha * own
	# dark gaps inside the turret are metal in shadow, not space
	body = ndimage.binary_fill_holes(ndimage.binary_closing(alpha > 0.3, iterations=3))
	hole = body & (alpha < 0.95)
	alpha = np.where(hole, 1.0, alpha)
	rgb = np.where(alpha[..., None] > 0.01, c / np.maximum(alpha[..., None], 0.01), 0)
	rgb = np.where(hole[..., None], np.maximum(c, 6.0), rgb)
	rgb = np.clip(rgb, 0, 255)
	rows = np.where(body.any(axis=1))[0]
	cols = np.where(body.any(axis=0))[0]
	y0, y1, x0, x1 = rows[0], rows[-1] + 1, cols[0], cols[-1] + 1
	# the ring is the tallest part: its centre column is the middle of the
	# columns that are at least 90% of the full height
	height = body[y0:y1, x0:x1].sum(axis=0)
	tall = np.where(height >= 0.9 * height.max())[0]
	pivot = (tall[0] + tall[-1]) * 0.5 / (x1 - x0)
	img = Image.fromarray(np.dstack([rgb, alpha * 255]).astype(np.uint8)[y0:y1, x0:x1], "RGBA")
	s = px / img.height
	img = img.resize((max(1, round(img.width * s)), px), Image.LANCZOS)
	name = icon if empire == "terran" else icon + "_" + empire
	img.save(os.path.join(root, name + ".png"))
	print('"%s": length %.2f pivot %.3f size %s' % (name, 1.44 * img.width / img.height, pivot, img.size))
