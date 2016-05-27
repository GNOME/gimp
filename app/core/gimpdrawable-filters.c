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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "core-types.h"

#include "gegl/gimpapplicator.h"
#include "gegl/gimp-gegl-apply-operation.h"

#include "gimp-utils.h"
#include "gimpdrawable.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-private.h"
#include "gimpdrawableundo.h"
#include "gimpfilter.h"
#include "gimpfilterstack.h"
#include "gimpimage.h"
#include "gimpimage-undo.h"
#include "gimpprogress.h"
#include "gimpprojection.h"


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

gboolean
gimp_drawable_merge_filter (GimpDrawable *drawable,
                            GimpFilter   *filter,
                            GimpProgress *progress,
                            const gchar  *undo_desc,
                            gboolean      cancellable)
{
  GeglRectangle rect;
  gboolean      success = TRUE;

  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (GIMP_IS_FILTER (filter), FALSE);
  g_return_val_if_fail (progress == NULL || GIMP_IS_PROGRESS (progress), FALSE);

  if (gimp_item_mask_intersect (GIMP_ITEM (drawable),
                                &rect.x, &rect.y,
                                &rect.width, &rect.height))
    {
      GimpImage      *image = gimp_item_get_image (GIMP_ITEM (drawable));
      GeglBuffer     *undo_buffer;
      GimpApplicator *applicator;
      GeglBuffer     *apply_buffer = NULL;
      GeglBuffer     *cache        = NULL;
      GeglRectangle  *rects        = NULL;
      gint            n_rects      = 0;

      undo_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                     rect.width, rect.height),
                                     gimp_drawable_get_format (drawable));

      gegl_buffer_copy (gimp_drawable_get_buffer (drawable),
                        GEGL_RECTANGLE (rect.x, rect.y,
                                        rect.width, rect.height),
                        GEGL_ABYSS_NONE,
                        undo_buffer,
                        GEGL_RECTANGLE (0, 0, 0, 0));

      applicator = gimp_filter_get_applicator (filter);

      if (applicator)
        {
          /*  disable the preview crop, this will force-process the
           *  cached result from the preview cache into the result
           *  cache, involving only the layer and affect nodes
           */
          gimp_applicator_set_preview (applicator, FALSE,
                                       GEGL_RECTANGLE (0, 0, 0, 0));

          /*  the apply_buffer will make a copy of the region that is
           *  actually processed in gimp_gegl_apply_cached_operation()
           *  below.
           */
          apply_buffer = gimp_applicator_dup_apply_buffer (applicator, &rect);

          /*  the cache and its valid rectangles are the region that
           *  has already been processed by this applicator.
           */
          cache = gimp_applicator_get_cache_buffer (applicator,
                                                    &rects, &n_rects);

          if (cache)
            {
              gint i;

              for (i = 0; i < n_rects; i++)
                {
                  g_printerr ("valid: %d %d %d %d\n",
                              rects[i].x, rects[i].y,
                              rects[i].width, rects[i].height);

                  /*  we have to copy the cached region to the apply_buffer,
                   *  because this region is not going to be processed.
                   */
                  gegl_buffer_copy (cache,
                                    &rects[i],
                                    GEGL_ABYSS_NONE,
                                    apply_buffer,
                                    GEGL_RECTANGLE (rects[i].x - rect.x,
                                                    rects[i].y - rect.y,
                                                    0, 0));
                }
            }

        }

      gimp_projection_stop_rendering (gimp_image_get_projection (image));

      if (gimp_gegl_apply_cached_operation (gimp_drawable_get_buffer (drawable),
                                            progress, undo_desc,
                                            gimp_filter_get_node (filter),
                                            gimp_drawable_get_buffer (drawable),
                                            &rect,
                                            cache, rects, n_rects,
                                            cancellable))
        {
          /*  finished successfully  */

          gimp_drawable_push_undo (drawable, undo_desc, undo_buffer,
                                   rect.x, rect.y,
                                   rect.width, rect.height);

          if (applicator)
            {
              GimpDrawableUndo *undo;

              undo = GIMP_DRAWABLE_UNDO (gimp_image_undo_get_fadeable (image));

              if (undo)
                {
                  undo->paint_mode = applicator->paint_mode;
                  undo->opacity    = applicator->opacity;

                  undo->applied_buffer = apply_buffer;
                  apply_buffer = NULL;
                }
            }
        }
      else
        {
          /*  canceled by the user  */

          gegl_buffer_copy (undo_buffer,
                            GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                            GEGL_ABYSS_NONE,
                            gimp_drawable_get_buffer (drawable),
                            GEGL_RECTANGLE (rect.x, rect.y, 0, 0));

          success = FALSE;
        }

      g_object_unref (undo_buffer);

      if (apply_buffer)
        g_object_unref (apply_buffer);

      if (cache)
        {
          g_object_unref (cache);
          g_free (rects);
        }

      gimp_drawable_update (drawable,
                            rect.x, rect.y,
                            rect.width, rect.height);
    }

  return success;
}
