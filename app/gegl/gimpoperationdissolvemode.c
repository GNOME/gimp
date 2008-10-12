/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationdissolvemode.c
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

#include "gimpoperationdissolvemode.h"


static gboolean gimp_operation_dissolve_mode_process (GeglOperation       *operation,
                                                      void                *in_buf,
                                                      void                *aux_buf,
                                                      void                *out_buf,
                                                      glong                samples,
                                                      const GeglRectangle *roi);


G_DEFINE_TYPE (GimpOperationDissolveMode, gimp_operation_dissolve_mode,
               GIMP_TYPE_OPERATION_LAYER_MODE)


static void
gimp_operation_dissolve_mode_class_init (GimpOperationDissolveModeClass *klass)
{
  GeglOperationClass          *operation_class = GEGL_OPERATION_CLASS (klass);
  GimpOperationLayerModeClass *mode_class      = GIMP_OPERATION_LAYER_MODE_CLASS (klass);

  operation_class->name        = "gimp-dissolve-mode";
  operation_class->description = "GIMP dissolve mode operation";

  mode_class->process          = gimp_operation_dissolve_mode_process;
}

static void
gimp_operation_dissolve_mode_init (GimpOperationDissolveMode *self)
{
}

static gboolean
gimp_operation_dissolve_mode_process (GeglOperation       *operation,
                                      void                *in_buf,
                                      void                *aux_buf,
                                      void                *out_buf,
                                      glong                samples,
                                      const GeglRectangle *roi)
{
  gfloat *src  = in_buf;
  gfloat *aux  = aux_buf;
  gfloat *dest = out_buf;

  while (samples--)
    {
      dest[RED_PIX]   = src[RED_PIX];
      dest[GREEN_PIX] = src[GREEN_PIX];
      dest[BLUE_PIX]  = src[BLUE_PIX];
      dest[ALPHA_PIX] = src[ALPHA_PIX];

      src  += 4;
      aux  += 4;
      dest += 4;
    }

  return TRUE;
}
