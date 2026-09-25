/*
 * Marine snow removal GEGL operation
 *
 * marine-snow.c
 * Copyright 2026 by David
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Removes marine snow from an underwater photo: the small bright specks of
 * particles in the water lit by a strobe or a video light. Only the specks
 * are replaced, so the rest of the photo keeps all its detail.
 *
 * After Farhadifard, Radolko and Freiherr von Lukas, "Single Image Marine
 * Snow Removal based on a Supervised Median Filtering Scheme", VISAPP 2017:
 * a speck is brighter than its surroundings, isolated, and nearly
 * colorless; it is replaced by the median of the pixels around it that are
 * not snow. Instead of their test of every overlapping patch, which is
 * slow, "bright and small" is found with a morphological top-hat (the image
 * minus its grayscale opening: whatever is brighter than its surroundings
 * and smaller than the opening), and "isolated" by counting such bright
 * spots in a larger window, as their density test does: in sand or on
 * scales there are many close together, marine snow is scattered. See
 * docs/research.md.
 */

#include <glib/gi18n-lib.h>

#ifdef GEGL_PROPERTIES

property_int (size, _("Speck size"), 5)
  description (_("The largest specks to remove, in pixels across. Bright "
                 "things larger than this are kept."))
  value_range (1, 40)
  ui_range (1, 20)
  ui_meta ("unit", "pixel-distance")

property_double (threshold, _("Threshold"), 0.08)
  description (_("How much brighter than its surroundings a speck must "
                 "be. Lower finds fainter specks."))
  value_range (0.0, 1.0)
  ui_range (0.01, 0.5)
  ui_digits (3)

property_double (texture_guard, _("Texture protection"), 0.5)
  description (_("Keep bright spots that have many others like them close "
                 "by, as grains in sand or highlights on scales and coral: "
                 "marine snow is scattered. 0 turns it off."))
  value_range (0.0, 1.0)
  ui_digits (2)

property_double (color_tolerance, _("Color tolerance"), 0.35)
  description (_("How colorful the light of a speck may be: specks lit by "
                 "a strobe add nearly white light. 1 removes bright specks "
                 "of any color."))
  value_range (0.0, 1.0)
  ui_digits (2)

property_boolean (show_mask, _("Show what is removed"), FALSE)
  description (_("Paint the pixels that are replaced in magenta, to tune "
                 "the settings"))

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     marine_snow
#define GEGL_OP_C_SOURCE marine-snow.c

#include "gegl-op.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* the opening removes what is smaller than 2r+1 pixels across */
static gint
opening_radius (GeglProperties *o)
{
  return (o->size + 1) / 2;
}

/* the window around a speck that its replacement and the variation of its
 * surroundings are taken from */
static gint
window_radius (GeglProperties *o)
{
  return opening_radius (o) + 2;
}

/* the window in which bright spots are counted, for the density test */
static gint
density_radius (GeglProperties *o)
{
  return 4 * opening_radius (o) + 2;
}

static gint
margin (GeglProperties *o)
{
  /* the mask is needed within the window around each output pixel; each
   * mask pixel counts the spots in its density window; the opening there
   * needs the input within twice its radius */
  return window_radius (o) + density_radius (o) + 2 * opening_radius (o) + 1;
}

static void
prepare (GeglOperation *operation)
{
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglProperties          *o    = GEGL_PROPERTIES (operation);
  const Babl              *space = gegl_operation_get_source_space (operation, "input");
  /* thresholds in perceptual units behave the way the eye sees */
  const Babl              *format = babl_format_with_space ("R'G'B'A float", space);
  gint                     m    = margin (o);

  area->left = area->right = area->top = area->bottom = m;

  gegl_operation_set_format (operation, "input",  format);
  gegl_operation_set_format (operation, "output", format);
}

/* the result has the size of the input (area filters grow by default) */
static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  const GeglRectangle *in = gegl_operation_source_get_bounding_box (operation, "input");

  return in ? *in : (GeglRectangle) { 0, 0, 0, 0 };
}

