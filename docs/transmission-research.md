
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
