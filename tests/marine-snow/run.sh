#!/bin/sh
# Runs underwater:marine-snow from build/ on the synthetic scene with the
# given properties (default settings without any) and measures the result.
#   tests/marine-snow/run.sh name [prop=value ...]
# GEGL and GIMP run isolated from your folders (tests/isolate.sh, with
# gimp-devtools/gimp-run.sh if it is there): HOME and the XDG
# folders point into tests/output/gimp-home, so nothing lands in your
# own GEGL, GIMP or babl folders.
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$(dirname "$here")")
out="$here/output"
mkdir -p "$out"
src=$top
GIMP_RUN_HOME=${GIMP_RUN_HOME:-$top/tests/output/gimp-home}
export GIMP_RUN_HOME
# shellcheck source=SCRIPTDIR/../isolate.sh
. "$top/tests/isolate.sh"
[ -f "$out/snow.png" ] || python3 "$here/make-scene.py" "$out"
mod="$out/modules"; rm -rf "$mod"; mkdir -p "$mod"
cp "$top/build/marine-snow.so" "$mod/"
name=${1:-default}; [ $# -gt 0 ] && shift
gimp_run --env=GEGL_PATH="$mod:$(pkg-config --variable=pluginsdir gegl-0.4)" \
  -- python3 "$top/tests/gegl-op.py" "$out/snow.png" "$out/$name.png" underwater:marine-snow "$@" 2>&1 | grep -v -i "leak\|warning\|^$" || true
python3 "$here/check.py" "$out" "$out/$name.png"
