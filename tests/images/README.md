# Test photos

Put underwater photos here to test the filter on. They are **not
committed** unless their photographer has agreed to publish them under a
free license (then list them below with that license); `.gitignore` keeps
all other files out of the repository.

What helps most is variety:

- shallow (under 5 m) and deep (over 15 m);
- blue ocean water and green water (lakes, coastal, murky);
- ambient light only, and with strobe or video light;
- subjects close to the camera and far away, and some open water in the
  frame;
- a few with something known to be white or gray (a slate, a white fin,
  a gray card) to judge the white balance;
- full resolution, and if possible both the camera JPEG and a 16-bit
  export of the raw file.

## The test set from Wikimedia Commons

`fetch.py` downloads 42 freely licensed underwater photos from Wikimedia
Commons (public domain, CC0, CC BY and CC BY-SA) as 3000 pixel wide
renditions, chosen to cover the list above: ambient blue and green water
(also murky), wrecks, kelp, reef, strobe-lit subjects, scenes with a
frame, slate or sign as a reference, and shallow water. `manifest.json`
names the author, license and Commons page of each; when a result made
from one of them is shown or published, credit it as the license asks
(author, license, link).

    tests/images/fetch.py

Their names say what they are: `ambient-blue-01.jpg`,
`strobe-lit-03.jpg`, ...

## Published photos

| File | Photographer | License | Notes |
|---|---|---|---|
| | | | |
