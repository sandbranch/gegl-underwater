#!/usr/bin/env python3
"""Puts the results of several runs of tests/run.sh side by side, the
original photo first: one sheet per 8 photos in tests/output/compare-N.jpg.
  tests/compare.py default v2            all photos
  ONLY=blue tests/compare.py default v2  only photos whose name has "blue"
  IMAGES=dir OUT=dir tests/compare.py a b  photos and runs of another folder
"""
import glob, os, sys
from PIL import Image

here = os.path.dirname(os.path.abspath(__file__))
runs = sys.argv[1:]
width = 300
images = os.environ.get('IMAGES', os.path.join(here, 'images'))
output = os.environ.get('OUT', os.path.join(here, 'output'))
photos = sorted(p for p in glob.glob(os.path.join(images, '*%s*' % os.environ.get('ONLY', '')))
                if p.lower().endswith('.jpg'))
for old in glob.glob(os.path.join(output, 'compare-*.jpg')):
    os.remove(old)
for k in range(0, len(photos), 8):
    rows = []
    for f in photos[k:k + 8]:
        name = os.path.splitext(os.path.basename(f))[0] + '.jpg'
        paths = [f] + [os.path.join(output, r, name) for r in runs]
        images = [Image.open(p) if os.path.exists(p) else None for p in paths]
        h = int(images[0].height * width / images[0].width)
        row = Image.new('RGB', (len(paths) * width, h))
        for j, im in enumerate(images):
            if im:
                row.paste(im.resize((width, h)), (j * width, 0))
        rows.append(row)
    sheet = Image.new('RGB', (rows[0].width, sum(r.height for r in rows)))
    y = 0
    for r in rows:
        sheet.paste(r, (0, y))
        y += r.height
    out = os.path.join(output, 'compare-%d.jpg' % (k // 8))
    sheet.save(out, quality=88)
    print(out, ' '.join(os.path.basename(f)[:-4] for f in photos[k:k + 8]))
