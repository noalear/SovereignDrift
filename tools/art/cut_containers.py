# Cut the ChatGPT sheet of six cargo containers (seen from above, on black,
# three on top and three below) into transparent PNGs, one per resource:
#   python3 tools/art/cut_containers.py <sheet.png> <out_dir> [width] [icon_width]
# Order: metals, ice, fuel on top; food, exotics, alloys below.  Writes
# <out_dir>/<resource>.png (the crate in space, width px) and
# <out_dir>/icons/<resource>.png (the cargo panels' icon, icon_width px);
# out_dir is normally assets/cargo.  Then run godot --import.
import sys
import numpy as np
from PIL import Image, ImageFilter
from scipy import ndimage
src, out = sys.argv[1], sys.argv[2]
width = int(sys.argv[3]) if len(sys.argv) > 3 else 160
icon_width = int(sys.argv[4]) if len(sys.argv) > 4 else 48
import os
os.makedirs(out + "/icons", exist_ok=True)
names = ["metals", "ice", "fuel", "food", "exotics", "alloys"]
im = Image.open(src).convert("RGB")
a = np.asarray(im)
mask = a.max(axis=2) > 14
lab, n = ndimage.label(ndimage.binary_dilation(mask, iterations=3))
sizes = ndimage.sum(np.ones_like(lab), lab, index=range(1, n + 1))
keep = [i + 1 for i in np.argsort(sizes)[::-1][:6]]
boxes = []
for k in keep:
	ys, xs = np.nonzero(lab == k)
	boxes.append((ys.min(), ys.max(), xs.min(), xs.max(), k))
# rows by centre height, then left to right
boxes.sort(key=lambda b: ((b[0] + b[1]) / 2 > a.shape[0] / 2, b[2]))
for name, (y0, y1, x0, x1, k) in zip(names, boxes):
	m = ndimage.binary_fill_holes((lab == k) & mask)
	m = ndimage.binary_erosion(m, iterations=1)
	alpha = Image.fromarray((m * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.8))
	rgba = im.copy()
	rgba.putalpha(alpha)
	pad = 2
	crop = rgba.crop((x0 - pad, y0 - pad, x1 + pad + 1, y1 + pad + 1))
	for w, path in [(width, "%s/%s.png" % (out, name)), (icon_width, "%s/icons/%s.png" % (out, name))]:
		crop.resize((w, round(crop.height * w / crop.width)), Image.LANCZOS).save(path)
	print(name, crop.size)
