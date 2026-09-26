# Transmission for bright objects and dark open water: research

Collected 2026-09-26 for `underwater:correct`. Research only: nothing in the
repository was changed. Papers were read in full unless marked "abstract
only" or "secondhand". Costs for 24 MP are estimates from operation counts
on a 512 px copy plus guided upsampling, not benchmarks.

## 1. The two failures, stated physically

Model (per channel c, B the water radiance at infinity along that line of sight):

    I_c = J_c t_c + B_c (1 - t_c)

Our estimate: `t = 1 - 0.9 * min_3x3 min(G/A_g, B/A_b)`, A from the water map.

**(a) Dark open water read as near** (the coordinator's finding from the
prototype maps). B is not one colour. It depends on the viewing direction:
the path radiance is the sky and sea radiance distribution convolved with
the phase function, and the effective attenuation runs from β - K_d
(looking up) to β + K_d (looking down) (Akkaynak & Treibitz, CVPR 2018,
Eq. 1; Schechner & Karpel, IEEE JOE 30(3) 2005, Eqs. 11 to 14). Water
looking down or away from the sun is darker than an A taken from a wide
average or the brightest block, so `min(G/A_g, B/A_b) < 1` and the dark
channel calls it near. Sea-thru itself assumes this away: its B∞ is global
and the paper says the model "is formulated for imaging in the horizontal
direction" (CVPR 2019). The same smooth-convolution argument says B(x) is a
**low-frequency function of image position**, which justifies a low-order
surface model.

