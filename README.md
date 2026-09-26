# gegl-underwater

An open-source filter for GIMP 3 (a GEGL operation) that restores the
colors of underwater photos: it rebuilds the red light that the water
absorbed, removes the color cast of the water and the veil of light that
the water scatters back, while keeping open water looking like water.

It runs as a non-destructive filter in GIMP: Colors > Underwater Color
Correction..., with on-canvas preview, split view and presets, and it stays
editable in the layer's filter list.

**Status:** `underwater:marine-snow` (Filters > Enhance > Remove Marine
Snow...) works: it removes the bright specks of particles lit by a strobe
and leaves the rest of the photo alone. The color correction
(`underwater:correct`) works in a first version: tested on 42 freely
licensed photos from Wikimedia Commons (blue and green water, wrecks,
reefs, strobe-lit subjects, neutral references), with known issues
listed in [docs/design.md](docs/design.md). See [PLAN.md](PLAN.md).

To test, after building:

- `tests/check.sh`: pass/fail tests of both operations on synthetic
  images (sizes from 1 x 1, uniform, gray, float images with NaN, the
  properties at their limits, parts of the image against the whole);
  needs no photos, network or display. With a build made with
  `-Db_sanitize=address,undefined` it also runs under AddressSanitizer
  and UBSan, see the script.
- `tests/gimp-check.sh`: both filters in GIMP, without a window, against
  plain GEGL, with a throwaway GIMP profile in `tests/output/`.
- `tests/images/fetch.py` downloads the test photos; `tests/run.sh`
  writes before/after sheets and measurements of them to
  `tests/output/`, for looking at, and `tests/gimp-test.sh` compares
  the installed filter in GIMP with the command line on one of them.

- [docs/design.md](docs/design.md): how it works and why, and the patent
  it stays clear of
- [docs/research.md](docs/research.md): existing tools and the literature
- [tests/images/README.md](tests/images/README.md): test photos wanted

## Building and installing

Needs meson, ninja, a C compiler and the GEGL development files (0.4.62 or
newer).

    meson setup build -Dmoduledir=$HOME/.local/share/gegl-0.4/plug-ins
    ninja -C build install

For the Flatpak version of GIMP, build inside it with
[gimp-plugin-devtools](https://github.com/sandbranch/gimp-plugin-devtools):

    gimp-build.sh . meson setup build -Dmoduledir=\$GEGL_OPDIR
    gimp-build.sh . ninja -C build install

Restart GIMP after installing.

## License

GPL version 3 or later, see COPYING.
