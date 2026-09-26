#!/usr/bin/env python3
"""Pass/fail tests of underwater:correct and underwater:marine-snow on
synthetic images, through GEGL's Python bindings (tests/check.sh runs it
inside the Flatpak GIMP with the modules from build/). No photos, network
or display needed, and no numpy: the images are small.

  check.py              all cases, each in a process of its own, so that a
                        crash is a FAIL and not the end of the run
  check.py name ...     only the cases whose name contains one of these
  check.py --case name  one case in this process (what the above runs)
"""
import array
import math
import os
import random
import subprocess
import sys

import gi
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl

FMT = 'RGBA float'
# seconds a case may take (under ASan it is much slower)
TIMEOUT = int(os.environ.get('CHECK_TIMEOUT', '120'))
CASES = {}


def case(fn):
    CASES[fn.__name__] = fn
    return fn


class Fail(Exception):
    pass


def check(cond, msg):
    if not cond:
        raise Fail(msg)


# images: flat array('f') of RGBA, row by row

def image(w, h, fn):
    a = array.array('f', bytes(16 * w * h))
    for y in range(h):
        for x in range(w):
            a[(y * w + x) * 4:(y * w + x) * 4 + 4] = array.array('f', fn(x, y))
    return a


def uniform(w, h, rgba):
    return array.array('f', list(rgba) * (w * h))


def scene(w, h, seed=1):
    """An underwater scene in linear light: water lighter and greener
    towards the top, a strobe-lit orange subject, a gray rock in ambient
    light (red absorbed), sand at the bottom, and sensor noise."""
    rnd = random.Random(seed)

    def px(x, y):
        t = y / max(h - 1, 1)
        r, g, b = 0.01 + 0.01 * t, 0.25 - 0.12 * t, 0.35 - 0.15 * t
        if (x - 0.3 * w) ** 2 / (0.15 * w) ** 2 + (y - 0.4 * h) ** 2 / (0.12 * h) ** 2 < 1:
            r, g, b = 0.7, 0.3, 0.08
        elif abs(x - 0.7 * w) < 0.12 * w and abs(y - 0.6 * h) < 0.15 * h:
            r, g, b = 0.03, 0.16, 0.18
        elif y > 0.8 * h:
            r, g, b = 0.08, 0.2, 0.18
        n = rnd.gauss(0, 0.01)
        return (max(r + n, 0), max(g + n, 0), max(b + n, 0), 1.0)
    return image(w, h, px)


# running an operation

def run(op, props, data, w, h, x0=0, y0=0, fmt=FMT, roi=None, in_fmt=None):
    """Runs op on the image (pixels in fmt) and returns (pixels of the
    output in fmt, bounding box of the output as (x, y, w, h)). roi: render only
    that part, (x, y, w, h), and return it."""
    buf = Gegl.Buffer.new(in_fmt or fmt, x0, y0, w, h)
    rect = Gegl.Rectangle.new(x0, y0, w, h)
    if in_fmt and in_fmt != fmt:
        # converted by GEGL (babl), as GIMP does for gray images
        tmp = Gegl.Buffer.new(fmt, x0, y0, w, h)
        tmp.set(rect, fmt, data.tobytes())
        buf.set(rect, fmt, tmp.get(rect, 1.0, fmt, Gegl.AbyssPolicy.NONE))
    else:
        buf.set(rect, fmt, data.tobytes())
    g = Gegl.Node()
    src = g.create_child('gegl:buffer-source')
    src.set_property('buffer', buf)
    node = g.create_child(op)
    for k, v in props.items():
        node.set_property(k, v)
    src.link(node)
    bb = node.get_bounding_box()
    box = (bb.x, bb.y, bb.width, bb.height)
    r = Gegl.Rectangle.new(*(roi or box))
    out = Gegl.Buffer.new(fmt, r.x, r.y, r.width, r.height)
    sink = g.create_child('gegl:write-buffer')
    sink.set_property('buffer', out)
    node.link(sink)
    proc = sink.new_processor(r)
    while proc.work()[0]:
        pass
    return array.array('f', out.get(r, 1.0, fmt, Gegl.AbyssPolicy.NONE)), box


