/* LIGMA - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libligmabase/ligmabase.h"

#include "core-types.h"

#include "gegl/ligmaapplicator.h"

#include "ligma.h"
#include "ligmachannel.h"
#include "ligmachunkiterator.h"
#include "ligmadrawable-combine.h"
#include "ligmaimage.h"


void
ligma_drawable_real_apply_buffer (LigmaDrawable           *drawable,
                                 GeglBuffer             *buffer,
                                 const GeglRectangle    *buffer_region,
                                 gboolean                push_undo,
                                 const gchar            *undo_desc,
                                 gdouble                 opacity,
                                 LigmaLayerMode           mode,
                                 LigmaLayerColorSpace     blend_space,
                                 LigmaLayerColorSpace     composite_space,
                                 LigmaLayerCompositeMode  composite_mode,
                                 GeglBuffer             *base_buffer,
                                 gint                    base_x,
                                 gint                    base_y)
{
  LigmaItem          *item  = LIGMA_ITEM (drawable);
  LigmaImage         *image = ligma_item_get_image (item);
  LigmaChannel       *mask  = ligma_image_get_mask (image);
  LigmaApplicator    *applicator;
  LigmaChunkIterator *iter;
  gint               x, y, width, height;
  gint               offset_x, offset_y;

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (LIGMA_DRAWABLE (mask) == drawable || ligma_channel_is_empty (mask))
    mask = NULL;

  if (! base_buffer)
    base_buffer = ligma_drawable_get_buffer (drawable);

  /*  get the layer offsets  */
  ligma_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  if (! ligma_rectangle_intersect (base_x, base_y,
                                  buffer_region->width, buffer_region->height,
                                  0, 0,
                                  ligma_item_get_width  (item),
                                  ligma_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      return;
    }

  if (mask)
    {
      LigmaItem *mask_item = LIGMA_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      if (! ligma_rectangle_intersect (x, y, width, height,
                                      -offset_x, -offset_y,
                                      ligma_item_get_width  (mask_item),
                                      ligma_item_get_height (mask_item),
                                      &x, &y, &width, &height))
        {
          return;
        }
    }

  if (push_undo)
    {
      ligma_drawable_push_undo (drawable, undo_desc,
                               NULL, x, y, width, height);
    }

  applicator = ligma_applicator_new (NULL);

  if (mask)
    {
      GeglBuffer *mask_buffer;

      mask_buffer = ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask));

      ligma_applicator_set_mask_buffer (applicator, mask_buffer);
      ligma_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
    }

  ligma_applicator_set_src_buffer (applicator, base_buffer);
  ligma_applicator_set_dest_buffer (applicator,
                                   ligma_drawable_get_buffer (drawable));

  ligma_applicator_set_apply_buffer (applicator, buffer);
  ligma_applicator_set_apply_offset (applicator,
                                    base_x - buffer_region->x,
                                    base_y - buffer_region->y);

  ligma_applicator_set_opacity (applicator, opacity);
  ligma_applicator_set_mode (applicator, mode,
                            blend_space, composite_space, composite_mode);
  ligma_applicator_set_affect (applicator,
                              ligma_drawable_get_active_mask (drawable));

  iter = ligma_chunk_iterator_new (cairo_region_create_rectangle (
    &(cairo_rectangle_int_t) {x, y, width, height}));

  while (ligma_chunk_iterator_next (iter))
    {
      GeglRectangle rect;

      while (ligma_chunk_iterator_get_rect (iter, &rect))
        ligma_applicator_blit (applicator, &rect);
    }

  g_object_unref (applicator);
}
