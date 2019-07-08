/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpdrawable-filters.c
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>
#include <cairo.h>

#include "core-types.h"

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-gegl-apply-operation.h"
#include "gegl/gimp-gegl-loops.h"
#include "gegl/gimp-gegl-utils.h"

#include "gimp-utils.h"
#include "gimpdrawable.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-private.h"
#include "gimpfilter.h"
#include "gimpfilterstack.h"
#include "gimpimage.h"
#include "gimpprogress.h"
#include "gimpprojection.h"


GimpContainer *
gimp_drawable_get_filters (GimpDrawable *drawable)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);

  return drawable->private->filter_stack;
}

gboolean
gimp_drawable_has_filters (GimpDrawable *drawable)
{
  GList *list;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);

  for (list = GIMP_LIST (drawable->private->filter_stack)->queue->head;
       list;
       list = g_list_next (list))
    {
      GimpFilter *filter = list->data;

      if (gimp_filter_get_active (filter))
        return TRUE;
    }

  return FALSE;
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

gboolean
gimp_drawable_merge_filter (GimpDrawable *drawable,
                            GimpFilter   *filter,
                            GimpProgress *progress,
                            const gchar  *undo_desc,
                            gboolean      cancellable,
                            gboolean      update)
{
  GimpImage      *image;
  GimpApplicator *applicator;
  gboolean        applicator_cache         = FALSE;
  const Babl     *applicator_output_format = NULL;
  GeglBuffer     *undo_buffer;
  GeglRectangle   undo_rect;
  GeglBuffer     *cache                    = NULL;
  GeglRectangle  *rects                    = NULL;
  gint            n_rects                  = 0;
  GeglRectangle   rect;
  gboolean        success                  = TRUE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_FILTER (filter), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  image      = gimp_item_get_image (GIMP_ITEM (drawable));
  applicator = gimp_filter_get_applicator (filter);

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                  &rect.x, &rect.y,
                                  &rect.width, &rect.height))
    {
      return TRUE;
    }

  if (applicator)
    {
      const GeglRectangle *crop_rect;

      crop_rect = gimp_applicator_get_crop (applicator);

      if (crop_rect && ! gegl_rectangle_intersect (&rect, &rect, crop_rect))
        return TRUE;

      /*  the cache and its valid rectangles are the region that
       *  has already been processed by this applicator.
       */
      cache = gimp_applicator_get_cache_buffer (applicator,
                                                &rects, &n_rects);

      /*  skip the cache and output-format conversion while processing
       *  the remaining area, so that the result is written directly to
       *  the drawable's buffer.
       */
      applicator_cache         = gimp_applicator_get_cache (applicator);
      applicator_output_format = gimp_applicator_get_output_format (applicator);

      gimp_applicator_set_cache (applicator, FALSE);
      if (applicator_output_format == gimp_drawable_get_format (drawable))
        gimp_applicator_set_output_format (applicator, NULL);
    }

  gimp_gegl_rectangle_align_to_tile_grid (
    &undo_rect,
    &rect,
    gimp_drawable_get_buffer (drawable));

  undo_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                 undo_rect.width,
                                                 undo_rect.height),
                                 gimp_drawable_get_format (drawable));

  gimp_gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                         &undo_rect,
                         GEGL_ABYSS_NONE,
                         undo_buffer,
                         GEGL_RECTANGLE (0, 0, 0, 0));

  gimp_projection_stop_rendering (gimp_image_get_projection (image));

  if (gimp_gegl_apply_cached_operation (gimp_drawable_get_buffer (drawable),
                                        progress, undo_desc,
                                        gimp_filter_get_node (filter),
                                        gimp_drawable_get_buffer (drawable),
                                        &rect, FALSE,
                                        cache, rects, n_rects,
                                        cancellable))
    {
      /*  finished successfully  */

      gimp_drawable_push_undo (drawable, undo_desc, undo_buffer,
                               undo_rect.x, undo_rect.y,
                               undo_rect.width, undo_rect.height);
    }
  else
    {
      /*  canceled by the user  */

      gimp_gegl_buffer_copy (undo_buffer,
                             GEGL_RECTANGLE (0, 0,
                                             undo_rect.width,
                                             undo_rect.height),
                             GEGL_ABYSS_NONE,
                             gimp_drawable_get_buffer (drawable),
                             &undo_rect);

      success = FALSE;
    }

  g_object_unref (undo_buffer);

  if (cache)
    {
      g_object_unref (cache);
      g_free (rects);
    }

  if (applicator)
    {
      gimp_applicator_set_cache (applicator, applicator_cache);
      gimp_applicator_set_output_format (applicator, applicator_output_format);
    }

  if (update)
    {
      gimp_drawable_update (drawable,
                            rect.x, rect.y,
                            rect.width, rect.height);
    }

  return success;
}
