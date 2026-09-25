#!/usr/bin/env python3
# Synthetic test scene for underwater:marine-snow, written to the folder
# given: clean.png (without snow), snow.png (with it) and specks.png (where
# the specks are). The scene has open water with sensor noise, sand full of
# bright grains (texture that must be kept), an orange fish, a large white
# object (bright but big: must be kept), and about 400 specks of 1 to 5
# pixels across, adding nearly white light, in the water and in front of the fish.
import os
import sys

import numpy as np
from PIL import Image

W, H = 800, 533
out = sys.argv[1]
os.makedirs(out, exist_ok=True)
rng = np.random.default_rng(7)
y, x = np.mgrid[0:H, 0:W].astype(np.float32)

img = np.empty((H, W, 3), np.float32)
t = y / H
img[..., 0] = 0.05 + 0.05 * t
img[..., 1] = 0.35 - 0.1 * t
img[..., 2] = 0.55 - 0.15 * t

sand = y > H * 0.75
grain = rng.random((H, W)) < 0.08
img[sand] = [0.55, 0.5, 0.38]
img[sand & grain] = [0.85, 0.82, 0.75]
img += rng.normal(0, 0.012, img.shape).astype(np.float32)

fish = ((x - 300) / 110) ** 2 + ((y - 230) / 45) ** 2 < 1
img[fish] = [0.9, 0.45, 0.1]
eye = (x - 380) ** 2 + (y - 222) ** 2 < 5 ** 2
img[eye] = [0.05, 0.05, 0.05]

big = (x - 620) ** 2 + (y - 150) ** 2 < 22 ** 2
img[big] = [0.95, 0.95, 0.93]

clean = np.clip(img, 0, 1)
snow = clean.copy()
specks = np.zeros((H, W), np.float32)
for i in range(420):
    if i < 380:
        cx, cy = rng.uniform(5, W - 5), rng.uniform(5, H * 0.72)
    else:
        a = rng.uniform(0, 2 * np.pi)
        cx, cy = 300 + 80 * rng.uniform(0, 1) * np.cos(a), 230 + 30 * rng.uniform(0, 1) * np.sin(a)
    sigma = rng.uniform(0.5, 1.4)
    amp = rng.uniform(0.25, 0.6)
    g = amp * np.exp(-((x - cx) ** 2 + (y - cy) ** 2) / (2 * sigma ** 2))
    # strobe light scattered by the particle, added to what is behind it,
    # a little blue-green after its path through the water
    tint = np.array([0.9, 1.0, 1.0])
    snow = snow + g[..., None] * tint
    specks = np.maximum(specks, g)
snow = np.clip(snow, 0, 1)

save = lambda a, n: Image.fromarray((np.clip(a, 0, 1) * 255 + 0.5).astype(np.uint8)).save(os.path.join(out, n))
save(clean, 'clean.png')
save(snow, 'snow.png')
Image.fromarray(((specks > 0.03) * 255).astype(np.uint8), 'L').save(os.path.join(out, 'specks.png'))
print('wrote', out)
