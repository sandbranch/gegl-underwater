#!/bin/sh
# Adds the installed underwater:correct to a test photo inside the Flatpak
# GIMP, without a window, as a non-destructive filter, and compares the
# result with the command line one. Run tests/images/fetch.py first,
# install (README), and close GIMP.
here=$(cd "$(dirname "$0")" && pwd)
flatpak run --filesystem="$here" --env=UW_TESTS="$here" \
  --command=gimp-console-3.2 org.gimp.GIMP --no-interface --no-data \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-test.py').read())" --quit 2>&1 | grep -E "filters on|saved|Error"

# the same photo on the command line, with the installed operation
flatpak run --filesystem="$here" --command=gegl org.gimp.GIMP \
  "$here/images/reef-02.jpg" -o "$here/output/cli-reef-02.jpg" -- underwater:correct 2>&1 | grep -v -i "leak\|^$"
python3 - "$here/output" <<'PY'
import sys
from PIL import Image, ImageChops, ImageStat
a = Image.open(sys.argv[1] + '/gimp-reef-02.jpg').convert('RGB')
b = Image.open(sys.argv[1] + '/cli-reef-02.jpg').convert('RGB')
d = sum(ImageStat.Stat(ImageChops.difference(a, b)).mean) / 3
print('GIMP vs command line: mean abs diff %.2f (of 255)' % d)
sys.exit(0 if d < 1.0 else 1)
PY
