/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmadrawable-filters.c
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

#include "gegl/ligmaapplicator.h"
#include "gegl/ligma-gegl-apply-operation.h"
#include "gegl/ligma-gegl-loops.h"

#include "ligma.h"
#include "ligma-utils.h"
#include "ligmadrawable.h"
#include "ligmadrawable-filters.h"
#include "ligmadrawable-private.h"
#include "ligmafilter.h"
#include "ligmafilterstack.h"
#include "ligmaimage.h"
#include "ligmaimage-undo.h"
#include "ligmalayer.h"
#include "ligmaprogress.h"
#include "ligmaprojection.h"


LigmaContainer *
ligma_drawable_get_filters (LigmaDrawable *drawable)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);

  return drawable->private->filter_stack;
}

gboolean
ligma_drawable_has_filters (LigmaDrawable *drawable)
{
  GList *list;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);

  for (list = LIGMA_LIST (drawable->private->filter_stack)->queue->head;
       list;
       list = g_list_next (list))
    {
      LigmaFilter *filter = list->data;

      if (ligma_filter_get_active (filter))
        return TRUE;
    }

  return FALSE;
}

void
ligma_drawable_add_filter (LigmaDrawable *drawable,
                          LigmaFilter   *filter)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (LIGMA_IS_FILTER (filter));
  g_return_if_fail (ligma_drawable_has_filter (drawable, filter) == FALSE);

  ligma_container_add (drawable->private->filter_stack,
                      LIGMA_OBJECT (filter));
}

void
ligma_drawable_remove_filter (LigmaDrawable *drawable,
                             LigmaFilter   *filter)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (LIGMA_IS_FILTER (filter));
  g_return_if_fail (ligma_drawable_has_filter (drawable, filter) == TRUE);

  ligma_container_remove (drawable->private->filter_stack,
                         LIGMA_OBJECT (filter));
}

gboolean
ligma_drawable_has_filter (LigmaDrawable *drawable,
                          LigmaFilter   *filter)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (LIGMA_IS_FILTER (filter), FALSE);

  return ligma_container_have (drawable->private->filter_stack,
                              LIGMA_OBJECT (filter));
}