**(b) Bright rock read as far.** If the rock really equals B(x) in all
three channels, **no colour prior can separate it from water**: DCP, UDCP,
Red Channel, MIP, GDCP, CAP, ULAP, haze-lines, the boundary constraint,
ROP, IDGCP and IDE all compute t from the colour relative to A. The papers
say so: He et al. 2011 (objects "inherently similar to the atmospheric
light", white marble), Meng 2013 ("white objects ... confusing color with
the hazes"), Berman 2016 Eq. 7 (colours collinear with A are ambiguous),
Carr & Hartley 2009, Fattal 2008 (the white flowers), Emberton 2015 ("nearby
bright objects are classified as being far away") and Emberton 2018 (bright
seabed). Two consequences:

- in the exactly ambiguous case, `J = (I - B)/t + B` stays close to B
  whatever t is, so what goes wrong on the rock is mostly the steps
  *gated* by t: the kept veil gets the water colour, and white balance and
  restoration fade out. The real subject colour cannot come back through t;
- a rock that differs from B even a little (brighter in G or B, more red
  left, lower saturation) *can* be separated by a colour prior, provided
  B(x) is right locally. With a global or too-wide A, the same rock sits
  below A (hence t = 0.16) where the local water behind it may actually be
  darker. **Failure (b) is partly a symptom of (a)**: a too-bright A makes
  every water-coloured thing look like water, a too-dark one makes water
  look near.

Only three kinds of cue work (the literature and our own reading agree):

1. leftover colour differences from B(x): brighter than B, more red than B,
   less saturated than B;
2. texture or blur: the cue of our failed floor; it breaks on fish schools,
   and published methods using it (IBLA's blurriness, Tarel's local std,
   Emberton's entropy) inherit the same failure;
3. region and geometry: connectivity to the open water region, a vertical
   ordering prior, region size.

## 2. Candidates

### 2.1 Carlevaris-Bianco, Mohan, Eustice: maximum intensity prior (MIP)

"Initial results in underwater single image dehazing", OCEANS 2010 MTS/IEEE
Seattle, pp. 1-8, doi:10.1109/OCEANS.2010.5664428. Full text read:
https://robots.engin.umich.edu/publications/ncarlevaris-2010a.pdf

    D(x) = max_{Ω(x)} I_R  -  max_{Ω(x)} max(I_G, I_B)        (Eq. 5)
    t(x) = D(x) + (1 - max_x D(x))                            (Eq. 6)
    t = ω where t < ω, 0.75 <= ω <= 0.95; patch 20 to 60 px   (Eq. 8)
    A = I(argmin t)                                           (Eq. 16)

Refined by Levin matting (a guided filter replaces it).

- Bright objects: a near white object keeps its red, D near 0, called near
  (right). An object with the water's colour gets the water's D (wrong).
  The paper: large solid-coloured objects are hard; the white surround of
  the target was "consistently estimated to be closer". Peng 2017: fails for
  a bright foreground on a dark background. Normalising by the global
  maximum makes the whole map depend on the one nearest red object.
- Cost: two max filters; trivial.
- Per image: ω and patch size by hand.
- Code: none official; wangyanckxx collection has Python, no license.
- If the ambient light at the rock's depth has no red at all, it gives
  nothing. Absolute red difference ignores that B has its own red (see
  2.2, which normalises by A_R).

### 2.2 Galdran, Pardo, Picón, Álvarez-Gila: Red Channel prior

"Automatic Red-Channel underwater image restoration", JVCIR 26 (2015)
132-145, doi:10.1016/j.jvcir.2014.11.006. Accepted manuscript read:
https://eprints.bournemouth.ac.uk/34884/1/JVCI-14-173.pdf

    J_RED = min( min_Ω (1 - J_R), min_Ω J_G, min_Ω J_B ) ≈ 0          (Eq. 4)
    t = 1 - min( min_Ω(1-I_R)/(1-A_R), min_Ω I_G/A_G, min_Ω I_B/A_B )  (Eq. 5)
    with saturation (Thm 2.7):
    t = 1 - min( ..., λ · min_Ω Sat ),  Sat = (max - min)/max        (Eq. 11)

A: among the top 10 % of the Red Channel image, the pixel with the lowest
red. λ in [0,1] "can be manually adjusted", no default in the manuscript.
Per-channel `t_c = t_R^(β_c/β_R)`; recovery with t0 = 0.1.

- Bright objects: the red term gives `t = (I_R - A_R)/(1 - A_R)` when it is
  the minimum, so anything with more red than the water rises (right for
  near objects that keep red, and right for distant fish, which have lost
  it). The Sat term floors t at `1 - λ Sat`, so any low-saturation pixel is
  near: right for white objects under artificial light, but a bluish-white
  rock close to the water's saturation gets little help, and dim
  low-saturation murk is also called near.
- Cost: four min filters; trivial.
- Per image: λ, patch size.
- Code: agaldran/UnderWater has only images and a metric script, no
  license.

### 2.3 Peng & Cosman: IBLA (TIP 2017)

"Underwater Image Restoration Based on Image Blurriness and Light
Absorption", IEEE TIP 26(4):1579-1594, doi:10.1109/TIP.2017.2663846. Read:
https://escholarship.org/uc/item/07z345gx

    P_init = (1/n) Σ_{i=1..4} |I_g - G_{r_i} * I_g|,  r_i = 2^i·4 + 1     (Eq. 13)
    P_r = max_{7x7} P_init;  P_blr = refine(fill_holes(P_r))              (14, 15)
    d_R = 1 - Fs(max_Ω I_r);  d_D = 1 - Fs(D_mip);  d_B = 1 - Fs(P_r)      (19-22)
    d_n = θ_b [θ_a d_D + (1-θ_a) d_R] + (1-θ_b) d_B                       (Eq. 23)
    θ_a = S(avg_c B_c, 0.5), θ_b = S(avg I_r, 0.1), S(a,v) = 1/(1+e^{-32(a-v)})
    d0 = 1 - max_{x,c} |B_c - I_c| / max(B_k, 1 - B_k);  d_f = 8 m · d_n + d0
    t_r = exp(-d_f / 7);  t_k = t_r^(β_k/β_r)

Fs is a min-max stretch. B from three candidates (blurriest 0.1 %,
lowest-variance and blurriest quadtree blocks) mixed by the fraction of
bright pixels.

- Bright objects: d_R and d_D call near red-keeping objects near; d_B calls
  any sharp object near, including a sharp fish school (our yellow-green
  failure). d_B only gets weight when mean red is very low.
- Cost: four Gaussians, a max filter, hole filling, guided filter; fine at
  512 px (the paper's 146 s is MATLAB with matting).
- Per image: none.
- Code: ytpeng-aimlab repo, MATLAB, no license.
- **Patent US 11,024,047 B2 (University of California, active to 2037)
  claims the blurriness-based depth.** Do not use d_B or P_init.

### 2.4 Peng, Cao, Cosman: GDCP (TIP 2018)

"Generalization of the Dark Channel Prior for Single Image Restoration",
IEEE TIP 27(6):2856-2868, doi:10.1109/TIP.2018.2813092. Read:
https://escholarship.org/content/qt4106986j/qt4106986j.pdf

    A: Sobel gradient → dilate, fill → D_r = 1 - Fs(G_m); fit I_c = b_c + a_c D_r;
       s_c = [a_c > 0], w_c = tanh(4|a_c|);
       D(x) = min_{c, Ω} (1 - w_c |s_c - I_c|);  A = mean I over top 0.1 % of D  (Eqs. 6, 7)
    t(x) = max_{c, y∈Ω(x)} |A_c - I_c(y)| / max(A_c, 1 - A_c)                 (Eq. 9)
    then median filter, stretch to [0.2, max t];  J = (I - A_φ)/max(t, 0.3) + A_φ

(The official code takes the per-pixel max over c, a 15x15 median, on a
480 px wide copy.)

- Bright objects: the absolute difference makes a pixel brighter than A
  near: `t = (I - A)/(1 - A)`, e.g. A_b = 0.7, I_b = 0.9 gives 0.67. Right
  for rock that is brighter than the water. Exactly equal to A: t = 0.
- **Against failure (a) it is worse with a global A**: dark deep water, far
  from A, gets t high by the same absolute difference. It only works with
  a B(x) that follows the water (section 3).
- The paper's own failures: several light sources, non-uniform light, and
  large smooth foreground objects (a hull) judged far in the A search.
- Cost: O(1) per pixel; the authors already run it at 480 px.
- Per image: none.
- Code: ytpeng-aimlab/Generalization-of-the-Dark-Channel-Prior-for-Single-Image-Restoration,
  **GPL-3.0** (GitHub API), so it may even be adapted with credit.
- Patent: none found (web search by authors and title; Google Patents
  returned HTTP 503 to every fetch today, so this is not exhaustive).

### 2.5 Zhu, Mai, Shao: Color Attenuation Prior (TIP 2015)

"A Fast Single Image Haze Removal Algorithm Using Color Attenuation Prior",
IEEE TIP 24(11):3522-3533, doi:10.1109/TIP.2015.2446191. **TIP text not
read** (closed); read the BMVC 2014 version and the official code.

    d(x) = θ0 + θ1 v(x) + θ2 s(x) + ε,  (v, s: HSV value, saturation)
    θ0 = 0.121779, θ1 = 0.959710, θ2 = -0.780245, σ(ε) = 0.041337 (TIP/code)
    d → min filter r 15 → guided filter; t = exp(-β d), β = 1

The θ are a least-squares fit on synthetic hazy images (three numbers, no
data shipped; whether that counts as "training data" is a policy call,
comparable to hand-tuning ω = 0.9).

- Underwater it is wrong both ways: bright saturated water reads near,
  bright low-saturation rock reads far. Not a candidate.
- Code: JiamingMai repo, no license.

### 2.6 Song et al.: ULAP (PCM 2018) and its extension (TBC 2020)

The authors are W. Song, Y. Wang, D. Huang, D. Tjondronegoro (not the list
in the request). "A Rapid Scene Depth Estimation Model Based on Underwater
Light Attenuation Prior for Underwater Image Restoration", PCM 2018, LNCS
11164, pp. 678-688, doi:10.1007/978-3-030-00776-8_62. **PCM text not
read** (closed); formulas from the authors' extension, Song, Wang, Huang,
Liotta, Perra, IEEE Trans. Broadcasting 66(1):153-169, 2020,
doi:10.1109/TBC.2019.2960942 (arXiv 1906.08673, read), and co-author code.

    d(x) = μ0 + μ1 max(G,B) + μ2 R,  μ = (0.53214829, 0.51309827, -0.91066194)
    d → global stretch (0.05 % clip) → guided filter (r 50)
    t_c = Nrer_c ^ (D∞ (d + d0)),  Nrer = (0.83, 0.95, 0.97), D∞ = 10 (8 in code)

- Bright objects: a water-coloured rock (low R, high max(G,B)) is far; it
  rises only through R. Essentially a learned MIP.
- TBC 2020 is the most relevant paper for white objects: the NUDCP map
  `t = (1 - min_c min_Ω I_c/B_c)/(1 - 0.1/B_max)` took "the white fish in
  the front of the image ... as the background"; the fix is
  `t_f = max(t, λ (1 - Sat))` with **λ = 0.7**, a reversed-saturation floor,
  shown correcting the white fish (Fig. 9a), plus an elementwise min with
  the ULAP map. Its statistical BL (fitted to 500 annotated images) is
  global.
- Cost: the cheapest here. Per image: none.
- Code: wangyanckxx collection, no license.
- Patent: not established. Prof. Song Wei (Shanghai Ocean University)
  holds 10 granted Chinese invention patents per her university page; a
  web search did not tie one to ULAP. Unchecked in CNIPA.

### 2.7 Berman et al.: haze-lines

- Non-Local Image Dehazing, CVPR 2016, doi:10.1109/CVPR.2016.185 (read).
- Single Image Dehazing Using Haze-Lines, TPAMI 42(3):720-734, 2020,
  doi:10.1109/TPAMI.2018.2882478 (not read, closed).
- Underwater Single Image Color Restoration Using Haze-Lines and a New
  Quantitative Dataset, TPAMI 43(8):2822-2837, 2021,
  doi:10.1109/TPAMI.2020.2977624, arXiv 1811.01343 (read, with the code).

```
I_A = I - A = t (J - A);  r = ||I_A||;  cluster I_A/r to 1000 (500 UW) sphere points
t~ = r / max_{x∈H} r        (UW: 0.9 r / r_max, per-channel medium compensation first)
t_LB = 1 - min_c I_c/A_c    (UW: max(1 - I_B/A_B, (1 - I_G/A_G)^β_BG, (1 - I_R/A_R)^β_BR))
WLS: Σ (t - t~_LB)²/σ²(H) + λ Σ_{N4} (t_x - t_y)²/||I_x - I_y||²,  λ = 0.1
```

Code weights: `min(1, n_H/50) · min(1, 3 max(0.001, s - 0.1))`, so short or
narrow lines count little. The underwater version takes A from the largest
connected textureless component of an edge map, **limited in the code to
the top 25 % of the frame**, and forces `t = t_LB` for pixels close to the
water in Mahalanobis distance (Eq. 9). The in-air code also sets the top
row to each column's minimum t where the data weight is low: two hidden
position priors.

- Bright objects: a rock brighter or whiter than A forms its own line and
  becomes r_max, so t near 1 (right). A rock with I ≈ A has r ≈ 0 and a
  noisy direction and gets low t; the underwater Mahalanobis step then
  forces the water's t on it. **Fails on our rock like UDCP.** Global A, so
  it does not address failure (a) either.
- Cost: nearest-of-500 lookup per pixel (a 3D LUT replaces the KD tree),
  two accumulation passes, guided filter instead of WLS: fits in 1 s.
- Per image: A, and the water type by search (about 10 Jerlov pairs).
- Code: danaberman/non-local-dehazing and underwater-hl, CC BY-NC-SA style
  non-commercial license, "Patent and trademark rights are not licensed".
- **Patents: correction to research.md.** US 10,885,611 B2 lapsed, but its
  continuation **US 11,810,272 B2** (Carmel Haifa University Economic
  Corp. and Ramot at Tel Aviv University; filed 2020-12-31, granted
  2023-11-07; continuation of PCT/IL2017/050426, so expiry about 2037;
  maintenance status not checked) is granted. Claim 1 (read from the USPTO
  PDF): convert an underwater image to medium-compensated images per water
  type, cluster RGB values into non-local haze-lines, estimate t from the
  haze-lines, restore, for each compensated image. Claim 2: haze-line
  clustering on a KD tree tessellation. The largest-blob veiling light
  appears only in dependent claims. **Anything without haze-line
  clustering is outside claims 1 and 2**; research.md's patent table should
  list US 11,810,272 as active.

### 2.8 Meng et al.: boundary constraint (ICCV 2013)

"Efficient Image Dehazing with Boundary Constraint and Contextual
Regularization", ICCV 2013, pp. 617-624, doi:10.1109/ICCV.2013.82. Read (CVF).

    C0 <= J <= C1  (defaults C0 = 20, C1 = 300 on 0-255)
    t_b = min( max_c max( (A_c - I_c)/(A_c - C0_c), (A_c - I_c)/(A_c - C1_c) ), 1 )   (Eq. 7)
    t^ = closing(t_b) (7x7);  then weighted L1 regularisation (8 Kirsch + Laplacian, FFT, λ = 1)
    J = (I - A)/max(t, ε)^δ + A

- Bright objects: for a pixel brighter than A the C1 term gives
  `(I_c - A_c)/(C1_c - A_c) > 0`; the paper says the constraint "still
  holds" for such pixels. This is the same two-sided bound as GDCP Eq. 9
  with explicit radiance limits. I ≈ A still gives t ≈ 0. The conclusion
  suggests scene geometry (Carr & Hartley) for white objects.
- Cost: closing plus about 9 FFT iterations; fine at 512 px; the closing
  alone is O(1).
- Per image: none beyond A.
- Code: third-party souhaiel1 repo, no license.
- Patent: not checked in CNIPA (Institute of Automation, CAS); none found
  by web search.

### 2.9 DCP itself and the bright-region family

He, Sun, Tang, TPAMI 33(12):2341-2353, 2011, doi:10.1109/TPAMI.2010.168
(read): `t = 1 - 0.95 min_Ω min_c I_c/A_c`, t0 = 0.1; A from the top 0.1 %
of the dark channel, the brightest input pixel there, because "white car
or white building" can be brighter than A; white marble without shadow is
a stated limitation without a fix. **US 8,340,461 B2 (Microsoft, DCP):
"Expired, fee related", lapsed for failure to pay maintenance fees**
(Google Patents page saved in the scratchpad). Other fixes seen:

- tolerance K (Jiang, Hou, Qi 2011, secondhand via Zhou et al., IJE 30(10)
  2017): `J = (I - A)/min(max(K/|I - A|, 1) max(t, t0), 1) + A`, K = 50:
  leaves everything near A undehazed, water included. No use to us;
- inverse-image DCP, `t = max(t_DCP(I), t_DCP(1 - I))`, and bright channels
  (Kim, Entropy 23(11):1438, 2021): help only pixels brighter than A, the
  same effect as the two-sided bound;
- Chen, Do, Wang, ECCV 2016: an L1 data term because "transmission values
  of white objects are often underestimated"; removes only small outliers;
  about 20 s at 480x270. No;
- Tarel & Hautière, ICCV 2009: median veil; texture lowers the veil, and
  white objects larger than the window are "erased". No.

### 2.10 Fattal: colour-lines (TOG 2014)

"Dehazing Using Color-Lines", ACM TOG 34(1):13, 2014, doi:10.1145/2651362
(read). Patch model `I = l R + (1 - t) A`; fit a line `lD + V` per 7x7 patch
by RANSAC; `min_{l,s} ||lD + V - sA||²` gives `t = 1 - s`, independent of
the object's brightness. Patches are **rejected** when the angle between D
and A is under 15°, support is under 40 %, and several other tests; t is
then filled in by a GMRF with weights `1/||I_x - I_y||²` and long-range links
to similar colours.

- Bright objects: a shaded bright surface of any albedo not parallel to A
  gets the right t. Our rock (D ≈ A) is rejected, which is the honest
  answer; but the fill-in follows colour similarity, which here is water,
  so it drifts back to water t unless the fill is constrained by region.
- Cost: 0.4 s/MP estimation plus 5 s/MP GMRF (paper, C, one core); at
  0.26 MP it fits, but RANSAC per patch is the most complex code here.
- Code: maxcrous and LittleLittleZE (MIT), Tomlk (GPL-3.0), none official.
- **Patent: WO2015125146A1 (Yissum, Hebrew University) is mentioned on the
  project page; status not checked.** Treat as a risk until checked.

### 2.11 Brightness-robust in-air priors (ROP, IDGCP, IDE)

- ROP: Liu et al., CVPR 2021 / TPAMI 45(7):8845-8860, 2023,
  doi:10.1109/TPAMI.2022.3226276. Haze amount = projection on the mean
  colour direction, so water-coloured rock gets water haze. Code: no
  LICENSE, README says non-commercial; **Chinese patent ZL 202011281893.3**.
- IDGCP: Ju et al., TIP 29:3104-3118, 2020, doi:10.1109/TIP.2019.2957852.
  Depth from blue only through one global curve. Fails on the rock.
- IDE: Ju et al., TIP 30:2180-2192, 2021, doi:10.1109/TIP.2021.3050643. Grey
  level only, discards colour. Worse than UDCP here.

None of the three helps; all are cheap.

### 2.12 Spatially varying background light (failure a)

- **Ancuti et al., "Day and Night-Time Dehazing by Local Airlight
  Estimation", TIP 29:6264-6275, 2020, doi:10.1109/TIP.2020.2988203** (CC BY,
  read): `A_c(x) = max_{Ψ(x)} min_{Ω(y)} I_c` per channel (an opening),
  Ω about 20 px at 800x600, Ψ = 4Ω by day; two patch scales fused by a
  Laplacian pyramid. Inside open water A(x) ≈ I(x), so dark deep water
  correctly gets low t; but any bright object larger than Ω becomes its own
  airlight, so (b) gets worse. O(N).
- Li, Tan, Brown, ICCV 2015, doi:10.1109/ICCV.2015.34 (read): brightest pixel
  per 15x15 cell, guided filter, `t = 1 - min_Ω min_c I_c/L_c`. The brightest
  pixel underwater is the rock or a sunlit fish: worse for (b).
- Zhang et al., MRP, CVPR 2017, doi:10.1109/CVPR.2017.742 (read): assumes a
  white-reflectance patch in every 15x15 patch; same bias.
- **Zhang, Gao, Zhang, "An Image dehazing approach based on the airlight
  field estimation", arXiv 1805.02142, 2018** (read): the only paper found
  fitting a low-order surface, `A(x) = Σ_{i<5} w_i g_i(x)` with 2-D Legendre
  polynomials, non-DC terms regularised small. The surface idea is directly
  reusable without its joint energy minimisation.
- Yang et al., "Effective estimation of background light in underwater
  image dehazing", OSA Continuum 2(3):767, 2019, doi:10.1364/OSAC.2.000767:
  **abstract only** (CAPTCHA): says most methods "assume a uniform
  background light" and models its "directional distribution". The one
  paper whose stated goal matches (a); worth getting the full text.
- Men et al., Optics and Laser Technology 192, 2025,
  doi:10.1016/j.optlastec.2025.113565: search snippet only: background by
  gradient, colour difference and area, local BL by a maximum reflectance
  prior. Not read.
- GUDCP (Liang et al., TCSVT 32(7) 2022, doi:10.1109/TCSVT.2021.3114230):
  abstract only; hierarchical backscatter search, global.
- Patent: **CN106991663B** (Huaihai Institute of Technology) per a Patsnap
  summary claims block-wise Poisson fitting of a *local* background light
  with Retinex transmission; China only, claim text not read directly. A
  global low-order least-squares surface is a different method, but read
  the claims before building.

### 2.13 Finding open water without its brightness

- Emberton, Chittka, Cavallaro, BMVC 2015, doi:10.5244/C.29.125 (read):
  hierarchical rank of three features (1 - GB dark channel, mean channel
  std, gradient magnitude) over 6 quadtree layers; background superpixels
  (SLIC + DBSCAN) get t = 0.85. Also: in regions where J would clip, search
  for the lowest t that avoids clipping (a brightness floor like Meng's C1).
- Emberton et al., CVIU 168:145-156, 2018, doi:10.1016/j.cviu.2017.08.003
  (open access, read): local entropy on 9x9, normalised to μ ± 3σ, Gaussian;
  Hartigan's dip test decides whether pure haze exists; the histogram
  valley splits it; pure-haze pixels get one t (about 0.6 to 0.7). Entropy
  is brightness-free, so dark deep water is found; fish are high entropy
  and stay "not water" (benign); smooth sand or rock is misread as water.
- Berman 2021: largest textureless component, top quarter of the frame.
- Lin, Sun, Ye, Frontiers in Marine Science 11:1457190, 2024,
  doi:10.3389/fmars.2024.1457190 (page summary): low gradient AND large
  inter-channel difference, morphology, components under 5 % of the image
  dropped.
- Carr & Hartley, "Improved Single Image Dehazing Using Geometry", DICTA
  2009, doi:10.1109/DICTA.2009.25 (read): 32-label MRF with a monotonic
  prior, a large penalty τ = 100λ when a pixel has a larger t than the pixel
  below it; the data cost is scaled by 0.25 where the dark channel is above
  0.9 ("too similar to the fog"). Note: monotonicity alone does **not** lift
  a rock at the bottom of the frame with only water above it; it helps
  only when something confidently near lies above it in the same column,
  and it breaks on overhangs and upward shots.

None of these separates "smooth, water-coloured, near" from "smooth,
water-coloured, far" by texture or colour alone; that needs geometry
(connectivity, a smooth B(x) surface) or the sign and size of I - B(x).

## 3. Ranked recommendation

Ranked for our two failures together: dark open water read as near (a,
now the larger one), bright rock read as far (b), and the constraint that
fish schools in open water must stay far.

### 1. A smooth background-light surface B(x) plus a two-sided bound against it

The core change; it addresses (a) directly and turns (b) into a question
of the rock's colour relative to the water *right behind it*.

**B(x).** Per channel, a quadratic in image coordinates fitted to open-water
samples on the 512 px copy by robust (Huber or Tukey) iteratively
reweighted least squares:

    B_c(x, y) = a0 + a1 x + a2 y + a3 x² + a4 xy + a5 y²      (c = R, G, B)

Fit in log space or clamp to (0, 1). Samples are chosen by
brightness-free cues: low local variation at two or three scales
(coefficient of variation, as `open_water()` in tests/transmission/candidates.py
already does, or Emberton's entropy), chromaticity within a few degrees of
the current water hue, and membership in a large connected region (drop
components under a few percent of the frame, after Lin 2024 and Berman's
largest component). Iterate twice: fit, drop samples far from the fit,
refit. A fish school only removes samples; the surface bridges the hole,
where the present 1/8-window mean is pulled by whatever is in the window.
If there are too few samples (reef filling the frame), fall back to the
current A/water map.

**t.** Meng's boundary constraint with C0 = 0, C1 = 1 on green and blue
against B(x), which is also GDCP Eq. 9 with a per-side denominator:

    t_b(x) = max_{c∈{G,B}} max( (B_c - I_c)/B_c , (I_c - B_c)/(1 - B_c) )
    t      = max over 3x3 of t_b  (or Meng's closing: min_3x3 max_3x3 t_b), then guided filter

The darker side is exactly our current UDCP term (without the 0.9, which
can stay as `1 - 0.9 (1 - ...)`); the brighter side is new: anything
brighter than the water behind it must have t > 0 and a bright J, so it
cannot be pure water. That argument uses no texture, so distant fish
(near the water's brightness, or darker) keep their low t.

- Why first: it is the only candidate that fixes (a), it helps (b)
  whenever the rock is even slightly brighter than B(x) in G or B, and it
  cannot turn open water or fish yellow-green by construction.
- Cost: a 6x6 normal-equation solve per channel per iteration, O(1) filters;
  milliseconds at 512 px.
- Per image: none (window scales tied to image size).
- Needs from the pipeline: the open-water score (already prototyped),
  replacing the water map in steps 2 to 4 and 8, and the 5 degree
  water-colour test of step 6 made against B(x).
- What it gets wrong: a subject exactly equal to B(x) is still water (no
  prior can fix that); a large smooth water-coloured subject can enter the
  fit (robust weights and the connectivity test limit it); a quadratic may
  be too stiff for a sunburst (use a coarse spline grid, 4x4 knots, if so);
  near-white noise on dark water can push the bright side up (the 3x3
  window and guided filter handle it).
- Patents: GDCP none found; Meng none found by web search (CNIPA not
  checked); CN106991663B claims block-wise Poisson-fitted local BL in China
  (read its claims before building); no haze-line clustering, so clear of
  US 11,810,272.

### 2. A red-retention term against B(x) (Galdran's red channel, normalised)

A second texture-free cue for near objects: red left relative to the water.

    t_r(x) = (max_{3x3} R - B_R(x)) / (1 - B_R(x)),  clamped to [0, 1]
    t = max(t_rank1, k · t_r),  k about 0.5 to 1

This is the red term of Galdran Eq. 5 with A replaced by B(x) (the MIP of
Carlevaris-Bianco is the unnormalised version). Distant fish and water
have lost their red (t_r ≈ 0, unchanged); a rock at 1 to 2 m keeps the red
of the ambient light, and because B∞_R = L_R b_R/β_R is small next to the
light's own red (β_R large), a near white object has proportionally more
red than the water even under blue ambient light (our reading of Akkaynak
& Treibitz 2018 Eq. 7). Optionally Song 2020's reversed-saturation floor
measured **relative to B(x)**: `λ (1 - Sat(I)/Sat(B(x)))`, λ about 0.7,
where the published absolute form would make dim grey murk near.

- Why second: cheap, independent of texture, and physically sound for
  our rock; but red is the noisiest channel on GoPro JPEGs in blue water
  and may be almost zero at depth, so it must be a max with, not a
  replacement for, the rank 1 estimate, and it needs a small dead zone
  above B_R.
- Cost: one max filter. Per image: k and the dead zone (fixed defaults
  should do).
- Needs: B_R(x) from rank 1 (a global A_R is not good enough: sunlit water
  near the surface has more red).
- Check first on the photos: sample R/G and saturation on the rock and on
  the water just behind it. If the rock is not measurably redder or less
  saturated, this cue cannot help and rank 3 is the only route.
- Patents: none found for Galdran et al. (not exhaustive; Google Patents
  unavailable today). IBLA's d_R term is similar, but US 11,024,047 claims
  the blurriness depth; a red-only term without blurriness is outside what
  research.md records of those claims (re-read claim 1 to confirm).

### 3. Mark ambiguous regions and fill them from their non-water neighbours

For the case ranks 1 and 2 cannot decide (I ≈ B(x)): do what colour-lines
and haze-lines do, refuse rather than guess, and constrain the fill by
region.

- Ambiguous pixels: colour within the water tolerance of B(x) but outside
  the large connected open-water region (enclosed by edges, not connected
  to it, or touching the bottom of the frame and a confidently near
  subject).
- Their t gets data weight near zero and is filled by normalised
  convolution (a masked guided filter) from **non-water** neighbours only;
  colour-similarity weights alone leak into water here.
- Optional Carr & Hartley column rule: a pixel cannot be farther than the
  confident subject directly above it (a per-column running maximum from
  the top, applied only to ambiguous pixels).
- Why third: the only route for a rock that truly equals the water, and
  it uses region rather than texture, so fish in open water (connected to
  the water region) stay far; but it is the most heuristic, fails on a
  rock seen only against water, and overhangs break the column rule.
- Cost: connected components and masked filters on 512 px; tens of ms.
- Per image: none, beyond the size threshold of a "large" region.
- Patents: none found for region-constrained fill-in. Fattal's
  colour-lines has WO2015125146A1 (status unchecked): do not do per-patch
  colour-line fitting without checking it.

Not recommended: CAP (wrong both ways underwater), ULAP (a learned MIP,
worse than rank 2), haze-lines (same failure as UDCP on the rock, active
US continuation), IBLA blurriness (patented, fish), Li/Tan/Brown and MRP
local airlight (biased to bright objects), Ancuti's local airlight as is
(fixes a, worsens b; rank 1's smooth surface is the constrained version of
it), ROP, IDGCP, IDE, tolerance, TGV refinement.

## 4. Code licences found

| Code | License |
|---|---|
| ytpeng-aimlab GDCP (MATLAB) | GPL-3.0 |
| ytpeng-aimlab IBLA | none |
| danaberman non-local-dehazing, underwater-hl | non-commercial (CC BY-NC-SA style), patents excluded |
| JiamingMai CAP | none |
| wangyanckxx (MIP, ULAP, others) | none |
| agaldran/UnderWater (no method code) | none |
| souhaiel1 BCCR (Meng) | none |
| maxcrous, LittleLittleZE colour-lines | MIT |
| Tomlk colour-lines | GPL-3.0 |
| junliumath/ROP | no LICENSE, README non-commercial |
| Jumingye/IDE | none |
| hainh/sea-thru | MIT; Teragion/Sea-Thru-Impl GPL-3.0 |

## 5. Patents found or checked

| Patent | Holder | Status | Relevance |
|---|---|---|---|
| US 11,810,272 B2 | Carmel Haifa Univ.; Ramot at Tel Aviv Univ. | granted 2023-11-07, continuation of the lapsed US 10,885,611; about 2037 | claims 1, 2 need haze-line clustering; **add to research.md as active** |
| US 11,024,047 B2 | Univ. of California (IBLA) | active to 2037 | blurriness depth; avoid |
| US 8,340,461 B2 | Microsoft (DCP) | expired, lapsed for non-payment of fees | DCP free |
| CN106991663B | Huaihai Institute of Technology | granted (Patsnap) | block-wise local BL, China only; claims not read directly |
| ZL 202011281893.3 | (ROP authors) | cited in the ROP README | ROP; not used |
| WO2015125146A1 | Yissum (Fattal colour-lines) | not checked | avoid per-patch colour-line fitting until checked |
| IL288277A | Carmel Haifa Univ.; SeaErra (Sea-thru) | pending per Patsnap | needs a range map |

Gaps: Google Patents answered HTTP 503 to every fetch today, so searches by
inventor (Galdran, Peng/Cosman beyond IBLA, Meng, Song, Zhu/Mai/Shao,
Emberton) went through web search and USPTO PDFs only and are not
exhaustive; CNIPA was not searched. A separate patent agent was still
running when this report was written. Our reading of the claims, not
legal advice.

## 6. Sources

- https://robots.engin.umich.edu/publications/ncarlevaris-2010a.pdf
- https://eprints.bournemouth.ac.uk/34884/1/JVCI-14-173.pdf
- https://escholarship.org/uc/item/07z345gx
- https://escholarship.org/content/qt4106986j/qt4106986j.pdf
- https://github.com/ytpeng-aimlab/Generalization-of-the-Dark-Channel-Prior-for-Single-Image-Restoration
- https://www.bmva-archive.org.uk/bmvc/2014/files/paper111.pdf
- https://github.com/JiamingMai/Color-Attenuation-Prior-Dehazing
- https://arxiv.org/abs/1906.08673
- https://arxiv.org/abs/1811.01343
- https://openaccess.thecvf.com/content_cvpr_2016/papers/Berman_Non-Local_Image_Dehazing_CVPR_2016_paper.pdf
- https://github.com/danaberman/underwater-hl
- https://image-ppubs.uspto.gov/dirsearch-public/print/downloadPdf/11810272
- https://patents.google.com/patent/US8340461B2/en
- https://openaccess.thecvf.com/content_iccv_2013/papers/Meng_Efficient_Image_Dehazing_2013_ICCV_paper.pdf
- https://people.csail.mit.edu/kaiming/publications/pami10dehaze.pdf
- https://www.cs.huji.ac.il/w~raananf/projects/dehaze_cl/
- https://users.cecs.anu.edu.au/~hartley/Papers/PDF/Carr:Fog-DICTA09.pdf
- https://chittkalab.sbcs.qmul.ac.uk/2015/Emberton%20et%20al%202015%20BMVC.pdf
- https://www.eecs.qmul.ac.uk/~andrea/papers/2017_CVIU_UnderwaterImageAndVideoDehazing__Emberton_Chittka_Cavallaro.pdf
- https://www.frontiersin.org/journals/marine-science/articles/10.3389/fmars.2024.1457190/full
- https://openaccess.thecvf.com/content_CVPR_2019/papers/Akkaynak_Sea-Thru_A_Method_for_Removing_Water_From_Underwater_Images_CVPR_2019_paper.pdf
- https://openaccess.thecvf.com/content_cvpr_2018/papers/Akkaynak_A_Revised_Underwater_CVPR_2018_paper.pdf
- https://www.ee.technion.ac.il/~yoav/publications/UWvision_JOE.pdf
- https://ieeexplore.ieee.org/document/9076820
- https://openaccess.thecvf.com/content_iccv_2015/papers/Li_Nighttime_Haze_Removal_ICCV_2015_paper.pdf
- https://openaccess.thecvf.com/content_cvpr_2017/papers/Zhang_Fast_Haze_Removal_CVPR_2017_paper.pdf
- https://arxiv.org/abs/1805.02142
- https://doi.org/10.1364/OSAC.2.000767
- https://arxiv.org/abs/2103.17126 and https://github.com/junliumath/ROP
- https://eureka.patsnap.com/patent-CN106991663B
- https://www.ije.ir/article_73028_21928dc6f3a4284db24178159b45a4f1.pdf
- https://cchen156.github.io/paper/16ECCV_Dehaze.pdf
- http://perso.lcpc.fr/tarel.jean-philippe/publis/jpt-iccv09.pdf
