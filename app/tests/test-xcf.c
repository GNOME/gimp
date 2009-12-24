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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <gegl.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "core/core-types.h"

#include "core/gimp.h"
#include "core/gimpdrawable.h"
#include "core/gimpgrid.h"
#include "core/gimpguide.h"
#include "core/gimpimage.h"
#include "core/gimpimage-grid.h"
#include "core/gimpimage-guides.h"
#include "core/gimpimage-sample-points.h"
#include "core/gimplayer.h"
#include "core/gimpsamplepoint.h"

#include "file/file-open.h"
#include "file/file-procedure.h"
#include "file/file-save.h"

#include "plug-in/gimppluginmanager.h"

#include "tests.h"

#include "gimp-app-test-utils.h"


#define GIMP_MAINIMAGE_WIDTH           100
#define GIMP_MAINIMAGE_HEIGHT          90
#define GIMP_MAINIMAGE_TYPE            GIMP_RGB
#define GIMP_MAINIMAGE_LAYER1_WIDTH    50
#define GIMP_MAINIMAGE_LAYER1_HEIGHT   51
#define GIMP_MAINIMAGE_LAYER1_TYPE     GIMP_RGBA_IMAGE
#define GIMP_MAINIMAGE_LAYER1_NAME     "foo"
#define GIMP_MAINIMAGE_LAYER1_OPACITY  1.0
#define GIMP_MAINIMAGE_LAYER1_MODE     GIMP_NORMAL_MODE
#define GIMP_MAINIMAGE_VGUIDE1_POS     42
#define GIMP_MAINIMAGE_VGUIDE2_POS     82
#define GIMP_MAINIMAGE_HGUIDE1_POS     3
#define GIMP_MAINIMAGE_HGUIDE2_POS     4
#define GIMP_MAINIMAGE_SAMPLEPOINT1_X  10
#define GIMP_MAINIMAGE_SAMPLEPOINT1_Y  12
#define GIMP_MAINIMAGE_SAMPLEPOINT2_X  41
#define GIMP_MAINIMAGE_SAMPLEPOINT2_Y  49
#define GIMP_MAINIMAGE_RESOLUTIONX     400
#define GIMP_MAINIMAGE_RESOLUTIONY     410
#define GIMP_MAINIMAGE_PARASITE_NAME   "test-parasite"
#define GIMP_MAINIMAGE_PARASITE_DATA   "foo"
#define GIMP_MAINIMAGE_PARASITE_SIZE   4                /* 'f' 'o' 'o' '\0' */
#define GIMP_MAINIMAGE_UNIT            GIMP_UNIT_PICA
#define GIMP_MAINIMAGE_GRIDXSPACING    25.0
#define GIMP_MAINIMAGE_GRIDYSPACING    27.0

typedef struct
{
  gint avoid_sizeof_zero;
} GimpTestFixture;


static void        gimp_write_and_read_current_format (GimpTestFixture *fixture,
                                                       gconstpointer    data);
static GimpImage * gimp_create_mainimage              (void);
static void        gimp_assert_mainimage              (GimpImage       *image);


static Gimp *gimp = NULL;


/**
 * gimp_write_and_read_current_format:
 * @fixture:
 * @data:
 *
 * Constructs the main test image and asserts its state, writes it to
 * a file, reads the image from the file, and asserts the state of the
 * loaded file.
 **/
