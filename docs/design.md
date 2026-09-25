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

1. **Statistics** over the whole image (the operation asks GEGL for all
   of its input): channel means and percentiles.

2. **Water color A (veiling light).** Estimated from the darkest RGB
   triplets and the brightest pixels of a green/blue dark channel (the
   ideas of Sea-thru and UDCP, done once for the whole image), or picked
   by the user from open water (`water-color` with `auto-water` off). A is
   kept within the range of physically plausible water colors (Akkaynak et
   al. 2017).

3. **Transmission and layers.** A transmission map t from a minimum
   filter on green and blue (UDCP), refined edge-aware (guided filter or
   `gegl:domain-transform`), with the rank-one projection (ROP) as a
   cross-check. A soft **ambient mask** marks the ambient-lit layer: low t,
   little red; strobe-lit areas are recognized by their saturation and
   red (the cue of Galdran et al.) and kept out of it.

4. **Backscatter removal** (`backscatter`, 0 to 1):
   `D = I - backscatter * A * (1 - t)`.

5. **Red restoration** (`red-restore`, 0 to 2) on D, as published by
   Ancuti et al. (TIP 2018, Eq. 4), with a the strength:

       R' = R + a * (mean(G) - mean(R)) * (1 - R) * G

   applied through the ambient mask and weighted per pixel by how much
   red is missing (the attenuation-weighted idea of ACDC and MLLE), so
   strobe-lit subjects and pixels that already have red are left alone.
   **Blue restoration** (`blue-restore`, Eq. 5) does the same for blue in
   green or turbid water. A local variant after 3C (Ancuti et al. 2020,
   opponent channels minus their large-scale mean) is kept as an option
   to evaluate.

6. **White balance** (`white-balance`): shades of gray (Minkowski p about
   6) on the non-water pixels, after the compensation, with clamped gains
   so that a tiny red mean cannot blow up noise. A local variant (local
   average color, as Sea-thru's LSAC or gray world in lαβ with integral
   images) is an option to evaluate for uneven light.

7. **Keep water color** (`keep-water`, 0 to 1): add back
   `keep-water * A * (1 - t)` (A white-balanced), so open water stays
   blue instead of turning gray.

Later, as optional finishing steps (PLAN.md): local contrast on L from
integral images (MLLE), and a gentle chroma curve on a*/b* (RGHS).

Clipping: none for float images; integer images are limited by their
precision when GIMP stores the result.

## Controls

| Property | Range | Default | What it does |
|---|---|---|---|
| red-restore | 0 to 2 | 1.0 | Rebuild absorbed red from green |
| blue-restore | 0 to 2 | 0.0 | Same for blue, for green water |
| white-balance | on/off | on | Remove the remaining cast |
| auto-water | on/off | on | Estimate the water color |
| water-color | color | #1e6478 | Picked water color when auto is off |
| backscatter | 0 to 1 | 0.5 | Remove the veil of scattered light |
| keep-water | 0 to 1 | 0.5 | Keep open water looking like water |

The defaults are placeholders until they are tuned on test photos.

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
