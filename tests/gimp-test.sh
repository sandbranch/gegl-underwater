#!/bin/sh
# Adds the installed underwater:correct to a test photo inside the Flatpak
# GIMP, without a window, as a non-destructive filter, and compares the
# result with the command line one. Run tests/images/fetch.py first and
# install (README). GIMP runs with a throwaway profile
# (tests/output/gimp-test-profile) and isolated from your folders
# (tests/isolate.sh): the installed operations are read from your GEGL
# folder of the Flatpak, nothing is written there or anywhere else of
# yours, and a GIMP of yours does not matter.
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$here")
src=$top
GIMP_RUN_HOME=${GIMP_RUN_HOME:-$here/output/gimp-home}
export GIMP_RUN_HOME
# shellcheck source=SCRIPTDIR/isolate.sh
. "$here/isolate.sh"
# the installed operations (read only), and GEGL's own
gegl_path="$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins:/app/lib/gegl-0.4"
mkdir -p "$here/output/gimp-test-profile"
gimp_run --flatpak --filesystem="$here" --env=UW_TESTS="$here" \
  --env=GIMP3_DIRECTORY="$here/output/gimp-test-profile" --env=GEGL_PATH="$gegl_path" \
  -- gimp-console-3.2 --no-interface --no-data \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-test.py').read())" --quit 2>&1 | grep -E "filters on|saved|Error"

# the same photo on the command line, with the installed operation
gimp_run --flatpak --filesystem="$here" --env=GEGL_PATH="$gegl_path" -- gegl \
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