def finite(a):
    return all(math.isfinite(v) for v in a)


def count_bad(a):
    return sum(1 for v in a if not math.isfinite(v))


def alpha_same(a, b):
    return all(a[i] == b[i] for i in range(3, len(a), 4))


def max_diff(a, b):
    return max((abs(p - q) for p, q in zip(a, b)), default=0.0)


def basic(op, props, data, w, h, **kw):
    """The invariants of every run: same size, finite, alpha kept."""
    out, box = run(op, props, data, w, h, **kw)
    x0, y0 = kw.get('x0', 0), kw.get('y0', 0)
    check(box == (x0, y0, w, h), '%s: output %s, input %s' % (op, box, (x0, y0, w, h)))
    check(finite(out), '%s: %d NaN/Inf values in the output' % (op, count_bad(out)))
    if kw.get('in_fmt') is None:
        check(alpha_same(out, data), '%s: alpha changed' % op)
    return out


UW, SNOW = 'underwater:correct', 'underwater:marine-snow'
# marine-snow works in perceptual units: its tests are written in them, so
# that the light a speck adds is the same in each channel, as it is meant
PERC = "R'G'B'A float"
TINY = [(1, 1), (1, 2), (2, 1), (2, 2), (3, 3), (7, 5), (1, 700), (700, 1),
        (2, 1600), (1600, 3), (1, 3000), (3000, 1)]


# underwater:correct

@case
def correct_tiny_sizes():
    for w, h in TINY:
        basic(UW, {}, scene(w, h), w, h)


@case
def correct_elongated():
    # narrower than the block of the small copy (the long side / 512)
    for w, h in [(2, 1100), (1100, 2), (1, 1537), (3, 3100), (3100, 5)]:
        a = basic(UW, {}, scene(w, h), w, h)
        b, _ = run(UW, {}, scene(w, h), w, h)
        check(a == b, 'underwater:correct: %dx%d not the same on a second run' % (w, h))


@case
def correct_uniform():
    """A uniform image has its water color everywhere: t = 0.1, so there is
    no restoration or white balance (both fade out below t = 0.15), and
    out = J (1 - veil) + A' veil with J = 0.1 A / 0.3^clarity and
    veil = 0.9 (1 - backscatter). A' is A with its Oklab chroma times
    keep-water."""
    for rgb in [(0.5, 0.5, 0.5), (0.02, 0.2, 0.3), (0.05, 0.3, 0.12), (0.3, 0.1, 0.05)]:
        for props in [{}, {'clarity': 0.0, 'backscatter': 0.0, 'keep-water': 1.0},
                      {'clarity': 1.0, 'backscatter': 1.0, 'keep-water': 0.0},
                      {'clarity': 0.3, 'backscatter': 0.2, 'keep-water': 0.7,
                       'red-restore': 2.0, 'blue-restore': 2.0, 'white-balance': False}]:
            c = props.get('clarity', 0.5)
            veil = 0.9 * (1 - props.get('backscatter', 0.5))
            keep = keep_water(rgb, props.get('keep-water', 0.5))
            want = [0.1 * rgb[i] / 0.3 ** c * (1 - veil) + keep[i] * veil for i in range(3)]
            out = basic(UW, props, uniform(16, 12, rgb + (1.0,)), 16, 12)
            got = out[0:3]
            check(max_diff(got, want) < 1e-4 and max_diff(out[:-4], out[4:]) < 1e-5,
                  'uniform %s %s: got %s, expected %s' % (rgb, props, list(got), want))


