# Design

An original GEGL operation, `underwater:correct`, that restores the colors of
a single underwater photo in GIMP 3, non-destructively (GIMP's filter dialog
gives on-canvas preview, split view, presets and editing later).

Constraints:

- one photo only: no depth map, no calibration target, no training data;
- classical, explainable steps with controls a photographer understands;
- fast enough to preview: seconds on a 24 megapixel photo, in C;
- any precision (8/16 bit, float), processed in linear light;
- original code, built from published physics and methods (see
  [research.md](research.md)), and staying clear of patented recipes
  (see "Patents" below).

## Why simple fixes fail

Water does two different things to light:

1. **Absorption** along the path from subject to camera: red is absorbed
   first, then orange and yellow; blue and green travel furthest. The
   subject loses red with distance.
2. **Backscatter**: particles scatter ambient light towards the camera,
   adding a veil of the water's own color that grows with distance.

A global white balance treats the whole photo as lit by one colored light.
It turns open water gray and lit areas pink, which is the classic complaint
(see the darktable discussion in research.md). Boosting red alone brightens
noise in a channel that holds little signal. The design therefore separates
the steps.

## Pipeline

All steps work on linear RGB (float) in the color space of the image, as
the physics requires (Berman et al.). The literature behind each step is
in [research.md](research.md).

Two principles from the research shape it:

- **Backscatter and color loss are different effects** with different
  coefficients (Akkaynak & Treibitz, revised model, 2018), so they are
  removed in separate steps with separate controls, backscatter first.
