#!/usr/bin/env python3
"""Paint the deep-space backdrop: a seamless (tiling) square of soft
nebula clouds and dark dust lanes, drawn far behind the stars.

Every field here is filtered noise made in Fourier space, so it wraps at
the edges by construction and the tile repeats without a seam; the clouds
are bent by a second field (domain warping) so they curl and stream
instead of looking like blobs.

    python3 tools/art/make_nebula.py [out.png] [size] [seed]
"""
import sys

import numpy as np
from PIL import Image

out = sys.argv[1] if len(sys.argv) > 1 else "assets/backdrop/nebula.jpg"
N = int(sys.argv[2]) if len(sys.argv) > 2 else 2048
seed = int(sys.argv[3]) if len(sys.argv) > 3 else 11
rng = np.random.default_rng(seed)

fy = np.fft.fftfreq(N)[:, None]
fx = np.fft.fftfreq(N)[None, :]
f = np.sqrt(fx * fx + fy * fy)
f[0, 0] = 1.0


def field(beta: float, lo: float = 0.0, hi: float = 0.5) -> np.ndarray:
	"""Periodic noise with a 1/f^beta spectrum between two cut-offs,
	scaled to 0..1."""
	spec = (rng.normal(size=(N, N)) + 1j * rng.normal(size=(N, N))) / f ** beta
	spec *= (f >= lo) & (f <= hi)
	spec[0, 0] = 0
	v = np.real(np.fft.ifft2(spec))
	v -= v.min()
	return v / v.max()


def warp(v: np.ndarray, dx: np.ndarray, dy: np.ndarray) -> np.ndarray:
	"""Sample v at (x + dx, y + dy), wrapping, bilinear."""
	y, x = np.mgrid[0:N, 0:N].astype(np.float32)
	sx = (x + dx) % N
	sy = (y + dy) % N
	x0 = np.floor(sx).astype(int)
	y0 = np.floor(sy).astype(int)
	tx = sx - x0
	ty = sy - y0
	x1 = (x0 + 1) % N
	y1 = (y0 + 1) % N
	return (v[y0, x0] * (1 - tx) * (1 - ty) + v[y0, x1] * tx * (1 - ty)
		+ v[y1, x0] * (1 - tx) * ty + v[y1, x1] * tx * ty)


def smooth(e0: float, e1: float, v: np.ndarray) -> np.ndarray:
	t = np.clip((v - e0) / (e1 - e0), 0.0, 1.0)
	return t * t * (3 - 2 * t)


# the warp fields: broad and slow, so clouds stream in long curls
wx = (field(2.6, 0.0, 0.02) - 0.5) * N * 0.22
wy = (field(2.6, 0.0, 0.02) - 0.5) * N * 0.22

# where the big clouds are (a few, with open dark space between them)
where = warp(field(2.4, 0.0, 0.012), wx * 0.5, wy * 0.5)
where = smooth(0.3, 0.78, where)

# the clouds' own detail, streaked along the warp
gas_a = warp(field(1.9, 0.002, 0.25), wx, wy)
gas_b = warp(field(2.0, 0.002, 0.25), wy, -wx)
fine = warp(field(1.6, 0.01, 0.5), wx * 0.7, wy * 0.7)

dens_a = smooth(0.35, 0.95, gas_a * 0.75 + fine * 0.25) * where
dens_b = smooth(0.4, 0.95, gas_b * 0.75 + fine * 0.25) * smooth(0.3, 0.8, where + 0.15)

# dark dust lanes eat into the light
dust = warp(field(2.2, 0.003, 0.3), wx * 1.2, -wy * 1.2)
lanes = 1.0 - 0.8 * smooth(0.55, 0.8, dust) * smooth(0.1, 0.5, where)

# colours: a cool blue-violet body, warm magenta-rose cores, a teal veil
blue = np.array([0.18, 0.28, 0.72])
violet = np.array([0.45, 0.22, 0.62])
rose = np.array([0.85, 0.32, 0.42])
teal = np.array([0.12, 0.55, 0.62])
mix_t = smooth(0.3, 0.7, field(2.8, 0.0, 0.01))[..., None]
cold = blue * (1 - mix_t) + violet * mix_t
img = cold * dens_a[..., None] * 0.95
img += rose * (smooth(0.4, 0.95, dens_a) ** 1.5)[..., None] * 0.6
img += teal * dens_b[..., None] * 0.4
img *= lanes[..., None]
# a faint even glow so the open sky is not dead black
img += np.array([0.012, 0.014, 0.03]) * (0.6 + 0.4 * where[..., None])
img = np.clip(img, 0.0, 1.0)
# dither so the dark gradients do not band
img = img * 255.0 + rng.uniform(-0.5, 0.5, size=img.shape)
Image.fromarray(np.clip(img, 0, 255).astype(np.uint8), "RGB").save(out, quality=93)
print(out, N)
