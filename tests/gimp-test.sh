#!/bin/sh
# Adds the installed underwater:correct to a test photo inside the Flatpak
# GIMP, without a window, as a non-destructive filter, and compares the
# result with the command line one. Run tests/images/fetch.py first,
# install (README), and close GIMP.
here=$(cd "$(dirname "$0")" && pwd)
flatpak run --filesystem="$here" --env=UW_TESTS="$here" \
  --command=gimp-console-3.2 org.gimp.GIMP --no-interface --no-data \
  --batch-interpreter python-fu-eval \
  -b "exec(open('$here/gimp-test.py').read())" --quit 2>&1 | grep -E "filters on|saved|Error"