- **A photo has two layers** (photographers' practice): a strobe-lit
  foreground that is about daylight-balanced, and an ambient-lit
  background that has lost its red. Corrections for water act on the
  ambient layer, so strobe-lit subjects do not turn red; this is where
  global filters fail.

The steps as implemented (first version, `operations/underwater-correct.c`):

1. **Whole image.** The operation reads all of its input as linear
   "RGBA float" and works on three sizes: a small copy (512 px on the
   long side, by block averaging) for the estimates, a medium copy (about
   1536 px) for the transmission map, and the full image for the result.

2. **Water color A (veiling light).** With `auto-water` on, a quadtree
   search on the small copy (after Kim et al. 2013): the image is split
   in four, the quarter with the best score is kept, down to 64 px or
   less, and A is the mean of that last block. The score is
   `mean(max(G, B) - R) - 2 * sd(luma)`: open water is blue or green, has
   little red and is smooth. The UDCP choice (the brightest pixel of the
   dark channel) was tried first and failed on real photos: it picked a
   bright gray shark as the water. With `auto-water` off, A is
   `water-color`.

3. **Transmission t.** `t = 1 - 0.9 * min over 3x3 of min(G/A_g, B/A_b)`
   (UDCP on green and blue), refined with a guided filter (He et al.) on
   luma, clamped to 0.1 to 1. It is refined a second time on the medium
   copy, guided by its luma, which removes most halos around subjects;
   the full image interpolates t from there.

4. **Subject and veil.** The subject is recovered as
   `J = max(I - A * (1 - t), 0) / max(t, 0.3)^clarity`: the veil is
   taken off completely to measure the subject, and `clarity` (0 to 1)
   sets how much of the lost contrast in the distance comes back.
   `backscatter` then decides how much of the veil stays in the result.

5. **Ambient weight.** `1 - smoothstep(0.35, 0.8, R/G)`: pixels that
   already have red (strobe-lit subjects) get little or no water
   correction. It is multiplied by `smoothstep(0.15, 0.6, t)`, so distant
   water is left to step 7.

6. **Red and blue restoration** on J, after Ancuti et al. (TIP 2018,
   Eq. 4), with one deviation: G is normalized by its mean,

       R' = R + a * m * (mean(G) - mean(R)) * (1 - R) * min(G/mean(G), 3)

   where m is the ambient weight. With plain G as in the paper, dark
   photos (mean G about 0.1 in linear light) got almost no correction.
   Blue gets the same with `blue-restore` plus an automatic part,
   `0.5 * red-restore * greenness`, where greenness is
   `(mean(G) - mean(B)) / mean(G)` of the subject: in green water, where
   blue is absorbed too, it turns the olive cast into natural colors
   without the user having to find the blue slider. The means are over
   the subject, weighted by the ambient weight.

7. **White balance** (`white-balance`): shades of gray (Minkowski p = 6)
   over the restored subject, weighted by t, gives the light's color. The
   gains are kept physically possible: red can only go up (1 to 2.5,
   water never adds red), blue relative to green 0.6 to 2, and in blue
   water blue can only go down, in green water only up. The gains are
   normalized to keep luma. They fade out with distance (by t) and in
   highlights (by the smallest channel from 0.6 to 1), so bright
   highlights do not turn magenta. Without these limits the tests showed
   violet water, strobe-lit reds cut away and green murk turning pink.

8. **Result and keep water color.**

       veil  = (1 - backscatter) * (1 - t)
       A'    = keep-water * A + (1 - keep-water) * luma(A)
       out   = J * gain * (1 - veil) + A' * veil

   so `keep-water` 1 keeps the water's own color in the veil and 0 makes
   it neutral gray of the same brightness. Pixels that were clipped in
   the photo (median channel 0.85 to 1) are pulled back to their original
   brightest channel, so blown highlights stay white.

Later, as optional finishing steps (PLAN.md): local contrast on L from
integral images (MLLE), and a gentle chroma curve on a*/b* (RGHS).

Clipping: none for float images; integer images are limited by their
precision when GIMP stores the result.

### Known issues (first version)

On the 42 test photos (`tests/run.sh`):

- sunlit water near the surface turns slightly cyan (ambient-blue-03);
- murky green water turns khaki gray rather than a clean green;
- a slight glow can remain around subjects against open water;
- a gray subject in blue water (the shark in ambient-blue-01) comes out
  slightly warm;
- speed: about 4 s on 24 MP on top of loading and saving (7.5 s against
  3.5 s for load and save only, on this machine).

## Controls

| Property | Range | Default | What it does |
|---|---|---|---|
| red-restore | 0 to 2 | 1.0 | Rebuild absorbed red from green |
| blue-restore | 0 to 2 | 0.0 | Same for blue, for green water |
| white-balance | on/off | on | Remove the remaining cast |
| auto-water | on/off | on | Estimate the water color |
| water-color | color | #1e6478 | Picked water color when auto is off |
| backscatter | 0 to 1 | 0.5 | Remove the veil of scattered light |
| clarity | 0 to 1 | 0.5 | Bring back contrast in the distance |
| keep-water | 0 to 1 | 0.5 | Keep open water looking like water |

The defaults are a first tuning on the test photos.

## Borrowing from other projects

Ideas and algorithms from papers and other projects are welcome; methods
are not protected by copyright (patents are, see below). Code is another
matter, and depends on its license:

- **GPL-3.0 or compatible** (e.g. bornfree/dive-color-corrector,
  GPL-3.0): may be copied or adapted, keeping the copyright notice and
  crediting the project in the file and in this document.
- **Permissive** (MIT, BSD, Apache-2.0; e.g. the CXH-Research list is
  MIT): may be copied, keeping the license notice.
- **No license** (e.g. nikolajbech/underwater-image-color-correction,
  wangyanckxx/Single-Underwater-Image-Enhancement-and-Color-Restoration):
  all rights reserved, so we take the ideas only and write our own code.

Every borrowed idea or piece of code is credited here with its source.

### Credits

(none yet)

## Patents

The patents found are listed in [research.md](research.md#patents). Two
shape this design:

- **US 12,373,929 B2** (Arashi Vision, active until 2042) claims (claim 1)
  a method with all of these steps: converting 8-bit RGB to linear sRGB;
  adjusting each channel by mean values; a weight per pixel; gains for the
  red and blue channels; converting back to 8-bit; ranking pixel values
  for maximum and minimum adjustment values per channel; adjusting by
  those; and fusing the original with the adjusted values by the per-pixel
  weights. This design works in float at any precision without converting
  to 8-bit, restores red from green (Ancuti) rather than by red and blue
  gains, removes backscatter from a transmission estimate, and has no
  ranking-based stretch and no weighted fusion of original and adjusted
  values.
- **US 11,024,047 B2** (University of California, IBLA, active until
  2037) claims depth from a multi-scale blurriness map with a maximum
  filter and refinement. This design uses no blurriness-based depth.

Any change to the pipeline is checked against these claims before it is
made. This is our reading of the claims, not legal advice.
