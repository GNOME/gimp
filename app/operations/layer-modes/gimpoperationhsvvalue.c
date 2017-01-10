/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationvaluemode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
 *               2017 Øyvind Kolås <pippin@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl-plugin.h>
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "../operations-types.h"

#include "gimpoperationhsvvalue.h"


static gboolean gimp_operation_hsv_value_process (GeglOperation       *operation,
                                                  void                *in_buf,
                                                  void                *aux_buf,
                                                  void                *aux2_buf,
                                                  void                *out_buf,
                                                  glong                samples,
                                                  const GeglRectangle *roi,
                                                  gint                 level);


G_DEFINE_TYPE (GimpOperationHsvValue, gimp_operation_hsv_value,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_hsv_value_class_init (GimpOperationHsvValueClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:hsv-value",
                                 "description", "GIMP value mode operation",
                                 NULL);

  point_class->process = gimp_operation_hsv_value_process;
}

static void
gimp_operation_hsv_value_init (GimpOperationHsvValue *self)
{
}

static gboolean
gimp_operation_hsv_value_process (GeglOperation       *operation,
                                  void                *in_buf,
                                  void                *aux_buf,
                                  void                *aux2_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  gfloat opacity = GIMP_OPERATION_POINT_LAYER_MODE (operation)->opacity;

  return gimp_operation_hsv_value_process_pixels (in_buf, aux_buf, aux2_buf, out_buf, opacity, samples, roi, level);
}

gboolean
gimp_operation_hsv_value_process_pixels (gfloat              *in,
                                         gfloat              *layer,
                                         gfloat              *mask,
                                         gfloat              *out,
                                         gfloat               opacity,
                                         glong                samples,
                                         const GeglRectangle *roi,
                                         gint                 level)
{
  const gboolean has_mask = mask != NULL;

  while (samples--)
    {
      GimpHSV layer_hsv, out_hsv;
      GimpRGB layer_rgb = {layer[0], layer[1], layer[2]};
      GimpRGB out_rgb   = {in[0], in[1], in[2]};
      gfloat  comp_alpha, new_alpha;

      comp_alpha = MIN (in[ALPHA], layer[ALPHA]) * opacity;
      if (has_mask)
        comp_alpha *= *mask;

      new_alpha = in[ALPHA] + (1.0 - in[ALPHA]) * comp_alpha;

      if (comp_alpha && new_alpha)
        {
          gint   b;
          gfloat out_tmp[3];
          gfloat ratio = comp_alpha / new_alpha;

          gimp_rgb_to_hsv (&layer_rgb, &layer_hsv);
          gimp_rgb_to_hsv (&out_rgb, &out_hsv);

          out_hsv.v = layer_hsv.v;
          gimp_hsv_to_rgb (&out_hsv, &out_rgb);

          out_tmp[0] = out_rgb.r;
          out_tmp[1] = out_rgb.g;
          out_tmp[2] = out_rgb.b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = out_tmp[b] * ratio + in[b] * (1.0 - ratio);
            }
        }
      else
        {
          gint b;

          for (b = RED; b < ALPHA; b++)
            {
              out[b] = in[b];
            }
        }

      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;

      if (has_mask)
        mask++;
    }

  return TRUE;
}
