/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-filter.c
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

#include "gegl/gimp-gegl-apply-operation.h"

#include "gimpdrawable.h"
#include "gimpdrawable-filter.h"
#include "gimpdrawable-private.h"
#include "gimpfilter.h"
#include "gimpfilterstack.h"
#include "gimpprogress.h"


void
gimp_drawable_add_filter (GimpDrawable *drawable,
                          GimpFilter   *filter)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_FILTER (filter));
  g_return_if_fail (gimp_drawable_has_filter (drawable, filter) == FALSE);

  gimp_container_add (drawable->private->filter_stack,
                      GIMP_OBJECT (filter));
}

void
gimp_drawable_remove_filter (GimpDrawable *drawable,
                             GimpFilter   *filter)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_FILTER (filter));
  g_return_if_fail (gimp_drawable_has_filter (drawable, filter) == TRUE);

  gimp_container_remove (drawable->private->filter_stack,
                         GIMP_OBJECT (filter));
}

gboolean
gimp_drawable_has_filter (GimpDrawable *drawable,
                          GimpFilter   *filter)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_FILTER (filter), FALSE);

  return gimp_container_have (drawable->private->filter_stack,
                              GIMP_OBJECT (filter));
}

void
gimp_drawable_merge_filter (GimpDrawable *drawable,
                            GimpFilter   *filter,
                            GimpProgress *progress,
                            const gchar  *undo_desc)
{
  GeglRectangle rect;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (GIMP_IS_FILTER (filter));
  g_return_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress));

  if (gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                &rect.x, &rect.y,
                                &rect.width, &rect.height))
    {
      gimp_drawable_push_undo (drawable, undo_desc, NULL,
                               rect.x, rect.y,
                               rect.width, rect.height);

      gimp_gegl_apply_operation (gimp_drawable_get_buffer (drawable),
                                 progress, undo_desc,
                                 gimp_filter_get_node (filter),
                                 gimp_drawable_get_buffer (drawable),
                                 &rect);

      gimp_drawable_update (drawable,
                            rect.x, rect.y,
                            rect.width, rect.height);
    }
}
