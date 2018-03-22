/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-operation.c
 * Copyright (C) 2007 Øyvind Kolås <pippin@gimp.org>
 *                    Sven Neumann <sven@gimp.org>
 *                    Michael Natterer <mitch@gimp.org>
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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gegl/gimp-gegl-apply-operation.h"

#include "gimpdrawable.h"
#include "gimpdrawable-operation.h"
#include "gimpdrawable-shadow.h"
#include "gimpprogress.h"
#include "gimpsettings.h"


/*  public functions  */

void
gimp_drawable_apply_operation (GimpDrawable *drawable,
                               GimpProgress *progress,
                               const gchar  *undo_desc,
                               GeglNode     *operation)
{
  GeglBuffer    *dest_buffer;
  GeglRectangle  rect;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (GEGL_IS_NODE (operation));

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &rect.x,     &rect.y,
                                  &rect.width, &rect.height))
    return;

  dest_buffer = gimp_drawable_get_shadow_buffer (drawable);

  gimp_gegl_apply_operation (gimp_drawable_get_buffer (drawable),
                             progress, undo_desc,
                             operation,
                             dest_buffer, &rect, FALSE);

  gimp_drawable_merge_shadow_buffer (drawable, TRUE, undo_desc);
  gimp_drawable_free_shadow_buffer (drawable);

  gimp_drawable_update (drawable, rect.x, rect.y, rect.width, rect.height);

  if (progress)
    gimp_progress_end (progress);
}

void
gimp_drawable_apply_operation_by_name (GimpDrawable *drawable,
                                       GimpProgress *progress,
                                       const gchar  *undo_desc,
                                       const gchar  *operation_type,
                                       GObject      *config)
{
  GeglNode *node;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));
  g_return_if_fail (undo_desc != NULL);
  g_return_if_fail (operation_type != NULL);
  g_return_if_fail (config == NULL || GIMP_IS_SETTINGS (config));

  node = g_object_new (GEGL_TYPE_NODE,
                       "operation", operation_type,
                       NULL);

  if (config)
    gegl_node_set (node,
                   "config", config,
                   NULL);

  gimp_drawable_apply_operation (drawable, progress, undo_desc, node);

  g_object_unref (node);
}
