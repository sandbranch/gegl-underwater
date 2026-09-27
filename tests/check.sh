#!/bin/sh
# Pass/fail tests of both operations on synthetic images (tests/check.py),
# with the modules from build/ (or the build folder in $BUILD) and the GEGL
# of the Flatpak GIMP, or the native GEGL with GIMP_FLATPAK=0. Needs no
# photos, network or display. Exits non-zero if a case fails.
#   tests/check.sh                 all cases
#   tests/check.sh snow tiny       only cases whose name has "snow" or "tiny"
#
# With AddressSanitizer and UndefinedBehaviorSanitizer (Flatpak only, runs
# with the SDK, which has their libraries):
#   gimp-build.sh . meson setup build-asan -Db_sanitize=address,undefined
#   gimp-build.sh . ninja -C build-asan
#   BUILD=build-asan SANITIZE=1 tests/check.sh
#
# GEGL and GIMP run isolated from your folders (tests/isolate.sh, with
# gimp-plugin-devtools/gimp-run.sh if it is there): HOME and the XDG
# folders inside the Flatpak point into tests/output/gimp-home, so
# nothing lands in ~/.var/app/org.gimp.GIMP.
# Before and after, it lists your folders of GIMP and the other apps
# (gimp-plugin-devtools/snapshot.sh, skipped without it) and fails if
# anything there changed.
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

src=$top
GIMP_RUN_HOME=${GIMP_RUN_HOME:-$here/output/gimp-home}
export GIMP_RUN_HOME
# shellcheck source=SCRIPTDIR/isolate.sh
. "$here/isolate.sh"
snapshot_take "$here/output/snapshot-check-before.txt"
rc=0
if [ "${GIMP_FLATPAK:-1}" != 0 ] && command -v flatpak >/dev/null 2>&1 &&
   flatpak info org.gimp.GIMP >/dev/null 2>&1; then
  if [ -n "$SANITIZE" ]; then
    # the modules are instrumented but python and GEGL are not: the
    # runtimes are loaded first. Leaks are not checked (python and GLib
    # keep much until exit); any other error ends the case as a FAIL
    gimp_run --flatpak --devel --filesystem="$top" --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" \
      --env=LD_PRELOAD=libasan.so.8:libubsan.so.1 \
      --env=ASAN_OPTIONS=detect_leaks=0:abort_on_error=0:exitcode=3 \
      --env=UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1:exitcode=4 \
      --env=CHECK_TIMEOUT="${CHECK_TIMEOUT:-600}" \
      -- python3 "$here/check.py" "$@" || rc=$?
  else
    gimp_run --flatpak --filesystem="$top" --env=GEGL_PATH="$mod:/app/lib/gegl-0.4" \
      -- python3 "$here/check.py" "$@" || rc=$?
  fi
else
  gimp_run --native --env=GEGL_PATH="$mod:$(pkg-config --variable=pluginsdir gegl-0.4)" \
    -- python3 "$here/check.py" "$@" || rc=$?
fi
snapshot_check "$here/output/snapshot-check-before.txt" "" || rc=1
exit $rc
