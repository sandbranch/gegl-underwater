#!/bin/sh
# Runs underwater:correct from build/ on every test photo (tests/images,
# see fetch.py there) with the given settings, using the GEGL of the Flatpak
# GIMP, and writes before/after sheets and measurements to tests/output/<name>.
#   tests/run.sh name [prop=value ...]      e.g. tests/run.sh default
#   ONLY=strobe tests/run.sh name ...       only photos whose name has "strobe"
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$here")
name=${1:-default}; [ $# -gt 0 ] && shift
out="$here/output/$name"
mkdir -p "$out"
mod="$here/output/modules"; rm -rf "$mod"; mkdir -p "$mod"
cp "$top/build/underwater-correct.so" "$mod/"
for f in "$here"/images/*${ONLY:-}*.jpg; do
  b=$(basename "$f" .jpg)
  flatpak run --filesystem="$here" --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" --command=gegl org.gimp.GIMP \
    "$f" -o "$out/$b.jpg" -- underwater:correct "$@" 2>&1 | grep -v -i "leak\|warning\|^$" || true
done
python3 "$here/sheet.py" "$here/images" "$out"
