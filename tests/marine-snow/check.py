#!/usr/bin/env python3
# Measures a result of underwater:marine-snow against the clean scene:
# how much of the snow is gone, and how much of the rest was changed.
import sys

import numpy as np
from PIL import Image

folder, result = sys.argv[1], sys.argv[2]
L = lambda n: np.asarray(Image.open(n if '/' in n else folder + '/' + n).convert('RGB')).astype(float) / 255
clean, snow, res = L('clean.png'), L('snow.png'), L(result)
specks = np.asarray(Image.open(folder + '/specks.png')) > 0
H, W = specks.shape
y, x = np.mgrid[0:H, 0:W]

before = np.abs(snow - clean).max(2)[specks].mean()
after = np.abs(res - clean).max(2)[specks].mean()
outside = ~specks
changed = np.abs(res - snow).max(2)
sand = (y > H * 0.75) & outside
big = ((x - 620) ** 2 + (y - 150) ** 2 < 22 ** 2)
fish = (((x - 300) / 110) ** 2 + ((y - 230) / 45) ** 2 < 1) & outside

print('snow removed          %5.1f %%  (error on the specks %.3f -> %.3f)' % (100 * (1 - after / before), before, after))
print('pixels changed outside the specks: water %.2f %%, sand %.2f %%, fish %.2f %%' % (
    100 * (changed[outside & (y < H * 0.72)] > 0.03).mean(),
    100 * (changed[sand] > 0.03).mean(),
    100 * (changed[fish] > 0.03).mean()))
print('large white object changed: %.4f (mean abs)' % changed[big].mean())

# per speck: is it still visible (a peak left of more than 0.06)?
from scipy import ndimage
labels, count = ndimage.label(specks)
peak_before = ndimage.maximum(np.abs(snow - clean).max(2), labels, range(1, count + 1))
peak_after = ndimage.maximum(np.abs(res - clean).max(2), labels, range(1, count + 1))
visible = np.array(peak_before) > 0.06
print('specks still visible: %d of %d (%.1f %%)' % (
    (np.array(peak_after)[visible] > 0.06).sum(), visible.sum(),
    100 * (np.array(peak_after)[visible] > 0.06).mean()))
