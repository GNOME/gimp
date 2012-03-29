/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationantierasemode.c
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

#include <gegl-plugin.h>

#include "gimp-gegl-types.h"

#include "gimpoperationantierasemode.h"


static gboolean gimp_operation_anti_erase_mode_process (GeglOperation       *operation,
                                                        void                *in_buf,
                                                        void                *aux_buf,
                                                        void                *out_buf,
                                                        glong                samples,
                                                        const GeglRectangle *roi,
                                                        gint                 level);


G_DEFINE_TYPE (GimpOperationAntiEraseMode, gimp_operation_anti_erase_mode,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_anti_erase_mode_class_init (GimpOperationAntiEraseModeClass *klass)
{
  GeglOperationClass              *operation_class;
  GeglOperationPointComposerClass *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
           "name"       , "gimp:anti-erase-mode",
           "description", "GIMP anti erase mode operation",
           NULL);

  point_class->process         = gimp_operation_anti_erase_mode_process;
}

static void
gimp_operation_anti_erase_mode_init (GimpOperationAntiEraseMode *self)
{
}

static gboolean
gimp_operation_anti_erase_mode_process (GeglOperation       *operation,
                                        void                *in_buf,
                                        void                *aux_buf,
                                        void                *out_buf,
                                        glong                samples,
                                        const GeglRectangle *roi,
                                        gint                 level)
{
  gfloat *in    = in_buf;
  gfloat *layer = aux_buf;
  gfloat *out   = out_buf;

  while (samples--)
    {
      out[RED]   = in[RED];
      out[GREEN] = in[GREEN];
      out[BLUE]  = in[BLUE];
      out[ALPHA] = in[ALPHA];

      in    += 4;
      layer += 4;
      out   += 4;
    }

  return TRUE;
}
