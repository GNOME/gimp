/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlchchroma.c
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
#include <math.h>
#include "../operations-types.h"
#include "gimpoperationlchchroma.h"

static gboolean gimp_operation_lch_chroma_process (GeglOperation       *operation,
                                                   void                *in_buf,
                                                   void                *aux_buf,
                                                   void                *aux2_buf,
                                                   void                *out_buf,
                                                   glong                samples,
                                                   const GeglRectangle *roi,
                                                   gint                 level);


G_DEFINE_TYPE (GimpOperationLchChroma, gimp_operation_lch_chroma,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

#define parent_class gimp_operation_lch_chroma_parent_class


static void
gimp_operation_lch_chroma_class_init (GimpOperationLchChromaClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  operation_class->want_in_place = FALSE;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:lch-chroma",
                                 "description", "GIMP LCH chroma mode operation",
                                 NULL);

  point_class->process = gimp_operation_lch_chroma_process;
}

static void
gimp_operation_lch_chroma_init (GimpOperationLchChroma *self)
{
}

static gboolean
gimp_operation_lch_chroma_process (GeglOperation       *operation,
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

  return (linear ? gimp_operation_lch_chroma_process_pixels_linear :
                   gimp_operation_lch_chroma_process_pixels)
    (in_buf, aux_buf, aux2_buf, out_buf, opacity, samples, roi, level);
}

static void
chroma_pre_process (const Babl   *from_fish,
                    const Babl   *to_fish,
                    const gfloat *in,
                    const gfloat *layer,
                    gfloat       *out,
                    glong         samples)
{
  gfloat tmp[4 * samples], *layer_lab = tmp;
  gint   i;

  babl_process (from_fish, in, out, samples);
  babl_process (from_fish, layer, layer_lab, samples);

  for (i = 0; i < samples; ++i)
    {
      gfloat A1 = out[4 * i + 1];
      gfloat B1 = out[4 * i + 2];
      gfloat c1 = hypotf (A1, B1);

      if (c1 != 0)
        {
          gfloat A2 = layer_lab[4 * i + 1];
          gfloat B2 = layer_lab[4 * i + 2];
          gfloat c2 = hypotf (A2, B2);
          gfloat A  = c2 * A1 / c1;
          gfloat B  = c2 * B1 / c1;

          out[4 * i + 1] = A;
          out[4 * i + 2] = B;
        }
    }

  babl_process (to_fish, out, out, samples);
}

gboolean
gimp_operation_lch_chroma_process_pixels_linear (gfloat              *in,
                                                 gfloat              *layer,
                                                 gfloat              *mask,
                                                 gfloat              *out,
                                                 gfloat               opacity,
                                                 glong                samples,
                                                 const GeglRectangle *roi,
                                                 gint                 level)
{
  static const Babl *from_fish;
  static const Babl *to_fish;
  
  if (!from_fish)
    from_fish = babl_fish ("RGBA float", "CIE Lab alpha float");
  if (!to_fish)
     to_fish = babl_fish ("CIE Lab alpha float", "RGBA float");

  chroma_pre_process (from_fish, to_fish, in, layer, out, samples);
  gimp_operation_layer_composite (in, layer, mask, out, opacity, samples);

  return TRUE;
}

gboolean
gimp_operation_lch_chroma_process_pixels (gfloat              *in,
                                          gfloat              *layer,
                                          gfloat              *mask,
                                          gfloat              *out,
                                          gfloat               opacity,
                                          glong                samples,
                                          const GeglRectangle *roi,
                                          gint                 level)
{
  static const Babl *from_fish = NULL;
  static const Babl *to_fish = NULL;
  
  if (!from_fish)
    from_fish = babl_fish ("R'G'B'A float", "CIE Lab alpha float");
  if (!to_fish)
     to_fish = babl_fish ("CIE Lab alpha float", "R'G'B'A float");

  chroma_pre_process (from_fish, to_fish, in, layer, out, samples);
  gimp_operation_layer_composite (in, layer, mask, out, opacity, samples);

  return TRUE;
}

