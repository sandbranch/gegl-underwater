#!/bin/sh
# Tests the installed underwater:marine-snow inside GIMP (Flatpak), without
# a window, as a non-destructive filter; then measures the result. GIMP
# runs with a throwaway profile (output/gimp-profile) and isolated from
# your folders (tests/isolate.sh): the installed operation is read from
# your GEGL folder of the Flatpak, nothing is written there or anywhere
# else of yours.
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$(dirname "$here")")
out="$here/output"
mkdir -p "$out" "$out/gimp-profile"
src=$top
GIMP_RUN_HOME=${GIMP_RUN_HOME:-$top/tests/output/gimp-home}
export GIMP_RUN_HOME
# shellcheck source=SCRIPTDIR/../isolate.sh
. "$top/tests/isolate.sh"
# the installed operations (read only), and GEGL's own
gegl_path="$HOME/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins:/app/lib/gegl-0.4"
[ -f "$out/snow.png" ] || python3 "$here/make-scene.py" "$out"
gimp_run --flatpak --filesystem="$here" --env=SNOW_TEST_OUT="$out" \
  --env=GIMP3_DIRECTORY="$out/gimp-profile" --env=GEGL_PATH="$gegl_path" \
  -- gimp-console-3.2 --no-interface --no-data \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-test.py').read())" --quit 2>&1 | grep -E "filters on|saved|Error"
python3 "$here/check.py" "$out" "$out/gimp-result.png"
