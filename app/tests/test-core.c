/* LIGMA - The GNU Image Manipulation Program
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

#include "widgets/ligmauimanager.h"

#include "core/ligma.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-new.h"

#include "operations/ligmalevelsconfig.h"

#include "tests.h"

#include "ligma-app-test-utils.h"


#define LIGMA_TEST_IMAGE_SIZE 100

#define ADD_IMAGE_TEST(function) \
  g_test_add ("/ligma-core/" #function, \
              LigmaTestFixture, \
              ligma, \
              ligma_test_image_setup, \
              function, \
              ligma_test_image_teardown);

#define ADD_TEST(function) \
  g_test_add ("/ligma-core/" #function, \
              LigmaTestFixture, \
              ligma, \
              NULL, \
              function, \
              NULL);


typedef struct
{
  LigmaImage *image;
} LigmaTestFixture;


static void ligma_test_image_setup    (LigmaTestFixture *fixture,
                                      gconstpointer    data);
static void ligma_test_image_teardown (LigmaTestFixture *fixture,
                                      gconstpointer    data);


/**
 * ligma_test_image_setup:
 * @fixture:
 * @data:
 *
 * Test fixture setup for a single image.
 **/
static void
ligma_test_image_setup (LigmaTestFixture *fixture,
                       gconstpointer    data)
{
  Ligma *ligma = LIGMA (data);

  fixture->image = ligma_image_new (ligma,
                                   LIGMA_TEST_IMAGE_SIZE,
                                   LIGMA_TEST_IMAGE_SIZE,
                                   LIGMA_RGB,
                                   LIGMA_PRECISION_FLOAT_LINEAR);
}

/**
 * ligma_test_image_teardown:
 * @fixture:
 * @data:
 *
 * Test fixture teardown for a single image.
 **/
static void
ligma_test_image_teardown (LigmaTestFixture *fixture,
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
 * and call ligma_item_rotate with center at (0, -10)
 * without triggering a failed assertion .
 **/
static void
rotate_non_overlapping (LigmaTestFixture *fixture,
                        gconstpointer    data)
{
  Ligma        *ligma    = LIGMA (data);
  LigmaImage   *image   = fixture->image;
  LigmaLayer   *layer;
  LigmaContext *context = ligma_context_new (ligma, "Test", NULL /*template*/);
  gboolean     result;

  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 0);

  layer = ligma_layer_new (image,
                          LIGMA_TEST_IMAGE_SIZE,
                          LIGMA_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          LIGMA_OPACITY_OPAQUE,
                          LIGMA_LAYER_MODE_NORMAL);

  g_assert_cmpint (LIGMA_IS_LAYER (layer), ==, TRUE);

  result = ligma_image_add_layer (image,
                                 layer,
                                 LIGMA_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  ligma_item_rotate (LIGMA_ITEM (layer), context, LIGMA_ROTATE_90, 0., -10., TRUE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 1);
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
add_layer (LigmaTestFixture *fixture,
           gconstpointer    data)
{
  LigmaImage *image = fixture->image;
  LigmaLayer *layer;
  gboolean   result;

  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 0);

  layer = ligma_layer_new (image,
                          LIGMA_TEST_IMAGE_SIZE,
                          LIGMA_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          LIGMA_OPACITY_OPAQUE,
                          LIGMA_LAYER_MODE_NORMAL);

  g_assert_cmpint (LIGMA_IS_LAYER (layer), ==, TRUE);

  result = ligma_image_add_layer (image,
                                 layer,
                                 LIGMA_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 1);
}

/**
 * remove_layer:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can remove a layer.
 **/
static void
remove_layer (LigmaTestFixture *fixture,
              gconstpointer    data)
{
  LigmaImage *image = fixture->image;
  LigmaLayer *layer;
  gboolean   result;

  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 0);

  layer = ligma_layer_new (image,
                          LIGMA_TEST_IMAGE_SIZE,
                          LIGMA_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          LIGMA_OPACITY_OPAQUE,
                          LIGMA_LAYER_MODE_NORMAL);

  g_assert_cmpint (LIGMA_IS_LAYER (layer), ==, TRUE);

  result = ligma_image_add_layer (image,
                                 layer,
                                 LIGMA_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 1);

  ligma_image_remove_layer (image,
                           layer,
                           FALSE,
                           NULL);

  g_assert_cmpint (ligma_image_get_n_layers (image), ==, 0);
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
white_graypoint_in_red_levels (LigmaTestFixture *fixture,
                               gconstpointer    data)
{
  LigmaRGB              black   = { 0, 0, 0, 0 };
  LigmaRGB              gray    = { 1, 1, 1, 1 };
  LigmaRGB              white   = { 1, 1, 1, 1 };
  LigmaHistogramChannel channel = LIGMA_HISTOGRAM_RED;
  LigmaLevelsConfig    *config;

  config = g_object_new (LIGMA_TYPE_LEVELS_CONFIG, NULL);

  ligma_levels_config_adjust_by_colors (config,
                                       channel,
                                       &black,
                                       &gray,
                                       &white);

  /* Make sure we didn't end up with an invalid gamma value */
  g_object_set (config,
                "gamma", config->gamma[channel],
                NULL);
}

int
main (int    argc,
      char **argv)
{
  Ligma *ligma;
  int   result;

  g_test_init (&argc, &argv, NULL);

  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/ligmadir");

  /* We share the same application instance across all tests */
  ligma = ligma_init_for_testing ();

  /* Add tests */
  ADD_IMAGE_TEST (add_layer);
  ADD_IMAGE_TEST (remove_layer);
  ADD_IMAGE_TEST (rotate_non_overlapping);
  ADD_TEST (white_graypoint_in_red_levels);

  /* Run the tests */
  result = g_test_run ();

  /* Don't write files to the source dir */
  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/ligmadir-output");

  /* Exit so we don't break script-fu plug-in wire */
  ligma_exit (ligma, TRUE);

  return result;
}
