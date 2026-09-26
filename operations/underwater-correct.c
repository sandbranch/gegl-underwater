/*
 * Underwater colour correction GEGL operation
 *
 * underwater-correct.c
 * Copyright 2026 by David
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Restores the colours of an underwater photo: rebuilds the red light
 * that the water absorbed, removes the colour cast of the water and the
 * veil of light scattered back by it. See docs/design.md.
 *
 * The steps (docs/design.md): the color of the water and a transmission
 * map are estimated on a small copy of the photo; then, per pixel, the
 * veil of backscatter is removed, the absorbed red (and optionally blue)
 * is rebuilt from green where the light is ambient (not near a strobe),
 * a robust white balance removes the remaining cast, and some of the
 * water's color is kept for open water.
 */

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_double (red_restore, _("Red restoration"), 1.0)
  description (_("How much of the red light that the water absorbed is "
                 "rebuilt from the green channel. 0 leaves red as it is."))
  value_range (0.0, 2.0)
  ui_digits (2)

property_double (blue_restore, _("Blue restoration"), 0.0)
  description (_("How much blue is rebuilt from the green channel, for "
                 "green water such as lakes, where blue is absorbed too."))
  value_range (0.0, 2.0)
  ui_digits (2)

property_boolean (white_balance, _("White balance"), TRUE)
  description (_("Remove the colour cast that remains after the red and "
                 "blue restoration."))

property_boolean (auto_water, _("Estimate the water color"), TRUE)
  description (_("Estimate the color of the water from the image. Turn off "
                 "to pick it from a patch of open water."))

property_color (water_color, _("Water color"), "#1e6478")
  description (_("The color of open water in the photo, which is the color "
                 "of the light scattered back by the water."))
  ui_meta ("visible", "! auto-water")

property_double (backscatter, _("Backscatter removal"), 0.5)
  description (_("How much of the veil of light that the water scatters "
                 "back towards the camera is removed."))
  value_range (0.0, 1.0)
  ui_digits (2)

property_double (clarity, _("Clarity"), 0.5)
  description (_("How much of the contrast that the water took away is "
                 "restored in the distance. Higher brings back more detail "
                 "and also more noise."))
  value_range (0.0, 1.0)
  ui_digits (2)

property_double (keep_water, _("Keep water color"), 0.5)
  description (_("How much of the color of open water is kept, so that the "
                 "photo still looks underwater while the subject is "
                 "corrected."))
  value_range (0.0, 1.0)
  ui_digits (2)

#else

#define GEGL_OP_FILTER
#define GEGL_OP_NAME     underwater_correct
#define GEGL_OP_C_SOURCE underwater-correct.c

#include "gegl-op.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* the size of the copy on which the water and transmission are estimated */
#define SMALL_SIZE 512
/* the white balance: the Minkowski norm (shades of gray), and how far blue
 * may be lowered against green. p = 6 looks mostly at the brightest
 * pixels, often sunlit water, and left blue water scenes violet; p = 2 is
 * closer to gray world and removes more of the blue */
#define WB_P    2.0
#define WB_BMIN 0.4f

typedef struct
{
  gint    w, h, f;        /* small size, and the factor from full size */
  gfloat *rgb;            /* small copy, linear RGB */
  gfloat *t;              /* transmission of the small copy */
  gint    mw, mh, mf;     /* medium size, on which t is refined along edges */
  gfloat *tm;             /* transmission at medium size */
  gfloat  water[3];       /* water color (veiling light), linear */
  gfloat *wmap;           /* the water color per pixel of the small copy:
                           * lighter towards the surface, darker below */
  gfloat *kmap;           /* the part of it that is kept: keep-water sets
                           * its chroma, with the same hue and lightness */
  gfloat  gain[3];        /* white balance gains */
  gfloat  mean_r, mean_g, mean_b; /* after removing the backscatter */
  gfloat  greenness;      /* 0 for blue water, 1 for green water, whose blue is absorbed too */
} Estimate;

/* a broken pixel (NaN or infinity, which float images can have) counts as
 * black in the estimates, so that it cannot spread over the whole image
 * through them */