/* min (erode) or max (dilate) over a square of radius r, separable */
static void
min_max_filter (const gfloat *src,
                gfloat       *dst,
                gfloat       *tmp,
                gint          w,
                gint          h,
                gint          r,
                gboolean      is_max)
{
  gint x, y, i;

  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      {
        const gfloat *row = src + (gsize) y * w;
        gint          x0  = MAX (x - r, 0);
        gint          x1  = MIN (x + r, w - 1);
        gfloat        v   = row[x0];

        for (i = x0 + 1; i <= x1; i++)
          v = is_max ? MAX (v, row[i]) : MIN (v, row[i]);
        tmp[(gsize) y * w + x] = v;
      }

  for (y = 0; y < h; y++)
    {
      gint y0 = MAX (y - r, 0);
      gint y1 = MIN (y + r, h - 1);

      for (x = 0; x < w; x++)
        {
          gfloat v = tmp[(gsize) y0 * w + x];

          for (i = y0 + 1; i <= y1; i++)
            {
              gfloat t = tmp[(gsize) i * w + x];
              v = is_max ? MAX (v, t) : MIN (v, t);
            }
          dst[(gsize) y * w + x] = v;
        }
    }
}

static int
compare_floats (const void *a,
                const void *b)
{
  gfloat fa = *(const gfloat *) a, fb = *(const gfloat *) b;

  return (fa > fb) - (fa < fb);
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o      = GEGL_PROPERTIES (operation);
  const Babl     *format = gegl_operation_get_format (operation, "output");
  const gint      m      = margin (o);
  const gint      r      = opening_radius (o);
  const gint      wr     = window_radius (o);
  GeglRectangle   reg    = { roi->x - m, roi->y - m, roi->width + 2 * m, roi->height + 2 * m };
  const gint      w      = reg.width, h = reg.height;
  const gsize     n      = (gsize) w * h;
  gfloat         *in     = g_new (gfloat, n * 4);
  gfloat         *luma   = g_new (gfloat, n);
  gfloat         *open   = g_new (gfloat, n);
  gfloat         *open_c = g_new (gfloat, n * 3);
  gfloat         *tmp    = g_new (gfloat, n);
  const gint      dr     = density_radius (o);
  /* spots per pixel: marine snow is about one per 1000 pixels or fewer,
   * sand or scales have one every few dozen */
  const gfloat    density_limit = 0.004f + 0.03f * (1.0f - o->texture_guard);
  guint32        *count  = g_new0 (guint32, (gsize) (w + 1) * (h + 1));
  guint8         *cand   = g_new0 (guint8, n);
  guint8         *mask   = g_new0 (guint8, n);
  guint8         *grown  = g_new0 (guint8, n);
  gfloat         *out    = g_new (gfloat, (gsize) roi->width * roi->height * 4);
  const gint      win    = (2 * wr + 1) * (2 * wr + 1);
  gfloat         *vals   = g_new (gfloat, (gsize) win * 3);
  gfloat         *dist   = g_new (gfloat, win);
  gint           *order  = g_new (gint, win);
  gsize           i;
  gint            x, y, c;

  gegl_buffer_get (input, &reg, 1.0, format, in, GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_CLAMP);

  for (i = 0; i < n; i++)
    luma[i] = 0.2126f * in[i * 4] + 0.7152f * in[i * 4 + 1] + 0.0722f * in[i * 4 + 2];

  /* grayscale opening: what is left when everything smaller than the
   * specks is gone */
  {
    gfloat *eroded = g_new (gfloat, n);
    gfloat *chan   = g_new (gfloat, n);

    min_max_filter (luma, eroded, tmp, w, h, r, FALSE);
    min_max_filter (eroded, open, tmp, w, h, r, TRUE);

    /* the opening of each channel: the color behind the specks */
    for (c = 0; c < 3; c++)
      {
        for (i = 0; i < n; i++)
          chan[i] = in[i * 4 + c];
        min_max_filter (chan, eroded, tmp, w, h, r, FALSE);
        min_max_filter (eroded, chan, tmp, w, h, r, TRUE);
        for (i = 0; i < n; i++)
          open_c[i * 3 + c] = chan[i];
      }
    g_free (chan);
    g_free (eroded);
  }

  /* candidates: bright and small (top-hat), and the light they add to
   * what is behind them nearly colorless: marine snow is strobe light
   * scattered by particles, added on top of the water or the subject */
  for (i = 0; i < n; i++)
    {
      const gfloat *p = in + i * 4;
      gfloat        e[3], hi, lo, sat;

      if (luma[i] - open[i] <= o->threshold)
        continue;

      for (c = 0; c < 3; c++)
        e[c] = MAX (p[c] - open_c[i * 3 + c], 0.0f);
      hi  = MAX (e[0], MAX (e[1], e[2]));
      lo  = MIN (e[0], MIN (e[1], e[2]));
      sat = hi > 1e-6f ? (hi - lo) / hi : 0.0f;
      if (o->color_tolerance < 1.0 && sat > o->color_tolerance)
        continue;

      cand[i] = 1;
    }

  /* an integral image of the spots (the brightest pixel of each
   * candidate), to count them around each candidate */
  for (y = 0; y < h; y++)
    {
      guint32 rs = 0;

      for (x = 0; x < w; x++)
        {
          gsize  j    = (gsize) y * w + x;
          gfloat t    = luma[j] - open[j];
          gint   peak = cand[j];
          gint   dx, dy;

          for (dy = -1; dy <= 1 && peak; dy++)
            for (dx = -1; dx <= 1 && peak; dx++)
              {
                gint xx = x + dx, yy = y + dy;

                if ((dx || dy) && xx >= 0 && yy >= 0 && xx < w && yy < h)
                  {
                    gsize k = (gsize) yy * w + xx;
                    gfloat tk = luma[k] - open[k];

                    /* ties go to the first pixel in reading order */
                    if (tk > t || (tk == t && k < j))
                      peak = 0;
                  }
              }
          rs += peak;
          count[(gsize) (y + 1) * (w + 1) + x + 1] = count[(gsize) y * (w + 1) + x + 1] + rs;
        }
    }

  /* snow is scattered: a candidate among many other spots is texture */
  for (y = 0; y < h; y++)
    for (x = 0; x < w; x++)
      {
        gsize j = (gsize) y * w + x;

        if (!cand[j])
          continue;

        if (o->texture_guard > 0.0)
          {
            gint x0 = MAX (x - dr, 0), x1 = MIN (x + dr + 1, w);
            gint y0 = MAX (y - dr, 0), y1 = MIN (y + dr + 1, h);
            guint32 c = count[(gsize) y1 * (w + 1) + x1] - count[(gsize) y0 * (w + 1) + x1]
                      - count[(gsize) y1 * (w + 1) + x0] + count[(gsize) y0 * (w + 1) + x0];

            if ((gfloat) c / ((x1 - x0) * (y1 - y0)) > density_limit)
              continue;
          }

        mask[j] = 1;
      }

  /* each speck takes its soft edge along, as long as the pixels there
   * still stand out a little (hysteresis, as in edge detection) */
  {
    const gfloat low = 0.3f * o->threshold;
    gint         step;

    for (step = 0; step < r + 1; step++)
      {
        gboolean changed = FALSE;

        memcpy (grown, mask, n);
        for (y = 1; y < h - 1; y++)
          for (x = 1; x < w - 1; x++)
            {
              gsize j = (gsize) y * w + x;

              if (mask[j] || luma[j] - open[j] <= low)
                continue;
              if (mask[j - 1] || mask[j + 1] || mask[j - w] || mask[j + w])
                {
                  grown[j] = 1;
                  changed = TRUE;
                }
            }
        memcpy (mask, grown, n);
        if (!changed)
          break;
      }
    memset (grown, 0, n);
  }

  /* grow by a pixel: the soft edge of a speck goes with it, and stays out
   * of the median (the dark rim of the paper) */
  for (y = 1; y < h - 1; y++)
    for (x = 1; x < w - 1; x++)
      {
        gsize j = (gsize) y * w + x;

        if (mask[j] || mask[j - 1] || mask[j + 1] || mask[j - w] || mask[j + w] ||
            mask[j - w - 1] || mask[j - w + 1] || mask[j + w - 1] || mask[j + w + 1])
          grown[j] = 1;
      }

  /* replace each speck pixel by the median of the pixels around it that
   * are not snow and look like what is behind it (close to the color of
   * the opening there), so that a speck on a small dark detail (an eye, a
   * hole in coral) keeps that detail */
  for (y = 0; y < roi->height; y++)
    for (x = 0; x < roi->width; x++)
      {
        gint          rx = x + m, ry = y + m;
        gsize         j  = (gsize) ry * w + rx;
        const gfloat *p  = in + j * 4;
        gfloat       *d  = out + ((gsize) y * roi->width + x) * 4;
        gint          count = 0, dx, dy;

        memcpy (d, p, 4 * sizeof (gfloat));

        if (!grown[j])
          continue;

        if (o->show_mask)
          {
            d[0] = 1.0f; d[1] = 0.0f; d[2] = 1.0f;
            continue;
          }

        {
          const gfloat *bg = open_c + j * 3;
          gint          keep, a;

          for (dy = -wr; dy <= wr; dy++)
            for (dx = -wr; dx <= wr; dx++)
              {
                gsize k = (gsize) (ry + dy) * w + (rx + dx);
                gfloat d2 = 0.0f;

                if (grown[k])
                  continue;
                for (c = 0; c < 3; c++)
                  {
                    gfloat v = in[k * 4 + c];

                    vals[count * 3 + c] = v;
                    d2 += (v - bg[c]) * (v - bg[c]);
                  }
                dist[count] = d2;
                order[count] = count;
                count++;
              }

          if (count < 3)
            continue;

          /* the neighbors that look like what is behind the speck */
          keep = 0;
          for (a = 0; a < count; a++)
            if (dist[a] < 0.15f * 0.15f)
              order[keep++] = a;

          /* none (the speck covers all of a small detail): the opening is
           * the best guess */
          if (keep == 0)
            {
              for (c = 0; c < 3; c++)
                d[c] = bg[c];
              continue;
            }

          for (c = 0; c < 3; c++)
            {
              gfloat sel[win];

              for (a = 0; a < keep; a++)
                sel[a] = vals[order[a] * 3 + c];
              qsort (sel, keep, sizeof (gfloat), compare_floats);
              d[c] = keep % 2 ? sel[keep / 2] : 0.5f * (sel[keep / 2 - 1] + sel[keep / 2]);
            }
        }
      }

  gegl_buffer_set (output, roi, 0, format, out, GEGL_AUTO_ROWSTRIDE);

  g_free (order);
  g_free (dist);
  g_free (vals);
  g_free (out);
  g_free (grown);
  g_free (mask);
  g_free (cand);
  g_free (count);
  g_free (tmp);
  g_free (open_c);
  g_free (open);
  g_free (luma);
  g_free (in);

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass       *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class    = GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  filter_class->process    = process;

  gegl_operation_class_set_keys (operation_class,
    "name",            "underwater:marine-snow",
    "title",           _("Remove Marine Snow"),
    "categories",      "enhance:noise-reduction",
    "description",     _("Removes marine snow from an underwater photo: the "
                         "small bright specks of particles lit by a strobe. "
                         "Only the specks are replaced, by the water or "
                         "subject around them."),
    "gimp:menu-path",  "<Image>/Filters/Enhance",
    "gimp:menu-label", _("Remove Marine Snow..."),
    NULL);
}

#endif
