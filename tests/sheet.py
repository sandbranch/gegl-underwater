#!/usr/bin/env python3
# Before/after sheets and measurements for tests/run.sh: for each photo the
# mean of each channel before and after, the spread of the channel means
# (0 is gray on average), and the share of clipped pixels.
import glob
import os
import sys

import numpy as np
from PIL import Image, ImageDraw

src, out = sys.argv[1], sys.argv[2]
rows = []
pairs = []
for f in sorted(glob.glob(out + '/*.jpg')):
    b = os.path.basename(f)
    if b.startswith('sheet'):
        continue
    before = Image.open(os.path.join(src, b)).convert('RGB')
    after = Image.open(f).convert('RGB')
    a0 = np.asarray(before.resize((600, int(600 * before.height / before.width)))).astype(float) / 255
    a1 = np.asarray(after.resize((600, int(600 * after.height / after.width)))).astype(float) / 255
    m0, m1 = a0.reshape(-1, 3).mean(0), a1.reshape(-1, 3).mean(0)
    spread = lambda m: (m.max() - m.min()) / max(m.mean(), 1e-6)
    clip = (a1 >= 0.999).any(2).mean() * 100
    rows.append('%-22s  before R %.2f G %.2f B %.2f spread %.2f   after R %.2f G %.2f B %.2f spread %.2f  clipped %4.1f %%'
                % (b[:-4], *m0, spread(m0), *m1, spread(m1), clip))
    pairs.append((b[:-4], before, after))
open(out + '/measurements.txt', 'w').write('\n'.join(rows) + '\n')
print('\n'.join(rows))

W, H = 380, 260
for s in range(0, len(pairs), 8):
    part = pairs[s:s + 8]
    sheet = Image.new('RGB', (2 * W * 2, ((len(part) + 1) // 2) * (H + 16)), 'white')
    d = ImageDraw.Draw(sheet)
    for k, (n, b0, b1) in enumerate(part):
        x, y = (k % 2) * 2 * W, (k // 2) * (H + 16)
        for j, im in enumerate((b0, b1)):
            t = im.copy(); t.thumbnail((W - 4, H - 4)); sheet.paste(t, (x + j * W + 2, y + 2))
        d.text((x + 4, y + H), n, fill='black')
    sheet.save(out + '/sheet%d.png' % (s // 8))
