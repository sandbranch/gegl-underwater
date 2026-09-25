#!/bin/sh
# Tests the installed underwater:marine-snow inside GIMP (Flatpak), without
# a window, as a non-destructive filter; then measures the result.
set -e
here=$(cd "$(dirname "$0")" && pwd)
out="$here/output"
mkdir -p "$out"
[ -f "$out/snow.png" ] || python3 "$here/make-scene.py" "$out"
flatpak run --filesystem="$here" --env=SNOW_TEST_OUT="$out" \
  --command=gimp-console-3.2 org.gimp.GIMP --no-interface --no-data \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-test.py').read())" --quit 2>&1 | grep -E "filters on|saved|Error"
python3 "$here/check.py" "$out" "$out/gimp-result.png"
