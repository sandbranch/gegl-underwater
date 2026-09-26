# Research notes

What exists for correcting underwater photos, and what the literature
offers for a single-photo, classical, fast filter. Collected September
2026. Citations were checked against publisher or Crossref metadata,
licenses against the repositories (GitHub API), patents on Google Patents.
Cost figures for 24 megapixels are estimates from operation counts, not
benchmarks.

## Tools photographers use

- **Lightroom / Photoshop**: manual work. White balance (often picked on
  a strobe-lit neutral in the foreground), per-channel levels, HSL, masks
  for subject and background with different temperatures, Dehaze against
  the veil. General plugins (Nik Color Efex, Retouch4me) are not
  underwater-specific.
  [UW Photography Guide](https://www.uwphotographyguide.com/articles/post-processing-articles/),
  [DivePhotoGuide on white balance](https://www.divephotoguide.com/underwater-photography-techniques/article/white-balance-editing-underwater-photos/)
- **Dive+** (mobile, with Weefine housings): one-tap, uses depth
  information, closed source.
- **darktable**: no dedicated module; white balance, color calibration and
  channel mixer by hand. [Feature request](https://redmine.darktable.org/issues/12198),
  [discussion of why white balance alone fails](https://discuss.pixls.us/t/white-balance-on-underwater-images/30463).
- **GIMP 2 scripts**: "Under Water red correction" (Script-Fu, 2006, the
  "Mandrake method" of red layers and blend modes) and
  [UnderwaterCorrection.py](https://gist.github.com/edouardklein/6fef6a268c8117a2b7ba)
  (2014, Python 2, stretches red to its 99.9th percentile). Neither runs in
  GIMP 3.
- **G'MIC** (in GIMP 3): no underwater filter; "DCP Dehaze" and
  "Simple Dehaze" are for haze on land.

What photographers know (e.g. Alex Mustard in
[DivePhotoGuide](https://www.divephotoguide.com/underwater-photography-techniques/article/balancing-ambient-strobe-light-underwater-photography)):
a photo is **two layers**, a strobe-lit foreground that is already about
daylight-balanced, and an ambient-lit background that has lost its red.
Correcting both the same way turns strobe-lit subjects red. Red is mostly
gone by about 5 m; color loss grows with the horizontal distance too, and
strobe light travels there and back ([DAN](https://dan.org/alert-diver/article/recovering-color-in-underwater-photography/)).

## Open-source implementations

| Project | License | What it does | For us |
|---|---|---|---|
| [bornfree/dive-color-corrector](https://github.com/bornfree/dive-color-corrector) | GPL-3.0 | global color matrix (red from green/blue) and level stretch | code usable with credit |
| [nikolajbech/underwater-image-color-correction](https://github.com/nikolajbech/underwater-image-color-correction) | none | the same idea in JavaScript | ideas only |
| [wangyanckxx/Single-Underwater-...](https://github.com/wangyanckxx/Single-Underwater-Image-Enhancement-and-Color-Restoration) | none | Python versions of many classical methods | ideas only |
| [Li-Chongyi/MMLE_code](https://github.com/Li-Chongyi/MMLE_code) (MLLE) | MIT file, but the algorithm is obfuscated MATLAB P-code; README says academic use | | paper only |
| [aire39/underwater-image-enchancement](https://github.com/aire39/underwater-image-enchancement) | MIT | third-party C++ MLLE, not checked against the paper | reference |
| [danaberman/underwater-hl](https://github.com/danaberman/underwater-hl) (haze-lines) | non-commercial, patents excluded | | paper only |
| Sea-thru ports: hainh/sea-thru (MIT), Teragion/Sea-Thru-Impl (GPL-3.0) | | need a depth map | ideas |
| [CXH-Research/Underwater-Image-Enhancement](https://github.com/CXH-Research/Underwater-Image-Enhancement) | MIT | list of published methods with code | index |

Most of the recent fast classical methods (MLLE, ACDC, WWPF, ICSP,
BayesRet) publish their core as MATLAB P-code, so they have to be
implemented from the papers.

## Physics

- **Akkaynak & Treibitz, "A Revised Underwater Image Formation Model",
  CVPR 2018**, [doi:10.1109/CVPR.2018.00703](https://doi.org/10.1109/CVPR.2018.00703).
  The direct signal and the backscatter have **different** coefficients:
  `I = J·exp(-β^D·z) + B^∞·(1 - exp(-β^B·z))`. β^B hardly changes with
  distance for a water type; β^D falls with distance. Methods from
  atmospheric dehazing that use one transmission for both are wrong in
  principle; calibrating on something near the camera over-corrects the
  rest ("a bad habit"); a mid-distance, moderate correction is best when
  only one is possible. **The main lesson for us: backscatter removal and
  color restoration are separate operations with separate controls.**
- Akkaynak et al., "What is the Space of Attenuation Coefficients in
  Underwater Computer Vision?", CVPR 2017,
  [doi:10.1109/CVPR.2017.68](https://doi.org/10.1109/CVPR.2017.68):
  plausible RGB attenuation coefficients lie on a small manifold, useful
  for bounding a water color.
- **Sea-thru**, Akkaynak & Treibitz, CVPR 2019,
  [doi:10.1109/CVPR.2019.00178](https://doi.org/10.1109/CVPR.2019.00178):
  needs a range map. Usable without depth: backscatter from the darkest
  RGB triplets (done once, globally), and a local average color as the
  illuminant (Ebner's LSAC without the range test).
- **Haze-lines**, Berman, Levy, Avidan & Treibitz, TPAMI 43(8), 2021,
  [doi:10.1109/TPAMI.2020.2977624](https://doi.org/10.1109/TPAMI.2020.2977624):
  veiling light from a water region; attenuation ratios β_B/β_R, β_B/β_G
  per Jerlov water type reduce restoration to one transmission; restore
  `J_c = A_c + (I_c - A_c) / t_B^(β_c/β_B)`; try each water type and keep
  the one that best satisfies gray-world outside the water. Works on
  linear data. The SQUID dataset (57 stereo pairs, color charts, distance
  maps) is the only one here with physical ground truth.

## Classical single-image methods

- **UDCP**, Drews et al., ICCV Workshops 2013,
  [doi:10.1109/ICCVW.2013.113](https://doi.org/10.1109/ICCVW.2013.113);
  CG&A 2016, [doi:10.1109/MCG.2016.26](https://doi.org/10.1109/MCG.2016.26):
  the dark channel on green and blue only. Cheap transmission cue; weak in
  turbid water.
- **Red Channel prior**, Galdran et al., JVCIR 26, 2015,
  [doi:10.1016/j.jvcir.2014.11.006](https://doi.org/10.1016/j.jvcir.2014.11.006):
  dark channel of (1 - R, G, B); a **saturation cue for artificially lit
  areas**; `t_G = t_R^λ_G` relations; an additive term that removes the
  cast.
- **IBLA**, Peng & Cosman, TIP 26(4), 2017,
  [doi:10.1109/TIP.2017.2663846](https://doi.org/10.1109/TIP.2017.2663846):
  depth from blurriness and light absorption. Slow (146 s for 1280x720 in
  MATLAB), and patented (below). Not used.
- **WCID**, Chiang & Chen, TIP 21(4), 2012,
  [doi:10.1109/TIP.2011.2179666](https://doi.org/10.1109/TIP.2011.2179666):
  detects and removes artificial light; separates attenuation from the
  surface (a global cast) from attenuation along the view.
- **Fusion**, Ancuti et al., CVPR 2012,
  [doi:10.1109/CVPR.2012.6247661](https://doi.org/10.1109/CVPR.2012.6247661):
  white-balanced and contrast-enhanced versions blended by weight maps.
- **Color balance and fusion**, Ancuti et al., TIP 27(1), 2018,
  [doi:10.1109/TIP.2017.2759252](https://doi.org/10.1109/TIP.2017.2759252).
  Red compensation as published (Eq. 4), with channels normalized to
  [0, 1]: `I_rc = I_r + α·(mean(I_g) − mean(I_r))·(1 − I_r)·I_g`,
  "a value of α = 1 is appropriate"; Eq. 5 the same for blue in turbid
  water. Then gray-world, then fusion. (A widely copied MATLAB version
  drops the `(1 − I_r)·I_g` factor.)
- **3C, Color Channel Compensation**, Ancuti et al., TIP 29, 2020,
  [doi:10.1109/TIP.2019.2951304](https://doi.org/10.1109/TIP.2019.2951304)
  (open access): in CIELAB, subtract a large Gaussian local mean from a*
  and b*, masked near light sources. Local, handles green water too; the
  2018 compensation is nearly a special case. Notes that a red channel
  missing completely turns yellowish with the 2018 formula.
- **MLLE**, Zhang et al., TIP 31, 2022,
  [doi:10.1109/TIP.2022.3177129](https://doi.org/10.1109/TIP.2022.3177129):
  locally adaptive color correction guided by a maximum-attenuation map,
  local contrast on L from integral images (O(1) per pixel for any
  window), balanced a*/b* means. Under 1 s for 1024x1024 in MATLAB.
- **ACDC**, Zhang et al., IEEE JOE 47(3), 2022: compensation weighted
  per pixel by attenuation, so pixels that have red get less.
- **WWPF**, Zhang et al., TCSVT 2023,
  [doi:10.1109/TCSVT.2023.3299314](https://doi.org/10.1109/TCSVT.2023.3299314):
  global and local contrast versions fused by wavelet band.
- **ROP, rank-one prior**, Liu et al., TPAMI 45(7), 2023,
  [arXiv:2103.17126](https://arxiv.org/abs/2103.17126): a transmission map
  from projecting each pixel on the image's mean color direction, smoothed
  by down/up-sampling; 0.8 s for 1080p on CPU. A second backscatter
  estimator.
- **RGHS**, Huang et al., MMM 2018,
  [doi:10.1007/978-3-319-73603-7_37](https://doi.org/10.1007/978-3-319-73603-7_37):
  histogram stretches with percentile limits; L stretched between 1% and
  99%; a*/b* S-curve `a·1.3^(1 - |a|/128)`. A related Chinese patent,
  CN107067386A (status not checked).
- **Retinex family**: Fu et al., ICIP 2014 (per-channel mean ± μσ
  stretch, then retinex on V), LAB-MSR (Zhang et al. 2017), BayesRet
  (Zhuang et al. 2021), HLRP (Zhuang et al., TIP 2022). The variational
  ones are too slow or iterative for a live preview at 24 MP.
- **Local gray world in lαβ with integral images**, Bianco et al. 2015,
  Neumann et al. 2018: the cheapest spatially varying white balance.
- **Histogram methods** (ICM, UCM, Rayleigh stretching, CLAHE): tend to
  over- or under-saturate (Wang et al. 2019); CLAHE only on L if at all.
  GEGL has no CLAHE operation.

## Noise and particles

- **Noise from restoring red.** Amplifying a weak red channel amplifies
  its noise; several papers address it, e.g. with wavelet decomposition
  so that noise in the high frequencies is not amplified
  ([Enhancing Underwater Image via Adaptive Color and Contrast
  Enhancement, and Denoising](https://arxiv.org/pdf/2104.01073);
  [adaptive color correction and stationary wavelet detail
  enhancement](https://ieeexplore.ieee.org/document/10399776/)). Our own
  wavelet denoise (gegl-wavelet, `wavelet:denoise`, CIELAB mode with
  stronger a*/b* thresholds) fits this directly.
- **Marine snow**, the bright specks of particles lit by the strobe:
  Farhadifard et al., "Single Image Marine Snow Removal based on a
  Supervised Median Filtering Scheme" (2017,
  [Semantic Scholar](https://www.semanticscholar.org/paper/Single-Image-Marine-Snow-Removal-based-on-a-Median-Farhadifard-Radolko/ae28a23a3a3df5cc5ce23fd8c03de08721e642e0)):
  detect the specks, then median-filter only them, so the rest stays
  sharp. Median filtering works when the specks are small (about 1-3
  pixels); for large ones a big filter blurs the image, which is where
  learned methods come in (e.g. a 3D network with an adaptive median
  filter, ICPR workshops 2018,
  [Springer](https://link.springer.com/chapter/10.1007/978-3-030-05792-3_2)).

  The detection rule, read from the paper itself (Farhadifard,
  Radolko, Freiherr von Lukas, VISAPP 2017, pp. 280-287,
  DOI 10.5220/0006261802800287). For each pixel p of a patch Ω, in RGB,
  a pixel is marine snow when all three hold:

  1. **Brighter than its patch:** `|p - mean(Ω)|^2 > W1 * σ(Ω)`.
  2. **Isolated (a density outlier):** few pixels v of the patch lie
     near p in color, `|p - v|^2 < W2 * σ(Ω)`; a real bright surface has
     many similar neighbors, a speck has few (the idea is from Gutzeit
     et al. 2010).
  3. **Nearly colorless:** `|p_c - p_l| < T` for every pair of channels,
     since specks lit by a strobe are close to white.

  A detected pixel gets the median of the patch's pixels that were not
  detected; the dark rim around a speck is left out of that median.
  Patches overlap, and a pixel is only replaced if it was marked in more
  than 80% of the n×n patches that contain it (a vote), with the median
  of its candidate values; otherwise it keeps its value. Several patch
  sizes are used, up to 19×19 for HD video frames.

  W1, W2 and T are set by hand; the paper gives no values. It says itself
  that reflections of the strobe (larger bright circles) are not handled.
  Evaluating every overlapping patch is costly, so we need a cheaper
  equivalent (e.g. means and variances from integral images, a single
  patch size tied to a "speck size" control).

  What `underwater:marine-snow` does instead (operations/marine-snow.c),
  and what the synthetic test showed on the way:

  1. **Bright and small**: a white top-hat of the luma (the image minus
     its grayscale opening with a square of the speck size) above a
     threshold. It replaces the brightness test and most of the patch
     machinery: whatever is brighter than its surroundings and smaller
     than the square is left in the top-hat.
  2. **Colorless light**: the color of the light the speck adds (the
     pixel minus the opening of each channel) must be nearly neutral.
     Testing the color of the pixel itself, as a first version did,
     misses specks over blue water, which look blue-white: marine snow
     is strobe light added on top of what is behind it.
  3. **Isolated** (their density test): the number of spots (brightest
     pixels of each candidate) in a window of about 4 speck sizes must
     be low. Counting candidate pixels instead, as a first version did,
     takes a single large speck for texture. Grains in sand are many
     spots close together and are kept.
  4. **The whole speck**: detected pixels grow into neighbors that still
     stand out a little (hysteresis, 30 % of the threshold), then by one
     pixel, so the soft edge goes too; without it faint rings remained.
  5. **Replacement**: the median of the unmarked neighbors whose color is
     close to the opening there (what is behind the speck), or the
     opening itself when there are none. A plain median of all unmarked
     neighbors, as in the paper, erased a small dark eye under a speck.

  Synthetic scene (tests/marine-snow): 384 visible specks; with the
  default settings 9 remain visible, 0.03 % of the sand and 0.06 % of the
  fish change, a large white object is untouched.

  Patents: none found for the authors (University of Rostock,
  Fraunhofer IGD). The only marine snow patents found are Jack Wade's
  US 11,710,245 and its continuation US 12,217,439 (see Patents below),
  which are about video.

## Color constancy underwater

Gray-world, max-RGB/white patch, shades of gray (Finlayson & Trezzi 2004)
and gray-edge (van de Weijer et al. 2007) are all cheap. Underwater:
max-RGB, white patch and gray-edge leave the cast; gray-world
over-compensates red in the background (dividing by a tiny red mean
amplifies noise). Use them only after red compensation, on a mask of
non-water pixels, with clamped gains.

## Learned models

FUnIE-GAN (about 7 M parameters, MIT), Shallow-UWnet (about 219 k, MIT,
no weights published), FA+Net (about 9 k, no license), Zero-UAE (17,699,
no code). GEGL has no ML runtime, the small models were trained at about
256 px, and the weights are unlicensed or missing, so none is embedded.
The idea kept: estimate a few parameters on a small proxy image, apply
them at full size.

## Benchmarks and metrics

- Guo et al., TCSVT 2025, [arXiv:2501.02701](https://arxiv.org/abs/2501.02701),
  table I, 14 classical methods on UIEB: best PSNR MLLE (18.74), WWPF
  (18.60), ROP (18.48); best SSIM ROP (0.849). Sea-thru, IBLA, HLRP and
  fusion ranked lower. Learned methods lead by about 5 dB. (Checked
  against the paper.)
- Wang et al., IEEE Access 7, 2019,
  [doi:10.1109/ACCESS.2019.2932130](https://doi.org/10.1109/ACCESS.2019.2932130):
  "none of the compared methods can improve the quality of all testing
  underwater images"; recommends combining model-based and model-free
  steps.
- UIEB (Li et al., TIP 29, 2020,
  [doi:10.1109/TIP.2019.2955241](https://doi.org/10.1109/TIP.2019.2955241)):
  950 images, references are the preferred output of 12 methods, not
  ground truth; non-commercial.
- UCIQE (Yang & Sowmya 2015) and UIQM (Panetta et al. 2016) favor
  over-enhanced, colorful results and do not measure color accuracy; in
  UID2021 ([arXiv:2204.08813](https://arxiv.org/abs/2204.08813)) their
  correlation with human opinion was 0.60 and 0.54. **Tune by eye on real
  dive photos; check color accuracy on SQUID's charts.**

## Patents

| Patent | Holder | Status | Covers | Our position |
|---|---|---|---|---|
| [US12373929B2](https://patents.google.com/patent/US12373929B2/en) | Arashi Vision (Insta360) | active to 2042 | 8-step recipe: 8-bit to linear, mean-based adjustment, per-pixel weights, red/blue gains, ranking-based stretch, weighted fusion with the original | do not use this combination (design.md) |
| [US11024047B2](https://patents.google.com/patent/US11024047B2/en) | Univ. of California (IBLA) | active to 2037 | depth from a multi-scale blurriness map, max filter, refinement | no blurriness-based depth |
| [US10885611B2](https://patents.google.com/patent/US10885611B2/en) | Tel Aviv Univ., Univ. of Haifa (haze-lines) | US: expired, maintenance fees not paid | clustering pixels into haze-lines | no haze-line clustering; other countries not checked |
| [US11810272B2](https://patents.google.com/patent/US11810272B2/en) | Carmel Haifa Univ., Ramot at Tel Aviv Univ. (continuation of the above) | US: granted 2023-11-07, in force (maintenance not checked) | claims 1 and 2 require clustering pixels into haze-lines | no haze-line clustering; see transmission-research.md |
| Sea-thru family (WO2020234886A1, US20220215509A1, EP3973500, ...) | Univ. of Haifa, SeaErra | US application abandoned; others not checked | recovery using a range map | no range map |
| [US11710245B2](https://patents.google.com/patent/US11710245B2/en), continuation [US12217439B2](https://patents.google.com/patent/US12217439B2/en) | Jack Wade | granted 2023 and 2025 | marine snow removal from live video: every independent claim needs a camera, optical flow between frames with the camera's own motion subtracted, and (in the continuation) a chroma mask per frame, video display, FPGA or DVR; removed pixels come from earlier frames or neighbors | a single photo has no frames and no optical flow; our spatial detection and median fill is outside these claims (read from the granted claims, all 6 and all 20) |

Our reading of the claims, not legal advice.

## Still open

The licenses of the EUVP and LSUI datasets; the Sea-thru and haze-lines
patents outside the US; official code for UDCP, the Red Channel prior and
WCID (none found).

## Transmission for bright objects and a varying water colour

[transmission-research.md](transmission-research.md) (2026-09-26): which
single-image methods estimate the transmission better for bright objects
lit by the water's light, and for a water colour that changes over the
photo; formulas, costs, failure modes, licences of reference code and
patents, and a ranked recommendation. The fitted background surface it
recommends is being tried on the branch `transmission-surface`.

