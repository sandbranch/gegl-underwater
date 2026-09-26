#!/bin/sh
# Pass/fail tests of both operations on synthetic images (tests/check.py),
# with the modules from build/ (or the build folder in $BUILD) and the GEGL
# of the Flatpak GIMP, or the native GEGL with GIMP_FLATPAK=0. Needs no
# photos, network or display. Exits non-zero if a case fails.
#   tests/check.sh                 all cases
#   tests/check.sh snow tiny       only cases whose name has "snow" or "tiny"
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$here")
build=${BUILD:-$top/build}
case $build in /*) ;; *) build=$top/$build ;; esac

# GEGL_PATH replaces GEGL's own list, and the JSON files meson leaves in a
# build folder crash GEGL: a clean folder with only the modules
mod="$here/output/check-modules"
rm -rf "$mod"; mkdir -p "$mod"
for m in underwater-correct marine-snow; do
  [ -f "$build/$m.so" ] || { echo "no $build/$m.so: build first (README)" >&2; exit 2; }
  cp "$build/$m.so" "$mod/"
done

if [ "${GIMP_FLATPAK:-1}" != 0 ] && command -v flatpak >/dev/null 2>&1 &&
   flatpak info org.gimp.GIMP >/dev/null 2>&1; then
  exec flatpak run --filesystem="$top" --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" \
    --command=python3 org.gimp.GIMP "$here/check.py" "$@"
fi
GEGL_PATH="$mod:$(pkg-config --variable=pluginsdir gegl-0.4)" exec python3 "$here/check.py" "$@"
