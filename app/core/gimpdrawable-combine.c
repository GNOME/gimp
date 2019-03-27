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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gegl/gimpapplicator.h"

#include "gimp.h"
#include "gimpchannel.h"
#include "gimpchunkiterator.h"
#include "gimpdrawable-combine.h"
#include "gimpimage.h"


void
gimp_drawable_real_apply_buffer (GimpDrawable           *drawable,
                                 GeglBuffer             *buffer,
                                 const GeglRectangle    *buffer_region,
                                 gboolean                push_undo,
                                 const gchar            *undo_desc,
                                 gdouble                 opacity,
                                 GimpLayerMode           mode,
                                 GimpLayerColorSpace     blend_space,
                                 GimpLayerColorSpace     composite_space,
                                 GimpLayerCompositeMode  composite_mode,
                                 GeglBuffer             *base_buffer,
                                 gint                    base_x,
                                 gint                    base_y)
{
  GimpItem          *item  = GIMP_ITEM (drawable);
  GimpImage         *image = gimp_item_get_image (item);
  GimpChannel       *mask  = gimp_image_get_mask (image);
  GimpApplicator    *applicator;
  GimpChunkIterator *iter;
  gint               x, y, width, height;
  gint               offset_x, offset_y;

  /*  don't apply the mask to itself and don't apply an empty mask  */
  if (GIMP_DRAWABLE (mask) == drawable || gimp_channel_is_empty (mask))
    mask = NULL;

  if (! base_buffer)
    base_buffer = gimp_drawable_get_buffer (drawable);

  /*  get the layer offsets  */
  gimp_item_get_offset (item, &offset_x, &offset_y);

  /*  make sure the image application coordinates are within drawable bounds  */
  if (! gimp_rectangle_intersect (base_x, base_y,
                                  buffer_region->width, buffer_region->height,
                                  0, 0,
                                  gimp_item_get_width  (item),
                                  gimp_item_get_height (item),
                                  &x, &y, &width, &height))
    {
      return;
    }

  if (mask)
    {
      GimpItem *mask_item = GIMP_ITEM (mask);

      /*  make sure coordinates are in mask bounds ...
       *  we need to add the layer offset to transform coords
       *  into the mask coordinate system
       */
      if (! gimp_rectangle_intersect (x, y, width, height,
                                      -offset_x, -offset_y,
                                      gimp_item_get_width  (mask_item),
                                      gimp_item_get_height (mask_item),
                                      &x, &y, &width, &height))
        {
          return;
        }
    }

  if (push_undo)
    {
      gimp_drawable_push_undo (drawable, undo_desc,
                               NULL, x, y, width, height);
    }

  applicator = gimp_applicator_new (NULL);

  if (mask)
    {
      GeglBuffer *mask_buffer;

      mask_buffer = gimp_drawable_get_buffer (GIMP_DRAWABLE (mask));

      gimp_applicator_set_mask_buffer (applicator, mask_buffer);
      gimp_applicator_set_mask_offset (applicator, -offset_x, -offset_y);
    }

  gimp_applicator_set_src_buffer (applicator, base_buffer);
  gimp_applicator_set_dest_buffer (applicator,
                                   gimp_drawable_get_buffer (drawable));

  gimp_applicator_set_apply_buffer (applicator, buffer);
  gimp_applicator_set_apply_offset (applicator,
                                    base_x - buffer_region->x,
                                    base_y - buffer_region->y);

  gimp_applicator_set_opacity (applicator, opacity);
  gimp_applicator_set_mode (applicator, mode,
                            blend_space, composite_space, composite_mode);
  gimp_applicator_set_affect (applicator,
                              gimp_drawable_get_active_mask (drawable));

  iter = gimp_chunk_iterator_new (cairo_region_create_rectangle (
    &(cairo_rectangle_int_t) {x, y, width, height}));

  while (gimp_chunk_iterator_next (iter))
    {
      GeglRectangle rect;

      while (gimp_chunk_iterator_get_rect (iter, &rect))
        gimp_applicator_blit (applicator, &rect);
    }

  g_object_unref (applicator);
}
