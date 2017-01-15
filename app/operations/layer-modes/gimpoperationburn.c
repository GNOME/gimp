/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationburnmode.c
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

#include "../operations-types.h"

#include "gimpoperationburn.h"
#include "gimpblendcomposite.h"

static gboolean gimp_operation_burn_process (GeglOperation       *operation,
                                             void                *in_buf,
                                             void                *aux_buf,
                                             void                *aux2_buf,
                                             void                *out_buf,
                                             glong                samples,
                                             const GeglRectangle *roi,
                                             gint                 level);


G_DEFINE_TYPE (GimpOperationBurn, gimp_operation_burn,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_burn_class_init (GimpOperationBurnClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:burn",
                                 "description", "GIMP burn mode operation",
                                 NULL);

  point_class->process = gimp_operation_burn_process;
}

static void
gimp_operation_burn_init (GimpOperationBurn *self)
{
}

static gboolean
gimp_operation_burn_process (GeglOperation       *operation,
                             void                *in_buf,
                             void                *aux_buf,
                             void                *aux2_buf,
                             void                *out_buf,
                             glong                samples,
                             const GeglRectangle *roi,
                             gint                 level)
{
  GimpOperationPointLayerMode *layer_mode = (void*)operation;

  return gimp_operation_burn_process_pixels (in_buf, aux_buf, aux2_buf,
                  out_buf, layer_mode->opacity, samples, roi, level,
                  layer_mode->blend_trc, layer_mode->composite_mode,
                  layer_mode->composite_mode);
} 


gboolean
gimp_operation_burn_process_pixels (gfloat                *in,
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
                        composite_trc, composite_mode, blendfun_burn);
  return TRUE;
}

