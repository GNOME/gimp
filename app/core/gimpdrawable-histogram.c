/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmahistogram module Copyright (C) 1999 Jay Cox <jaycox@ligma.org>
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

#include "core-types.h"

#include "gegl/ligma-gegl-nodes.h"
#include "gegl/ligmatilehandlervalidate.h"

#include "ligmaasync.h"
#include "ligmachannel.h"
#include "ligmadrawable-filters.h"
#include "ligmadrawable-histogram.h"
#include "ligmahistogram.h"
#include "ligmaimage.h"
#include "ligmaprojectable.h"


/*  local function prototypes  */

static LigmaAsync * ligma_drawable_calculate_histogram_internal (LigmaDrawable  *drawable,
                                                               LigmaHistogram *histogram,
                                                               gboolean       with_filters,
                                                               gboolean       run_async);


/*  private functions  */


static LigmaAsync *
ligma_drawable_calculate_histogram_internal (LigmaDrawable  *drawable,
                                            LigmaHistogram *histogram,
                                            gboolean       with_filters,
                                            gboolean       run_async)
{
  LigmaAsync   *async = NULL;
  LigmaImage   *image;
  LigmaChannel *mask;
  gint         x, y, width, height;

  if (! ligma_item_mask_intersect (LIGMA_ITEM (drawable), &x, &y, &width, &height))
    goto end;

  image = ligma_item_get_image (LIGMA_ITEM (drawable));
  mask  = ligma_image_get_mask (image);

  if (FALSE)
    {
      GeglNode      *node = gegl_node_new ();
      GeglNode      *source;
      GeglNode      *histogram_sink;
      GeglProcessor *processor;

      if (with_filters)
        {
          source = ligma_drawable_get_source_node (drawable);
        }
      else
        {
          source =
            ligma_gegl_add_buffer_source (node,
                                         ligma_drawable_get_buffer (drawable),
                                         0, 0);
        }

      histogram_sink =
        gegl_node_new_child (node,
                             "operation", "ligma:histogram-sink",
                             "histogram", histogram,
                             NULL);

      gegl_node_connect_to (source,         "output",
                            histogram_sink, "input");

      if (! ligma_channel_is_empty (mask))
        {
          GeglNode *mask_source;
          gint      off_x, off_y;

          g_printerr ("adding mask aux\n");

          ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

          mask_source =
            ligma_gegl_add_buffer_source (node,
                                         ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)),
                                         -off_x, -off_y);

          gegl_node_connect_to (mask_source,    "output",
                                histogram_sink, "aux");
        }

      processor = gegl_node_new_processor (histogram_sink,
                                           GEGL_RECTANGLE (x, y, width, height));

      while (gegl_processor_work (processor, NULL));

      g_object_unref (processor);
      g_object_unref (node);
    }
  else
    {
      GeglBuffer      *buffer      = ligma_drawable_get_buffer (drawable);
      LigmaProjectable *projectable = NULL;

      if (with_filters && ligma_drawable_has_filters (drawable))
        {
          LigmaTileHandlerValidate *validate;
          GeglNode                *node;

          node = ligma_drawable_get_source_node (drawable);

          buffer = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                                    gegl_buffer_get_format (buffer));

          validate =
            LIGMA_TILE_HANDLER_VALIDATE (ligma_tile_handler_validate_new (node));

          ligma_tile_handler_validate_assign (validate, buffer);

          g_object_unref (validate);

          ligma_tile_handler_validate_invalidate (validate,
                                                 gegl_buffer_get_extent (buffer));

#if 0
          /*  this would keep the buffer updated across drawable or
           *  filter changes, but the histogram is created in one go
           *  and doesn't need the signal connection
           */
          g_signal_connect_object (node, "invalidated",
                                   G_CALLBACK (ligma_tile_handler_validate_invalidate),
                                   validate, G_CONNECT_SWAPPED);
#endif

          if (LIGMA_IS_PROJECTABLE (drawable))
            projectable = LIGMA_PROJECTABLE (drawable);
        }
      else
        {
          g_object_ref (buffer);
        }

      if (projectable)
        ligma_projectable_begin_render (projectable);

      if (! ligma_channel_is_empty (mask))
        {
          gint off_x, off_y;

          ligma_item_get_offset (LIGMA_ITEM (drawable), &off_x, &off_y);

          if (run_async)
            {
              async = ligma_histogram_calculate_async (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)),
                GEGL_RECTANGLE (x + off_x, y + off_y,
                                width, height));
            }
          else
            {
              ligma_histogram_calculate (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                ligma_drawable_get_buffer (LIGMA_DRAWABLE (mask)),
                GEGL_RECTANGLE (x + off_x, y + off_y,
                                width, height));
            }
        }
      else
        {
          if (run_async)
            {
              async = ligma_histogram_calculate_async (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                NULL, NULL);
            }
          else
            {
              ligma_histogram_calculate (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                NULL, NULL);
            }
        }

      if (projectable)
        ligma_projectable_end_render (projectable);

      g_object_unref (buffer);
    }

end:
  if (run_async && ! async)
    {
      async = ligma_async_new ();

      ligma_async_finish (async, NULL);
    }

  return async;
}


/*  public functions  */


void
ligma_drawable_calculate_histogram (LigmaDrawable  *drawable,
                                   LigmaHistogram *histogram,
                                   gboolean       with_filters)
{
  g_return_if_fail (LIGMA_IS_DRAWABLE (drawable));
  g_return_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)));
  g_return_if_fail (histogram != NULL);

  ligma_drawable_calculate_histogram_internal (drawable,
                                              histogram, with_filters,
                                              FALSE);
}

LigmaAsync *
ligma_drawable_calculate_histogram_async (LigmaDrawable  *drawable,
                                         LigmaHistogram *histogram,
                                         gboolean       with_filters)
{
  g_return_val_if_fail (LIGMA_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (ligma_item_is_attached (LIGMA_ITEM (drawable)), NULL);
  g_return_val_if_fail (histogram != NULL, NULL);

  return ligma_drawable_calculate_histogram_internal (drawable,
                                                     histogram, with_filters,
                                                     TRUE);
}