static void
gimp_write_and_read_current_format (GimpTestFixture *fixture,
                                    gconstpointer    data)
{
  GimpImage           *image        = NULL;
  GimpImage           *loaded_image = NULL;
  GimpPlugInProcedure *proc         = NULL;
  gchar               *uri          = NULL;
  GimpPDBStatusType    status       = 0;

  /* Create the image */
  image = gimp_create_mainimage ();

  /* Assert valid state */
  gimp_assert_mainimage (image);

  /* Write to file */
  uri  = g_build_filename (g_get_tmp_dir (), "gimp-test.xcf", NULL);
  proc = file_procedure_find (image->gimp->plug_in_manager->save_procs,
                              uri,
                              NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             uri,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export*/,
             NULL /*error*/);

  /* Load from file */
  proc = file_procedure_find (image->gimp->plug_in_manager->load_procs,
                              uri,
                              NULL /*error*/);
  loaded_image = file_open_image (gimp,
                                  gimp_get_user_context (gimp),
                                  NULL /*progress*/,
                                  uri,
                                  "irrelevant" /*entered_filename*/,
                                  FALSE /*as_new*/,
                                  proc,
                                  GIMP_RUN_NONINTERACTIVE,
                                  &status,
                                  NULL /*mime_type*/,
                                  NULL /*error*/);
  g_free (uri);

  /* Assert on the loaded file. If success, it means that there is no
   * significant information loss when we wrote the image to a file
   * and loaded it again
   */
  gimp_assert_mainimage (loaded_image);
}

/**
 * gimp_create_mainimage:
 *
 * Creates the main test image, i.e. the image that we use for most of
 * our XCF testing purposes.
 *
 * Returns: The #GimpImage
 **/
static GimpImage *
gimp_create_mainimage (void)
{
  GimpImage    *image    = NULL;
  GimpLayer    *layer    = NULL;
  GimpParasite *parasite = NULL;
  GimpGrid     *grid     = NULL;

  /* Image size and type */
  image = gimp_image_new (gimp,
                          GIMP_MAINIMAGE_WIDTH,
                          GIMP_MAINIMAGE_HEIGHT,
                          GIMP_MAINIMAGE_TYPE);

  /* Layer */
  layer = gimp_layer_new (image,
                          GIMP_MAINIMAGE_LAYER1_WIDTH,
                          GIMP_MAINIMAGE_LAYER1_HEIGHT,
                          GIMP_MAINIMAGE_LAYER1_TYPE,
                          GIMP_MAINIMAGE_LAYER1_NAME,
                          GIMP_MAINIMAGE_LAYER1_OPACITY,
                          GIMP_MAINIMAGE_LAYER1_MODE);
  gimp_image_add_layer (image,
                        layer,
                        NULL,
                        -1,
                        FALSE/*push_undo*/);

  /* Image compression type
   *
   * We don't do any explicit test, only implicit when we read tile
   * data in other tests
   */

  /* Guides, note we add them in reversed order */
  gimp_image_add_hguide (image,
                         GIMP_MAINIMAGE_HGUIDE2_POS,
                         FALSE /*push_undo*/);
  gimp_image_add_hguide (image,
                         GIMP_MAINIMAGE_HGUIDE1_POS,
                         FALSE /*push_undo*/);
  gimp_image_add_vguide (image,
                         GIMP_MAINIMAGE_VGUIDE2_POS,
                         FALSE /*push_undo*/);
  gimp_image_add_vguide (image,
                         GIMP_MAINIMAGE_VGUIDE1_POS,
                         FALSE /*push_undo*/);


  /* Sample points */
  gimp_image_add_sample_point_at_pos (image,
                                      GIMP_MAINIMAGE_SAMPLEPOINT1_X,
                                      GIMP_MAINIMAGE_SAMPLEPOINT1_Y,
                                      FALSE /*push_undo*/);
  gimp_image_add_sample_point_at_pos (image,
                                      GIMP_MAINIMAGE_SAMPLEPOINT2_X,
                                      GIMP_MAINIMAGE_SAMPLEPOINT2_Y,
                                      FALSE /*push_undo*/);

  /* Tatto
   * We don't bother testing this, not yet at least
   */

  /* Resolution */
  gimp_image_set_resolution (image,
                             GIMP_MAINIMAGE_RESOLUTIONX,
                             GIMP_MAINIMAGE_RESOLUTIONY);


  /* Parasites */
  parasite = gimp_parasite_new (GIMP_MAINIMAGE_PARASITE_NAME,
                                GIMP_PARASITE_PERSISTENT,
                                GIMP_MAINIMAGE_PARASITE_SIZE,
                                GIMP_MAINIMAGE_PARASITE_DATA);
  gimp_image_parasite_attach (image,
                              parasite);

  /* Unit */
  gimp_image_set_unit (image,
                       GIMP_MAINIMAGE_UNIT);

  /* Grid */
  grid = g_object_new (GIMP_TYPE_GRID,
                       "xspacing", GIMP_MAINIMAGE_GRIDXSPACING,
                       "yspacing", GIMP_MAINIMAGE_GRIDYSPACING,
                       NULL);
  gimp_image_set_grid (image,
                       grid,
                       FALSE /*push_undo*/);
  g_object_unref (grid);


  /* Todo, test somehow:
   *
   * - Color maps
   * - Custom user units
   * - Old-style paths, gimp_vectors_compat_is_compatible()
   * - New-style paths
   * - Layers, more compliated compositions
   * - Channels, including selection
   * - Floating selection
   */
  
  return image;
}

