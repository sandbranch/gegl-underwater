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

All steps work on linear RGB (float) in the color space of the image.

1. **Statistics.** Channel means, and robust percentiles, over the whole
   image (the operation asks GEGL for the whole input, like
   `gegl:stretch-contrast`).

2. **Water color (veiling light).** Either estimated from the image, from
   the pixels that are furthest away: bright, low-contrast, and dominated
   by blue/green; or picked by the user from open water with GIMP's color
   picker (`water-color`, shown when `auto-water` is off).

3. **Backscatter removal** (`backscatter`, 0 to 1). Estimate per pixel how
   much of it is veil, from a dark-channel-like measure on the green and
   blue channels (red carries little information underwater), smoothed so
   that it follows scene structure, and subtract that share of the water
   color. The strength scales the estimate.

4. **Red restoration** (`red-restore`, 0 to 2) from the green channel,
   which keeps detail where red has none, following the red channel
   compensation of Ancuti et al. (TIP 2018), as published:

       R' = R + a * (mean(G) - mean(R)) * (1 - R) * G

   with R, G in 0..1 and `a` the strength. The factor (1 - R) * G restores
   red where it is missing and where green has signal, and leaves pixels
   that already have red alone. Note: a widely copied MATLAB version
   simplifies this to `R + a * (mean(G) - mean(R))`; we follow the paper,
   to be checked against the paper itself (PLAN.md).

   **Blue restoration** (`blue-restore`, default 0) does the same for blue,
   for green water where blue is absorbed too.

5. **White balance** (`white-balance`). A robust gray-world estimate of the
   remaining cast (ignoring the brightest and darkest percentiles), as a
   per-channel gain in linear light.

6. **Keep water color** (`keep-water`, 0 to 1). The corrected photo is
   blended back towards the water color where the backscatter estimate
   says a pixel is mostly water, so open water stays blue while the
   subject is corrected.

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

US 12,373,929 B2 (Arashi Vision, active until 2042), "Underwater image
color restoration method and apparatus", claims (claim 1) a method with
all of these steps: converting 8-bit RGB to linear sRGB; adjusting each
channel by mean values; a weight per pixel; gains for the red and blue
channels; converting back to 8-bit; ranking pixel values for maximum and
minimum adjustment values per channel; adjusting by those; and fusing the
original with the adjusted values by the per-pixel weights.

This design does not use that combination: it works in float at any
precision without converting to 8-bit, restores red from green (Ancuti)
rather than by red and blue gains, estimates backscatter from a
dark-channel measure, and has no ranking-based stretch or weighted fusion
of original and adjusted values. Any change to the pipeline is checked
against the claim before it is made. This is our reading of the claim, not
legal advice.

No patent was found on the red channel compensation of Ancuti et al.
(searched: Google Patents, September 2026); see research.md.
