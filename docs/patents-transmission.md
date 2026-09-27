# Patents on transmission and depth priors (checked 2026-09-27)

Claims read on Google Patents pages (legal-events data for the status),
US11810272B2 also from the USPTO PDF. The EPO Register and CNIPA could not
be reached directly. Not legal advice; a record of what was found.

## Bottom line

Two families are live and relevant:

- **Haze-lines**: US11810272B2 active until 2037-11-27 (its parent
  US10885611B2 lapsed).
- **ULAP**: CN108921887B, active in China only.

Two narrow Chinese patents touch CAP and the rank-one prior (items 4 and
11). Everything else found is expired, abandoned, withdrawn, rejected, or
never patented.

## By method

1. **Maximum intensity prior** (Carlevaris-Bianco, Mohan, Eustice 2010):
   no patent found (searched the inventors and the phrase).
2. **Red channel prior** (Galdran et al., Tecnalia 2015): no patent by the
   authors. Third party: CN106485681B (Hohai, 2016) terminated 2024-10-15
   for non-payment; CN106570839A (Tianjin, 2016) deemed withdrawn 2019.
3. **GDCP** (Peng and Cosman): no patent. US11024047B2 (IBLA blurriness)
   active to 2037-11-07; claim 1 needs a multi-scale blurriness map, a
   max-filtered rough depth, and closing by morphological reconstruction
   with guided filtering, hole filling or matting. Avoid the blurriness
   term.
4. **Color attenuation prior** (Zhu, Mai, Shao): no patent claims CAP as
   such. CN105979120B (SCAU, Mai Jiaming, active, China only): claim 1 is
   an Apache Storm distributed video dehazing system; it spells out the
   CAP transmission (0.1893 / 1.0267 / -1.2966, 15x15, beta 1) but only
   reads on that distributed pipeline. CN105719247B (active, China): a
   neural network on DCP and CAP features. CN116563143B (2026): "improved
   CAP" with AOD-Net. SIAT's 2013 to 2015 filings are nearly all rejected,
   withdrawn or lapsed.
5. **ULAP** (Song et al., Shanghai Ocean Univ.): **CN108921887B**, granted
   2022-06-24, active, China only (no US, EP or WO), expected expiry
   2038-06-07. Claim 1 requires all of: the observation that max(G,B)
   minus R grows with depth; a training set of hand-picked, guided-filtered
   depth maps blending red-channel, MIP and blurriness depths with sigmoid
   weights (s = 32); learning a linear model on MVGB and VR (7:3 split,
   10-fold cross validation); applying it. Also CN108596853B (active,
   China): a statistical background-light model.
6. **Haze-lines** (Berman, Treibitz, Avidan): WO2017175231A1 (PCT done);
   US10885611B2 lapsed 2025-01-05; **US11810272B2 active to 2037-11-27**;
   EP3440627A1 deemed withdrawn 2024-09-25; IL262175B2 granted 2023;
   IL300998A (divisional) pending; no CN member. US11810272B2: claim 1
   (underwater) requires converting to medium-compensated images for
   several water types, clustering RGB into non-local haze-lines,
   transmission from the haze-lines, dehazing, for each compensated image;
   claim 2 the unit-sphere tessellation in a KD-tree; claim 7 a system of
   claim 1. IL262175B2: clustering in spherical coordinates around the
   airlight. IL300998A: plain haze-lines clustering. In the US, plain
   haze-lines with another clustering and no per-water-type loop is not
   covered by claims 1, 2 or 7 as written. Sea-thru's US20220215509A1 is
   abandoned in the US (2024-08-16); its EP, AU, IL and WO members were not
   checked.
7. **Boundary constraint** (Meng et al., CASIA): no patent by the authors.
   CN106023108A (third party) rejected 2019.
8. **Dark channel prior** (He, Sun, Tang): **US8340461B2 (Microsoft)
   expired for non-payment of the 7.5-year fee, lapse effective
   2020-12-25**, US only. Guided filter: no patent found.
9. **Fattal**: colour-lines WO2015125146A1 ceased, US20170178297A1
   abandoned; Fattal 2008 US8350933B2 expired 2021-01-08 for non-payment.
10. **Depth from the image row, or a water segmentation plus dark
    channel**: nothing claims either. Nearest: CN117152003A (pending),
    CN113269763B (active; needs a monocular depth network), CN116645284A
    (pending; needs a blur-based bright-channel depth).
11. **Rank-one prior**: no patent by the authors. CN119090771A (Tianjin,
    2024, pending): broad claim 1 on a "unified spectrum value" from the
    image, transmission and ambient light from it; watch it if an
    ROP-style projection is built.
12. **Emberton, Chittka, Cavallaro** (veiling light): no patent found.

## For this operation

The dark channel on green and blue, the guided filter, the quadtree water
search, the fitted background surface (branch transmission-surface) and
the divisions by the water's colour touch none of the live claims above.
Avoid: blurriness depth (US11024047B2), haze-lines clustering with
per-water-type compensation (US11810272B2), and the learned ULAP linear
model (CN108921887B, China).
