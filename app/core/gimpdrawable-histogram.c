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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>

#include "core-types.h"

#include "base/pixel-region.h"

#include "gegl/gimp-gegl-nodes.h"

#include "gimp.h" /* gimp_use_gegl */
#include "gimpchannel.h"
#include "gimpdrawable-histogram.h"
#include "gimphistogram.h"
#include "gimpimage.h"


void
gimp_drawable_calculate_histogram (GimpDrawable  *drawable,
                                   GimpHistogram *histogram)
{
  GimpImage   *image;
  GimpChannel *mask;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (histogram != NULL);

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return;

  image = gimp_item_get_image (GIMP_ITEM (drawable));
  mask  = gimp_image_get_mask (image);

  if (FALSE) // gimp_use_gegl (image->gimp))
    {
      GeglNode      *node = gegl_node_new ();
      GeglNode      *buffer_source;
      GeglNode      *histogram_sink;
      GeglProcessor *processor;

      buffer_source =
        gimp_gegl_add_buffer_source (node,
                                     gimp_drawable_get_buffer (drawable),
                                     0, 0);

      histogram_sink =
        gegl_node_new_child (node,
                             "operation", "gimp:histogram-sink",
                             "histogram", histogram,
                             NULL);

      gegl_node_connect_to (buffer_source,  "output",
                            histogram_sink, "input");

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
      PixelRegion  region;
      PixelRegion  mask_region;

      pixel_region_init (&region, gimp_drawable_get_tiles (drawable),
                         x, y, width, height, FALSE);

      if (! gimp_channel_is_empty (mask))
        {
          gint off_x, off_y;

          gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
          pixel_region_init (&mask_region,
                             gimp_drawable_get_tiles (GIMP_DRAWABLE (mask)),
                             x + off_x, y + off_y, width, height, FALSE);
          gimp_histogram_calculate (histogram, &region, &mask_region);
        }
      else
        {
          gimp_histogram_calculate (histogram, &region, NULL);
        }
    }
}
