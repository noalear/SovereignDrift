# Cut a munitions sheet (a ChatGPT painting on black: two rows of three,
# every item nose right) into assets/munitions/<empire>/<kind>.png.
#   python3 tools/art/cut_munitions.py <sheet.png> <empire> [longest_px]
# Order on the sheet: missile, seeker, torpedo / shell, mine, drone.
#   python3 tools/art/cut_munitions.py <sheet.png> ammo [longest_px]
# cuts the gun-ammunition sheet instead: two rows of four, slugs on top
# and rapid-fire rounds below, Terran, Arcean, Drengin, Pirate left to
# right, into <empire>/slug.png and <empire>/bolt.png.
import os
import sys
import numpy as np
from PIL import Image
from scipy import ndimage

KINDS = ["missile", "seeker", "torpedo", "shell", "mine", "drone"]
src, empire = sys.argv[1], sys.argv[2]
px = int(sys.argv[3]) if len(sys.argv) > 3 else 192
root = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "munitions")
a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)
H, W, _ = a.shape
if empire == "ammo":
	grid = 4
	jobs = [(r, c, e, kind) for r, kind in enumerate(["slug", "bolt"]) for c, e in enumerate(["terran", "arcean", "drengin", "pirate"])]
else:
	grid = 3
	jobs = [(k // 3, k % 3, empire, kind) for k, kind in enumerate(KINDS)]
for r, c, emp, kind in jobs:
	out_dir = os.path.join(root, emp)
	os.makedirs(out_dir, exist_ok=True)
	cell = a[H * r // 2:H * (r + 1) // 2, W * c // grid:W * (c + 1) // grid]
	m = cell.max(axis=2)
	alpha = np.clip((m - 12.0) / 36.0, 0.0, 1.0)
	# spikes and fins a hair apart from the body still belong to it
	lab, n = ndimage.label(ndimage.binary_dilation(alpha > 0.25, iterations=5))
	sizes = ndimage.sum(np.ones_like(lab), lab, index=range(1, n + 1))
	own = lab == (int(np.argmax(sizes)) + 1)
	alpha = alpha * own
	# dark gaps inside the item are metal in shadow, not space
	body = ndimage.binary_fill_holes(ndimage.binary_closing(alpha > 0.3, iterations=3))
	hole = body & (alpha < 0.95)
	alpha = np.where(hole, 1.0, alpha)
	rgb = np.where(alpha[..., None] > 0.01, cell / np.maximum(alpha[..., None], 0.01), 0)
	rgb = np.where(hole[..., None], np.maximum(cell, 6.0), rgb)
	rgb = np.clip(rgb, 0, 255)
	rows = np.where((alpha > 0.05).any(axis=1))[0]
	cols = np.where((alpha > 0.05).any(axis=0))[0]
	y0, y1, x0, x1 = rows[0], rows[-1] + 1, cols[0], cols[-1] + 1
	img = Image.fromarray(np.dstack([rgb, alpha * 255]).astype(np.uint8)[y0:y1, x0:x1], "RGBA")
	s = px / max(img.width, img.height)
	img = img.resize((max(1, round(img.width * s)), max(1, round(img.height * s))), Image.LANCZOS)
	img.save(os.path.join(out_dir, kind + ".png"))
	print(emp, kind, img.size, "aspect %.2f" % (img.width / img.height))
