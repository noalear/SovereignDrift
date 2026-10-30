#!/usr/bin/env python3
"""Make a caller's call-window frames from one painting and edits of it.

For callers painted as one waist-up shot on chroma green (no head sheet):
the neutral painting is the body, and the talking and blinking frames are
edits of the same painting (ChatGPT: "the exact same image, only the mouth
open").  An edit never comes back pixel-for-pixel, so each one is lined up
on the neutral painting by its features and only the part that really
changed (the mouth, the eyes) is laid over the neutral one, feathered, so
the caller never shifts between frames.

    python3 tools/art/frame_callers.py <body.png> <speaker> [--talk T.png] [--blink B.png]

Writes assets/comms/people/<speaker>[_talk|_blink].png, 768 px tall, as
rebody_callers.py does.  Needs opencv-python (SIFT).
"""
import argparse
import os

import cv2
import numpy as np
from PIL import Image

import cut_callers as cc
from rebody_callers import HEIGHT, gray


def load(path: str) -> tuple:
	rgb = np.asarray(Image.open(path).convert("RGB")).astype(np.float32) / 255.0
	return cc.key_green(rgb)


def align(frame: tuple, ref: tuple) -> tuple:
	"""The edit warped onto the neutral painting (similarity transform)."""
	(fc, fa), (rc, ra) = frame, ref
	H, W = ra.shape
	sift = cv2.SIFT_create()
	k1, d1 = sift.detectAndCompute(gray(fc), (fa > 0.5).astype(np.uint8) * 255)
	k2, d2 = sift.detectAndCompute(gray(rc), (ra > 0.5).astype(np.uint8) * 255)
	good = [m for m, n in cv2.BFMatcher().knnMatch(d1, d2, k=2) if m.distance < 0.75 * n.distance]
	p1 = np.float32([k1[m.queryIdx].pt for m in good])
	p2 = np.float32([k2[m.trainIdx].pt for m in good])
	M, inliers = cv2.estimateAffinePartial2D(p1, p2, method=cv2.RANSAC, ransacReprojThreshold=3)
	if M is None or inliers.sum() < 20:
		raise SystemExit("the edit does not line up with the painting")
	print("edit on painting: scale %.4f, shift %.1f,%.1f, %d of %d matches" % (np.hypot(M[0, 0], M[1, 0]), M[0, 2], M[1, 2], inliers.sum(), len(good)))
	wc = cv2.warpAffine(fc, M, (W, H), flags=cv2.INTER_LANCZOS4, borderMode=cv2.BORDER_REPLICATE)
	wa = np.clip(cv2.warpAffine(fa, M, (W, H), borderMode=cv2.BORDER_REPLICATE), 0.0, 1.0)
	return np.clip(wc, 0, 1), wa


def changed(frame: tuple, ref: tuple, grow: float) -> np.ndarray:
	"""A feathered mask over the main region where the edit differs from the
	painting (the moving part), ignoring the faint repaint everywhere else."""
	H, W = ref[1].shape
	d = (np.abs(frame[0] - ref[0]).sum(axis=2) + np.abs(frame[1] - ref[1]) * 2.0)
	d = cv2.GaussianBlur(d, (0, 0), H * 0.008)
	hot = (d > max(0.18, d.max() * 0.3)).astype(np.uint8)
	n, lab, stats, _ = cv2.connectedComponentsWithStats(hot)
	if n <= 1:
		return np.zeros((H, W), np.float32)
	big = stats[1:, cv2.CC_STAT_AREA].max()
	keep = np.zeros((H, W), np.uint8)
	for i in range(1, n):
		if stats[i, cv2.CC_STAT_AREA] >= big * 0.15:
			keep[lab == i] = 1
	r = max(3, int(H * grow))
	keep = cv2.dilate(keep, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (2 * r + 1, 2 * r + 1)))
	m = cv2.GaussianBlur(keep.astype(np.float32), (0, 0), r * 0.6)
	print("changed region: %d px, %d part(s)" % (int((m > 0.5).sum()), int((stats[1:, cv2.CC_STAT_AREA] >= big * 0.15).sum())))
	return np.clip(m, 0.0, 1.0)


def main() -> None:
	ap = argparse.ArgumentParser()
	ap.add_argument("body")
	ap.add_argument("speaker")
	ap.add_argument("--talk")
	ap.add_argument("--blink")
	ap.add_argument("--grow", type=float, default=0.025, help="how far past the changed part to blend, as a share of the height")
	args = ap.parse_args()
	ref = load(args.body)
	H, W = ref[1].shape
	out = {"": ref}
	for suffix, path in (("_talk", args.talk), ("_blink", args.blink)):
		if not path:
			continue
		f = align(load(path), ref)
		m = changed(f, ref, args.grow)[..., None]
		c = f[0] * m + ref[0] * (1.0 - m)
		a = f[1] * m[..., 0] + ref[1] * (1.0 - m[..., 0])
		out[suffix] = (c, a)
	# one crop for all frames: the caller plus a margin, down to the bottom edge
	solid = np.max([o[1] for o in out.values()], axis=0) > 0.5
	rows = np.where(solid.any(axis=1))[0]
	cols = np.where(solid.any(axis=0))[0]
	top = max(0, int(rows[0]) - int(H * 0.03))
	x0 = max(0, int(cols[0]) - int(W * 0.03))
	x1 = min(W, int(cols[-1]) + int(W * 0.03))
	h, w = H - top, x1 - x0
	# soften a side only where the caller runs off it
	fade_x = np.ones(w, np.float32)
	if x0 == 0 or x1 == W:
		fade_x = np.clip(np.minimum(np.arange(w), np.arange(w)[::-1]) / (w * 0.06), 0.0, 1.0)
	os.makedirs(cc.OUT, exist_ok=True)
	for suffix, (c, a) in out.items():
		img = np.dstack([c[top:, x0:x1], a[top:, x0:x1] * fade_x[None, :]])
		im = Image.fromarray((np.clip(img, 0, 1) * 255).astype(np.uint8), "RGBA")
		im = im.resize((round(w * HEIGHT / h), HEIGHT), Image.LANCZOS)
		path = os.path.join(cc.OUT, "%s%s.png" % (args.speaker, suffix))
		im.save(path, optimize=True)
		print(path, im.size)


if __name__ == "__main__":
	main()
