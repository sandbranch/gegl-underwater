# Runs inside GIMP (tests/gimp-test.sh): adds underwater:correct to a test
# photo as a non-destructive filter and saves the result.
import os

import gi
gi.require_version('Gimp', '3.0')
from gi.repository import Gimp, Gio

here = os.environ['UW_TESTS']
image = Gimp.file_load(Gimp.RunMode.NONINTERACTIVE, Gio.File.new_for_path(os.path.join(here, 'images', 'reef-02.jpg')))
layer = image.get_layers()[0]
f = Gimp.DrawableFilter.new(layer, 'underwater:correct', 'Underwater Color Correction')
f.update()
layer.append_filter(f)
print('filters on the layer:', [x.get_name() for x in layer.get_filters()])
os.makedirs(os.path.join(here, 'output'), exist_ok=True)
Gimp.file_save(Gimp.RunMode.NONINTERACTIVE, image, Gio.File.new_for_path(os.path.join(here, 'output', 'gimp-reef-02.jpg')), None)
print('saved')
