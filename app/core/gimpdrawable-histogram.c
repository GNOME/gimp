/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
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

#include "gegl/gimp-gegl-nodes.h"
#include "gegl/gimptilehandlervalidate.h"

#include "gimpasync.h"
#include "gimpchannel.h"
#include "gimpdrawable-filters.h"
#include "gimpdrawable-histogram.h"
#include "gimphistogram.h"
#include "gimpimage.h"
#include "gimpprojectable.h"


/*  local function prototypes  */

static GimpAsync * gimp_drawable_calculate_histogram_internal (GimpDrawable  *drawable,
                                                               GimpHistogram *histogram,
                                                               gboolean       with_filters,
                                                               gboolean       run_async);


/*  private functions  */


static GimpAsync *
gimp_drawable_calculate_histogram_internal (GimpDrawable  *drawable,
                                            GimpHistogram *histogram,
                                            gboolean       with_filters,
                                            gboolean       run_async)
{
  GimpAsync   *async = NULL;
  GimpImage   *image;
  GimpChannel *mask;
  gint         x, y, width, height;

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    goto end;

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  mask  = gimp_image_get_mask (image);

  if (FALSE)
    {
      GeglNode      *node = gegl_node_new ();
      GeglNode      *source;
      GeglNode      *histogram_sink;
      GeglProcessor *processor;

      if (with_filters)
        {
          source = gimp_drawable_get_source_node (drawable);
        }
      else
        {
          source =
            gimp_gegl_add_buffer_source (node,
                                         gimp_drawable_get_buffer (drawable),
                                         0, 0);
        }

      histogram_sink =
        gegl_node_new_child (node,
                             "operation", "gimp:histogram-sink",
                             "histogram", histogram,
                             NULL);

      gegl_node_link (source, histogram_sink);

      if (! gimp_channel_is_empty (mask))
        {
          GeglNode *mask_source;
          gint      off_x, off_y;

          g_printerr ("adding mask aux\n");

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          mask_source =
            gimp_gegl_add_buffer_source (node,
                                         gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)),
                                         -off_x, -off_y);

          gegl_node_connect (mask_source, "output", histogram_sink, "aux");
        }

      processor = gegl_node_new_processor (histogram_sink,
                                           GEGL_RECTANGLE (x, y, width, height));

      while (gegl_processor_work (processor, NULL));

      g_object_unref (processor);
      g_object_unref (node);
    }
  else
    {
      GeglBuffer      *buffer      = gimp_drawable_get_buffer (drawable);
      GimpProjectable *projectable = NULL;

      if (with_filters && gimp_drawable_has_filters (drawable))
        {
          GimpTileHandlerValidate *validate;
          GeglNode                *node;

          node = gimp_drawable_get_source_node (drawable);

          buffer = gegl_buffer_new (gegl_buffer_get_extent (buffer),
                                    gegl_buffer_get_format (buffer));

          validate =
            GIMP_TILE_HANDLER_VALIDATE (gimp_tile_handler_validate_new (node));

          gimp_tile_handler_validate_assign (validate, buffer);

          g_object_unref (validate);

          gimp_tile_handler_validate_invalidate (validate,
                                                 gegl_buffer_get_extent (buffer));

#if 0
          /*  this would keep the buffer updated across drawable or
           *  filter changes, but the histogram is created in one go
           *  and doesn't need the signal connection
           */
          g_signal_connect_object (node, "invalidated",
                                   G_CALLBACK (gimp_tile_handler_validate_invalidate),
                                   validate, G_CONNECT_SWAPPED);
#endif

          if (GIMP_IS_PROJECTABLE (drawable))
            projectable = GIMP_PROJECTABLE (drawable);
        }
      else
        {
          g_object_ref (buffer);
        }

      if (projectable)
        gimp_projectable_begin_render (projectable);

      if (! gimp_channel_is_empty (mask))
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);

          if (run_async)
            {
              async = gimp_histogram_calculate_async (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)),
                GEGL_RECTANGLE (x + off_x, y + off_y,
                                width, height));
            }
          else
            {
              gimp_histogram_calculate (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                gimp_drawable_get_buffer (GIMP_DRAWABLE (mask)),
                GEGL_RECTANGLE (x + off_x, y + off_y,
                                width, height));
            }
        }
      else
        {
          if (run_async)
            {
              async = gimp_histogram_calculate_async (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                NULL, NULL);
            }
          else
            {
              gimp_histogram_calculate (
                histogram, buffer,
                GEGL_RECTANGLE (x, y, width, height),
                NULL, NULL);
            }
        }

      if (projectable)
        gimp_projectable_end_render (projectable);

      g_object_unref (buffer);
    }

end:
  if (run_async && ! async)
    {
      async = gimp_async_new ();

      gimp_async_finish (async, NULL);
    }

  return async;
}


/*  public functions  */


void
gimp_drawable_calculate_histogram (GimpDrawable  *drawable,
                                   GimpHistogram *histogram,
                                   gboolean       with_filters)
{
  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (histogram != NULL);

  gimp_drawable_calculate_histogram_internal (drawable,
                                              histogram, with_filters,
                                              FALSE);
}

GimpAsync *
gimp_drawable_calculate_histogram_async (GimpDrawable  *drawable,
                                         GimpHistogram *histogram,
                                         gboolean       with_filters)
{
  g_return_val_if_fail (GIMP_IS_DRAWABLE (drawable), NULL);
  g_return_val_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)), NULL);
  g_return_val_if_fail (histogram != NULL, NULL);

  return gimp_drawable_calculate_histogram_internal (drawable,
                                                     histogram, with_filters,
                                                     TRUE);
}