def keep_water(rgb, k):
    """The kept water color of the operation, from and to Oklab."""
    r, g, b = rgb
    l = (0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b) ** (1 / 3)
    m = (0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b) ** (1 / 3)
    s = (0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b) ** (1 / 3)
    L = 0.2104542553 * l + 0.7936177850 * m - 0.0040720468 * s
    A = (1.9779984951 * l - 2.4285922050 * m + 0.4505937099 * s) * k
    B = (0.0259040371 * l + 0.7827717662 * m - 0.8086757660 * s) * k
    l = (L + 0.3963377774 * A + 0.2158037573 * B) ** 3
    m = (L - 0.1055613458 * A - 0.0638541728 * B) ** 3
    s = (L - 0.0894841775 * A - 1.2914855480 * B) ** 3
    return [max(4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s, 0),
            max(-1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s, 0),
            max(-0.0041960863 * l - 0.7034186147 * m + 1.7076882690 * s, 0)]


@case
def correct_black_and_white():
    out = basic(UW, {}, uniform(20, 10, (0, 0, 0, 1)), 20, 10)
    check(max(out[i] for i in range(len(out)) if i % 4 != 3) == 0.0, 'black does not stay black')
    # clipped pixels keep their brightest channel
    out = basic(UW, {}, uniform(20, 10, (1, 1, 1, 1)), 20, 10)
    check(max_diff(out, uniform(20, 10, (1, 1, 1, 1))) < 1e-6, 'white does not stay white')


