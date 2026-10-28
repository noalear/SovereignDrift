#!/usr/bin/env python3
"""Size a painted call background for the call window: the painting is
cut to 3:2 round its centre, softened a touch so the caller stands out,
and saved WIDTH px wide as
assets/comms/places/<place>.png (CallView fills its window from it).

    python3 tools/art/cut_place.py <painting.png> <place>
"""
import os
import sys

from PIL import Image, ImageFilter

WIDTH = 960
BLUR = 1.4
OUT = os.path.join(os.path.dirname(__file__), "..", "..", "assets", "comms", "places")


def main() -> None:
	src, place = sys.argv[1], sys.argv[2]
	im = Image.open(src).convert("RGB")
	w, h = im.size
	if w / h > 1.5:
		cw = round(h * 1.5)
		im = im.crop(((w - cw) // 2, 0, (w - cw) // 2 + cw, h))
	else:
		ch = round(w / 1.5)
		im = im.crop((0, (h - ch) // 2, w, (h - ch) // 2 + ch))
	im = im.resize((WIDTH, round(WIDTH / 1.5)), Image.LANCZOS).filter(ImageFilter.GaussianBlur(BLUR))
	os.makedirs(OUT, exist_ok=True)
	path = os.path.join(OUT, place + ".png")
	im.save(path, optimize=True)
	print(path, im.size, os.path.getsize(path))


if __name__ == "__main__":
	main()