/**
 * gimp_assert_mainimage:
 * @image:
 *
 * Verifies that the passed #GimpImage contains all the information
 * that was put in it by gimp_create_mainimage().
 **/
static void
gimp_assert_mainimage (GimpImage *image)
{
  const GimpParasite *parasite     = NULL;
  GimpLayer          *layer        = NULL;
  GList              *iter         = NULL;
  GimpGuide          *guide        = NULL;
  GimpSamplePoint    *sample_point = NULL;
  gdouble             xres         = 0.0;
  gdouble             yres         = 0.0;
  GimpGrid           *grid         = NULL;
  gdouble             xspacing     = 0.0;
  gdouble             yspacing     = 0.0;

  /* Image size and type */
  g_assert_cmpint (gimp_image_get_width (image),
                   ==,
                   GIMP_MAINIMAGE_WIDTH);
  g_assert_cmpint (gimp_image_get_height (image),
                   ==,
                   GIMP_MAINIMAGE_HEIGHT);
  g_assert_cmpint (gimp_image_base_type (image),
                   ==,
                   GIMP_MAINIMAGE_TYPE);

  /* Layer */
  layer = gimp_image_get_layer_by_name (image,
                                        GIMP_MAINIMAGE_LAYER1_NAME);
  g_assert_cmpint (gimp_item_get_width (GIMP_ITEM (layer)),
                   ==,
                   GIMP_MAINIMAGE_LAYER1_WIDTH);
  g_assert_cmpint (gimp_item_get_height (GIMP_ITEM (layer)),
                   ==,
                   GIMP_MAINIMAGE_LAYER1_HEIGHT);
  g_assert_cmpint (gimp_drawable_type (GIMP_DRAWABLE (layer)),
                   ==,
                   GIMP_MAINIMAGE_LAYER1_TYPE);
  g_assert_cmpstr (gimp_object_get_name (GIMP_DRAWABLE (layer)),
                   ==,
                   GIMP_MAINIMAGE_LAYER1_NAME);
  g_assert_cmpfloat (gimp_layer_get_opacity (layer),
                     ==,
                     GIMP_MAINIMAGE_LAYER1_OPACITY);
  g_assert_cmpint (gimp_layer_get_mode (layer),
                   ==,
                   GIMP_MAINIMAGE_LAYER1_MODE);

  /* Guides, note that we rely on internal ordering */
  iter = gimp_image_get_guides (image);
  g_assert (iter != NULL);
  guide = GIMP_GUIDE (iter->data);
  g_assert_cmpint (gimp_guide_get_position (guide),
                   ==,
                   GIMP_MAINIMAGE_VGUIDE1_POS);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  guide = GIMP_GUIDE (iter->data);
  g_assert_cmpint (gimp_guide_get_position (guide),
                   ==,
                   GIMP_MAINIMAGE_VGUIDE2_POS);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  guide = GIMP_GUIDE (iter->data);
  g_assert_cmpint (gimp_guide_get_position (guide),
                   ==,
                   GIMP_MAINIMAGE_HGUIDE1_POS);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  guide = GIMP_GUIDE (iter->data);
  g_assert_cmpint (gimp_guide_get_position (guide),
                   ==,
                   GIMP_MAINIMAGE_HGUIDE2_POS);
  iter = g_list_next (iter);
  g_assert (iter == NULL);

  /* Sample points, we rely on the same ordering as when we added
   * them, although this ordering is not a necessaity
   */
  iter = gimp_image_get_sample_points (image);
  g_assert (iter != NULL);
  sample_point = (GimpSamplePoint *) iter->data;
  g_assert_cmpint (sample_point->x,
                   ==,
                   GIMP_MAINIMAGE_SAMPLEPOINT1_X);
  g_assert_cmpint (sample_point->y,
                   ==,
                   GIMP_MAINIMAGE_SAMPLEPOINT1_Y);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  sample_point = (GimpSamplePoint *) iter->data;
  g_assert_cmpint (sample_point->x,
                   ==,
                   GIMP_MAINIMAGE_SAMPLEPOINT2_X);
  g_assert_cmpint (sample_point->y,
                   ==,
                   GIMP_MAINIMAGE_SAMPLEPOINT2_Y);
  iter = g_list_next (iter);
  g_assert (iter == NULL);

  /* Resolution */
  gimp_image_get_resolution (image, &xres, &yres);
  g_assert_cmpint (xres,
                   ==,
                   GIMP_MAINIMAGE_RESOLUTIONX);
  g_assert_cmpint (yres,
                   ==,
                   GIMP_MAINIMAGE_RESOLUTIONY);

  /* Parasites */
  parasite = gimp_image_parasite_find (image,
                                       GIMP_MAINIMAGE_PARASITE_NAME);
  g_assert_cmpint (gimp_parasite_data_size (parasite),
                   ==,
                   GIMP_MAINIMAGE_PARASITE_SIZE);
  g_assert_cmpstr (gimp_parasite_data (parasite),
                   ==,
                   GIMP_MAINIMAGE_PARASITE_DATA);

  /* Unit */
  g_assert_cmpint (gimp_image_get_unit (image),
                   ==,
                   GIMP_MAINIMAGE_UNIT);
  
  /* Grid */
  grid = gimp_image_get_grid (image);
  g_object_get (grid,
                "xspacing", &xspacing,
                "yspacing", &yspacing,
                NULL);
  g_assert_cmpint (xspacing,
                   ==,
                   GIMP_MAINIMAGE_GRIDXSPACING);
  g_assert_cmpint (yspacing,
                   ==,
                   GIMP_MAINIMAGE_GRIDYSPACING);
}


/**
 * main:
 * @argc:
 * @argv:
 *
 * These tests, when done, will
 *
 *  - Make sure that we are backwards compatible with files created by
 *    older version of GIMP, i.e. that we can load files from earlier
 *    version of GIMP
 *
 *  - Make sure that the information put into a #GimpImage is not lost
 *    when the #GimpImage is written to a file and then read again
 **/
int
main (int    argc,
      char **argv)
{
  g_type_init ();
  gtk_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  gimp_test_utils_set_gimp2_directory ("gimpdir");

  /* We share the same application instance across all tests. We need
   * the GUI variant for the file procs
   */
  gimp = gimp_init_for_gui_testing (TRUE, FALSE);

  /* Setup the tests */
  g_test_add ("/gimp-xcf/write-and-read-current-format",
              GimpTestFixture,
              NULL,
              NULL,
              gimp_write_and_read_current_format,
              NULL);

  /* Exit so we don't break script-fu plug-in wire */
  gimp_exit (gimp, TRUE);

  /* Run the tests and return status */
  return g_test_run ();
}
