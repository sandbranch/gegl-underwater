# Runs inside GIMP (tests/marine-snow/gimp-test.sh): adds
# underwater:marine-snow to the snowy test scene as a non-destructive
# filter, saves the result, and an XCF that keeps the filter editable.
import os

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp, Gio

out = os.environ['SNOW_TEST_OUT']
image = Gimp.file_load(Gimp.RunMode.NONINTERACTIVE, Gio.File.new_for_path(os.path.join(out, 'snow.png')))
layer = image.get_layers()[0]
f = Gimp.DrawableFilter.new(layer, 'underwater:marine-snow', 'Remove Marine Snow')
f.get_config().set_property('size', 5)
f.update()
layer.append_filter(f)
print('filters on the layer:', [x.get_name() for x in layer.get_filters()])
Gimp.file_save(Gimp.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(os.path.join(out, 'gimp-result.png')), None)
Gimp.file_save(Gimp.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(os.path.join(out, 'gimp-result.xcf')), None)
print('saved')
