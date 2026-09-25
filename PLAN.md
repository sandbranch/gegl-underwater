# Plan

**Where we left off (2026-09-25):** skeleton, research and design are
done, and so is marine snow removal (4b). Next is milestone 1, which
waits on test photos in `tests/images/`. The status of all the GIMP 3
work is in
[gimp-plugin-devtools/STATUS.md](https://github.com/sandbranch/gimp-plugin-devtools/blob/main/STATUS.md).

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
- [x] Check the red compensation formula against the paper itself
      (Ancuti et al., TIP 2018, Eq. 4; see research.md)
- [ ] SQUID (Berman et al.) as the accuracy test: its color charts at
      known distances give a measurable error, unlike UCIQE/UIQM

## 2. Color: red/blue restoration and white balance

- [ ] Statistics pass (means, percentiles) over the whole image
- [ ] Red and blue restoration, attenuation-weighted (design step 5)
- [ ] Robust white balance (design step 6)
- [ ] Look at the results on all test photos; tune the defaults

## 3. Water: color estimate, backscatter, keep water color

- [ ] Water color estimate and the picked color (design step 2)
- [ ] Transmission map, ambient mask and backscatter removal (design steps 3-4)
- [ ] Keep water color (design step 7)
- [ ] Tune on the test photos; compare against simple red boosting and
      plain white balance to show the difference

## 4. Evaluate options and finishing

- [ ] Local red compensation after 3C versus Eq. 4; local white balance
      (LSAC / lαβ with integral images) versus global
- [ ] Optional local contrast on L from integral images (MLLE) and a
      gentle a*/b* chroma curve (RGHS)
- [ ] Compare against MLLE's published results as the reference among
      classical methods

## 4b. Noise and particles

- [ ] "Reduce red noise": wavelet denoise (the algorithm of
      gegl-wavelet's `wavelet:denoise`) on the restored channels,
      weighted by how much red was rebuilt, so well-lit areas keep their
      detail. Until then: stack Wavelet Denoise (CIELAB, strong a*/b*,
      gentle L*) after this filter in GIMP.
- [x] Check the detection rule of Farhadifard et al. 2017 and patents
      (research.md, "Noise and particles" and "Patents")
- [x] A separate operation `underwater:marine-snow` (Filters > Enhance >
      Remove Marine Snow...), after Farhadifard et al. 2017, with fast
      stand-ins for their tests (research.md). On the synthetic scene
      (`tests/marine-snow/run.sh`): 97.7 % of the specks no longer
      visible, sand 0.03 % and the fish 0.06 % of pixels changed, a large
      white object untouched, a dark eye under a speck kept; 1 to 2 s on
      24 MP. Works as a non-destructive filter in GIMP
      (`tests/marine-snow/gimp-test.sh`).
- [ ] Marine snow on real photos: check small white fish, plankton that
      is part of the subject, strobe reflections (larger bright circles,
      which the paper also leaves alone), and specks on busy reef

## 5. Speed and polish

- [ ] Spread the per-pixel steps over GEGL's threads (as in
      gegl-wavelet's denoise); target: under a few seconds for 24 MP
- [ ] Slider labels, descriptions and ranges checked in GIMP's dialog
- [ ] A note on the known GEGL command line warning about a leaked
      buffer, which whole-image operations such as gegl:stretch-contrast
      also show; check it does not appear in GIMP

## 6. Release

- [ ] README with before/after examples (photos used with permission)
- [ ] Install instructions for GIMP (native and Flatpak), tested
- [ ] Offer it to the community: discuss.pixls.us, GIMP forums, and the
      diving photography communities
