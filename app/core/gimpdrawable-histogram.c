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

#include "base/gimphistogram.h"
#include "base/pixel-region.h"

#include "gimpchannel.h"
#include "gimpdrawable-histogram.h"
#include "gimpimage.h"


void
gimp_drawable_calculate_histogram (GimpDrawable  *drawable,
                                   GimpHistogram *histogram)
{
  GimpImage   *image;
  PixelRegion  region;
  PixelRegion  mask;
  gint         x, y, width, height;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (histogram != NULL);

  if (! gimp_item_mask_intersect (GIMP_ITEM (drawable), &x, &y, &width, &height))
    return;

  pixel_region_init (&region, gimp_drawable_get_tiles (drawable),
                     x, y, width, height, FALSE);

  image = gimp_item_get_image (GIMP_ITEM (drawable));

  if (! gimp_channel_is_empty (gimp_image_get_mask (image)))
    {
      GimpChannel *sel_mask;
      GimpImage   *image;
      gint         off_x, off_y;

      image   = gimp_item_get_image (GIMP_ITEM (drawable));
      sel_mask = gimp_image_get_mask (image);

      gimp_item_get_offset (GIMP_ITEM (drawable), &off_x, &off_y);
      pixel_region_init (&mask,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (sel_mask)),
                         x + off_x, y + off_y, width, height, FALSE);
      gimp_histogram_calculate (histogram, &region, &mask);
    }
  else
    {
      gimp_histogram_calculate (histogram, &region, NULL);
    }
}
