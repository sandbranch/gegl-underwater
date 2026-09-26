# Runs inside GIMP (tests/gimp-check.sh): applies both operations to a
# synthetic float image as non-destructive filters, merges them, and
# compares the pixels with the same operations run in plain GEGL. Prints
# PASS or FAIL per check.
import array
import os
import random

import gi
gi.require_version('Gimp', '3.0')
gi.require_version('Gegl', '0.4')
from gi.repository import Gimp, Gegl

W, H = 320, 200
FMT = 'RGBA float'
failed = []


def result(name, ok, msg=''):
    print('%s  gimp_%s%s' % ('PASS' if ok else 'FAIL', name, (': ' + msg) if msg and not ok else ''))
    if not ok:
        failed.append(name)


def scene():
    rnd = random.Random(1)
    a = array.array('f')
    for y in range(H):
        for x in range(W):
            t = y / (H - 1)
            r, g, b = 0.01 + 0.01 * t, 0.25 - 0.12 * t, 0.35 - 0.15 * t
            if (x - 100) ** 2 / 50 ** 2 + (y - 80) ** 2 / 30 ** 2 < 1:
                r, g, b = 0.7, 0.3, 0.08
            if rnd.random() < 0.003:
                r, g, b = r + 0.4, g + 0.4, b + 0.4
            n = rnd.gauss(0, 0.01)
            a.extend((max(r + n, 0), max(g + n, 0), max(b + n, 0), 1.0))
    return a


def gegl_result(op, props, data):
    rect = Gegl.Rectangle.new(0, 0, W, H)
    buf = Gegl.Buffer.new(FMT, 0, 0, W, H)
    buf.set(rect, FMT, data.tobytes())
    g = Gegl.Node()
    src = g.create_child('gegl:buffer-source')
    src.set_property('buffer', buf)
    node = g.create_child(op)
    for k, v in props.items():
        node.set_property(k, v)
    out = Gegl.Buffer.new(FMT, 0, 0, W, H)
    sink = g.create_child('gegl:write-buffer')
    sink.set_property('buffer', out)
    src.link(node)
    node.link(sink)
    sink.process()
    return array.array('f', out.get(rect, 1.0, FMT, Gegl.AbyssPolicy.NONE))


def gimp_result(op, props, data):
    image = Gimp.Image.new_with_precision(W, H, Gimp.ImageBaseType.RGB, Gimp.Precision.FLOAT_LINEAR)
    layer = Gimp.Layer.new(image, 'scene', W, H, Gimp.ImageType.RGBA_IMAGE, 100, Gimp.LayerMode.NORMAL)
    image.insert_layer(layer, None, 0)
    rect = Gegl.Rectangle.new(0, 0, W, H)
    buf = layer.get_buffer()
    buf.set(rect, FMT, data.tobytes())
    buf.flush()
    layer.update(0, 0, W, H)
    f = Gimp.DrawableFilter.new(layer, op, op)
    cfg = f.get_config()
    for k, v in props.items():
        cfg.set_property(k, v)
    f.update()
    layer.append_filter(f)
    names = [x.get_operation_name() for x in layer.get_filters()]
    layer.merge_filters()
    out = array.array('f', layer.get_buffer().get(rect, 1.0, FMT, Gegl.AbyssPolicy.NONE))
    image.delete()
    return names, out


# GIMP runs the filters in its own process; plain GEGL here, in the
# plug-in's, needs its own start
Gegl.init(None)
data = scene()
for op, props in (('underwater:correct', {}),
                  ('underwater:correct', {'red-restore': 1.5, 'keep-water': 0.2, 'white-balance': False}),
                  ('underwater:marine-snow', {}),
                  ('underwater:marine-snow', {'size': 9, 'threshold': 0.05})):
    names, got = gimp_result(op, props, data)
    want = gegl_result(op, props, data)
    d = max(abs(p - q) for p, q in zip(got, want))
    label = '%s_%s' % (op.split(':')[1].replace('-', '_'), 'defaults' if not props else 'set')
    result(label + '_is_a_filter', names == [op], str(names))
    result(label + '_same_as_gegl', d < 1e-5, 'max difference %.6f' % d)

print('%d failed' % len(failed))
with open(os.environ['UW_CHECK_STATUS'], 'w') as fh:
    fh.write('%d\n' % len(failed))
