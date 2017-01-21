/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationsubtractmode.c
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

#include "gimpoperationsubtract.h"
#include "gimpblendcomposite.h"

G_DEFINE_TYPE (GimpOperationSubtract, gimp_operation_subtract,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_subtract_class_init (GimpOperationSubtractClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:subtract",
                                 "description", "GIMP subtract mode operation",
                                 NULL);

  point_class->process = gimp_operation_subtract_process;
}

static void
gimp_operation_subtract_init (GimpOperationSubtract *self)
{
}

gboolean
gimp_operation_subtract_process (GeglOperation       *op,
                                 void                *in,
                                 void                *layer,
                                 void                *mask,
                                 void                *out,
                                 glong                samples,
                                 const GeglRectangle *roi,
                                 gint                 level)
{
  gimp_composite_blend (op, in, layer, mask, out, samples,
                        blendfun_subtract);
  return TRUE;
}
