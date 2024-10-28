/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets/widgets-types.h"

#include "widgets/gimpuimanager.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimplayer-new.h"

#include "operations/gimplevelsconfig.h"

#include "tests.h"

#include "gimp-app-test-utils.h"


#define GIMP_TEST_IMAGE_SIZE 100

#define ADD_IMAGE_TEST(function) \
  g_test_add ("/gimp-core/" #function, \
              GimpTestFixture, \
              gimp, \
              gimp_test_image_setup, \
              function, \
              gimp_test_image_teardown);

#define ADD_TEST(function) \
  g_test_add ("/gimp-core/" #function, \
              GimpTestFixture, \
              gimp, \
              NULL, \
              function, \
              NULL);


typedef struct
{
  GimpImage *image;
} GimpTestFixture;


static void gimp_test_image_setup    (GimpTestFixture *fixture,
                                      gconstpointer    data);
static void gimp_test_image_teardown (GimpTestFixture *fixture,
                                      gconstpointer    data);


/**
 * gimp_test_image_setup:
 * @fixture:
 * @data:
 *
 * Test fixture setup for a single image.
 **/
static void
gimp_test_image_setup (GimpTestFixture *fixture,
                       gconstpointer    data)
{
  Gimp *gimp = GIMP (data);

  fixture->image = gimp_image_new (gimp,
                                   GIMP_TEST_IMAGE_SIZE,
                                   GIMP_TEST_IMAGE_SIZE,
                                   GIMP_RGB,
                                   GIMP_PRECISION_FLOAT_LINEAR);
}

/**
 * gimp_test_image_teardown:
 * @fixture:
 * @data:
 *
 * Test fixture teardown for a single image.
 **/
static void
gimp_test_image_teardown (GimpTestFixture *fixture,
                          gconstpointer    data)
{
  g_object_unref (fixture->image);
}

/**
 * rotate_non_overlapping:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can add a layer
 * and call gimp_item_rotate with center at (0, -10)
 * without triggering a failed assertion .
 **/
static void
rotate_non_overlapping (GimpTestFixture *fixture,
                        gconstpointer    data)
{
  Gimp        *gimp    = GIMP (data);
  GimpImage   *image   = fixture->image;
  GimpLayer   *layer;
  GimpContext *context = gimp_context_new (gimp, "Test", NULL /*template*/);
  gboolean     result;

  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 0);

  layer = gimp_layer_new (image,
                          GIMP_TEST_IMAGE_SIZE,
                          GIMP_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          GIMP_OPACITY_OPAQUE,
                          GIMP_LAYER_MODE_NORMAL);

  g_assert_cmpint (GIMP_IS_LAYER (layer), ==, TRUE);

  result = gimp_image_add_layer (image,
                                 layer,
                                 GIMP_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  gimp_item_rotate (GIMP_ITEM (layer), context, GIMP_ROTATE_DEGREES90, 0., -10., TRUE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 1);
  g_object_unref (context);
}

/**
 * add_layer:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can add a layer.
 **/
static void
add_layer (GimpTestFixture *fixture,
           gconstpointer    data)
{
  GimpImage *image = fixture->image;
  GimpLayer *layer;
  gboolean   result;

  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 0);

  layer = gimp_layer_new (image,
                          GIMP_TEST_IMAGE_SIZE,
                          GIMP_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          GIMP_OPACITY_OPAQUE,
                          GIMP_LAYER_MODE_NORMAL);

  g_assert_cmpint (GIMP_IS_LAYER (layer), ==, TRUE);

  result = gimp_image_add_layer (image,
                                 layer,
                                 GIMP_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 1);
}

/**
 * remove_layer:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can remove a layer.
 **/
static void
remove_layer (GimpTestFixture *fixture,
              gconstpointer    data)
{
  GimpImage *image = fixture->image;
  GimpLayer *layer;
  gboolean   result;

  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 0);

  layer = gimp_layer_new (image,
                          GIMP_TEST_IMAGE_SIZE,
                          GIMP_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          GIMP_OPACITY_OPAQUE,
                          GIMP_LAYER_MODE_NORMAL);

  g_assert_cmpint (GIMP_IS_LAYER (layer), ==, TRUE);

  result = gimp_image_add_layer (image,
                                 layer,
                                 GIMP_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 1);

  gimp_image_remove_layer (image,
                           layer,
                           FALSE,
                           NULL);

  g_assert_cmpint (gimp_image_get_n_layers (image), ==, 0);
}

/**
 * white_graypoint_in_red_levels:
 * @fixture:
 * @data:
 *
 * Makes sure the levels algorithm can handle when the graypoint is
 * white. It's easy to get a divide by zero problem when trying to
 * calculate what gamma will give a white graypoint.
 **/
static void
white_graypoint_in_red_levels (GimpTestFixture *fixture,
                               gconstpointer    data)
{
  GeglColor            *black   = gegl_color_new ("transparent");
  GeglColor            *gray    = gegl_color_new ("white");
  GeglColor            *white   = gegl_color_new ("white");
  GimpHistogramChannel  channel = GIMP_HISTOGRAM_RED;
  GimpLevelsConfig     *config;

  config = g_object_new (GIMP_TYPE_LEVELS_CONFIG, NULL);

  gimp_levels_config_adjust_by_colors (config,
                                       channel,
                                       NULL,
                                       black,
                                       gray,
                                       white);

  /* Make sure we didn't end up with an invalid gamma value */
  g_object_set (config,
                "gamma", config->gamma[channel],
                NULL);

  g_clear_object (&black);
  g_clear_object (&gray);
  g_clear_object (&white);
}

int
main (int    argc,
      char **argv)
{
  Gimp *gimp;
  int   result;

  g_test_init (&argc, &argv, NULL);

  gimp_test_utils_set_gimp3_directory ("GIMP_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/gimpdir");

  /* We share the same application instance across all tests */
  gimp = gimp_init_for_testing ();

  /* Add tests */
  ADD_IMAGE_TEST (add_layer);
  ADD_IMAGE_TEST (remove_layer);
  ADD_IMAGE_TEST (rotate_non_overlapping);
  ADD_TEST (white_graypoint_in_red_levels);

  /* Run the tests */
  result = g_test_run ();

  /* Don't write files to the source dir */
  gimp_test_utils_set_gimp3_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");

  /* Exit so we don't break script-fu plug-in wire */
  gimp_exit (gimp, TRUE);

  return result;
}
