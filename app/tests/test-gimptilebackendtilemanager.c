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

#include "base/pixel-region.h"
#include "base/tile-cache.h"
#include "base/tile-manager.h"
#include "base/tile.h"

#include "gegl/gimptilebackendtilemanager.h"

#include "paint-funcs/paint-funcs.h"

#include "tests.h"
#include "gimp-app-test-utils.h"


#define ADD_TEST(function) \
  g_test_add_func ("/gimptilebackendtilemanager/" #function, function);


static void basic_read  (GeglRectangle rect);
static void basic_write (GeglRectangle rect);


static const guchar  opaque_magenta8[4]    = { 0xff,   0x00,   0xff,   0xff   };
static const guchar  transparent_black8[4] = { 0x00,   0x00,   0x00,   0x00   };

static const guint16 opaque_magenta16[4]   = { 0xffff, 0x0000, 0xffff, 0xffff };


/**
 * basic_read:
 * @rect: The rect to use. Vary this to vary number of tiles and their
 *        effective sizes used for the test.
 *
 * Test that the backend can be used for basic reading of TileManager
 * data.
 **/
static void
basic_read (GeglRectangle rect)
{
  PixelRegion      pr;
  TileManager     *tm;
  GeglTileBackend *backend;
  GeglBuffer      *buffer;
  guint16          actual_data[4];
  GeglRectangle    center_pixel = { rect.width / 2, rect.height / 2, 1,  1  };


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
 * @rect: The rect to use. Vary this to vary number of tiles and their
 *        effective sizes used for the test.
 *
 * Test that the backend can be used for basic writing of TileManager
 * data.
 **/
static void
basic_write (GeglRectangle rect)
{
  PixelRegion      pr;
  TileManager     *tm;
  GeglTileBackend *backend;
  GeglBuffer      *buffer;
  guchar           actual_data[4];
  gint             x, y;
  GeglRectangle    center_pixel = { rect.width / 2, rect.height / 2, 1,  1  };

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

static void
basic_read_1x1 (void)
{
  GeglRectangle rect = { 0, 0, 1, 1 };
  basic_read (rect);
}

static void
basic_write_1x1 (void)
{
  GeglRectangle rect = { 0, 0, 1, 1 };
  basic_write (rect);
}

static void
basic_read_10x10 (void)
{
  GeglRectangle rect = { 0, 0, 10, 10 };
  basic_read (rect);
}

static void
basic_write_10x10 (void)
{
  GeglRectangle rect = { 0, 0, 10, 10 };
  basic_write (rect);

}

static void
basic_read_TILE_WIDTHxTILE_HEIGHT (void)
{
  GeglRectangle rect = { 0, 0, TILE_WIDTH, TILE_HEIGHT };
  basic_read (rect);
}

static void
basic_write_TILE_WIDTHxTILE_HEIGHT (void)
{
  GeglRectangle rect = { 0, 0, TILE_WIDTH, TILE_HEIGHT };
  basic_write (rect);
}

static void
basic_read_3TILE_WIDTHx3TILE_HEIGHT (void)
{
  GeglRectangle rect = { 0, 0, 3 * TILE_WIDTH, 3 * TILE_HEIGHT };
  basic_read (rect);
}

static void
basic_write_3TILE_WIDTHx3TILE_HEIGHT (void)
{
  GeglRectangle rect = { 0, 0, 3 * TILE_WIDTH, 3 * TILE_HEIGHT };
  basic_write (rect);
}

static void
basic_read_2TILE_WIDTHx10 (void)
{
  GeglRectangle rect = { 0, 0, 2 * TILE_WIDTH, 10 };
  basic_read (rect);
}

static void
basic_write_2TILE_WIDTHx10 (void)
{
  GeglRectangle rect = { 0, 0, 2 * TILE_WIDTH, 10 };
  basic_write (rect);
}

static void
basic_read_10x2TILE_WIDTH (void)
{
  GeglRectangle rect = { 0, 0, 10, 2 * TILE_HEIGHT };
  basic_read (rect);
}

static void
basic_write_10x2TILE_WIDTH (void)
{
  GeglRectangle rect = { 0, 0, 10, 2 * TILE_HEIGHT };
  basic_write (rect);
}

static void
basic_read_100x100 (void)
{
  GeglRectangle rect = { 0, 0, 100, 100 };
  basic_read (rect);
}

static void
basic_write_100x100 (void)
{
  GeglRectangle rect = { 0, 0, 100, 100 };
  basic_write (rect);
}

int
main (int    argc,
      char **argv)
{
  g_type_init ();
  tile_cache_init (G_MAXUINT32);
  gegl_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  ADD_TEST (basic_read_1x1);
  ADD_TEST (basic_write_1x1);
  ADD_TEST (basic_read_10x10);
  ADD_TEST (basic_write_10x10);
  ADD_TEST (basic_read_TILE_WIDTHxTILE_HEIGHT);
  ADD_TEST (basic_write_TILE_WIDTHxTILE_HEIGHT);
  ADD_TEST (basic_read_3TILE_WIDTHx3TILE_HEIGHT);
  ADD_TEST (basic_write_3TILE_WIDTHx3TILE_HEIGHT);
  ADD_TEST (basic_read_2TILE_WIDTHx10);
  ADD_TEST (basic_write_2TILE_WIDTHx10);
  ADD_TEST (basic_read_10x2TILE_WIDTH);
  ADD_TEST (basic_write_10x2TILE_WIDTH);
  ADD_TEST (basic_read_100x100);
  ADD_TEST (basic_write_100x100);

  return g_test_run ();
}
