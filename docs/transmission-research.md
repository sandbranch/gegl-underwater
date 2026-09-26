
## Findings from trying it (2026-09-26, branch transmission-surface)

On the 42 Commons photos and 57 GoPro dive photos (tested locally, not
committed):

- The fitted surface gives the right distance map in open water: dark
  open water is far, fish schools in it stay far, divers and sharks
  against it are near. As the only water colour it clearly improves
  blue-water scenes (no violet, natural blue water, sunbursts not blown).
- But it makes murky green and teal scenes greener (sea floor, fins),
  and the cause is not the fit's colour: with only the transmission
  taken from the surface (the local average kept for everything else)
  the green stays. The cause is that the correction is weaker with
  distance (red restoration, white balance and the kept veil are all
  faded by t, which is what keeps open water from turning violet). In
  murk the sea floor really is fairly far, so a right t switches its
  correction off; the old, wrong t called it near and corrected it.
- Mixing the two maps by the open water mask keeps the murk right but
  loses most of the blue-water gain, and draws a ring where the maps
  meet.
- So the next step is not the transmission but the gating: the
  correction should fade with "is this water" (the open water mask),
  not with distance. A far sea floor is still a subject.

### Gating by the open water mask (tried 2026-09-26)

Fading the red restoration and white balance by "not open water" (the
fit's mask) instead of by t did not remove the green of far sea floor in
murk; the green comes from the kept veil. Fading the kept veil over
non-water too (half, or all) removes the green but makes it worse: dark,
blotchy patches with hard edges along the mask's border, red sea floor
under the turtle (ambient-green-04), black shadows. The mask is a hard,
coarse region; the veil it gates is large, so every error in the mask
shows. A soft, continuous measure of "water in front of this pixel" is
needed rather than a region mask; the branch keeps the build switches
UW_GATE and UW_VEIL for trying more.

### Survey ideas tried (2026-09-26)

After docs/survey-2026.md. Build switches on this branch: UW_DIV (divide
green and blue by the water's own balance of them), UW_SOFT (a soft
water key after Berman's haze-lines code, replacing the distance gate),
UW_BLUE (turn the kept water colour of green water towards blue and make
it less saturated), UW_TMIX (0: main's transmission; 1: from the fitted
surface).

- With the surface transmission (UW_TMIX 1), every variant leaves far
  murk dark teal: the floor is correctly far, so it is mostly veil.
- UW_SOFT leaves green fringes along subjects in green water. Dropped.
- UW_DIV everywhere turns subjects in blue water yellow green (blue is
  halved). Only in green water (weighted by the water's greenness) and
  with UW_BLUE 0.7, on main's transmission, it is a small, clean gain:
  bright green water above a reef becomes a calmer blue green
  (ambient-green-08), blue water scenes are unchanged, the GoPro set is
  unchanged or slightly better. Candidate for main:
  -DUW_TMIX=0.0f -DUW_GATE=0 -DUW_DIV=1.0f -DUW_BLUE=0.7f
