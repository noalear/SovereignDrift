# Cut one part out of a row of parts painted on black (a ChatGPT sheet
# with a few variations side by side) into assets/parts/<name>.png.
#   python3 tools/art/cut_part.py <sheet.png> <index> <count> <out.png> [height_px]
# Prints the cut's size and where the turret ring's centre sits, as a
# fraction of the width (the turret's pivot, ShipArt.TURRET_PIVOT).
import sys
import numpy as np
from PIL import Image
from scipy import ndimage
src, k, count, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
px = int(sys.argv[5]) if len(sys.argv) > 5 else 120
a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)
H, W, _ = a.shape
x0, x1 = W * k // count, W * (k + 1) // count
c = a[:, x0:x1]
m = c.max(axis=2)
alpha = np.clip((m - 14.0) / 40.0, 0.0, 1.0)
lab, n = ndimage.label(ndimage.binary_dilation(alpha > 0.3, iterations=2))
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
y0, y1, cx0, cx1 = rows[0], rows[-1] + 1, cols[0], cols[-1] + 1
# the ring is the tallest part: its centre column is the middle of the
# columns that are at least 90% of the full height
height = body[y0:y1, cx0:cx1].sum(axis=0)
tall = np.where(height >= 0.9 * height.max())[0]
pivot = (tall[0] + tall[-1]) * 0.5 / (cx1 - cx0)
img = Image.fromarray(np.dstack([rgb, alpha * 255]).astype(np.uint8)[y0:y1, cx0:cx1], "RGBA")
s = px / img.height
img = img.resize((max(1, round(img.width * s)), px), Image.LANCZOS)
img.save(out)
print(out, img.size, "pivot %.3f" % pivot)
