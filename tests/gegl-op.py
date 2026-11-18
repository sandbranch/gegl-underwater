#!/usr/bin/env python3
"""Runs one GEGL operation on an image file, like the gegl command line
tool does (`gegl in -o out -- op prop=value ...`), through GEGL's Python
bindings, so the tests need no `gegl` binary (Debian and Ubuntu ship it in
a package of its own). The operations come from GEGL_PATH as usual.

  gegl-op.py IN OUT OP [prop=value ...]

Values are converted to the type of the property: numbers, true/false,
enum nicks, colors (as gegl:color takes them) and strings.
"""
import sys

import gi
gi.require_version('Gegl', '0.4')
from gi.repository import Gegl, GObject


def value(op, name, text):
    spec = Gegl.Operation.find_property(op, name)
    if spec is None:
        sys.exit('%s has no property %s' % (op, name))
    t = spec.value_type
    if t == GObject.TYPE_BOOLEAN:
        return text.lower() in ('1', 'true', 'yes', 'on')
    if t in (GObject.TYPE_INT, GObject.TYPE_UINT, GObject.TYPE_LONG,
             GObject.TYPE_ULONG, GObject.TYPE_INT64, GObject.TYPE_UINT64):
        return int(text)
    if t in (GObject.TYPE_DOUBLE, GObject.TYPE_FLOAT):
        return float(text)
    if t.is_a(GObject.TYPE_ENUM):
        for v in spec.enum_class.__enum_values__.values():
            if text in (v.value_nick, v.value_name, str(int(v))):
                return int(v)
        sys.exit('%s: no value %s for %s' % (op, text, name))
    if t.is_a(Gegl.Color.__gtype__):
        return Gegl.Color.new(text)
    return text


def main(argv):
    if len(argv) < 4:
        sys.exit(__doc__)
    src, dst, op = argv[1:4]
    # as the gegl tool does: GEGL offers GPL3 operations only to
    # applications that say they are GPL3 too (said before Gegl.init, or
    # GEGL loads its modules twice)
    Gegl.config().set_property('application-license', 'GPL3')
    Gegl.init(None)
    g = Gegl.Node()
    load = g.create_child('gegl:load')
    load.set_property('path', src)
    node = g.create_child(op)
    if node is None or node.get_operation() != op:
        sys.exit('no operation %s (GEGL_PATH?)' % op)
    for arg in argv[4:]:
        name, _, text = arg.partition('=')
        node.set_property(name, value(op, name, text))
    save = g.create_child('gegl:save')
    save.set_property('path', dst)
    load.link(node)
    node.link(save)
    save.process()
    del g, load, node, save
    Gegl.exit()


if __name__ == '__main__':
    main(sys.argv)
