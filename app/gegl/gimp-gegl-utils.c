/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimp-gegl-utils.h
 * Copyright (C) 2007 Michael Natterer <mitch@gimp.org>
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

#include "gimp-gegl-types.h"

#include "base/tile-manager.h"

#include "gimp-gegl-utils.h"


/**
 * gimp_bpp_to_babl_format:
 * @bpp: bytes per pixel
 * @linear: whether the pixels are linear or gamma-corrected.
 *
 * Return the Babl format to use for a given number of bytes per pixel.
 * This function assumes that the data is 8bit.
 *
 * Return value: the Babl format to use
 **/
const Babl *
gimp_bpp_to_babl_format (guint    bpp,
                         gboolean linear)
{
  g_return_val_if_fail (bpp > 0 && bpp <= 4, NULL);

  if (linear)
    {
      switch (bpp)
        {
        case 1:
          return babl_format ("Y u8");
        case 2:
          return babl_format ("YA u8");
        case 3:
          return babl_format ("RGB u8");
        case 4:
          return babl_format ("RGBA u8");
        }
    }
  else
    {
      switch (bpp)
        {
        case 1:
          return babl_format ("Y' u8");
        case 2:
          return babl_format ("Y'A u8");
        case 3:
          return babl_format ("R'G'B' u8");
        case 4:
          return babl_format ("R'G'B'A u8");
        }
    }

  return NULL;
}

static gint
gimp_babl_format_to_legacy_bpp (const Babl *format)
{
  return babl_format_get_n_components (format);
}

TileManager *
gimp_buffer_to_tiles (GeglBuffer *buffer)
{
  const Babl    *format     = gegl_buffer_get_format (buffer);
  TileManager   *new_tiles  = NULL;
  GeglNode      *source     = NULL;
  GeglNode      *sink       = NULL;

  g_return_val_if_fail (buffer != NULL, NULL);

  /* Setup and process the graph */
  new_tiles = tile_manager_new (gegl_buffer_get_width (buffer),
                                gegl_buffer_get_height (buffer),
                                gimp_babl_format_to_legacy_bpp (format));
  source = gegl_node_new_child (NULL,
                                "operation", "gegl:buffer-source",
                                "buffer",    buffer,
                                NULL);
  sink = gegl_node_new_child (NULL,
                              "operation",    "gimp:tilemanager-sink",
                              "tile-manager", new_tiles,
                              NULL);
  gegl_node_link_many (source, sink, NULL);
  gegl_node_process (sink);

  /* Clenaup */
  g_object_unref (sink);
  g_object_unref (source);

  return new_tiles;
}
