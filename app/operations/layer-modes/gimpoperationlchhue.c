/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlchhue.c
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
#include "gimpoperationlchhue.h"
#include "gimpblendcomposite.h"


static gboolean gimp_operation_lch_hue_process (GeglOperation       *operation,
                                                void                *in_buf,
                                                void                *aux_buf,
                                                void                *aux2_buf,
                                                void                *out_buf,
                                                glong                samples,
                                                const GeglRectangle *roi,
                                                gint                 level);


G_DEFINE_TYPE (GimpOperationLchHue, gimp_operation_lch_hue,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

#define parent_class gimp_operation_lch_hue_parent_class


static void
gimp_operation_lch_hue_class_init (GimpOperationLchHueClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  operation_class->want_in_place = FALSE;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:lch-hue",
                                 "description", "GIMP LCH hue mode operation",
                                 NULL);

  point_class->process = gimp_operation_lch_hue_process;
}

static void
gimp_operation_lch_hue_init (GimpOperationLchHue *self)
{
}

static gboolean
gimp_operation_lch_hue_process (GeglOperation       *operation,
                                void                *in_buf,
                                void                *aux_buf,
                                void                *aux2_buf,
                                void                *out_buf,
                                glong                samples,
                                const GeglRectangle *roi,
                                gint                 level)
{
  GimpOperationPointLayerMode *layer_mode = (GimpOperationPointLayerMode*)operation;

  return (layer_mode->linear ? gimp_operation_lch_hue_process_pixels_linear :
                   gimp_operation_lch_hue_process_pixels)
    (in_buf, aux_buf, aux2_buf, out_buf, layer_mode->opacity, samples, roi, level, layer_mode->blend_trc, layer_mode->composite_trc, layer_mode->composite_mode);
}

static void
hue_pre_process (const Babl   *from_fish,
                 const Babl   *to_fish,
                 const gfloat *in,
                 const gfloat *layer,
                 gfloat       *out,
                 glong         samples)
{
  gint i;
  gfloat tmp[4 * samples], *layer_lab = tmp;

  babl_process (from_fish, in, out, samples);
  babl_process (from_fish, layer, layer_lab, samples);

  for (i = 0; i < samples; ++i)
    {
      gfloat A2 = layer_lab[4 * i + 1];
      gfloat B2 = layer_lab[4 * i + 2];
      gfloat c2 = hypotf (A2, B2);

      if (c2 > 0.1f)
        {
          gfloat A1 = out[4 * i + 1];
          gfloat B1 = out[4 * i + 2];
          gfloat c1 = hypotf (A1, B1);
          gfloat A = c1 * A2 / c2;
          gfloat B = c1 * B2 / c2;

          out[4 * i + 1] = A;
          out[4 * i + 2] = B;
        }
    }

  babl_process (to_fish, out, out, samples);
}

/* XXX: this should be removed along with _pre_process */
gboolean
gimp_operation_lch_hue_process_pixels (gfloat                *in,
                                       gfloat                *layer,
                                       gfloat                *mask,
                                       gfloat                *out,
                                       gfloat                 opacity,
                                       glong                  samples,
                                       const GeglRectangle   *roi,
                                       gint                   level,
                                       GimpLayerBlendTRC      blend_trc,
                                       GimpLayerBlendTRC      composite_trc,
                                       GimpLayerCompositeMode composite_mode)
{
  static const Babl *from_fish = NULL;
  static const Babl *to_fish = NULL;
  
  if (!from_fish)
    from_fish = babl_fish ("R'G'B'A float", "CIE Lab alpha float");
  if (!to_fish)
     to_fish = babl_fish ("CIE Lab alpha float", "R'G'B'A float");

  hue_pre_process (from_fish, to_fish, in, layer, out, samples);
  compfun_src_atop (in, layer, mask, opacity, out, samples);

  return TRUE;
}


gboolean
gimp_operation_lch_hue_process_pixels_linear (gfloat                *in,
                                              gfloat                *layer,
                                              gfloat                *mask,
                                              gfloat                *out,
                                              gfloat                 opacity,
                                              glong                  samples,
                                              const GeglRectangle   *roi,
                                              gint                   level,
                                              GimpLayerBlendTRC      blend_trc,
                                              GimpLayerBlendTRC      composite_trc,
                                              GimpLayerCompositeMode composite_mode)
{
  gimp_composite_blend (in, layer, mask, out, opacity, samples, blend_trc,
                        composite_trc, composite_mode, blendfun_lch_hue);
  return TRUE;
}
