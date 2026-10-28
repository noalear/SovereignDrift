#!/usr/bin/env python3
"""Cut a caller sheet painted on chroma green into the call window's frames.

A sheet holds three busts of one caller side by side: neutral, talking
(mouth open) and blinking.  The green is keyed out (with the green spill
taken off the edges), the frames are lined up on the neutral one so the
face does not jump when the mouth opens, and each is saved at HEIGHT px
tall as assets/comms/people/<speaker>[_talk|_blink].png.

    python3 tools/art/cut_callers.py <sheet.png> <speaker> [--frames 3]

With --frames 1 the sheet is a single bust (no talk or blink frames: the
call window falls back to the neutral one).
"""
import argparse
import os

import numpy as np
from PIL import Image

HEIGHT = 512
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "comms", "people")
SUFFIX = ["", "_talk", "_blink"]


def key_green(rgb: np.ndarray) -> tuple:
	"""Alpha from how much greener than red and blue a pixel is, and the
	colour with the green spill clamped off."""
	r, g, b = rgb[..., 0], rgb[..., 1], rgb[..., 2]
	spill = g - np.maximum(r, b)
	alpha = (1.0 - np.clip((spill - 0.1) / (0.32 - 0.1), 0.0, 1.0)) ** 1.4
	out = rgb.copy()
	out[..., 1] = np.minimum(g, (r + b) * 0.5 + 0.05)
	return out, alpha


def splits(alpha: np.ndarray, n: int) -> list:
	"""Columns between the busts: the emptiest column near each 1/n."""
	w = alpha.shape[1]
	cols = alpha.sum(axis=0)
	cuts = [0]
	for i in range(1, n):
		c = int(w * i / n)
		lo, hi = c - w // 10, c + w // 10
		cuts.append(lo + int(np.argmin(cols[lo:hi])))
	cuts.append(w)
	return cuts


def shift_to(ref: np.ndarray, a: np.ndarray, reach: int = 40) -> tuple:
	"""The (dx, dy) that best lays mask a over mask ref, on the upper body."""
	h = ref.shape[0]
	best, at = None, (0, 0)
	r = ref[: int(h * 0.75)]
	for dy in range(-reach // 2, reach // 2 + 1, 2):
		for dx in range(-reach, reach + 1, 2):
			m = np.roll(np.roll(a, dy, axis=0), dx, axis=1)[: int(h * 0.75)]
			d = np.abs(m - r).mean()
			if best is None or d < best:
				best, at = d, (dx, dy)
	return at


def main() -> None:
	ap = argparse.ArgumentParser()
	ap.add_argument("sheet")
	ap.add_argument("speaker")
	ap.add_argument("--frames", type=int, default=3)
	args = ap.parse_args()
	rgb = np.asarray(Image.open(args.sheet).convert("RGB")).astype(np.float32) / 255.0
	col, alpha = key_green(rgb)
	cuts = splits(alpha, args.frames)
	width = min(cuts[i + 1] - cuts[i] for i in range(args.frames))
	frames = []
	for i in range(args.frames):
		mid = (cuts[i] + cuts[i + 1]) // 2
		x0 = max(0, min(mid - width // 2, rgb.shape[1] - width))
		frames.append((col[:, x0 : x0 + width], alpha[:, x0 : x0 + width]))
	# line the talking and blinking frames up on the neutral one
	ref = frames[0][1]
	for i in range(1, len(frames)):
		dx, dy = shift_to(ref, frames[i][1])
		c, a = frames[i]
		frames[i] = (np.roll(np.roll(c, dy, axis=0), dx, axis=1), np.roll(np.roll(a, dy, axis=0), dx, axis=1))
	# one crop for all: from the top of the head to the bottom edge
	rows = np.where(np.max([f[1].max(axis=1) for f in frames], axis=0) > 0.5)[0]
	top = max(0, int(rows[0]) - int(rgb.shape[0] * 0.03))
	h, w = rgb.shape[0] - top, width
	# soften the cut sides where a shoulder runs off the frame
	fade = np.clip(np.minimum(np.arange(w), np.arange(w)[::-1]) / (w * 0.06), 0.0, 1.0)
	os.makedirs(OUT, exist_ok=True)
	for i, (c, a) in enumerate(frames):
		a = a[top:] * fade[None, :]
		img = np.dstack([c[top:], a])
		im = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8), "RGBA")
		im = im.resize((round(w * HEIGHT / h), HEIGHT), Image.LANCZOS)
		path = os.path.join(OUT, "%s%s.png" % (args.speaker, SUFFIX[i]))
		im.save(path, optimize=True)
		print(path, im.size)


if __name__ == "__main__":
	main()
