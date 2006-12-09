/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997-2004 Adam D. Moss <adam@gimp.org>
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

#include "libgimpcolor/gimpcolor.h"

#include "core-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "gimpdrawable.h"
#include "gimpdrawable-convert.h"


void
gimp_drawable_convert_rgb (GimpDrawable      *drawable,
                           TileManager       *new_tiles,
                           GimpImageBaseType  old_base_type)
{
  PixelRegion   srcPR, destPR;
  gint          row, col;
  gint          offset;
  gint          has_alpha;
  const guchar *src, *s;
  guchar       *dest, *d;
  const guchar *cmap;
  gpointer      pr;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (new_tiles != NULL);

  has_alpha = gimp_drawable_has_alpha (drawable);

  g_return_if_fail (tile_manager_bpp (new_tiles) == (has_alpha ? 4 : 3));

  cmap = gimp_drawable_get_colormap (drawable);

  pixel_region_init (&srcPR, drawable->tiles,
                     0, 0,
                     GIMP_ITEM (drawable)->width,
                     GIMP_ITEM (drawable)->height,
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     GIMP_ITEM (drawable)->width,
                     GIMP_ITEM (drawable)->height,
                     TRUE);


  switch (old_base_type)
    {
    case GIMP_GRAY:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src  = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;

              for (col = 0; col < srcPR.w; col++)
                {
                  d[RED_PIX] = *s;
                  d[GREEN_PIX] = *s;
                  d[BLUE_PIX] = *s;

                  d += 3;
                  s++;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    case GIMP_INDEXED:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src  = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;

              for (col = 0; col < srcPR.w; col++)
                {
                  offset = *s++ * 3;
                  d[RED_PIX] = cmap[offset + 0];
                  d[GREEN_PIX] = cmap[offset + 1];
                  d[BLUE_PIX] = cmap[offset + 2];

                  d += 3;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    default:
      break;
    }
}

void
gimp_drawable_convert_grayscale (GimpDrawable      *drawable,
                                 TileManager       *new_tiles,
                                 GimpImageBaseType  old_base_type)
{
  PixelRegion   srcPR, destPR;
  gint          row, col;
  gint          offset, val;
  gboolean      has_alpha;
  const guchar *src, *s;
  guchar       *dest, *d;
  const guchar *cmap;
  gpointer      pr;

  g_return_if_fail (GIMP_IS_DRAWABLE (drawable));
  g_return_if_fail (new_tiles != NULL);

  has_alpha = gimp_drawable_has_alpha (drawable);

  g_return_if_fail (tile_manager_bpp (new_tiles) == (has_alpha ? 2 : 1));

  cmap = gimp_drawable_get_colormap (drawable);

  pixel_region_init (&srcPR, drawable->tiles,
                     0, 0,
                     GIMP_ITEM (drawable)->width,
                     GIMP_ITEM (drawable)->height,
                     FALSE);
  pixel_region_init (&destPR, new_tiles,
                     0, 0,
                     GIMP_ITEM (drawable)->width,
                     GIMP_ITEM (drawable)->height,
                     TRUE);

  switch (old_base_type)
    {
    case GIMP_RGB:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;
              for (col = 0; col < srcPR.w; col++)
                {
                  val = GIMP_RGB_LUMINANCE (s[RED_PIX],
                                            s[GREEN_PIX],
                                            s[BLUE_PIX]) + 0.5;
                  *d++ = (guchar) val;
                  s += 3;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    case GIMP_INDEXED:
      for (pr = pixel_regions_register (2, &srcPR, &destPR);
           pr != NULL;
           pr = pixel_regions_process (pr))
        {
          src = srcPR.data;
          dest = destPR.data;

          for (row = 0; row < srcPR.h; row++)
            {
              s = src;
              d = dest;

              for (col = 0; col < srcPR.w; col++)
                {
                  offset = *s++ * 3;
                  val = GIMP_RGB_LUMINANCE (cmap[offset+0],
                                            cmap[offset+1],
                                            cmap[offset+2]) + 0.5;
                  *d++ = (guchar) val;
                  if (has_alpha)
                    *d++ = *s++;
                }

              src += srcPR.rowstride;
              dest += destPR.rowstride;
            }
        }
      break;

    default:
      break;
    }
}
