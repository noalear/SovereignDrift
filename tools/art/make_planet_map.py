# Turn a painted surface map (a ChatGPT equirectangular painting: the whole
# globe unwrapped, north pole at the top) into assets/planets/<look>.jpg,
# the map planet.gdshader wraps round the world.
#   python3 tools/art/make_planet_map.py <painting.png> <look> [width_px] [saturation]
# The painting is stretched to 2:1, its left and right edges are blended
# into each other so the globe has no seam where they meet, and the rows
# nearest the poles are evened out (they are squeezed to a point on the
# globe, where any detail would pinch into a star).
import os
import sys
import numpy as np
from PIL import Image

src, look = sys.argv[1], sys.argv[2]
width = int(sys.argv[3]) if len(sys.argv) > 3 else 2048
sat = float(sys.argv[4]) if len(sys.argv) > 4 else 1.0
img = Image.open(src).convert("RGB")
# trim any frame the painter left round the edge
a = np.asarray(img).astype(np.float32)
lum = a.mean(axis=2)
rows = np.where(lum.mean(axis=1) > 6.0)[0]
cols = np.where(lum.mean(axis=0) > 6.0)[0]
a = a[rows[0]:rows[-1] + 1, cols[0]:cols[-1] + 1]
img = Image.fromarray(a.astype(np.uint8)).resize((width, width // 2), Image.LANCZOS)
a = np.asarray(img).astype(np.float32)
H, W, _ = a.shape
# the seam: in a band at each end, cross-fade to the same map turned half
# way round, which meets itself at the edges
band = W // 10
x = np.arange(W, dtype=np.float32)
d = np.minimum(x, W - 1 - x) / band
m = np.clip(d, 0.0, 1.0)
m = m * m * (3.0 - 2.0 * m)
rolled = np.roll(a, W // 2, axis=1)
a = a * m[None, :, None] + rolled * (1.0 - m[None, :, None])
# the poles: blend each row towards its own average near the top and bottom
y = np.arange(H, dtype=np.float32)
p = np.clip(np.minimum(y, H - 1 - y) / (H * 0.06), 0.0, 1.0)
p = p * p * (3.0 - 2.0 * p)
mean = a.mean(axis=1, keepdims=True)
a = a * p[:, None, None] + mean * (1.0 - p[:, None, None])
# (a painting can come out too vivid: 0.6 keeps 60% of its colour)
grey = a.mean(axis=2, keepdims=True)
a = grey + (a - grey) * sat
out_dir = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "planets")
os.makedirs(out_dir, exist_ok=True)
out = os.path.join(out_dir, look.lower() + ".jpg")
Image.fromarray(np.clip(a, 0, 255).astype(np.uint8)).save(out, quality=90)
print(out, (W, H))
