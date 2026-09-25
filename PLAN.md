# Plan

## 0. Skeleton (done)

- [x] Repository, license (GPL-3.0-or-later), meson build
- [x] GEGL operation `underwater:correct` with all properties, passing the
      input through; loads in GEGL, appears as Colors > Underwater Color
      Correction... in GIMP
- [x] Research notes (docs/research.md) and design (docs/design.md)

## 1. Test photos and measurements

- [ ] A set of real underwater photos in `tests/images/` (see its README
      for what is wanted and the rules for committing them)
- [ ] `tests/run.sh`: runs the operation with given settings on every
      photo and writes before/after sheets to `tests/output/`
- [ ] Simple measurements per photo: channel means, gray-world deviation,
      clipped pixels; the quality metrics UCIQE/UIQM from the literature
      as a second opinion only (their known weaknesses: research.md)
- [ ] Check the red compensation formula against the paper itself
      (Ancuti et al., TIP 2018), not only secondary sources

## 2. Color: red/blue restoration and white balance

- [ ] Statistics pass (means, percentiles) over the whole image
- [ ] Red and blue restoration (design step 4)
- [ ] Robust white balance (design step 5)
- [ ] Look at the results on all test photos; tune the defaults

## 3. Water: color estimate, backscatter, keep water color

- [ ] Water color estimate (design step 2) and the picked color
- [ ] Backscatter estimate and removal (design step 3)
- [ ] Keep water color (design step 6)
- [ ] Tune on the test photos; compare against simple red boosting and
      plain white balance to show the difference

## 4. Speed and polish

- [ ] Spread the per-pixel steps over GEGL's threads (as in
      gegl-wavelet's denoise); target: under a few seconds for 24 MP
- [ ] Slider labels, descriptions and ranges checked in GIMP's dialog
- [ ] A note on the known GEGL command line warning about a leaked
      buffer, which whole-image operations such as gegl:stretch-contrast
      also show; check it does not appear in GIMP

## 5. Release

- [ ] README with before/after examples (photos used with permission)
- [ ] Install instructions for GIMP (native and Flatpak), tested
- [ ] Offer it to the community: discuss.pixls.us, GIMP forums, and the
      diving photography communities
