#!/usr/bin/env python3
"""Transmission maps side by side, for trying other ways of estimating
the transmission before building one into the operation.

For each photo: the photo, the transmission of underwater:correct (from
the operation itself, UNDERWATER_DEBUG=t, from build/), and the maps of
the candidates in candidates.py, all on a small linear copy (512 px on
the long side, as the operation's estimate). White is near (t = 1),
black is far (open water).

  tests/transmission/compare.py photo.jpg ...  [-o sheet.jpg] [-c name,name]

Photos can be anywhere (private photos stay where they are); write the
sheet outside the repository for those.
"""
import argparse
import os
import subprocess
import sys
import tempfile

import numpy as np
from PIL import Image, ImageDraw

here = os.path.dirname(os.path.abspath(__file__))
top = os.path.dirname(os.path.dirname(here))
sys.path.insert(0, here)
import candidates  # noqa: E402

SMALL = 512


def linear(img):
    a = np.asarray(img).astype(np.float32) / 255.0
    return np.where(a <= 0.04045, a / 12.92, ((a + 0.055) / 1.055) ** 2.4)


def small_copy(path):
    im = Image.open(path).convert('RGB')
    scale = SMALL / max(im.size)
    im = im.resize((max(1, round(im.width * scale)), max(1, round(im.height * scale))),
                   Image.BOX)
    return im, linear(im)


def operation_t(path, size):
    """The operation's own transmission and water colour."""
    mod = os.path.join(top, 'tests', 'output', 'modules-t')
    os.makedirs(mod, exist_ok=True)
    subprocess.run(['cp', os.path.join(top, 'build', 'underwater-correct.so'), mod], check=True)
    with tempfile.TemporaryDirectory(dir=os.path.join(top, 'tests', 'output')) as tmp:
        out = os.path.join(tmp, 't.png')
        src = os.path.dirname(os.path.abspath(path))
        r = subprocess.run(['flatpak', 'run', '--filesystem=%s:ro' % src,
                            '--filesystem=%s' % top,
                            '--env=UNDERWATER_DEBUG=t',
                            '--env=GEGL_PATH=%s:/app/lib/gegl-0.4' % mod,
                            '--command=gegl', 'org.gimp.GIMP', os.path.abspath(path),
                            '-o', out, '--', 'underwater:correct'],
                           capture_output=True, text=True)
        water = None
        for line in r.stderr.splitlines():
            if line.startswith('underwater: water'):
                water = np.array([float(v) for v in line.split()[2:5]], np.float32)
        t = Image.open(out).convert('L').resize(size, Image.BOX)
        return np.asarray(t).astype(np.float32) / 255.0, water


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('photos', nargs='+')
    ap.add_argument('-o', '--out', default=os.path.join(top, 'tests', 'output', 'transmission.jpg'))
    ap.add_argument('-c', '--candidates', default=','.join(candidates.CANDIDATES))
    args = ap.parse_args()
    names = [n for n in args.candidates.split(',') if n]

    W = 300
    rows = []
    for path in args.photos:
        im, lin = small_copy(path)
        t_op, water = operation_t(path, im.size)
        maps = [('photo', np.asarray(im)), ('operation', t_op)]
        for n in names:
            maps.append((n, candidates.CANDIDATES[n](lin, water, t_op)))
        h = round(W * im.height / im.width)
        row = Image.new('RGB', (W * len(maps), h + 16), 'white')
        d = ImageDraw.Draw(row)
        for i, (label, m) in enumerate(maps):
            if m.ndim == 2:
                m = (np.clip(m, 0, 1) * 255).astype(np.uint8)
            tile = Image.fromarray(m).convert('RGB').resize((W, h))
            row.paste(tile, (i * W, 16))
            d.text((i * W + 4, 2), label if i else os.path.basename(path)[:36], fill='black')
        rows.append(row)
    sheet = Image.new('RGB', (max(r.width for r in rows), sum(r.height for r in rows)), 'white')
    y = 0
    for r in rows:
        sheet.paste(r, (0, y))
        y += r.height
    sheet.save(args.out, quality=88)
    print(args.out)


if __name__ == '__main__':
    main()
