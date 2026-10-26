# Cut the hardpoint socket sheet (four empty turret rings in a row on
# black: Terran, Arcean, Drengin, Pirate, feed conduits pointing left)
# into assets/parts/socket_<empire>.png.
#   python3 tools/art/cut_sockets.py <ref-hull-sockets.png> <out_dir> [px]
import sys
import numpy as np
from PIL import Image
from scipy import ndimage
src, out = sys.argv[1], sys.argv[2]
px = int(sys.argv[3]) if len(sys.argv) > 3 else 160
a = np.asarray(Image.open(src).convert("RGB")).astype(np.float32)
H, W, _ = a.shape
for k, name in enumerate(["terran", "arcean", "drengin", "pirate"]):
    x0, x1 = W * k // 4, W * (k + 1) // 4
    c = a[:, x0:x1]
    m = c.max(axis=2)
    alpha = np.clip((m - 12.0) / 40.0, 0.0, 1.0)
    # keep the biggest piece, then fill the ring's dark centre: it is the
    # empty socket, not space
    lab, n = ndimage.label(ndimage.binary_dilation(alpha > 0.3, iterations=2))
    sizes = ndimage.sum(np.ones_like(lab), lab, index=range(1, n + 1))
    own = lab == (int(np.argmax(sizes)) + 1)
    alpha = alpha * own
    body = ndimage.binary_fill_holes(ndimage.binary_closing(alpha > 0.3, iterations=3))
    hole = body & (alpha < 0.95)
    alpha = np.where(hole, 1.0, alpha)
    rgb = np.where(alpha[..., None] > 0.01, c / np.maximum(alpha[..., None], 0.01), 0)
    rgb = np.where(hole[..., None], np.maximum(c, 6.0), rgb)
    rgb = np.clip(rgb, 0, 255)
    rows = np.where(body.any(axis=1))[0]
    cols = np.where(body.any(axis=0))[0]
    y0, y1, cx0, cx1 = rows[0], rows[-1] + 1, cols[0], cols[-1] + 1
    # square box centred on the ring (the right part), conduits stick out left
    img = Image.fromarray(np.dstack([rgb, alpha * 255]).astype(np.uint8)[y0:y1, cx0:cx1], "RGBA")
    s = px / img.height
    img = img.resize((max(1, round(img.width * s)), px), Image.LANCZOS)
    img.save(f"{out}/socket_{name}.png")
    print(name, img.size)
