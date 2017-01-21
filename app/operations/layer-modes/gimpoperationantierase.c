/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationantierase.c
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

#include <gegl-plugin.h>

#include "../operations-types.h"

#include "gimpoperationantierase.h"


G_DEFINE_TYPE (GimpOperationAntiErase, gimp_operation_anti_erase,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_anti_erase_class_init (GimpOperationAntiEraseClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:anti-erase",
                                 "description", "GIMP anti erase mode operation",
                                 NULL);

  point_class->process = gimp_operation_anti_erase_process;
}

static void
gimp_operation_anti_erase_init (GimpOperationAntiErase *self)
{
}

gboolean
gimp_operation_anti_erase_process (GeglOperation       *op,
                                   void                *in_p,
                                   void                *layer_p,
                                   void                *mask_p,
                                   void                *out_p,
                                   glong                samples,
                                   const GeglRectangle *roi,
                                   gint                 level)
{
  GimpOperationLayerMode *layer_mode = (gpointer) op;
  gfloat                 *in         = in_p;
  gfloat                 *out        = out_p;
  gfloat                 *layer      = layer_p;
  gfloat                 *mask       = mask_p;
  gfloat                  opacity    = layer_mode->opacity;
  const gboolean          has_mask   = mask != NULL;

  while (samples--)
    {
      gfloat value = opacity;
      gint   b;

      if (has_mask)
        value *= *mask;

      out[ALPHA] = in[ALPHA] + (1.0 - in[ALPHA]) * layer[ALPHA] * value;

      for (b = RED; b < ALPHA; b++)
        {
          out[b] = in[b];
        }

      in    += 4;
      layer += 4;
      out   += 4;

      if (has_mask)
        mask++;
    }

  return TRUE;
}
