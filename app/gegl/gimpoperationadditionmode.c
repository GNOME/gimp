/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationadditionmode.c
 * Copyright (C) 2008 Michael Natterer <mitch@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <gegl.h>

#include "gegl-types.h"

#include "gimpoperationadditionmode.h"


static gboolean gimp_operation_addition_mode_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *aux_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi);


G_DEFINE_TYPE (GimpOperationAdditionMode, gimp_operation_addition_mode,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_addition_mode_class_init (GimpOperationAdditionModeClass *klass)
{
  GeglOperationClass          *operation_class = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *mode_class      = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  operation_class->name        = "gimp-addition-mode";
  operation_class->description = "GIMP addition mode operation";

  mode_class->process          = gimp_operation_addition_mode_process;
}

static void
gimp_operation_addition_mode_init (GimpOperationAdditionMode *self)
{
}

static gboolean
gimp_operation_addition_mode_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *aux_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi)
{
  gfloat *in    = in_buf;
  gfloat *layer = aux_buf;
  gfloat *out   = out_buf;

  while (samples--)
    {
      /* To be more mathematically correct we would have to either
       * adjust the formula for the resulting opacity or adapt the
       * other channels to the change in opacity. Compare to the
       * 'plus' compositiong operation in SVG 1.2.
       *
       * Since this doesn't matter for completely opaque layers, and
       * since consistency in how the alpha channel of layers is
       * interpreted is more important than mathematically correct
       * results, we don't bother.
       */
      out[RED]   = in[RED]   + layer[RED];
      out[GREEN] = in[GREEN] + layer[GREEN];
      out[BLUE]  = in[BLUE]  + layer[BLUE];
      out[ALPHA] = in[ALPHA] + layer[ALPHA] - in[ALPHA] * layer[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;
    }

  return TRUE;
}
