/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationcolorerase.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *               2012 Ville Sokk <ville.sokk@gmail.com>
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

#include <cairo.h>
#include <gegl-plugin.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "libgimpcolor/gimpcolor.h"

#include "../operations-types.h"

#include "gimpoperationcolorerase.h"


static gboolean gimp_operation_color_erase_process (GeglOperation       *operation,
                                                    void                *in_buf,
                                                    void                *aux_buf,
                                                    void                *aux2_buf,
                                                    void                *out_buf,
                                                    glong                samples,
                                                    const GeglRectangle *roi,
                                                    gint                 level);


G_DEFINE_TYPE (GimpOperationColorErase, gimp_operation_color_erase,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_color_erase_class_init (GimpOperationColorEraseClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:color-erase",
                                 "description", "GIMP color erase mode operation",
                                 NULL);

  point_class->process = gimp_operation_color_erase_process;
}

static void
gimp_operation_color_erase_init (GimpOperationColorErase *self)
{
}

static gboolean
gimp_operation_color_erase_process (GeglOperation       *operation,
                                    void                *in_buf,
                                    void                *aux_buf,
                                    void                *aux2_buf,
                                    void                *out_buf,
                                    glong                samples,
                                    const GeglRectangle *roi,
                                    gint                 level)
{
  gfloat opacity = GIMP_OPERATION_POINT_LAYER_MODE (operation)->opacity;

  return gimp_operation_color_erase_process_pixels (in_buf, aux_buf, aux2_buf, out_buf, opacity, samples, roi, level);
}

gboolean
gimp_operation_color_erase_process_pixels (gfloat              *in,
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
      gfloat  layer_alpha;
      GimpRGB bgcolor, color, alpha;

      layer_alpha = layer[ALPHA] * opacity;
      if (has_mask)
        layer_alpha *= *mask;

      gimp_rgba_set (&color, in[0], in[1], in[2], in[3]);
      gimp_rgba_set (&bgcolor, layer[0], layer[1], layer[2], layer_alpha);

      /* start of helper function copied from legacy 8-bit blending code */
      alpha.a = color.a;

      if (bgcolor.r < 0.0001)
        alpha.r = color.r;
      else if (GEGL_FLOAT_EQUAL (color.r, bgcolor.r))
        alpha.r = 0.0;
      else if (color.r > bgcolor.r)
        alpha.r = (color.r - bgcolor.r) / (1.0 - bgcolor.r);
      else
        alpha.r = (bgcolor.r - color.r) / bgcolor.r;

      if (bgcolor.g < 0.0001)
        alpha.g = color.g;
      else if (GEGL_FLOAT_EQUAL (color.g, bgcolor.g))
        alpha.g = 0.0;
      else if ( color.g > bgcolor.g )
        alpha.g = (color.g - bgcolor.g) / (1.0 - bgcolor.g);
      else
        alpha.g = (bgcolor.g - color.g) / (bgcolor.g);

      if (bgcolor.b < 0.0001)
        alpha.b = color.b;
      else if (GEGL_FLOAT_EQUAL (color.b, bgcolor.b))
        alpha.b = 0.0;
      else if ( color.b > bgcolor.b )
        alpha.b = (color.b - bgcolor.b) / (1.0 - bgcolor.b);
      else
        alpha.b = (bgcolor.b - color.b) / (bgcolor.b);

      if (alpha.r > alpha.g)
        {
          if (alpha.r > alpha.b)
            {
              color.a = alpha.r;
            }
          else
            {
              color.a = alpha.b;
            }
        }
      else if (alpha.g > alpha.b)
        {
          color.a = alpha.g;
        }
      else
        {
          color.a = alpha.b;
        }

      color.a = (1.0 - bgcolor.a) + (color.a * bgcolor.a);

      if (color.a > 0.0001)
        {
          color.r = (color.r - bgcolor.r) / color.a + bgcolor.r;
          color.g = (color.g - bgcolor.g) / color.a + bgcolor.g;
          color.b = (color.b - bgcolor.b) / color.a + bgcolor.b;

          color.a *= alpha.a;
        }
      /* end of helper function */

      out[0] = color.r;
      out[1] = color.g;
      out[2] = color.b;
      out[3] = color.a;

      in    += 4;
      layer += 4;
      out   += 4;

      if (has_mask)
        mask++;
    }

  return TRUE;
}
