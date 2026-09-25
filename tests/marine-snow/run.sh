#!/bin/sh
# Runs underwater:marine-snow from build/ on the synthetic scene with the
# given properties (default settings without any) and measures the result.
#   tests/marine-snow/run.sh name [prop=value ...]
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$(dirname "$here")")
out="$here/output"
mkdir -p "$out"
[ -f "$out/snow.png" ] || python3 "$here/make-scene.py" "$out"
mod="$out/modules"; rm -rf "$mod"; mkdir -p "$mod"
cp "$top/build/marine-snow.so" "$mod/"
name=${1:-default}; [ $# -gt 0 ] && shift
flatpak run --filesystem="$here" --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" --command=gegl org.gimp.GIMP \
  "$out/snow.png" -o "$out/$name.png" -- underwater:marine-snow "$@" 2>&1 | grep -v -i "leak\|warning\|^$" || true
python3 "$here/check.py" "$out" "$out/$name.png"