static inline gfloat
finite_or_0 (gfloat v)
{
  return isfinite (v) ? v : 0.0f;
}

static inline gfloat
smoothstep (gfloat e0, gfloat e1, gfloat x)
{
  gfloat t = CLAMP ((x - e0) / (e1 - e0), 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

/* box mean over a square of radius r, through an integral image */
static void
box_mean (const gfloat *src, gfloat *dst, gint w, gint h, gint r)
{
  gdouble *sum = g_new0 (gdouble, (gsize) (w + 1) * (h + 1));
  gint     x, y;

  for (y = 0; y < h; y++)
    {
      gdouble rs = 0.0;
      for (x = 0; x < w; x++)
        {
          rs += src[(gsize) y * w + x];
          sum[(gsize) (y + 1) * (w + 1) + x + 1] = sum[(gsize) y * (w + 1) + x + 1] + rs;
        }
    }
  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      {
        gint x0 = MAX (x - r, 0), x1 = MIN (x + r + 1, w);
        gint y0 = MAX (y - r, 0), y1 = MIN (y + r + 1, h);
        gdouble s = sum[(gsize) y1 * (w + 1) + x1] - sum[(gsize) y0 * (w + 1) + x1]
                  - sum[(gsize) y1 * (w + 1) + x0] + sum[(gsize) y0 * (w + 1) + x0];
        dst[(gsize) y * w + x] = s / ((x1 - x0) * (y1 - y0));
      }
  g_free (sum);
}

/* guided filter (He et al.): smooths p while following the edges of I */
static void
guided_filter (const gfloat *I, gfloat *p, gint w, gint h, gint r, gfloat eps)
{
  gsize   n = (gsize) w * h, i;
  gfloat *mI = g_new (gfloat, n), *mp = g_new (gfloat, n);
  gfloat *II = g_new (gfloat, n), *Ip = g_new (gfloat, n);
  gfloat *a = g_new (gfloat, n), *b = g_new (gfloat, n);

  for (i = 0; i < n; i++)
    {
      II[i] = I[i] * I[i];
      Ip[i] = I[i] * p[i];
    }
  box_mean (I, mI, w, h, r);
  box_mean (p, mp, w, h, r);
  box_mean (II, II, w, h, r);
  box_mean (Ip, Ip, w, h, r);
  for (i = 0; i < n; i++)
    {
      gfloat var = II[i] - mI[i] * mI[i];
      gfloat cov = Ip[i] - mI[i] * mp[i];

      a[i] = cov / (var + eps);
      b[i] = mp[i] - a[i] * mI[i];
    }
  box_mean (a, a, w, h, r);
  box_mean (b, b, w, h, r);
  for (i = 0; i < n; i++)
    p[i] = a[i] * I[i] + b[i];

  g_free (b); g_free (a); g_free (Ip); g_free (II); g_free (mp); g_free (mI);
}

/* Red (and blue) restoration after Ancuti et al. (TIP 2018, Eq. 4),
 * R' = R + a (mean G - mean R) (1 - R) G, with G divided by its mean: in
 * linear light the values of a dark photo are small, and the published
 * form would then add almost nothing; divided, it adds about the
 * difference of the means where green is average, as intended. */
static inline void
restore (GeglProperties *o, const Estimate *e, gfloat *v, gfloat m)
{
  gfloat R = CLAMP (v[0], 0.0f, 1.0f), G = CLAMP (v[1], 0.0f, 1.0f), B = CLAMP (v[2], 0.0f, 1.0f);
  gfloat g = MIN (G / MAX (e->mean_g, 1e-4f), 3.0f);

  v[0] += m * o->red_restore  * MAX (e->mean_g - e->mean_r, 0.0f) * (1.0f - R) * g;
  v[2] += m * (o->blue_restore + 0.5f * o->red_restore * e->greenness)
          * MAX (e->mean_g - e->mean_b, 0.0f) * (1.0f - B) * g;
}

static inline gfloat ambient_weight (const gfloat *d);

/* the subject of a pixel: without all of the veil, and with the contrast
 * the water took away restored as far as the clarity says */
static inline void
subject (GeglProperties *o, const gfloat *water, const gfloat *p, gfloat t, gfloat *v)
{
  gfloat div = powf (MAX (t, 0.3f), o->clarity);
  gint   c;

  for (c = 0; c < 3; c++)
    v[c] = MAX (p[c] - water[c] * (1.0f - t), 0.0f) / div;
}

/* transmission of the small copy from the dark channel of green and blue
 * relative to the water color there, refined along the edges */
static void
transmission (Estimate *e, const gfloat *luma)
{
  gsize n = (gsize) e->w * e->h, i;
  gint  x, y;

  for (y = 0; y < e->h; y++)
    for (x = 0; x < e->w; x++)
      {
        const gfloat *a = e->wmap + ((gsize) y * e->w + x) * 3;
        gfloat v = G_MAXFLOAT;
        gint   dx, dy;
        for (dy = -1; dy <= 1; dy++)
          for (dx = -1; dx <= 1; dx++)
            {
              gint xx = CLAMP (x + dx, 0, e->w - 1), yy = CLAMP (y + dy, 0, e->h - 1);
              const gfloat *p = e->rgb + ((gsize) yy * e->w + xx) * 3;
              v = MIN (v, MIN (p[1] / a[1], p[2] / a[2]));
            }
        e->t[(gsize) y * e->w + x] = 1.0f - 0.9f * v;
      }
  guided_filter (luma, e->t, e->w, e->h, MAX (4, e->w / 40), 1e-3f);
  for (i = 0; i < n; i++)
    e->t[i] = CLAMP (e->t[i], 0.1f, 1.0f);
}

/* Oklab (Ottosson 2020) from and to linear RGB with the sRGB primaries,
 * for mixing the water color with neutral: mixed in linear RGB, blue
 * water turns lavender and green water khaki */
static void
to_oklab (const gfloat *rgb, gfloat *lab)
{
  gfloat l = cbrtf (0.4122214708f * rgb[0] + 0.5363325363f * rgb[1] + 0.0514459929f * rgb[2]);
  gfloat m = cbrtf (0.2119034982f * rgb[0] + 0.6806995451f * rgb[1] + 0.1073969566f * rgb[2]);
  gfloat s = cbrtf (0.0883024619f * rgb[0] + 0.2817188376f * rgb[1] + 0.6299787005f * rgb[2]);

  lab[0] = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
  lab[1] = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
  lab[2] = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
}

static void
from_oklab (const gfloat *lab, gfloat *rgb)
{
  gfloat l = lab[0] + 0.3963377774f * lab[1] + 0.2158037573f * lab[2];
  gfloat m = lab[0] - 0.1055613458f * lab[1] - 0.0638541728f * lab[2];
  gfloat s = lab[0] - 0.0894841775f * lab[1] - 1.2914855480f * lab[2];

  l = l * l * l; m = m * m * m; s = s * s * s;
  rgb[0] = MAX ( 4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s, 0.0f);
  rgb[1] = MAX (-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s, 0.0f);
  rgb[2] = MAX (-0.0041960863f * l - 0.7034186147f * m + 1.7076882690f * s, 0.0f);
}

/* the kept water color: its chroma times keep-water */
static void
kept_map (GeglProperties *o, Estimate *e)
{
  gsize n = (gsize) e->w * e->h, i;

  for (i = 0; i < n; i++)
    {
      gfloat lab[3];

      to_oklab (e->wmap + i * 3, lab);
      lab[1] *= o->keep_water;
      lab[2] *= o->keep_water;
      from_oklab (lab, e->kmap + i * 3);
    }
}

/* The water color varies over a photo: open water is lighter and greener
 * towards the sunlit surface and darker and bluer below. With one color
 * for all of it, water lighter than that color counts as subject, and its
 * excess turns green or cyan. The map is the average color of the open
 * water (low transmission, red absorbed) around each pixel, over a wide
 * window, and the one estimated color where there is little open water. */
static void
water_map (Estimate *e)
{
  gsize   n = (gsize) e->w * e->h, i;
  gint    c, r = MAX (8, MAX (e->w, e->h) / 8);
  gfloat *wt = g_new (gfloat, n), *num = g_new (gfloat, n), *den = g_new (gfloat, n);
  const gfloat k = 0.05f;

  for (i = 0; i < n; i++)
    {
      const gfloat *p = e->rgb + i * 3;
      wt[i] = (1.0f - smoothstep (0.2f, 0.5f, e->t[i])) * ambient_weight (p);
    }
  box_mean (wt, den, e->w, e->h, r);
  for (c = 0; c < 3; c++)
    {
      for (i = 0; i < n; i++)
        num[i] = wt[i] * e->rgb[i * 3 + c];
      box_mean (num, num, e->w, e->h, r);
      for (i = 0; i < n; i++)
        e->wmap[i * 3 + c] = MAX ((num[i] + k * e->water[c]) / (den[i] + k), 1e-4f);
    }
  g_free (den);
  g_free (num);
  g_free (wt);
}

/* the ambient mask: 1 where the light is ambient (red absorbed), 0 where
 * a strobe or video light lights the subject (red kept) */
static inline gfloat
ambient_weight (const gfloat *d)
{
  gfloat ratio = d[0] / (d[1] + 1e-4f);

  return 1.0f - smoothstep (0.35f, 0.8f, ratio);
}

static void
estimate (GeglProperties *o, const Babl *format, const gfloat *in, gint W, gint H, Estimate *e)
{
  gint    x, y, c;
  gsize   n, i;
  gfloat *luma;

  e->f = MAX (1, (MAX (W, H) + SMALL_SIZE - 1) / SMALL_SIZE);
  e->w = MAX (1, W / e->f);
  e->h = MAX (1, H / e->f);
  n    = (gsize) e->w * e->h;
  e->rgb = g_new0 (gfloat, n * 3);
  e->t   = g_new (gfloat, n);
  e->wmap = g_new (gfloat, n * 3);
  e->kmap = g_new (gfloat, n * 3);

  /* a small copy by averaging blocks; an image narrower than a block
   * (1 x 3000) repeats its last row or column */
  for (y = 0; y < e->h; y++)
    for (x = 0; x < e->w; x++)
      {
        gfloat s[3] = { 0, 0, 0 };
        gint   dx, dy;

        for (dy = 0; dy < e->f; dy++)
          for (dx = 0; dx < e->f; dx++)
            {
              gint          xx = MIN (x * e->f + dx, W - 1), yy = MIN (y * e->f + dy, H - 1);
              const gfloat *p  = in + ((gsize) yy * W + xx) * 4;
              for (c = 0; c < 3; c++)
                s[c] += finite_or_0 (p[c]);
            }
        for (c = 0; c < 3; c++)
          e->rgb[((gsize) y * e->w + x) * 3 + c] = s[c] / (e->f * e->f);
      }

  luma = g_new (gfloat, n);
  for (i = 0; i < n; i++)
    {
      const gfloat *p = e->rgb + i * 3;
      luma[i] = 0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2];
    }

  if (o->auto_water)
    {
      /* hierarchical search (as in Kim et al. 2013 for haze): split into
       * quarters and go on in the one that looks most like open water:
       * smooth, and with its red absorbed (green or blue well above red),
       * which a bright subject, a strobe-lit reef or the sun is not */
      gint x0 = 0, y0 = 0, x1 = e->w, y1 = e->h;

      while ((x1 - x0) * (y1 - y0) > 64)
        {
          gint   mx = (x0 + x1) / 2, my = (y0 + y1) / 2, q, best = 0;
          gfloat best_score = -G_MAXFLOAT;
          gint   qx0[4] = { x0, mx, x0, mx }, qx1[4] = { mx, x1, mx, x1 };
          gint   qy0[4] = { y0, y0, my, my }, qy1[4] = { my, my, y1, y1 };

          for (q = 0; q < 4; q++)
            {
              gdouble sw = 0, sl = 0, sl2 = 0;
              gint    cnt = 0;

              for (y = qy0[q]; y < qy1[q]; y++)
                for (x = qx0[q]; x < qx1[q]; x++)
                  {
                    const gfloat *p = e->rgb + ((gsize) y * e->w + x) * 3;
                    gfloat l = luma[(gsize) y * e->w + x];

                    sw  += MAX (p[1], p[2]) - p[0];
                    sl  += l;
                    sl2 += l * l;
                    cnt++;
                  }
              if (cnt == 0)
                continue;
              {
                gdouble mean = sl / cnt, sd = sqrt (MAX (sl2 / cnt - mean * mean, 0.0));
                gfloat  score = sw / cnt - 2.0 * sd;

                if (score > best_score)
                  {
                    best_score = score;
                    best = q;
                  }
              }
            }
          x0 = qx0[best]; x1 = qx1[best]; y0 = qy0[best]; y1 = qy1[best];
          if (x1 - x0 < 2 || y1 - y0 < 2)
            break;
        }

      {
        gdouble sum[3] = { 0, 0, 0 };
        gint    cnt = 0;

        for (y = y0; y < y1; y++)
          for (x = x0; x < x1; x++)
            {
              for (c = 0; c < 3; c++)
                sum[c] += e->rgb[((gsize) y * e->w + x) * 3 + c];
              cnt++;
            }
        for (c = 0; c < 3; c++)
          e->water[c] = sum[c] / MAX (cnt, 1);
      }
    }
  else
    {
      gfloat rgb[3];
      /* in the color space of the image, as its pixels are: a color
       * picked from the photo is then exactly that of its pixels */
      gegl_color_get_pixel (o->water_color,
                            babl_format_with_space ("RGB float", babl_format_get_space (format)),
                            rgb);
      for (c = 0; c < 3; c++)
        e->water[c] = rgb[c];
    }
  for (c = 0; c < 3; c++)
    e->water[c] = MAX (e->water[c], 1e-4f);

  /* transmission against the one water color, then against the map of
   * the water color made with it */
  for (i = 0; i < n; i++)
    for (c = 0; c < 3; c++)
      e->wmap[i * 3 + c] = e->water[c];
  transmission (e, luma);
  if (o->auto_water)
    {
      water_map (e);
      transmission (e, luma);
    }
  kept_map (o, e);

  /* the means after the backscatter is removed, for the red and blue
   * restoration, over the pixels in ambient light */
  {
    gdouble sr = 0, sg = 0, sb = 0, sw = 0;

    for (i = 0; i < n; i++)
      {
        gfloat d[3], wgt;
        subject (o, e->wmap + i * 3, e->rgb + i * 3, e->t[i], d);
        wgt = ambient_weight (d);
        sr += wgt * d[0]; sg += wgt * d[1]; sb += wgt * d[2]; sw += wgt;
      }
    sw = MAX (sw, 1e-6);
    e->mean_r = sr / sw; e->mean_g = sg / sw; e->mean_b = sb / sw;
    /* green water absorbs blue as well: how much blue the subjects miss */
    e->greenness = CLAMP ((e->mean_g - e->mean_b) / MAX (e->mean_g, 1e-4f), 0.0f, 1.0f);
  }

  /* white balance: shades of gray (Minkowski p = WB_P) on the restored small
   * copy, weighted to the near (not water) pixels */
  if (o->white_balance)
    {
      gdouble s[3] = { 0, 0, 0 }, sw = 0;

      for (i = 0; i < n; i++)
        {
          gfloat d[3], wgt;
          subject (o, e->wmap + i * 3, e->rgb + i * 3, e->t[i], d);
          restore (o, e, d, ambient_weight (d));
          wgt = e->t[i];
          for (c = 0; c < 3; c++)
            s[c] += wgt * pow (MAX (d[c], 0.0f), WB_P);
          sw += wgt;
        }
      {
        gfloat ill[3], rr, rb, norm;

        for (c = 0; c < 3; c++)
          ill[c] = pow (s[c] / MAX (sw, 1e-9), 1.0 / WB_P);
        /* gains relative to green, within what the physics allows: red
         * is absorbed first, so it is never too strong (a strobe-lit red
         * is real); and the water's own color is never strengthened
         * against the other of green and blue, which in blue water turns
         * open water violet; and limited, so that a tiny red cannot blow
         * up the noise */
        rr = CLAMP (ill[1] / MAX (ill[0], 1e-6f), 1.0f, 2.5f);
        rb = CLAMP (ill[1] / MAX (ill[2], 1e-6f), WB_BMIN, 2.0f);
        if (e->water[2] >= e->water[1])
          rb = MIN (rb, 1.0f);
        else
          rb = MAX (rb, 1.0f);
        /* the same brightness as before */
        norm = 0.2126f * rr + 0.7152f + 0.0722f * rb;
        e->gain[0] = rr / norm;
        e->gain[1] = 1.0f / norm;
        e->gain[2] = rb / norm;
      }
    }
  else
    for (c = 0; c < 3; c++)
      e->gain[c] = 1.0f;

  if (g_getenv ("UNDERWATER_DEBUG"))
    {
      gfloat tmin = 1, tmax = 0;
      for (i = 0; i < n; i++) { tmin = MIN (tmin, e->t[i]); tmax = MAX (tmax, e->t[i]); }
      g_printerr ("underwater: water %.4f %.4f %.4f  greenness %.2f  means r %.4f g %.4f b %.4f  gains %.2f %.2f %.2f  t %.2f..%.2f\n",
                  e->water[0], e->water[1], e->water[2], e->greenness, e->mean_r, e->mean_g, e->mean_b,
                  e->gain[0], e->gain[1], e->gain[2], tmin, tmax);
    }
  g_free (luma);
}

/* The transmission refined at a medium size (about 1536 pixels across),
 * along the edges of the photo there: estimated on the small copy only,
 * it would leave a wide band of wrong transmission around the edges of
 * subjects against open water, where the water left in them turns into
 * a colored halo. */
static void
refine (const gfloat *in, gint W, gint H, Estimate *e)
{
  gint    x, y;
  gsize   n;
  gfloat *guide;

  e->mf = MAX (1, (MAX (W, H) + 1535) / 1536);
  e->mw = MAX (1, W / e->mf);
  e->mh = MAX (1, H / e->mf);
  n = (gsize) e->mw * e->mh;
  e->tm = g_new (gfloat, n);
  guide = g_new (gfloat, n);

  for (y = 0; y < e->mh; y++)
    for (x = 0; x < e->mw; x++)
      {
        gfloat l = 0.0f;
        gint   dx, dy;

        for (dy = 0; dy < e->mf; dy++)
          for (dx = 0; dx < e->mf; dx++)
            {
              gint          xx = MIN (x * e->mf + dx, W - 1), yy = MIN (y * e->mf + dy, H - 1);
              const gfloat *p  = in + ((gsize) yy * W + xx) * 4;
              l += finite_or_0 (0.2126f * p[0] + 0.7152f * p[1] + 0.0722f * p[2]);
            }
        guide[(gsize) y * e->mw + x] = l / (e->mf * e->mf);

        /* the small transmission, interpolated */
        {
          gfloat fx = CLAMP ((x + 0.5f) * e->mf / e->f - 0.5f, 0.0f, e->w - 1.0f);
          gfloat fy = CLAMP ((y + 0.5f) * e->mf / e->f - 0.5f, 0.0f, e->h - 1.0f);
          gint   x0 = (gint) fx, y0 = (gint) fy;
          gint   x1 = MIN (x0 + 1, e->w - 1), y1 = MIN (y0 + 1, e->h - 1);
          gfloat ax = fx - x0, ay = fy - y0;
          const gfloat *t = e->t;

          e->tm[(gsize) y * e->mw + x] =
              (1 - ay) * ((1 - ax) * t[(gsize) y0 * e->w + x0] + ax * t[(gsize) y0 * e->w + x1])
            +      ay  * ((1 - ax) * t[(gsize) y1 * e->w + x0] + ax * t[(gsize) y1 * e->w + x1]);
        }
      }

  guided_filter (guide, e->tm, e->mw, e->mh, MAX (4, e->mw / 100), 1e-4f);
  for (gsize i = 0; i < n; i++)
    e->tm[i] = CLAMP (e->tm[i], 0.1f, 1.0f);

  g_free (guide);
}

/* the transmission at a full-size pixel, interpolated from the medium copy */
static inline gfloat
sample_t (const Estimate *e, gint x, gint y)
{
  gfloat fx = CLAMP ((x + 0.5f) / e->mf - 0.5f, 0.0f, e->mw - 1.0f);
  gfloat fy = CLAMP ((y + 0.5f) / e->mf - 0.5f, 0.0f, e->mh - 1.0f);
  gint   x0 = (gint) fx, y0 = (gint) fy;
  gint   x1 = MIN (x0 + 1, e->mw - 1), y1 = MIN (y0 + 1, e->mh - 1);
  gfloat ax = fx - x0, ay = fy - y0;
  const gfloat *t = e->tm;

  return (1 - ay) * ((1 - ax) * t[(gsize) y0 * e->mw + x0] + ax * t[(gsize) y0 * e->mw + x1])
       +      ay  * ((1 - ax) * t[(gsize) y1 * e->mw + x0] + ax * t[(gsize) y1 * e->mw + x1]);
}

/* a color map at a full-size pixel, interpolated from the small copy */
static inline void
sample_map (const Estimate *e, const gfloat *m, gint x, gint y, gfloat *a)
{
  gfloat fx = CLAMP ((x + 0.5f) / e->f - 0.5f, 0.0f, e->w - 1.0f);
  gfloat fy = CLAMP ((y + 0.5f) / e->f - 0.5f, 0.0f, e->h - 1.0f);
  gint   x0 = (gint) fx, y0 = (gint) fy;
  gint   x1 = MIN (x0 + 1, e->w - 1), y1 = MIN (y0 + 1, e->h - 1);
  gfloat ax = fx - x0, ay = fy - y0;
  gint   c;

  for (c = 0; c < 3; c++)
    a[c] = (1 - ay) * ((1 - ax) * m[((gsize) y0 * e->w + x0) * 3 + c] + ax * m[((gsize) y0 * e->w + x1) * 3 + c])
         +      ay  * ((1 - ax) * m[((gsize) y1 * e->w + x0) * 3 + c] + ax * m[((gsize) y1 * e->w + x1) * 3 + c]);
}

typedef struct
{
  GeglProperties *o;
  const Estimate *e;
  const gfloat   *in;
  gfloat         *out;     /* may be in: each pixel is read before it is written */
  gint            W;
  gboolean        show_t;  /* UNDERWATER_DEBUG=t: the transmission map */
} RowData;

static void
correct_rows (gsize offset, gsize count, gpointer user_data)
{
  RowData        *d = user_data;
  GeglProperties *o = d->o;
  const Estimate *e = d->e;
  gsize           y;
  gint            x, c;

  for (y = offset; y < offset + count; y++)
    for (x = 0; x < d->W; x++)
      {
        const gfloat *p = d->in + (y * d->W + x) * 4;
        gfloat       *q = d->out + (y * d->W + x) * 4;
        gfloat        t = sample_t (e, x, y);
        gfloat        v[3], a[3], k[3], m;

        /* 3.-4. the subject, without the veil */
        sample_map (e, e->wmap, x, y, a);
        sample_map (e, e->kmap, x, y, k);
        subject (o, a, p, t, v);

        /* 5. red and blue restoration where the light is ambient, for
         * subjects: open water in the distance must stay water, not turn
         * violet from red added to its blue */
        m = ambient_weight (v) * smoothstep (0.15f, 0.6f, t);
        restore (o, e, v, m);

        /* 6. white balance, eased back to neutral for near-white pixels so
         * that highlights do not turn magenta; 7. the part of the veil that
         * is kept, in a mix of the water's own color and neutral */
        {
          gfloat hi = MIN (v[0], MIN (v[1], v[2]));
          gfloat protect = smoothstep (0.6f, 1.0f, hi);
          gfloat veil = (1.0f - o->backscatter) * (1.0f - t);
          /* clipped in the photo (a torch, the sun): no color to correct */
          gfloat pmax = MAX (p[0], MAX (p[1], p[2]));
          gfloat pmid = MAX (MIN (p[0], p[1]), MIN (MAX (p[0], p[1]), p[2]));
          gfloat clipped = smoothstep (0.85f, 1.0f, pmid);
          /* the white balance is for subjects; in the distance there is
           * mostly water, whose red would be boosted to violet */
          gfloat near = smoothstep (0.15f, 0.6f, t);

          for (c = 0; c < 3; c++)
            {
              gfloat gain = 1.0f + near * (1.0f - protect) * (e->gain[c] - 1.0f);
              q[c] = v[c] * gain * (1.0f - veil) + k[c] * veil;
              q[c] = q[c] + clipped * (pmax - q[c]);
            }
        }
        q[3] = p[3];
        if (d->show_t)
          q[0] = q[1] = q[2] = t;
      }
}

static void
prepare (GeglOperation *operation)
{
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  /* the physics of absorption and scattering apply to linear light */
  const Babl *format = babl_format_with_space ("RGBA float", space);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
}

/* the corrections use statistics of the whole image, so every result
   needs all of the input, and is computed once for all of it; an input
   without bounds (such as gegl:color) is passed through */
static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  const GeglRectangle *in = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");

  return in && !gegl_rectangle_is_infinite_plane (in) ? *in : *roi;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  const GeglRectangle *in = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");

  return in && !gegl_rectangle_is_infinite_plane (in) ? *in : *roi;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  GeglProperties      *o      = GEGL_PROPERTIES (operation);
  const Babl          *format = gegl_operation_get_format (operation, "output");
  const GeglRectangle *whole  = gegl_operation_source_get_bounding_box (operation, "input");
  Estimate             e      = { 0, };
  RowData              rows;
  gsize                n;
  gfloat              *buf;
  gint64               t0, t1, t2, t3, t4;

  if (!whole || whole->width < 1 || whole->height < 1)
    return TRUE;
  if (gegl_rectangle_is_infinite_plane (whole))
    {
      gegl_buffer_copy (input, result, GEGL_ABYSS_NONE, output, result);
      return TRUE;
    }

  /* the whole image in one piece, 16 bytes a pixel; the result is written
   * over it, as each pixel only needs itself once the estimates are made.
   * Without the memory for it the photo is left as it is: an abort would
   * take GIMP down with it */
  n   = (gsize) whole->width * whole->height;
  buf = n <= G_MAXSIZE / (4 * sizeof (gfloat)) ? g_try_new (gfloat, n * 4) : NULL;
  if (!buf)
    {
      g_warning ("underwater:correct: not enough memory for an image of %d x %d",
                 whole->width, whole->height);
      gegl_buffer_copy (input, whole, GEGL_ABYSS_NONE, output, whole);
      return TRUE;
    }
  t0 = g_get_monotonic_time ();

  gegl_buffer_get (input, whole, 1.0, format, buf, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  t1 = g_get_monotonic_time ();

  estimate (o, format, buf, whole->width, whole->height, &e);
  refine (buf, whole->width, whole->height, &e);
  t2 = g_get_monotonic_time ();

  rows.o = o; rows.e = &e; rows.in = buf; rows.out = buf; rows.W = whole->width;
  rows.show_t = g_strcmp0 (g_getenv ("UNDERWATER_DEBUG"), "t") == 0;
  gegl_parallel_distribute_range (whole->height, 64, correct_rows, &rows);
  t3 = g_get_monotonic_time ();

  gegl_buffer_set (output, whole, 0, format, buf, GEGL_AUTO_ROWSTRIDE);
  t4 = g_get_monotonic_time ();
  if (g_getenv ("UNDERWATER_DEBUG"))
    g_printerr ("underwater: get %.2f s, estimate %.2f s, correct %.2f s, set %.2f s\n",
                (t1 - t0) / 1e6, (t2 - t1) / 1e6, (t3 - t2) / 1e6, (t4 - t3) / 1e6);

  g_free (e.kmap);
  g_free (e.wmap);
  g_free (e.tm);
  g_free (e.t);
  g_free (e.rgb);
  g_free (buf);
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->threaded = FALSE;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
    "name",            "underwater:correct",
    "title",           _("Underwater Color Correction"),
    "categories",      "color",
    "description",     _("Restores the colors of an underwater photo: "
                         "rebuilds the red light absorbed by the water, "
                         "removes the color cast of the water and the veil "
                         "of light it scatters back."),
    "gimp:menu-path",  "<Image>/Colors",
    "gimp:menu-label", _("Underwater Color Correction..."),
    NULL);
}

#endif
