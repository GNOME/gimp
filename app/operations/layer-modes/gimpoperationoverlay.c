/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpoperationoverlay.c
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

#include "gimpoperationoverlay.h"
#include "gimpblendcomposite.h"

G_DEFINE_TYPE (GimpOperationOverlay, gimp_operation_overlay,
               GIMP_TYPE_OPERATION_POINT_LAYER_MODE)


static void
gimp_operation_overlay_class_init (GimpOperationOverlayClass *klass)
{
  GeglOperationClass               *operation_class;
  GeglOperationPointComposer3Class *point_class;

  operation_class = GEGL_OPERATION_CLASS (klass);
  point_class     = GEGL_OPERATION_POINT_COMPOSER3_CLASS (klass);

  gegl_operation_class_set_keys (operation_class,
                                 "name",        "gimp:overlay",
                                 "description", "GIMP overlay mode operation",
                                 NULL);

  point_class->process = gimp_operation_overlay_process;
}

static void
gimp_operation_overlay_init (GimpOperationOverlay *self)
{
}

gboolean
gimp_operation_overlay_process (GeglOperation       *operation,
                                void                *in,
                                void                *layer,
                                void                *mask,
                                void                *out,
                                glong                samples,
                                const GeglRectangle *roi,
                                gint                 level)
{
  gimp_composite_blend (operation, in, layer, mask, out, samples,
                        blendfun_overlay);
  return TRUE;
}
