/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * GimpText
 * Copyright (C) 2002-2003  Sven Neumann <sven@gimp.org>
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
#include <pango/pangoft2.h>

#include "text-types.h"

#include "base/pixel-region.h"
#include "base/tile-manager.h"

#include "paint-funcs/paint-funcs.h"

#include "gimptext-render.h"


TileManager *
gimp_text_render_layout (PangoLayout *layout,
			 gint         x,
			 gint         y,
			 gint         width,
			 gint         height)
{
  TileManager    *mask;
  FT_Bitmap       bitmap;
  PixelRegion     maskPR;
  gint            i;

  g_return_val_if_fail (PANGO_IS_LAYOUT (layout), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  bitmap.width = width;
  bitmap.rows  = height;
  bitmap.pitch = width;
  if (bitmap.pitch & 3)
    bitmap.pitch += 4 - (bitmap.pitch & 3);

  bitmap.buffer = g_malloc0 (bitmap.rows * bitmap.pitch);
      
  pango_ft2_render_layout (&bitmap, layout, x, y);

  mask = tile_manager_new (width, height, 1);
  pixel_region_init (&maskPR, mask, 0, 0, width, height, TRUE);

  for (i = 0; i < height; i++)
    pixel_region_set_row (&maskPR,
			  0, i, width, bitmap.buffer + i * bitmap.pitch);

  g_free (bitmap.buffer);

  return mask;
}
