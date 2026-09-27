#!/bin/sh
# Both operations as non-destructive filters in the Flatpak GIMP, without
# a window, on a synthetic float image, against plain GEGL
# (tests/gimp-check.py). Uses the modules from build/ (or $BUILD) and a
# throwaway GIMP profile in tests/output/gimp-profile (GIMP3_DIRECTORY),
# so the installed filters and the user's GIMP settings are not touched.
# The first run takes about a minute (GIMP sets up the new profile).
# GEGL and GIMP run isolated from your folders (tests/isolate.sh, with
# gimp-devtools/gimp-run.sh if it is there): HOME and the XDG
# folders inside the Flatpak point into tests/output/gimp-home, so
# nothing lands in ~/.var/app/org.gimp.GIMP.
# Before and after, it lists your folders of GIMP and the other apps
# (gimp-devtools/snapshot.sh, skipped without it) and fails if
# anything there changed.
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
src=$top
GIMP_RUN_HOME=${GIMP_RUN_HOME:-$here/output/gimp-home}
export GIMP_RUN_HOME
# shellcheck source=SCRIPTDIR/isolate.sh
. "$here/isolate.sh"
snapshot_take "$here/output/snapshot-gimp-before.txt"
status="$here/output/gimp-check.status"
rm -f "$status"
gimp_run --flatpak --filesystem="$top" \
  --env=GIMP3_DIRECTORY="$here/output/gimp-profile" \
  --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" \
  --env=UW_CHECK_STATUS="$status" \
  -- gimp-console-3.2 --no-interface --no-data --no-fonts \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-check.py').read())" --quit 2>&1 | grep -E "^(PASS|FAIL)|failed$|Error|Traceback" || true
rc=0
[ "$(cat "$status" 2>/dev/null)" = 0 ] || rc=1
snapshot_check "$here/output/snapshot-gimp-before.txt" "" || rc=1
exit $rc
