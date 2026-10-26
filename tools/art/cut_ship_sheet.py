# Cut a ChatGPT reference sheet of six hulls (Scout..Dreadnought, nose
# right, on black, names along the bottom) into transparent PNGs.  Ships
# stacked one above the other are read top to bottom instead (the hull
# sheets ref-hulls-<empire>-small/large.png: pass label_top 1.0).
#   python3 tools/art/cut_ship_sheet.py <sheet.png> <out_dir> [names] [label_top]
# names: comma-separated, left to right (default the six hulls); label_top:
# the fraction of the height where the labels start (default 0.82).  The
# supply ships: ... ref-supply-ships.png out Terran,Arcean,Drengin 0.72
# Then copy the PNGs into assets/ships/<empire>/ and run godot --import.
import sys
import numpy as np
from PIL import Image
src, out = sys.argv[1], sys.argv[2]
im = Image.open(src).convert("RGB")
a = np.asarray(im).astype(np.float32)
H, W, _ = a.shape
mx = a.max(axis=2)
names = sys.argv[3].split(",") if len(sys.argv) > 3 else ["Scout", "Destroyer", "Cruiser", "BattleCruiser", "Battleship", "Dreadnought"]
label_top = int(H * (float(sys.argv[4]) if len(sys.argv) > 4 else 0.82))   # names sit below this line
mask = mx > 22
mask[label_top:, :] = False
from scipy import ndimage
# group pixels into ships: dilate so a ship's thin antennae and pods join
# its hull, then take the biggest blobs, one per name, left to right
grown = ndimage.binary_dilation(mask, iterations=4)
lab, n = ndimage.label(grown)
sizes = ndimage.sum(np.ones_like(lab), lab, index=range(1, n + 1))
keep = sorted(range(1, n + 1), key=lambda i: -sizes[i - 1])[:len(names)]
boxes = ndimage.find_objects(lab)
blobs = sorted([(boxes[i - 1][1].start, boxes[i - 1][1].stop, i) for i in keep])
# a stacked sheet: the ships overlap left to right, so order them by height
ys = sorted([(boxes[i - 1][0].start, i) for i in keep])
stacked = any(b[0] < a[1] - 10 for a, b in zip(blobs, blobs[1:]))
if stacked:
    blobs = [(boxes[i - 1][1].start, boxes[i - 1][1].stop, i) for _, i in ys]
merged = [(x0, x1) for x0, x1, _ in blobs]
labels_kept = [i for _, _, i in blobs]
print("runs", merged)
assert len(merged) == len(names), len(merged)
for name, (x0, x1), li in zip(names, merged, labels_kept):
    own = (lab == li)
    sub = own[:, x0:x1]
    rows = np.where(sub.sum(axis=1) > 0)[0]
    y0, y1 = rows[0], rows[-1] + 1
    pad = 3
    x0p, x1p, y0p, y1p = max(x0 - pad, 0), min(x1 + pad, W), max(y0 - pad, 0), min(y1 + pad, label_top)
    c = a[y0p:y1p, x0p:x1p]
    m = c.max(axis=2)
    alpha = np.clip((m - 10.0) / 45.0, 0.0, 1.0)
    # only this ship: blank out a neighbour's wing that pokes into the box
    region = ndimage.binary_dilation(own, iterations=3)[y0p:y1p, x0p:x1p]
    alpha = alpha * region
    # dark shading inside the hull is paint, not space: fill enclosed holes
    # only small holes: the black crescents behind antenna arcs are space
    body = ndimage.binary_closing(alpha > 0.35, iterations=2)
    holes = ndimage.binary_fill_holes(body) & ~body
    hl, hn = ndimage.label(holes)
    if hn:
        hs = ndimage.sum(np.ones_like(hl), hl, index=range(1, hn + 1))
        small = np.isin(hl, [i + 1 for i in range(hn) if hs[i] < 350])
        alpha = np.where(ndimage.binary_dilation(small, iterations=1), np.maximum(alpha, 1.0), alpha)
    # un-mix the black background from the soft edges
    rgb = np.where(alpha[..., None] > 0.01, c / np.maximum(alpha[..., None], 0.01), 0)
    rgb = np.clip(rgb, 0, 255)
    rgba = np.dstack([rgb, alpha * 255]).astype(np.uint8)
    Image.fromarray(rgba, "RGBA").save(f"{out}/{name}.png")
    print(name, rgba.shape[1], "x", rgba.shape[0])
