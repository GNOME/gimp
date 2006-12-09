/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimphistogram module Copyright (C) 1999 Jay Cox <jaycox@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib-object.h>

#include "core-types.h"

#include "base/gimphistogram.h"
#include "base/pixel-region.h"

#include "gimpdrawable.h"
#include "gimpdrawable-histogram.h"
#include "gimpimage.h"


void
gimp_drawable_calculate_histogram (GimpDrawable  *drawable,
                                   GimpHistogram *histogram)
{
  PixelRegion region;
  PixelRegion mask;
  gint        x1, y1, x2, y2;
  gboolean    have_mask;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (gimp_item_is_attached (GIMP_ITEM (drawable)));
  g_return_if_fail (histogram != NULL);

  have_mask = gimp_drawable_mask_bounds (drawable, &x1, &y1, &x2, &y2);

  if ((x1 == x2) || (y1 == y2))
    return;

  pixel_region_init (&region, gimp_drawable_get_tiles (drawable),
                     x1, y1, (x2 - x1), (y2 - y1), FALSE);

  if (have_mask)
    {
      GimpChannel *sel_mask;
      GimpImage   *image;
      gint         off_x, off_y;

      image   = gimp_item_get_image (GIMP_ITEM (drawable));
      sel_mask = gimp_image_get_mask (image);

      gimp_item_offsets (GIMP_ITEM (drawable), &off_x, &off_y);
      pixel_region_init (&mask,
                         gimp_drawable_get_tiles (GIMP_DRAWABLE (sel_mask)),
                         x1 + off_x, y1 + off_y, (x2 - x1), (y2 - y1), FALSE);
      gimp_histogram_calculate (histogram, &region, &mask);
    }
  else
    {
      gimp_histogram_calculate (histogram, &region, NULL);
    }
}
