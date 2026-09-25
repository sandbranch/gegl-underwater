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
 * SKELETON: the properties are in place, the processing is not yet;
 * the operation passes its input through unchanged.
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
   needs all of the input, and is computed once for all of it */
static GeglRectangle
get_required_for_output (GeglOperation       *operation,
                         const gchar         *input_pad,
                         const GeglRectangle *roi)
{
  const GeglRectangle *in = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");

  return in ? *in : *roi;
}

static GeglRectangle
get_cached_region (GeglOperation       *operation,
                   const GeglRectangle *roi)
{
  const GeglRectangle *in = gegl_operation_source_get_bounding_box (operation,
                                                                     "input");

  return in ? *in : *roi;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *result,
         gint                 level)
{
  /* SKELETON: pass the input through, see PLAN.md */
  gegl_buffer_copy (input, result, GEGL_ABYSS_NONE, output, result);
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
