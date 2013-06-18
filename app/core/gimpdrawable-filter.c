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

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-gegl-apply-operation.h"

#include "gimpdrawable.h"
#include "gimpdrawable-filter.h"
#include "gimpdrawable-private.h"
#include "gimpdrawableundo.h"
#include "gimpfilter.h"
#include "gimpfilterstack.h"
#include "gimpimage-undo.h"
#include "gimpprogress.h"


GimpContainer *
gimp_drawable_get_filters (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->private->filter_stack;
}

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
      GimpApplicator *applicator;
      GeglBuffer     *buffer;
      GeglNode       *node;
      GeglNode       *src_node;

      gimp_drawable_push_undo (drawable, undo_desc, NULL,
                               rect.x, rect.y,
                               rect.width, rect.height);

      node = gimp_filter_get_node (filter);

      /* dup() because reading and writing the same buffer doesn't
       * work with area ops when using a processor. See bug #701875.
       */
      buffer = gegl_buffer_dup (gimp_drawable_get_buffer (drawable));

      src_node = gegl_node_new_child (NULL,
                                      "operation", "gegl:buffer-source",
                                      "buffer",    buffer,
                                      NULL);

      g_object_unref (buffer);

      gegl_node_connect_to (src_node, "output",
                            node,     "input");

      applicator = gimp_filter_get_applicator (filter);

      if (applicator)
        {
          GimpImage        *image = gimp_item_get_image (GIMP_ITEM (drawable));
          GimpDrawableUndo *undo;

          undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

          if (undo)
            {
              undo->paint_mode = applicator->paint_mode;
              undo->opacity    = applicator->opacity;

              undo->applied_buffer =
                gimp_applicator_dup_apply_buffer (applicator, &rect);
            }
        }

      gimp_gegl_apply_operation (NULL,
                                 progress, undo_desc,
                                 node,
                                 gimp_drawable_get_buffer (drawable),
                                 &rect);

      g_object_unref (src_node);

      gimp_drawable_update (drawable,
                            rect.x, rect.y,
                            rect.width, rect.height);
    }
}
