#!/usr/bin/env python3
"""Put a caller's painted head on a new waist-up body so the call window
shows shoulders and arms.

The first caller sheets (cut by cut_callers.py) were three busts side by
side, so each bust was cropped at the shoulders and the callers looked
armless.  A body painting is one waist-up shot of the same caller on chroma
green with both arms in the picture.  The head from each frame of the old
sheet (neutral, talking, blinking) is found on the body by its features,
laid over the body's own head and faded out below the chin, so all frames
share one body and the face still moves.

    python3 tools/art/rebody_callers.py <sheet.png> <body.png> <speaker> [--frames 3] [--chin Y] [--no-blink]

--chin is the row (in body pixels) where the old head starts fading into
the new body; by default it is found from where the mouth and eyes move.
Needs opencv-python (SIFT).
"""
import argparse
import os

import cv2
import numpy as np
from PIL import Image

import cut_callers as cc

HEIGHT = 768


def sheet_frames(path: str, n: int) -> list:
	"""The sheet's frames keyed and lined up on the neutral one, as cut_callers does."""
	rgb = np.asarray(Image.open(path).convert("RGB")).astype(np.float32) / 255.0
	col, alpha = cc.key_green(rgb)
	cuts = cc.splits(alpha, n)
	width = min(cuts[i + 1] - cuts[i] for i in range(n))
	frames = []
	for i in range(n):
		mid = (cuts[i] + cuts[i + 1]) // 2
		x0 = max(0, min(mid - width // 2, rgb.shape[1] - width))
		frames.append((col[:, x0 : x0 + width], alpha[:, x0 : x0 + width]))
	ref = frames[0][1]
	for i in range(1, n):
		dx, dy = cc.shift_to(ref, frames[i][1])
		c, a = frames[i]
		frames[i] = (np.roll(np.roll(c, dy, axis=0), dx, axis=1), np.roll(np.roll(a, dy, axis=0), dx, axis=1))
	return frames


def gray(c: np.ndarray) -> np.ndarray:
	return cv2.cvtColor((np.clip(c, 0, 1) * 255).astype(np.uint8), cv2.COLOR_RGB2GRAY)


def head_transform(head: tuple, body: tuple) -> np.ndarray:
	"""Scale, turn and shift (2x3) that lays the sheet's head on the body's."""
	hc, ha = head
	bc, ba = body
	m1 = np.zeros(ha.shape, np.uint8)
	m1[: int(ha.shape[0] * 0.6)] = 255
	m1[ha < 0.5] = 0
	m2 = np.zeros(ba.shape, np.uint8)
	m2[: int(ba.shape[0] * 0.5)] = 255
	m2[ba < 0.5] = 0
	sift = cv2.SIFT_create()
	k1, d1 = sift.detectAndCompute(gray(hc), m1)
	k2, d2 = sift.detectAndCompute(gray(bc), m2)
	good = [m for m, n in cv2.BFMatcher().knnMatch(d1, d2, k=2) if m.distance < 0.75 * n.distance]
	p1 = np.float32([k1[m.queryIdx].pt for m in good])
	p2 = np.float32([k2[m.trainIdx].pt for m in good])
	M, inliers = cv2.estimateAffinePartial2D(p1, p2, method=cv2.RANSAC, ransacReprojThreshold=4)
	if M is None or inliers.sum() < 12:
		raise SystemExit("the head was not found on the body (%d matches)" % (0 if inliers is None else inliers.sum()))
	print("head on body: scale %.3f, %d of %d matches" % (np.hypot(M[0, 0], M[1, 0]), inliers.sum(), len(good)))
	return M


def moving_row(a: tuple, b: tuple, lo: int, hi: int) -> int:
	"""Row between lo and hi where frame b differs most from frame a, in the
	middle half of the frame (the mouth opening, the eyes closing)."""
	d = np.abs(b[0] - a[0]).sum(axis=2) * (a[1] > 0.5)
	w = d.shape[1]
	d[:, : w // 4] = 0.0
	d[:, 3 * w // 4 :] = 0.0
	rows = cv2.GaussianBlur(d, (0, 0), 6).max(axis=1)
	return lo + int(np.argmax(rows[lo:hi]))


def main() -> None:
	ap = argparse.ArgumentParser()
	ap.add_argument("sheet")
	ap.add_argument("body")
	ap.add_argument("speaker")
	ap.add_argument("--frames", type=int, default=3)
	ap.add_argument("--chin", type=float, default=-1.0)
	ap.add_argument("--no-blink", action="store_true", help="the third frame does not blink (a visor): save neutral and talk only")
	args = ap.parse_args()
	frames = sheet_frames(args.sheet, args.frames)
	if args.no_blink:
		frames = frames[:2]
		args.frames = 2
	b = np.asarray(Image.open(args.body).convert("RGB")).astype(np.float32) / 255.0
	bcol, ba = cc.key_green(b)
	H, W = ba.shape
	M = head_transform(frames[0], (bcol, ba))
	k = float(np.hypot(M[0, 0], M[1, 0]))
	# where the old head fades into the new body: a little below the mouth
	sh = frames[0][1].shape[0]
	top = int(np.where(frames[0][1].max(axis=1) > 0.5)[0][0])
	eyes = moving_row(frames[0], frames[2], top, int(top + sh * 0.55)) if args.frames > 2 else int(top + sh * 0.33)
	mouth = moving_row(frames[0], frames[1], int(eyes + sh * 0.06), int(sh * 0.8)) if args.frames > 1 else int(eyes + sh * 0.2)
	span = (mouth - eyes) * k
	mouth_b = M[1, 0] * frames[0][1].shape[1] * 0.5 + M[1, 1] * mouth + M[1, 2]
	chin = args.chin if args.chin >= 0 else mouth_b + span * 0.3
	fade = span * 0.35
	print("mouth at row %.0f, fade %.0f..%.0f" % (mouth_b, chin, chin + fade))
	ramp = np.clip((chin + fade - np.arange(H, dtype=np.float32)[:, None]) / fade, 0.0, 1.0)
	w = cv2.GaussianBlur(ramp * np.ones((1, W), np.float32), (0, 0), 1.5)[..., None]
	out = []
	for c, a in frames:
		wc = cv2.warpAffine(c, M, (W, H), flags=cv2.INTER_LANCZOS4)
		wa = np.clip(cv2.warpAffine(a, M, (W, H)), 0.0, 1.0)
		# the old head laid over the body above the fade, the body alone
		# below it, the head fading out across it
		over = wa[..., None] * w
		cpm = wc * over + bcol * ba[..., None] * (1.0 - over)
		alpha = over[..., 0] + ba * (1.0 - over[..., 0])
		col = cpm / np.maximum(alpha, 1e-4)[..., None]
		out.append((np.clip(col, 0, 1), np.clip(alpha, 0, 1)))
	# one crop for all frames: the caller plus a margin, down to the bottom edge
	solid = np.max([o[1] for o in out], axis=0) > 0.5
	rows = np.where(solid.any(axis=1))[0]
	cols = np.where(solid.any(axis=0))[0]
	top = max(0, int(rows[0]) - int(H * 0.03))
	x0 = max(0, int(cols[0]) - int(W * 0.03))
	x1 = min(W, int(cols[-1]) + int(W * 0.03))
	h, w = H - top, x1 - x0
	# soften a side only where the caller runs off it
	fade_x = np.ones(w, np.float32)
	ramp_x = np.clip(np.minimum(np.arange(w), np.arange(w)[::-1]) / (w * 0.06), 0.0, 1.0)
	if x0 == 0 or x1 == W:
		fade_x = ramp_x
	os.makedirs(cc.OUT, exist_ok=True)
	for i, (c, a) in enumerate(out):
		img = np.dstack([c[top:, x0:x1], a[top:, x0:x1] * fade_x[None, :]])
		im = Image.fromarray((img * 255).astype(np.uint8), "RGBA")
		im = im.resize((round(w * HEIGHT / h), HEIGHT), Image.LANCZOS)
		path = os.path.join(cc.OUT, "%s%s.png" % (args.speaker, cc.SUFFIX[i]))
		im.save(path, optimize=True)
		print(path, im.size)


if __name__ == "__main__":
	main()
