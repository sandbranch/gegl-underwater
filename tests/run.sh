#!/bin/sh
# Runs underwater:correct from build/ on every test photo (tests/images,
# see fetch.py there) with the given settings, using the native GEGL
# that GIMP uses (tests/gegl-op.py), and writes before/after sheets and measurements to tests/output/<name>.
#   tests/run.sh name [prop=value ...]      e.g. tests/run.sh default
#   ONLY=strobe tests/run.sh name ...       only photos whose name has "strobe"
#   IMAGES=dir OUT=dir tests/run.sh name    photos from another folder, results
#                                           into OUT/<name>: for private photos,
#                                           keep both outside the repository
# GEGL and GIMP run isolated from your folders (tests/isolate.sh, with
# gimp-devtools/gimp-run.sh if it is there): HOME and the XDG
# folders point into tests/output/gimp-home, so nothing lands in your
# own GEGL, GIMP or babl folders.
set -e
here=$(cd "$(dirname "$0")" && pwd)
top=$(dirname "$here")
name=${1:-default}; [ $# -gt 0 ] && shift
src=$top
GIMP_RUN_HOME=${GIMP_RUN_HOME:-$here/output/gimp-home}
export GIMP_RUN_HOME
# shellcheck source=SCRIPTDIR/isolate.sh
. "$here/isolate.sh"
images=$(cd "${IMAGES:-$here/images}" && pwd)
mkdir -p "${OUT:-$here/output}"
outdir=$(cd "${OUT:-$here/output}" && pwd)
out="$outdir/$name"
mkdir -p "$out"
mod="$here/output/modules"; rm -rf "$mod"; mkdir -p "$mod"
cp "$top/build/underwater-correct.so" "$mod/"
gegl_path="$mod:$(pkg-config --variable=pluginsdir gegl-0.4)"
for f in "$images"/*${ONLY:-}*.[jJ][pP][gG]; do
  [ -e "$f" ] || continue
  b=$(basename "$f"); b=${b%.*}
  gimp_run --env=GEGL_PATH="$gegl_path" -- python3 "$here/gegl-op.py" \
    "$f" "$out/$b.jpg" underwater:correct "$@" 2>&1 | grep -v -i "leak\|warning\|^$" || true
done
python3 "$here/sheet.py" "$images" "$out"
