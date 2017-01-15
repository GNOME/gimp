/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationlchlightness.c
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

#include "gimpoperationlchlightness.h"
#include "gimpblendcomposite.h"


static gboolean gimp_operation_lch_lightness_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *aux_buf,
                                                      void                *aux2_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi,
                                                      gint                 level);

G_DEFINE_TYPE (GimpOperationLchLightness, gimp_operation_lch_lightness,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)

#define parent_class gimp_operation_lch_lightness_parent_class


static void
gimp_operation_lch_lightness_class_init (GimpOperationLchLightnessClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  operation_class->want_in_place = FALSE;

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:lch-lightness",
                                 "description", "GIMP LCH lightness mode operation",
                                 NULL);

  point_class->process = gimp_operation_lch_lightness_process;
}

static void
gimp_operation_lch_lightness_init (GimpOperationLchLightness *self)
{
}

static gboolean
gimp_operation_lch_lightness_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *aux_buf,
                                      void                *aux2_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi,
                                      gint                 level)
{
  GimpOperationPointLayerMode *layer_mode = (GimpOperationPointLayerMode*)operation;
  return (layer_mode->linear ? gimp_operation_lch_lightness_process_pixels_linear : gimp_operation_lch_lightness_process_pixels)
    (in_buf, aux_buf, aux2_buf, out_buf, layer_mode->opacity, samples, roi, level, layer_mode->blend_trc, layer_mode->composite_trc, layer_mode->composite_mode);
}

static void
lightness_pre_process (const Babl   *from_fish_la,
                       const Babl   *from_fish_laba,
                       const Babl   *to_fish,
                       const gfloat *in,
                       const gfloat *layer,
                       gfloat       *out,
                       glong         samples)
{
  gfloat lightness_alpha[samples * 2];
  gint   i;

  babl_process (from_fish_laba, in, out, samples);
  babl_process (from_fish_la, layer, lightness_alpha, samples);

  for (i = 0; i < samples; ++i)
    out[4 * i] = lightness_alpha[2 * i];

  babl_process (to_fish, out, out, samples);
}

/* XXX: this should be remove along with _pre_process */
gboolean
gimp_operation_lch_lightness_process_pixels (gfloat                *in,
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
  static const Babl *from_fish_laba = NULL;
  static const Babl *from_fish_la = NULL;
  static const Babl *to_fish = NULL;
  
  if (!from_fish_laba)
    from_fish_laba  = babl_fish ("R'G'B'A float", "CIE Lab alpha float");
  if (!from_fish_la)
    from_fish_la =  babl_fish ("R'G'B'A float", "CIE L alpha float");
  if (!to_fish)
     to_fish = babl_fish ("CIE Lab alpha float", "R'G'B'A float");


  lightness_pre_process (from_fish_la, from_fish_laba, to_fish, in, layer, out, samples);
  compfun_src_atop (in, layer, mask, opacity, out, samples);

  return TRUE;
}


gboolean
gimp_operation_lch_lightness_process_pixels_linear (gfloat                *in,
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
  gimp_composite_blend (in, layer, mask, out, opacity, samples,
     blend_trc, composite_trc, composite_mode, blendfun_lch_lightness);
  return TRUE;
}
