# What others do about green murk and varying open water: survey

Collected 2026-09-26 for `underwater:correct`. Research only: no code in
the repository was changed. The survey is aimed at our two open problems,
not at listing products:

1. **Murky green or teal water:** the far sea floor stays green. Red
   restoration, white balance and the kept veil all fade with distance
   (by t), which is what keeps open water from turning violet; in murk the
   floor really is far, so its correction is switched off, and the kept
   veil (the water colour) is large. Gating by the open water mask of the
   `transmission-surface` branch was too coarse: blotchy, hard-edged
   patches (transmission-research.md on that branch, "Gating by the open
   water mask").
2. **Open water whose colour changes across the frame**, and the rings or
   blotches where subject meets water.

How it was checked. Code was read in the repositories (clones in
`/tmp/claude-1000/uw-survey/`), licences from the LICENSE file and the
GitHub API. Papers are marked **full text**, **abstract only** or
**secondhand** (a summary by a fetch tool or a helper, not read by us
line by line). Forum posts marked **unverified** could not be opened
(Wetpixel refused connections, Reddit, DPReview, scubadiving.com and the
Blackmagic forum returned 403 or were blocked); only search snippets were
seen. Earlier surveys: [research.md](research.md) ("Tools photographers
use", "Open-source implementations") and
[transmission-research.md](transmission-research.md).

## 1. Short answer

Nobody we found fades the correction by distance the way we do. The
approaches that handle green murk fall in three groups:

- **Remove the water's own colour by a local division, independent of
  distance** (Lin et al. 2024; Sea-thru's local illuminant; the global
  form is what dive-color-corrector and every manual white balance do).
  The far floor loses its cast like the near floor. Water then goes
  toward neutral grey, not violet, because the divisor *is* the water's
  colour.
- **Keep water looking like water by a separate, soft decision on what
  is water**, not by distance: a feathered colour key (Berman's
  Mahalanobis water map with a guided filter; the Hue vs Hue key of
  Resolve colourists), or a manual mask with soft edges (Lightroom
  "Select Background").
- **Do not keep green water green**: experienced editors rotate the
  water's green and aqua towards blue and lower its saturation rather
  than keeping it or neutralising it; strobe shooters in green water set
  a cool white balance (Backscatter: 4500 K) so the background turns
  blue. Our kept veil keeps the green, which the branch found is where
  the far floor's green comes from.

Ranked recommendation in section 6.

## 2. Problem 1: correcting distant subjects in green murk

### 2.1 Divide by the local water colour (Lin, Sun, Ye 2024)

"Underwater image restoration via attenuated incident optical model and
background segmentation", Frontiers in Marine Science 11:1457190, 2024,
[doi:10.3389/fmars.2024.1457190](https://doi.org/10.3389/fmars.2024.1457190).
**Full text** (JATS XML from the publisher; equations checked by us).

    B(x)  = L(x) ω_c(x)                                   (Eq. 14)
    I_c   = L ω_c R_c t_c + L ω_c (1 - t_c)                (Eq. 15)
    per block Ω: max over Ω of R_c ≈ 1  =>  M_c = max_Ω I_c ≈ L ω_c
    L_Ω   = max_c M_c,   ω_c = M_c / L_Ω, refined by a guided filter  (Eq. 18)
    I'_c  = I_c / ω_c = L R_c t_c + L (1 - t_c)            (Eq. 19)

After Eq. 19 the veil is **colourless** (L(1 - t)), so the cast is gone
at every distance, and t is estimated afterwards on a cast-free image.
The foreground and the open water are then separated by a **binary**
mask (gradient, channel difference, regions under 5 % dropped,
morphology) and joined with a bilateral filter: the same coarseness we
hit, so take the ω_c idea and not the mask. 0.447 s in MATLAB, image
size not stated. No code.

For us: our water map (the local mean of open water) is already an
estimate of B(x) = L ω_c. Dividing the subject by the water map's
chromaticity (ω_c normalised to keep luma) is a local white balance
**by the water's colour**, not by the subject's, so it cannot push water
past neutral into violet; it fits single image, classical, fast, pixels
only. What it cannot do: tell strobe-lit red from ambient-lit subject, so
the ambient weight (R/G) must still gate it. Our inference, not tested.

### 2.2 Keep the water, but not green (community practice)

The consistent manual recipe for green water (all read unless marked):

- Tint towards magenta "until roughly right", Colour Mixer on
  Green/Aqua/Blue, separate soft-edged masks for water and subject, a
  small dip of the green tone curve: "You're not trying to turn the water
  into a perfect shade of blue. You're trying to make the remaining green
  less distracting."
  ([ajust Presets](https://www.ajustpresets.com/en-us/blogs/underwater/how-to-fix-green-underwater-photos-in-lightroom))
- Select Background, tint to magenta **and desaturate** the background;
  "overcorrecting can make the water look purple or brown."
  ([Robert Herb, 2025](https://robertherb.blogspot.com/2025/04/underwater-water-color-correction.html))
- Desaturate Aqua, since it "straddles the line between green and blue";
  dehaze sparingly.
  ([Trent Ogilvie](https://www.trentogilvie.com/blog/how-to-edit-underwater-photos))
- "Ban the cyan": shift the Aqua hue towards blue and lower its
  saturation; lower Green saturation if the floor looks green.
  ([scubadiving.com](https://www.scubadiving.com/imaging-tutorial-how-adjust-water-color), **unverified**, snippet)
- Channel Mixer: green output reduced about 12 %, then a hue layer.
  ([Wetpixel](https://wetpixel.com/articles/using-the-channel-mixer-to-correct-green-water), **unverified**, snippet)
- Picking white balance on a neutral "too far in the background" gives
  "too much of a red tint".
  ([DivePhotoGuide](https://www.divephotoguide.com/underwater-photography-techniques/article/white-balance-editing-underwater-photos/))
- Adobe's own underwater tutorial ends with a Colour Mixer hue shift to
  pull a purple background back to blue, the standard symptom of over
  correction. ([Adobe](https://www.adobe.com/au/learn/lightroom-cc/web/underwater-photo-editing))
- With strobes: "Choose the 4500K diffuser in green water to make
  backgrounds blue. Set your camera White Balance manually to 4500K to
  turn green water to blue."
  ([Backscatter Hybrid Flash review](https://www.backscatter.com/reviews/post/Backscatter-Hybrid-Flash-Underwater-Strobe-Video-Light-Review));
  the Magic Filters GreenWater filter "does tend to remove some green
  from the water too, producing more blue-ish water colours"
  ([Mike's Dive Store](https://www.mikesdivestore.com/pages/more-info-on-the-green-water-magic-filter)).

For us: the kept veil `A' * veil` (step 8) keeps the water's hue at
`keep-water` chroma. In green water the recipe says: rotate that hue
towards teal or blue and lower its chroma, in Oklab, by an amount that
grows with the water's greenness. Since the branch found the far floor's
green comes from the kept veil, this acts exactly there, with no mask and
no dependence on t. Pixels only, trivial cost. What it gets wrong: water
that really is green comes out blue-grey, which is what editors choose,
but it should be a control (or folded into `keep-water`).

### 2.3 Attenuation ratios per water type, chosen on the subject (Berman et al.)

Haze-lines underwater, Berman, Levy, Avidan, Treibitz, TPAMI 43(8) 2021,
[doi:10.1109/TPAMI.2020.2977624](https://doi.org/10.1109/TPAMI.2020.2977624);
code [danaberman/underwater-hl](https://github.com/danaberman/underwater-hl)
(read by us: `uw_restoration.m`, `uw_restoration_type.m`).

    t_R = t_B^(β_R/β_B),  t_G = t_B^(β_G/β_B),   J_c = A_c + (I_c - A_c)/max(t_c, 0.1)
    for each of 10 Jerlov types (β_B/β_G, β_B/β_R pairs, open ocean to coastal green):
        restore, then score = std of the three channel means over NON-WATER pixels (gray world)
        drop types where mean t on objects is not at least 0.1 above mean t in water
    keep the type with the smallest score

The strength of the red and green restoration follows t_B through fixed
ratios: it **grows** with distance for subjects, it does not fade. Water
is protected not by distance but by the soft water map (2.4) and by
choosing the type on non-water pixels only. The green coastal types
(β_B/β_G above 1) are in the list, so green water is a case of the same
formula.

- Licence: custom non-commercial (Haifa and Tel Aviv universities), not
  GPL compatible: method from the paper, no code.
- Patent: US 11,810,272 B2 claims 1 and 2 need haze-line clustering (see
  transmission-research.md). The per-type ratios, the gray-world choice
  and the water map are not the clustering; our reading, not legal
  advice.
- For us: the type choice replaces our fixed red-restore strength; its
  gray-world score on non-water pixels is cheap on the 512 px copy. What
  it gets wrong: needs a transmission we trust for subjects (our bright
  rock problem), and the searched ratios are for clean Jerlov types, not
  turbid harbour water.

### 2.4 A soft "is this water" weight instead of distance

Three concrete soft weights, from most to least useful for us:

**Mahalanobis water map** (Berman, `uw_restoration_type.m`, read):

    d(x)     = Mahalanobis distance of I(x) to the colours of the textureless water pixels
    T_water  = mean(d_self) + 2 std(d_self),   T_not = max(d_self) + std(d_self)
    water(x) = 1 - clamp((d(x) - T_water)/(T_not - T_water), 0, 1)
    t        = t_lowerbound * water + (1 - water) * t_est
    then a guided filter, radius 30, eps 1e-3, guide gamma 1/2.2

The covariance of the water samples widens "water" along the direction
the water colour actually varies (lighter and greener up, darker and
bluer down), so varying open water stays water; the linear ramp between
two thresholds and the guided filter give a soft border instead of a
hard mask. This is the closest published answer to "a soft, continuous
measure" that the branch notes ask for. Against our B(x) surface it
would be the distance of I(x) to B(x) scaled by the residual covariance
of the fit. Pixels only, O(1) per pixel. It says "water-coloured", not
"water in front", so a water-coloured rock is still water (the known
ambiguity).

**Darkness weight** (Wang, Sun, Ren, "Underwater Color Disparities",
TCSVT 34, 2024, [doi:10.1109/TCSVT.2023.3289566](https://doi.org/10.1109/TCSVT.2023.3289566),
**abstract only**; code [gitee wanghaoupc/Underwater_Color_Disparities](https://gitee.com/wanghaoupc/Underwater_Color_Disparities),
MIT, `src/tool.py` read by us):

    A(x) = max_c (1 - I_c^γ),  γ = 1.2
    out  = highpass(I) + A * corrected + (1 - A) * I

Dark, absorbed pixels get the full correction. Also in the code: red
compensation `R += a (1 - R)(mean G - mean R) G` iterated while the R/G
histogram correlation improves towards 0.99, and not beyond when mean R
exceeds mean G (the means taken once, before the loop). The Lab target
window (a* in 126..134, b* in 128..140 of 255) sits in a loop that
`break`s at once in the published code, so it does nothing there.

**Red to blue weight** (US 12,373,929 B2, Insta360, from the USPTO PDF,
read by a helper by OCR, **secondhand**): `w = (r' + a) B / (b' + a)`,
a = 0.00015, B = 3, clamped to 0.1 to 0.8, then `out = corrected * w +
original * (1 - w)`; the patent says the blue and green of the background
water are kept. **Claim 1 of this patent includes per-pixel weights and
fusion of the original and adjusted values**; design.md already rules out
that combination. Any soft gate we add should gate a correction term
inside our model, not blend a corrected image with the original, and
must be checked against the claims first. A field test of the shipped
product: in turbid green water "green becomes an artificial cyan, and
skin tones take on a mauve tint"
([aquaexposure](https://www.aquaexposure.com/en/blog/insta360-ace-pro-2-underwater-ai-testing-and-limitations)).

### 2.5 Local chroma mean subtraction

Ancuti et al. 3C (TIP 29, 2020, in research.md) subtracts a large
Gaussian local mean of a* and b*. A 2025 variant, Qiu, Liu, Wang, Wan,
npj Heritage Science 13:319,
[doi:10.1038/s40494-025-01899-1](https://doi.org/10.1038/s40494-025-01899-1)
(**secondhand**, full text read by a helper), makes it grow with
distance: `a* ← a* - κ E(x) G*a*`, `E = 1/(1 + exp(-k (D - D_thresh)))`.
The strength grows with distance, the opposite of ours; its background
light partitions are hard intervals (avoid). No code.

### 2.6 Products

Only those with a concrete method for problem 1:

- **Camera and filter makers** switch between green and blue presets:
  SeaLife Micro 3.0 "Underwater Green" and "Underwater Deep" white
  balance ([SeaLife](https://www.sealife-cameras.com/product/micro-3-0-underwater-camera/));
  Paralenz uses a 3x3 matrix per depth from a pressure sensor with
  separate Green and Blue profiles (US 2019/0306481,
  [FPO](https://www.freepatentsonline.com/y2019/0306481.html)), and
  reviewers say colours "become washed out and blue" beyond about 1 m
  from the subject ([UWPG](https://www.uwphotographyguide.com/paralenz-dive-camera-review/)).
  All global per frame; none handles distance.
- **dive-color-corrector** (section 5) is global too, and its green
  results show what a distance-free correction does.
- Dive+, Luminar, Capture One, DxO, Affinity, GoPro Quik, Final Cut
  plug-ins: nothing documented about green water or distance; Dive+ has
  no documented green mode (**unverified**). Not searched further after
  the scope was narrowed.

## 3. Problem 2: open water whose colour varies

### 3.1 A local veil colour layer (Lin 2024, section 2.1)

`ω_c(x) = M_c / L` from block maxima, guided-filtered, is a local water
colour map. It is biased like every "brightest pixel" airlight (a bright
subject in the block becomes the airlight), so for us it is a second
opinion next to the fitted B(x), not a replacement.

### 3.2 Smooth blend of a global and a local veil

Zhu, Zhang, "Adaptive Atmospheric Light Estimation for Dehazing via a
Novel Decoupled Scattering Model with Neutral-Pixel and Visual-Depth
Priors", J. Imaging 12(5):218, 2026,
[doi:10.3390/jimaging12050218](https://doi.org/10.3390/jimaging12050218)
(**secondhand**; the DOI resolves, MDPI returned 403 to us):

    I = t J H + V(x)(1 - t) H          (H global chromaticity, V intensity)
    V(x) = (1 - w) V_g + w V_l(x),   w(x) = exp(-V_l(x)/T)
    V_l: local maximum (radius 15) then guided filter; T by Otsu

On land, intensity only; 0.51 s at 900x900. The idea for us: blend a
global A and the local B(x) by a smooth weight rather than a region, and
do it per channel (colour, not only intensity).

### 3.3 Hue-keyed correction with a feathered range (video colourists)

In DaVinci Resolve, water is corrected with **Hue vs Hue**: eyedrop the
water, spread the outer points to feather the range, move the centre
towards blue if the water looks purple, then Hue vs Sat on the same
range ([Ikelite, Resolve grading](https://www.ikelite.com/blogs/advanced-techniques/r3d-underwater-color-grading-in-davinci-resolve-nikon-zr-video)).
Qualifiers and tracked windows are the other route (Blackmagic forum,
**unverified**). A hue key follows the water's colour wherever it is,
with no spatial border, which is why it does not ring; it fails where a
subject has the water's hue. For us: the kept veil's hue rotation (2.2)
can be weighted by a feathered hue distance to B(x), which is the
colourists' key made continuous.

### 3.4 Gradients from surface to depth

Manual answers for varying water are gradients: "try a gradient across
and use the gradient's color temperatures to try to get top to bottom
subject similarity"
([Lightroom Queen forum](https://www.lightroomqueen.com/community/threads/underwater-color-correction.41009/));
commercial adaptive presets combine subject masks with gradients that
"simulate light from the surface and darkness from the depths"
([Brittany Ilardi presets](https://brittanyilardiphoto.com/products/adaptive-ai-powered-underwater-presets-50-presets)).
Our quadratic B(x) on the branch is the fitted version of this.

### 3.5 Patterns to avoid

Hard masks or partitions for water: Lin 2024 and Men et al. 2025
(Optics & Laser Technology 113565,
[doi:10.1016/j.optlastec.2025.113565](https://doi.org/10.1016/j.optlastec.2025.113565),
**abstract only**) segment foreground and background with a binary
mask; Qiu 2025 uses hard brightness intervals; hainh/sea-thru copies
the original back into the zero-depth region
(`res[nmap==0] = img[nmap==0]`). These are the blotches we saw. Halos at
the subject border: no underwater-specific recipe was found in any
readable source beyond "soften the mask edges"; the guided filter on the
water map (2.4) is the only concrete technique.

## 4. Open source: what exists (narrowed)

| Project | Licence (checked) | Relevant to our problems |
|---|---|---|
| [bornfree/dive-color-corrector](https://github.com/bornfree/dive-color-corrector) (153 stars, last commit 2025-11-26) | GPL-3.0 (LICENSE file and API) | global; see section 5 |
| [danaberman/underwater-hl](https://github.com/danaberman/underwater-hl) | custom non-commercial | soft water map, water types (2.3, 2.4) |
| [gitee wanghaoupc/Underwater_Color_Disparities](https://gitee.com/wanghaoupc/Underwater_Color_Disparities) | MIT | darkness weight, stop rule (2.4) |
| [hainh/sea-thru](https://github.com/hainh/sea-thru) | MIT | local illuminant `J = (I - B)/illum` needs depth regions |
| [bilityniu/underimage-fusion-enhancement](https://github.com/bilityniu/underimage-fusion-enhancement) | none | Ancuti red compensation on a 5 px local window; blue compensation for murky water commented out |
| [MinjieWan/C3HLM](https://github.com/MinjieWan/C3HLM) (TGRS 2024) | MIT (helper) | global rank-order compensation onto the dominant channel; haze-lines part (patent check needed) |
| darktable, RawTherapee, ART, G'MIC, OpenCV, ffmpeg | | nothing underwater-specific: darktable haze removal uses a global A; RawTherapee has two "UnderWater" white balance presets; ffmpeg `grayworld` cites Bianco 2015 |

darktable's Sea-thru request was closed as not planned
([#3434](https://github.com/darktable-org/darktable/issues/3434)).
MLLE, ACDC, ICSP, PCDE, HFM, CBLA and the 2025 FBS-UOI ship their cores
as MATLAB P-code.

## 5. Comparison run: dive-color-corrector

**What it does** (`correct.py` read; commit ad80dcc, 2025-11-26): on a
256x256 copy, find the hue rotation that lifts mean red to 60/255; the
new red is the hue-rotated mix of R, G and B (blue weighted 1.2); then
per channel a level stretch between the ends of the largest gap between
"sparse" histogram bins (count under N/2000). One global 4x5 matrix:
only the red row mixes channels, G and B are gain plus **offset**. The
negative offsets are a global black point per channel, which removes a
constant veil everywhere, whatever the distance.

**Run**: 42 photos, Python venv in `/tmp/claude-1000/uw-survey/`
(`opencv-python-headless` 5.0.0 instead of `opencv-python`, same API),
the command-line `image` mode, 13 s for all. Results in
`tests/output/dcc/`, sheets `tests/output/compare-0.jpg` to
`compare-5.jpg` (original, cur, dcc). `tests/output/cur` was from
17:49 today; a fresh `tests/run.sh cur-check` with the present build
differs from it by about 1/255 on average (isolated pixels more), so cur
stands for the current code.

Mean Oklab of the top and bottom of each photo (L, a, b; negative a is
green, negative b is blue), for the green photos:

| Photo | Region | Original | Ours (cur) | dcc |
|---|---|---|---|---|
| ambient-green-04 (turtle) | bottom | 0.61 -0.117 +0.027 | 0.62 -0.027 +0.050 | 0.76 -0.101 +0.040 |
| ambient-green-05 (fins) | bottom | 0.56 -0.105 +0.043 | 0.55 -0.021 +0.041 | 0.58 -0.071 +0.019 |
| ambient-green-06 (murk) | bottom | 0.48 -0.101 +0.036 | 0.47 -0.015 +0.021 | 0.65 +0.002 +0.063 |
| ambient-green-07 (whale shark) | top | 0.58 -0.082 -0.052 | 0.49 -0.033 -0.029 | 0.71 -0.093 -0.045 |
| ambient-green-08 (green reef) | top | 0.72 -0.174 +0.104 | 0.65 -0.081 +0.050 | 0.80 -0.048 +0.033 |
| ambient-green-08 | bottom | 0.41 -0.107 +0.080 | 0.45 -0.005 +0.011 | 0.47 -0.007 +0.089 |
| ambient-green-10 (box) | bottom | 0.32 -0.073 +0.038 | 0.30 -0.002 +0.005 | 0.38 -0.004 -0.021 |

What the sheets show (looked at by eye, at 300 and 560 px wide):

**Where dcc does better than ours**

- **ambient-green-08**: the water above the reef loses its green (a*
  -0.048 against our -0.081), and there is no cyan ring around the torch
  and no grey band along the reef edge. Our result has the known flaw:
  grey reef, bright green water, a hard change between them.
- **ambient-green-06, -10** (uniform murk): the green goes at every
  distance, the far background included; ours leaves a grey-green
  (06) or an olive-brown floor and a green box (10).
- **ambient-green-07** (whale shark): the water stays a clear cyan blue;
  ours turns it a violet grey.
- **Blue open water** (ambient-blue-01 to -05, -07): water stays blue and
  bright. Ours darkens it (L down 0.05 to 0.1, the known issue) and
  gives it a violet or indigo tint (blue-02, -04), and the sunburst in
  ambient-blue-05 becomes a flat white disc in ours, a soft sunburst in
  dcc.

**Where dcc does worse**

- **ambient-green-04, -05** (teal water, sand and weed floor): almost no
  correction; the floor stays green (a* -0.10, -0.07). Ours makes the
  floor tan and natural, clearly better.
- **ambient-green-08** reef: turns uniformly orange-yellow (b* +0.089)
  and the torch magenta; ours is grey. Neither is right.
- **ambient-green-03, -06**: the murk turns cream or tan, too warm and
  washed; ours is more neutral.
- **Blue reef scenes** (ambient-blue-02, -03): the foreground reef gets
  much less warmth than ours; ours restores the reef colours better.
- Water in ambient-blue-01 and -07 goes pale, nearly white cyan; the
  stretch flattens contrast in bright scenes.

**What it teaches for our two problems**: a correction that does not fade
with distance does remove the green from far murk (06, 08 top, 10), and
it does not make water violet, because it never raises blue relative to
green and adds red as a mix of G and B; it fails by overshooting to
orange or cream, because it is one global gain for water and subject
alike. That is the argument for 2.1 (a local, distance-free division by
the water colour) plus 2.2 (decide the water's final colour
separately), rather than for fading.

## 6. Ranked: most likely to fix our two problems

1. **Rotate and desaturate the kept veil in green water** (2.2). The
   branch found the far floor's green comes from the kept veil; every
   manual recipe and the green-water strobe practice move green water
   towards blue and lower its saturation instead of keeping it. Oklab hue
   rotation of A' by greenness; no mask, no t, microseconds. Problem 1.
2. **Remove the cast by the local water colour, independent of t** (2.1,
   Lin 2024 Eq. 19, with our water map or B(x) as ω_c). Far subjects lose
   the cast like near ones; water goes to grey, not violet; the kept veil
   (item 1) then decides the water's colour. Problems 1 and 2. Needs the
   ambient weight to spare strobe-lit subjects.
3. **A soft water weight from colour distance to B(x) with the fit's
   covariance and a guided filter** (2.4, Berman's water map). Replaces
   the hard mask as the gate for anything that must spare water; the
   feathered ramp and the guided filter are what avoid rings and
   blotches. Problems 1 and 2.
4. **Smooth global to local blend of the veil** (3.2) and a feathered hue
   key (3.3) for varying open water. Problem 2. Secondhand source for
   3.2.
5. **Attenuation ratios per water type chosen by gray world on
   non-water pixels** (2.3): correction that grows with distance for
   subjects. Larger change; depends on a trustworthy t for subjects.
6. **Guards**: stop red compensation once mean R reaches mean G (UCD);
   test for purple or brown water after over correction, cyan water and
   mauve skin in turbid water (reported failures of Lightroom edits and
   Insta360).

Not recommended: per-pixel blending of a corrected image with the
original by a red/blue or darkness weight (Insta360 claim 1 territory;
design.md), hard foreground/background masks or partitions (3.5), global
histogram stretches (dcc's orange and cream overshoot).

## 7. Not verified or not reached

- Zhu & Zhang 2026, Qiu 2025, the Insta360 weight formula: secondhand.
- Men et al. 2025, ICSP (TCSVT 2023,
  [doi:10.1109/TCSVT.2023.3290363](https://doi.org/10.1109/TCSVT.2023.3290363)),
  TSF (TCSVT 2024, [doi:10.1109/TCSVT.2024.3508102](https://doi.org/10.1109/TCSVT.2024.3508102)),
  turbid Retinex (TCSVT 2025, [doi:10.1109/TCSVT.2025.3575846](https://doi.org/10.1109/TCSVT.2025.3575846)),
  PBPE (IEEE JOE 50(3) 2025, [doi:10.1109/JOE.2025.3555684](https://doi.org/10.1109/JOE.2025.3555684)),
  SALST (OLT 2025, [doi:10.1016/j.optlastec.2025.113195](https://doi.org/10.1016/j.optlastec.2025.113195)),
  UIEAR (IEEE TCI 2025, [doi:10.1109/TCI.2025.3544065](https://doi.org/10.1109/TCI.2025.3544065)):
  abstract only. PBPE's "pixelwise transmission from the mapping between
  transmission and backscattered light" may be a continuous "water in
  front" measure; worth the full text.
- Wetpixel, Reddit, DPReview, scubadiving.com, Blackmagic forum: not
  readable; snippets only. No text by Alex Mustard, Martin Edge or
  Brent Durand on editing green water was found in readable form.
- Products not searched after narrowing: GoPro Quik, Capture One, DxO,
  Affinity, ON1, Final Cut plug-ins, LUT packs, most mobile apps.
- Patents: none searched for the new papers; C3HLM's haze-lines part
  against US 11,810,272 not checked.

## 8. Sources

Papers and code: see the links in each section. Community and products:

- https://www.ajustpresets.com/en-us/blogs/underwater/how-to-fix-green-underwater-photos-in-lightroom
- https://robertherb.blogspot.com/2025/04/underwater-water-color-correction.html
- https://www.trentogilvie.com/blog/how-to-edit-underwater-photos
- https://www.goaskerin.com/tutorials/dial-a-blue-adjusting-the-color-of-water-using-adobe-lightroom/
- https://www.divephotoguide.com/underwater-photography-techniques/article/white-balance-editing-underwater-photos/
- https://www.adobe.com/au/learn/lightroom-cc/web/underwater-photo-editing
- https://www.lightroomqueen.com/community/threads/underwater-color-correction.41009/
- https://creativecow.net/forums/thread/underwater-colour-grade/
- https://www.ikelite.com/blogs/advanced-techniques/r3d-underwater-color-grading-in-davinci-resolve-nikon-zr-video
- https://www.ikelite.com/blogs/faq/using-lightroom-landscape-masking-for-underwater-photos-video
- https://discuss.pixls.us/t/white-balance-on-underwater-images/30463
- https://scubaboard.com/community/threads/color-correction-filter-vs-software.551572/
- https://scubaboard.com/community/threads/getting-the-green-out-of-a-gopro.549906/
- https://scubaboard.com/community/threads/diving-in-kelp-red-or-magenta-filter.631435/
- https://www.uwphotographyguide.com/when-to-use-gopro-filters-underwater/
- https://www.naturettl.com/colour-photography-underwater/
- https://dan.org/alert-diver/article/seeing-green/
- https://www.backscatter.com/reviews/post/Backscatter-Hybrid-Flash-Underwater-Strobe-Video-Light-Review
- https://keldanlights.com/products/filters/filters-for-video-lights/filters-for-video-lights.html
- https://www.mikesdivestore.com/pages/more-info-on-the-green-water-magic-filter
- https://www.aquaexposure.com/en/blog/insta360-ace-pro-2-underwater-ai-testing-and-limitations
- https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/12373929
- https://www.freepatentsonline.com/y2019/0306481.html
- https://www.uwphotographyguide.com/paralenz-dive-camera-review/
- https://www.sealife-cameras.com/product/micro-3-0-underwater-camera/
- https://github.com/bornfree/dive-color-corrector/issues/3
