/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlchcolor.c
 * Copyright (C) 2015 Elle Stone <ellestone@ninedegreesbelow.com>
 *                    Massimo Valentini <mvalentini@src.gnome.org>
 *                    Thomas Manni <thomas.manni@free.fr>
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
#include "../operations-types.h"
#include "gimpoperationlchcolor.h"


static gboolean gimp_operation_lch_color_process (GeglOperation       *operation,
                                                  void                *in_buf,
                                                  void                *aux_buf,
                                                  void                *aux2_buf,
                                                  void                *out_buf,
                                                  glong                samples,
                                                  const GeglRectangle *roi,
                                                  gint                 level);


G_DEFINE_TYPE (GimpOperationLchColor, gimp_operation_lch_color,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

#define parent_class gimp_operation_lch_color_parent_class


static void
gimp_operation_lch_color_class_init (GimpOperationLchColorClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  operation_class->want_in_place = FALSE;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:lch-color",
                                 "description", "GIMP LCH color mode operation",
                                 NULL);

  point_class->process = gimp_operation_lch_color_process;
}

static void
gimp_operation_lch_color_init (GimpOperationLchColor *self)
{
}

static gboolean
gimp_operation_lch_color_process (GeglOperation       *operation,
                                  void                *in_buf,
                                  void                *aux_buf,
                                  void                *aux2_buf,
                                  void                *out_buf,
                                  glong                samples,
                                  const GeglRectangle *roi,
                                  gint                 level)
{
  GimpOperationPointLayerMode *gimp_op = GIMP_OPERATION_POINT_LAYER_MODE (operation);
  gfloat                       opacity = gimp_op->opacity;
  gboolean                     linear  = gimp_op->linear;

  return (linear ? gimp_operation_lch_color_process_pixels_linear :
                   gimp_operation_lch_color_process_pixels)
    (in_buf, aux_buf, aux2_buf, out_buf, opacity, samples, roi, level);
}

static void
color_pre_process (const Babl   *format,
                   const gfloat *in,
                   const gfloat *layer,
                   gfloat       *out,
                   glong         samples)
{
  gfloat tmp[4 * samples], *layer_lab = tmp;
  gint   i;

  babl_process (babl_fish (format, "CIE L alpha float"), in, &out[2 * samples], samples);
  babl_process (babl_fish (format, "CIE Lab alpha float"), layer, layer_lab, samples);

  for (i = 0; i < samples; ++i)
    {
      out[4 * i + 0] = out[2 * samples + 2 * i + 0];
      out[4 * i + 1] = layer_lab[4 * i + 1];
      out[4 * i + 2] = layer_lab[4 * i + 2];
      out[4 * i + 3] = out[2 * samples + 2 * i + 1];
    }

  babl_process (babl_fish ("CIE Lab alpha float", format), out, out, samples);
}

static void
color_post_process (const gfloat *in,
                    const gfloat *layer,
                    const gfloat *mask,
                    gfloat       *out,
                    gfloat        opacity,
                    glong         samples)
{
  while (samples--)
    {
      gfloat comp_alpha = layer[ALPHA] * opacity;

      if (mask)
        comp_alpha *= *mask++;

      if (comp_alpha != 0.0f)
        {
          out[RED]   = out[RED]   * comp_alpha + in[RED]   * (1.0f - comp_alpha);
          out[GREEN] = out[GREEN] * comp_alpha + in[GREEN] * (1.0f - comp_alpha);
          out[BLUE]  = out[BLUE]  * comp_alpha + in[BLUE]  * (1.0f - comp_alpha);
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
    }
}

gboolean
gimp_operation_lch_color_process_pixels_linear (gfloat              *in,
                                                gfloat              *layer,
                                                gfloat              *mask,
                                                gfloat              *out,
                                                gfloat               opacity,
                                                glong                samples,
                                                const GeglRectangle *roi,
                                                gint                 level)
{
  color_pre_process (babl_format ("RGBA float"), in, layer, out, samples);
  color_post_process (in, layer, mask, out, opacity, samples);

  return TRUE;
}

gboolean
gimp_operation_lch_color_process_pixels (gfloat              *in,
                                         gfloat              *layer,
                                         gfloat              *mask,
                                         gfloat              *out,
                                         gfloat               opacity,
                                         glong                samples,
                                         const GeglRectangle *roi,
                                         gint                 level)
{
  color_pre_process (babl_format ("R'G'B'A float"), in, layer, out, samples);
  color_post_process (in, layer, mask, out, opacity, samples);

  return TRUE;
}