gboolean
ligma_drawable_merge_filter (LigmaDrawable *drawable,
                            LigmaFilter   *filter,
                            LigmaProgress *progress,
                            const gchar  *undo_desc,
                            const Babl   *format,
                            gboolean      clip,
                            gboolean      cancellable,
                            gboolean      update)
{
  LigmaImage      *image;
  LigmaApplicator *applicator;
  gboolean        applicator_cache         = FALSE;
  const Babl     *applicator_output_format = NULL;
  GeglBuffer     *buffer                   = NULL;
  GeglBuffer     *dest_buffer;
  GeglBuffer     *undo_buffer              = NULL;
  GeglRectangle   undo_rect;
  GeglBuffer     *cache                    = NULL;
  GeglRectangle  *rects                    = NULL;
  gint            n_rects                  = 0;
  GeglRectangle   rect;
  gboolean        success                  = TRUE;

  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), FALSE);
  g_return_val_if_fail (LIGMA_IS_FILTER (filter), FALSE);
  g_return_val_if_fail (progress == NULL || LIGMA_IS_PROGRESS (progress), FALSE);

  image       = ligma_item_get_image (LIGMA_ITEM (drawable));
  applicator  = ligma_filter_get_applicator (filter);
  dest_buffer = ligma_drawable_get_buffer (drawable);

  if (! format)
    format = ligma_drawable_get_format (drawable);

  rect = gegl_node_get_bounding_box (ligma_filter_get_node (filter));

  if (! clip && gegl_rectangle_equal (&rect,
                                      gegl_buffer_get_extent (dest_buffer)))
    {
      clip = TRUE;
    }

  if (clip)
    {
      if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable),
                                      &rect.x, &rect.y,
                                      &rect.width, &rect.height))
        {
          return TRUE;
        }

      if (format != ligma_drawable_get_format (drawable))
        {
          buffer = gegl_buffer_new (gegl_buffer_get_extent (dest_buffer),
                                    format);

          dest_buffer = buffer;
        }
    }
  else
    {
      buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0, rect.width, rect.height),
                                format);

      dest_buffer = g_object_new (GEGL_TYPE_BUFFER,
                                  "source",  buffer,
                                  "shift-x", -rect.x,
                                  "shift-y", -rect.y,
                                  NULL);
    }

  if (applicator)
    {
      const GeglRectangle *crop_rect;

      crop_rect = ligma_applicator_get_crop (applicator);

      if (crop_rect && ! gegl_rectangle_intersect (&rect, &rect, crop_rect))
        return TRUE;

      /*  the cache and its valid rectangles are the region that
       *  has already been processed by this applicator.
       */
      cache = ligma_applicator_get_cache_buffer (applicator,
                                                &rects, &n_rects);

      /*  skip the cache and output-format conversion while processing
       *  the remaining area, so that the result is written directly to
       *  the drawable's buffer.
       */
      applicator_cache         = ligma_applicator_get_cache (applicator);
      applicator_output_format = ligma_applicator_get_output_format (applicator);

      ligma_applicator_set_cache (applicator, FALSE);
      if (applicator_output_format == format)
        ligma_applicator_set_output_format (applicator, NULL);
    }

  if (! buffer)
    {
      gegl_rectangle_align_to_buffer (
        &undo_rect,
        &rect,
        ligma_drawable_get_buffer (drawable),
        GEGL_RECTANGLE_ALIGNMENT_SUPERSET);

      undo_buffer = gegl_buffer_new (GEGL_RECTANGLE (0, 0,
                                                     undo_rect.width,
                                                     undo_rect.height),
                                     ligma_drawable_get_format (drawable));

      ligma_gegl_buffer_copy (ligma_drawable_get_buffer (drawable),
                             &undo_rect,
                             GEGL_ABYSS_NONE,
                             undo_buffer,
                             GEGL_RECTANGLE (0, 0, 0, 0));
    }

  ligma_projection_stop_rendering (ligma_image_get_projection (image));

  /* make sure we have a source node - this connects the filter stack to the
   * underlying source node
   */
  (void) ligma_drawable_get_source_node (drawable);

  if (ligma_gegl_apply_cached_operation (ligma_drawable_get_buffer (drawable),
                                        progress, undo_desc,
                                        ligma_filter_get_node (filter), FALSE,
                                        dest_buffer, &rect, FALSE,
                                        cache, rects, n_rects,
                                        cancellable))
    {
      /*  finished successfully  */

      if (clip)
        {
          if (buffer)
            {
              ligma_drawable_set_buffer_full (drawable,
                                             TRUE, undo_desc,
                                             buffer, NULL,
                                             FALSE);
            }
          else
            {
              ligma_drawable_push_undo (drawable, undo_desc, undo_buffer,
                                       undo_rect.x, undo_rect.y,
                                       undo_rect.width, undo_rect.height);
            }
        }
      else
        {
          LigmaLayerMask *mask = NULL;
          gint           offset_x;
          gint           offset_y;

          ligma_item_get_offset (LIGMA_ITEM (drawable), &offset_x, &offset_y);

          if (LIGMA_IS_LAYER (drawable))
            mask = ligma_layer_get_mask (LIGMA_LAYER (drawable));

          if (mask)
            {
              ligma_image_undo_group_start (image, LIGMA_UNDO_GROUP_DRAWABLE_MOD,
                                           undo_desc);
            }

          ligma_drawable_set_buffer_full (
            drawable, TRUE, undo_desc, buffer,
            GEGL_RECTANGLE (offset_x + rect.x, offset_y + rect.y, 0, 0),
            FALSE);

          if (mask)
            {
              ligma_item_resize (LIGMA_ITEM (mask),
                                ligma_get_default_context (image->ligma),
                                LIGMA_FILL_TRANSPARENT,
                                rect.width, rect.height,
                                -rect.x, -rect.y);

              ligma_image_undo_group_end (image);
            }
        }
    }
  else
    {
      /*  canceled by the user  */

      if (clip)
        {
          ligma_gegl_buffer_copy (undo_buffer,
                                 GEGL_RECTANGLE (0, 0,
                                                 undo_rect.width,
                                                 undo_rect.height),
                                 GEGL_ABYSS_NONE,
                                 ligma_drawable_get_buffer (drawable),
                                 &undo_rect);
        }

      success = FALSE;
    }

  if (clip)
    {
      g_clear_object (&undo_buffer);
      g_clear_object (&buffer);
    }
  else
    {
      g_object_unref (buffer);
      g_object_unref (dest_buffer);
    }

  if (cache)
    {
      g_object_unref (cache);
      g_free (rects);
    }

  if (applicator)
    {
      ligma_applicator_set_cache (applicator, applicator_cache);
      ligma_applicator_set_output_format (applicator, applicator_output_format);
    }

  if (update)
    {
      ligma_drawable_update (drawable,
                            rect.x, rect.y,
                            rect.width, rect.height);
    }

  return success;
}
