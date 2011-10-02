/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * test-gimptilebackendtilemanager.c
 * Copyright (C) 2011 Martin Nordholts <martinn@src.gnome.org>
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

#include <gegl.h>
#include <gtk/gtk.h>
#include <string.h>

#include "widgets/widgets-types.h"

#include "base/tile-manager.h"
#include "base/pixel-region.h"
#include "base/tile-cache.h"

#include "gegl/gimptilebackendtilemanager.h"

#include "paint-funcs/paint-funcs.h"

#include "tests.h"
#include "gimp-app-test-utils.h"


#define ADD_TEST(function) \
  g_test_add_func ("/gimptilebackendtilemanager/" #function, function);


static const guchar  opaque_magenta8[4]    = { 0xff,   0x00,   0xff,   0xff   };
static const guchar  transparent_black8[4] = { 0x00,   0x00,   0x00,   0x00   };

static const guint16 opaque_magenta16[4]   = { 0xffff, 0x0000, 0xffff, 0xffff };

/* FIXME: Add tests for non-tile sized rects, they currently won't
 * pass
 */
static const GeglRectangle rect            = { 0, 0, 64, 64 };
static const GeglRectangle center_pixel    = { 5, 5, 1,  1  };


/**
 * basic_read:
 * @data:
 *
 * Test that the backend can be used for basic reading of TileManager
 * data.
 **/
static void
basic_read (void)
{
  PixelRegion      pr;
  TileManager     *tm;
  GeglTileBackend *backend;
  GeglBuffer      *buffer;
  guint16          actual_data[4];

  /* Write some pixels to the tile manager */
  tm = tile_manager_new (rect.width, rect.height, 4);
  pixel_region_init (&pr, tm, rect.x, rect.y, rect.width, rect.height, TRUE);
  color_region (&pr, opaque_magenta8);

  /* Make sure we can read them through the GeglBuffer using the
   * TileManager backend. Use u16 to complicate code paths, decreasing
   * risk of the test accidentally passing
   */
  backend = gimp_tile_backend_tile_manager_new (tm);
  buffer  = gegl_buffer_new_for_backend (NULL, backend);
  gegl_buffer_get (buffer, 1.0 /*scale*/, &center_pixel,
                   babl_format ("RGBA u16"), actual_data,
                   GEGL_AUTO_ROWSTRIDE);
  g_assert_cmpint (0, ==, memcmp (opaque_magenta16,
                                  actual_data,
                                  sizeof (actual_data)));
}

/**
 * basic_write:
 * @data:
 *
 * Test that the backend can be used for basic writing of TileManager
 * data.
 **/
static void
basic_write (void)
{
  PixelRegion      pr;
  TileManager     *tm;
  GeglTileBackend *backend;
  GeglBuffer      *buffer;
  guchar           actual_data[4];
  gint             x, y;

  /* Clear the TileManager */
  tm = tile_manager_new (rect.width, rect.height, 4);
  pixel_region_init (&pr, tm, rect.x, rect.y, rect.width, rect.height, TRUE);
  color_region (&pr, transparent_black8);

  /* Write some data using the GeglBuffer and the backend. Use u16 to
   * complicate code paths, decreasing risk of the test accidentally
   * passing
   */
  backend = gimp_tile_backend_tile_manager_new (tm);
  buffer  = gegl_buffer_new_for_backend (NULL, backend);
  for (y = 0; y < rect.height; y++)
    for (x = 0; x < rect.width; x++)
      {
        GeglRectangle moving_rect = { x, y, 1, 1 };
        gegl_buffer_set (buffer, &moving_rect,
                         babl_format ("RGBA u16"), (gpointer) opaque_magenta16,
                         GEGL_AUTO_ROWSTRIDE);
      }

  /* Make sure we can read the written data from the TileManager */
  tile_manager_read_pixel_data_1 (tm, center_pixel.x, center_pixel.y,
                                  actual_data);
  g_assert_cmpint (0, ==, memcmp (opaque_magenta8,
                                  actual_data,
                                  sizeof (actual_data)));
}

int
main (int    argc,
      char **argv)
{
  g_type_init ();
  tile_cache_init (G_MAXUINT32);
  gegl_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (basic_read);
  ADD_TEST (basic_write);

  return g_test_run ();
}
