#!/usr/bin/env python3
"""Put several PNG frames side by side:  python tools/compose_strip.py a.png b.png c.png -o strip.png"""
import argparse

from PIL import Image

ap = argparse.ArgumentParser()
ap.add_argument("frames", nargs="+")
ap.add_argument("-o", "--out", required=True)
ap.add_argument("--gap", type=int, default=8)
a = ap.parse_args()
imgs = [Image.open(p).convert("RGB") for p in a.frames]
h = max(i.height for i in imgs)
w = sum(i.width for i in imgs) + a.gap * (len(imgs) - 1)
strip = Image.new("RGB", (w, h), (9, 15, 27))
x = 0
for im in imgs:
    strip.paste(im, (x, 0))
    x += im.width + a.gap
strip.save(a.out, optimize=True)
print("wrote", a.out)