@case
def correct_neutral_stays_neutral():
    """A gray image, with structure: the water color, the map and the
    white balance are then all gray, and so is the result."""
    rnd = random.Random(3)

    def px(x, y):
        v = 0.05 + 0.5 * x / 99 + (0.3 if (x // 10 + y // 10) % 2 else 0.0) + rnd.uniform(0, 0.02)
        return (v, v, v, 1.0)
    out = basic(UW, {}, image(100, 80, px), 100, 80)
    worst = max(max(out[i:i + 3]) - min(out[i:i + 3]) for i in range(0, len(out), 4))
    check(worst < 1e-4, 'a gray image gets color: max channel spread %.6f' % worst)


@case
def correct_alpha():
    base = scene(60, 40)
    for i in range(3, len(base), 4):
        base[i] = ((i // 4) % 7) / 6.0
    basic(UW, {}, base, 60, 40)
    for i in range(3, len(base), 4):
        base[i] = 0.0
    basic(UW, {}, base, 60, 40)


@case
def correct_properties_at_limits():
    data = scene(48, 36)
    for rr in (0.0, 2.0):
        for br in (0.0, 2.0):
            for bs in (0.0, 1.0):
                for cl in (0.0, 1.0):
                    for kw in (0.0, 1.0):
                        for wb in (False, True):
                            for aw in (False, True):
                                basic(UW, {'red-restore': rr, 'blue-restore': br, 'backscatter': bs,
                                           'clarity': cl, 'keep-water': kw, 'white-balance': wb,
                                           'auto-water': aw}, data, 48, 36)


@case
def correct_water_colors():
    for c in ['#000000', '#ffffff', '#ff0000', '#1e6478', '#00ff00']:
        basic(UW, {'auto-water': False, 'water-color': Gegl.Color.new(c)}, scene(40, 30), 40, 30)


@case
def correct_deterministic():
    data = scene(300, 200)
    a, _ = run(UW, {}, data, 300, 200)
    b, _ = run(UW, {}, data, 300, 200)
    check(a == b, 'two runs differ')
    cfg = Gegl.config()
    cfg.set_property('chunk-size', 4096)
    cfg.set_property('threads', 1)
    c, _ = run(UW, {}, data, 300, 200)
    check(a == c, 'differs with one thread and small chunks: %.6f' % max_diff(a, c))


@case
def correct_part_of_the_image():
    """Rendering a part (a preview of the visible area) gives the same
    pixels there as rendering all of it."""
    data = scene(200, 150)
    whole, _ = run(UW, {}, data, 200, 150)
    part, _ = run(UW, {}, data, 200, 150, roi=(50, 40, 70, 30))
    ref = array.array('f', [whole[((40 + y) * 200 + 50 + x) * 4 + c]
                            for y in range(30) for x in range(70) for c in range(4)])
    check(part == ref, 'part differs from the whole: %.6f' % max_diff(part, ref))


@case
def correct_offset_origin():
    data = scene(120, 90)
    a, _ = run(UW, {}, data, 120, 90)
    b = basic(UW, {}, data, 120, 90, x0=37, y0=-12)
    check(a == b, 'result depends on where the layer is: %.6f' % max_diff(a, b))


@case
def correct_gray_input():
    data = scene(50, 40)
    for fmt in ('Y float', 'YA float', "Y' u8", "Y'A u8", "R'G'B' u8", "R'G'B'A u16"):
        out = basic(UW, {}, data, 50, 40, in_fmt=fmt)
        if fmt.startswith('Y'):
            worst = max(max(out[i:i + 3]) - min(out[i:i + 3]) for i in range(0, len(out), 4))
            check(worst < 1e-4, '%s: gray input gets color %.6f' % (fmt, worst))


@case
def correct_nan_inf_input():
    """A few broken pixels (float images can have them) stay where they
    are and do not spread over the whole image through the estimates."""
    for bad in (float('nan'), float('inf'), -float('inf')):
        data = scene(80, 60)
        data[(30 * 80 + 40) * 4 + 1] = bad
        out, _ = run(UW, {}, data, 80, 60)
        n = sum(1 for i in range(0, len(out), 4) if not finite(out[i:i + 4]))
        check(n <= 1, '%s in one pixel: %d pixels of the output are not finite' % (bad, n))


@case
def correct_out_of_range_input():
    """Negative and very bright values (float and HDR images)."""
    for lo, hi in ((-0.5, 1.0), (0.0, 50.0), (-2.0, 1000.0)):
        rnd = random.Random(5)
        data = image(64, 48, lambda x, y: (rnd.uniform(lo, hi), rnd.uniform(lo, hi), rnd.uniform(lo, hi), 1.0))
        basic(UW, {}, data, 64, 48)


@case
def correct_infinite_input():
    """An input without bounds (gegl:color) must not ask for an image of
    infinite size."""
    g = Gegl.Node()
    col = g.create_child('gegl:color')
    col.set_property('value', Gegl.Color.new('#1e6478'))
    node = g.create_child(UW)
    col.link(node)
    r = Gegl.Rectangle.new(0, 0, 16, 16)
    buf = Gegl.Buffer.new(FMT, 0, 0, 16, 16)
    sink = g.create_child('gegl:write-buffer')
    sink.set_property('buffer', buf)
    node.link(sink)
    proc = sink.new_processor(r)
    while proc.work()[0]:
        pass
    out = array.array('f', buf.get(r, 1.0, FMT, Gegl.AbyssPolicy.NONE))
    check(finite(out), 'not finite')


# underwater:marine-snow

def snow(props, data, w, h, **kw):
    return basic(SNOW, props, data, w, h, fmt=PERC, **kw)


def run_snow(props, data, w, h, **kw):
    return run(SNOW, props, data, w, h, fmt=PERC, **kw)


def speck_scene(w, h, seed=2, specks=40, grains=True):
    """Water with noise, sand with many bright grains (texture to keep) in
    the lower third, and isolated whitish specks of 1 to 3 pixels, in
    perceptual units."""
    rnd = random.Random(seed)
    data = image(w, h, lambda x, y: (0.3 + rnd.gauss(0, 0.01), 0.6 - 0.1 * y / h, 0.7, 1.0)
                 if y < 2 * h // 3 else
                 ((0.95, 0.92, 0.88, 1.0) if grains and rnd.random() < 0.08 else (0.75, 0.7, 0.62, 1.0)))
    spots = []
    for _ in range(specks):
        cx, cy = rnd.randrange(3, w - 3), rnd.randrange(3, 2 * h // 3 - 3)
        s = rnd.choice((0, 1))
        spots.append((cx, cy))
        for y in range(cy - s, cy + s + 1):
            for x in range(cx - s, cx + s + 1):
                i = (y * w + x) * 4
                for c in range(3):
                    data[i + c] = min(data[i + c] + 0.3, 1.0)
    return data, spots


@case
def snow_tiny_sizes():
    for w, h in TINY[:8]:
        for size in (1, 5, 40):
            snow({'size': size}, uniform(w, h, (0.2, 0.3, 0.5, 1.0)), w, h)
            data = image(w, h, lambda x, y: (1.0, 1.0, 1.0, 1.0) if (x + y) % 3 == 0 else (0.1, 0.3, 0.5, 1.0))
            snow({'size': size}, data, w, h)


@case
def snow_uniform_unchanged():
    for rgb in ((0, 0, 0), (1, 1, 1), (0.3, 0.6, 0.7)):
        data = uniform(40, 30, rgb + (1.0,))
        out = snow({}, data, 40, 30)
        check(out == data, 'uniform %s changed' % (rgb,))


@case
def snow_one_speck():
    w, h = 41, 31
    data = uniform(w, h, (0.3, 0.6, 0.7, 1.0))
    i = (15 * w + 20) * 4
    data[i:i + 3] = array.array('f', (0.6, 0.9, 1.0))
    out = snow({}, data, w, h)
    check(max_diff(out[i:i + 4], (0.3, 0.6, 0.7, 1.0)) < 1e-6, 'speck not removed: %s' % list(out[i:i + 4]))
    rest = [k for k in range(0, len(out), 4) if k != i]
    check(all(out[k:k + 4] == data[k:k + 4] for k in rest), 'pixels besides the speck changed')
    # the mask shows it in magenta
    out = snow({'show-mask': True}, data, w, h)
    check(list(out[i:i + 3]) == [1.0, 0.0, 1.0], 'speck not shown in the mask')


@case
def snow_removes_specks_keeps_sand():
    w, h = 160, 120
    data, spots = speck_scene(w, h)
    out = snow({}, data, w, h)
    left = sum(1 for x, y in spots if out[(y * w + x) * 4] > 0.45)
    check(left <= len(spots) // 10, '%d of %d specks left' % (left, len(spots)))
    sand = [i for i in range((2 * h // 3 + 1) * w * 4, len(out), 4)]
    changed = sum(1 for i in sand if out[i:i + 3] != data[i:i + 3])
    check(changed < 0.01 * len(sand), 'sand changed at %d of %d pixels' % (changed, len(sand)))


def soft_specks(w, h, seed, n):
    """Many soft specks close together, near the limit of the density
    test, whose soft edges the mask grows into: where the decisions of
    the operation reach furthest."""
    rnd = random.Random(seed)
    data = image(w, h, lambda x, y: (0.3 + rnd.gauss(0, 0.01), 0.55, 0.65, 1.0))
    for _ in range(n):
        cx, cy = rnd.uniform(0, w), rnd.uniform(0, h)
        sg, amp = rnd.uniform(0.5, 1.2), rnd.uniform(0.1, 0.4)
        for y in range(max(0, int(cy - 6)), min(h, int(cy + 7))):
            for x in range(max(0, int(cx - 6)), min(w, int(cx + 7))):
                g = amp * math.exp(-((x - cx) ** 2 + (y - cy) ** 2) / (2 * sg * sg))
                for c in range(3):
                    data[(y * w + x) * 4 + c] = min(data[(y * w + x) * 4 + c] + g, 1.0)
    return data


@case
def snow_tiles_do_not_matter():
    """The result must not depend on how GEGL splits the image into parts
    (tiles, threads, the visible area in GIMP): each part must read enough
    around it."""
    w, h = 240, 180
    for seed, n in ((1, 900), (2, 1100), (4, 900)):
        data = soft_specks(w, h, seed, n)
        rnd = random.Random(seed)
        for size in (1, 2, 5):
            whole, _ = run_snow({'size': size}, data, w, h)
            for _ in range(40):
                rx, ry = rnd.randrange(0, 200), rnd.randrange(0, 140)
                rw, rh = rnd.randrange(10, 40), rnd.randrange(10, 40)
                part, _ = run_snow({'size': size}, data, w, h, roi=(rx, ry, rw, rh))
                ref = array.array('f', [whole[((ry + y) * w + rx + x) * 4 + c]
                                        for y in range(rh) for x in range(rw) for c in range(4)])
                bad = sum(1 for k in range(0, len(ref), 4) if part[k:k + 4] != ref[k:k + 4])
                check(bad == 0, 'seed %d size %d: part %s differs from the whole at %d pixels'
                      % (seed, size, (rx, ry, rw, rh), bad))


@case
def snow_properties_at_limits():
    w, h = 50, 40
    data, _ = speck_scene(w, h, specks=10)
    for size in (1, 40):
        for th in (0.0, 1.0):
            for tg in (0.0, 1.0):
                for ct in (0.0, 1.0):
                    for sm in (False, True):
                        snow({'size': size, 'threshold': th, 'texture-guard': tg,
                                     'color-tolerance': ct, 'show-mask': sm}, data, w, h)


@case
def snow_deterministic_and_offset():
    w, h = 120, 90
    data, _ = speck_scene(w, h, seed=6)
    a, _ = run_snow({}, data, w, h)
    b, _ = run_snow({}, data, w, h)
    check(a == b, 'two runs differ')
    c = snow({}, data, w, h, x0=-33, y0=21)
    check(a == c, 'result depends on where the layer is')


@case
def snow_gray_input():
    data, _ = speck_scene(60, 45)
    for fmt in ('Y float', 'YA float', "Y' u8", "R'G'B' u8", "R'G'B'A u16"):
        snow({}, data, 60, 45, in_fmt=fmt)


@case
def snow_nan_inf_input():
    """Broken pixels are not repaired, but must not spread beyond the
    window of the filter."""
    w, h = 80, 60
    for bad in (float('nan'), float('inf')):
        data, _ = speck_scene(w, h, specks=0, grains=False)
        data[(30 * w + 40) * 4 + 1] = bad
        out, _ = run_snow({}, data, w, h)
        far = [k for k in range(0, len(out), 4)
               if abs((k // 4) % w - 40) > 8 or abs((k // 4) // w - 30) > 8]
        check(all(finite(out[k:k + 4]) for k in far), '%s spreads over the image' % bad)


# running the cases

def main():
    args = sys.argv[1:]
    if args[:1] == ['--case']:
        Gegl.init(None)
        for op in (UW, SNOW):
            if not Gegl.has_operation(op):
                print('%s is not loaded (GEGL_PATH?)' % op)
                sys.exit(2)
        try:
            CASES[args[1]]()
        except Fail as e:
            print(e)
            sys.exit(1)
        sys.exit(0)

    names = [n for n in CASES if not args or any(a in n for a in args)]
    failed = []
    for n in names:
        try:
            p = subprocess.run([sys.executable, os.path.abspath(__file__), '--case', n],
                               stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                               timeout=TIMEOUT)
            code, text = p.returncode, p.stdout
        except subprocess.TimeoutExpired as e:
            code, text = None, (e.stdout or b'').decode(errors='replace')
        msg = '\n'.join(l for l in text.splitlines()
                        if 'leak' not in l.lower() and l.strip())
        if code == 0:
            print('PASS  %s' % n, flush=True)
        else:
            why = ('no result after %d s' % TIMEOUT if code is None else
                   'crashed (signal %d)' % -code if code < 0 else 'exit %d' % code)
            print('FAIL  %s: %s' % (n, why), flush=True)
            for l in msg.splitlines()[-8:]:
                print('      ' + l)
            failed.append(n)
    print('%d passed, %d failed' % (len(names) - len(failed), len(failed)))
    sys.exit(1 if failed else 0)


if __name__ == '__main__':
    main()
