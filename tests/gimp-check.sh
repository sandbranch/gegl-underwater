#!/bin/sh
# Both operations as non-destructive filters in the Flatpak GIMP, without
# a window, on a synthetic float image, against plain GEGL
# (tests/gimp-check.py). Uses the modules from build/ (or $BUILD) and a
# throwaway GIMP profile in tests/output/gimp-profile (GIMP3_DIRECTORY),
# so the installed filters and the user's GIMP settings are not touched.
# The first run takes about a minute (GIMP sets up the new profile).
# Exits non-zero if a check fails.
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$here")
build=${BUILD:-$top/build}
case $build in /*) ;; *) build=$top/$build ;; esac
mod="$here/output/gimp-check-modules"
rm -rf "$mod"; mkdir -p "$mod" "$here/output/gimp-profile"
for m in underwater-correct marine-snow; do
  [ -f "$build/$m.so" ] || { echo "no $build/$m.so: build first (README)" >&2; exit 2; }
  cp "$build/$m.so" "$mod/"
done
status="$here/output/gimp-check.status"
rm -f "$status"
flatpak run --filesystem="$top" \
  --env=GIMP3_DIRECTORY="$here/output/gimp-profile" \
  --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" \
  --env=UW_CHECK_STATUS="$status" \
  --command=gimp-console-3.2 org.gimp.GIMP --no-interface --no-data --no-fonts \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-check.py').read())" --quit 2>&1 | grep -E "^(PASS|FAIL)|failed$|Error|Traceback" || true
[ "$(cat "$status" 2>/dev/null)" = 0 ]
