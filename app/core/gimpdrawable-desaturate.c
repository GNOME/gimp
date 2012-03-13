/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl.h>

#include "core-types.h"

#include "gegl/gimpdesaturateconfig.h"

#include "gimpdrawable.h"
#include "gimpdrawable-desaturate.h"
#include "gimpdrawable-operation.h"
#include "gimpprogress.h"

#include "gimp-intl.h"


void
gimp_drawable_desaturate (GimpDrawable       *drawable,
                          GimpProgress       *progress,
                          GimpDesaturateMode  mode)
{
  GeglNode *node;
  GObject  *config;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_drawable_is_rgb (drawable));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", "gimp:desaturate",
                       NULL);

  config = g_object_new (GIMP_TYPE_DESATURATE_CONFIG,
                         "mode", mode,
                         NULL);

  gegl_node_set (node,
                 "config", config,
                 NULL);

  g_object_unref (config);

  gimp_drawable_apply_operation (drawable, progress, _("Desaturate"),
                                 node, TRUE);
  g_object_unref (node);
}
