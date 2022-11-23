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

#include "config.h"

#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"

#include "widgets/widgets-types.h"

#include "widgets/ligmauimanager.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmachannel-select.h"
#include "core/ligmadrawable.h"
#include "core/ligmagrid.h"
#include "core/ligmagrouplayer.h"
#include "core/ligmaguide.h"
#include "core/ligmaimage.h"
#include "core/ligmaimage-grid.h"
#include "core/ligmaimage-guides.h"
#include "core/ligmaimage-sample-points.h"
#include "core/ligmalayer.h"
#include "core/ligmalayer-new.h"
#include "core/ligmasamplepoint.h"
#include "core/ligmaselection.h"

#include "vectors/ligmaanchor.h"
#include "vectors/ligmabezierstroke.h"
#include "vectors/ligmavectors.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"

#include "tests.h"

#include "ligma-app-test-utils.h"


/* we continue to use LEGACY layers for testing, so we can use the
 * same test image for all tests, including loading
 * files/ligma-2-6-file.xcf which can't have any non-LEGACY modes
 */

#define LIGMA_MAINIMAGE_WIDTH            100
#define LIGMA_MAINIMAGE_HEIGHT           90
#define LIGMA_MAINIMAGE_TYPE             LIGMA_RGB
#define LIGMA_MAINIMAGE_PRECISION        LIGMA_PRECISION_U8_NON_LINEAR

#define LIGMA_MAINIMAGE_LAYER1_NAME      "layer1"
#define LIGMA_MAINIMAGE_LAYER1_WIDTH     50
#define LIGMA_MAINIMAGE_LAYER1_HEIGHT    51
#define LIGMA_MAINIMAGE_LAYER1_FORMAT    babl_format ("R'G'B'A u8")
#define LIGMA_MAINIMAGE_LAYER1_OPACITY   LIGMA_OPACITY_OPAQUE
#define LIGMA_MAINIMAGE_LAYER1_MODE      LIGMA_LAYER_MODE_NORMAL_LEGACY

#define LIGMA_MAINIMAGE_LAYER2_NAME      "layer2"
#define LIGMA_MAINIMAGE_LAYER2_WIDTH     25
#define LIGMA_MAINIMAGE_LAYER2_HEIGHT    251
#define LIGMA_MAINIMAGE_LAYER2_FORMAT    babl_format ("R'G'B' u8")
#define LIGMA_MAINIMAGE_LAYER2_OPACITY   LIGMA_OPACITY_TRANSPARENT
#define LIGMA_MAINIMAGE_LAYER2_MODE      LIGMA_LAYER_MODE_MULTIPLY_LEGACY

#define LIGMA_MAINIMAGE_GROUP1_NAME      "group1"

#define LIGMA_MAINIMAGE_LAYER3_NAME      "layer3"

#define LIGMA_MAINIMAGE_LAYER4_NAME      "layer4"

#define LIGMA_MAINIMAGE_GROUP2_NAME      "group2"

#define LIGMA_MAINIMAGE_LAYER5_NAME      "layer5"

#define LIGMA_MAINIMAGE_VGUIDE1_POS      42
#define LIGMA_MAINIMAGE_VGUIDE2_POS      82
#define LIGMA_MAINIMAGE_HGUIDE1_POS      3
#define LIGMA_MAINIMAGE_HGUIDE2_POS      4

#define LIGMA_MAINIMAGE_SAMPLEPOINT1_X   10
#define LIGMA_MAINIMAGE_SAMPLEPOINT1_Y   12
#define LIGMA_MAINIMAGE_SAMPLEPOINT2_X   41
#define LIGMA_MAINIMAGE_SAMPLEPOINT2_Y   49

#define LIGMA_MAINIMAGE_RESOLUTIONX      400
#define LIGMA_MAINIMAGE_RESOLUTIONY      410

#define LIGMA_MAINIMAGE_PARASITE_NAME    "test-parasite"
#define LIGMA_MAINIMAGE_PARASITE_DATA    "foo"
#define LIGMA_MAINIMAGE_PARASITE_SIZE    4                /* 'f' 'o' 'o' '\0' */

#define LIGMA_MAINIMAGE_COMMENT          "Created with code from "\
                                        "app/tests/test-xcf.c in the LIGMA "\
                                        "source tree, i.e. it was not created "\
                                        "manually and may thus look weird if "\
                                        "opened and inspected in LIGMA."

#define LIGMA_MAINIMAGE_UNIT             LIGMA_UNIT_PICA

#define LIGMA_MAINIMAGE_GRIDXSPACING     25.0
#define LIGMA_MAINIMAGE_GRIDYSPACING     27.0

#define LIGMA_MAINIMAGE_CHANNEL1_NAME    "channel1"
#define LIGMA_MAINIMAGE_CHANNEL1_WIDTH   LIGMA_MAINIMAGE_WIDTH
#define LIGMA_MAINIMAGE_CHANNEL1_HEIGHT  LIGMA_MAINIMAGE_HEIGHT
#define LIGMA_MAINIMAGE_CHANNEL1_COLOR   { 1.0, 0.0, 1.0, 1.0 }

#define LIGMA_MAINIMAGE_SELECTION_X      5
#define LIGMA_MAINIMAGE_SELECTION_Y      6
#define LIGMA_MAINIMAGE_SELECTION_W      7
#define LIGMA_MAINIMAGE_SELECTION_H      8

#define LIGMA_MAINIMAGE_VECTORS1_NAME    "vectors1"
#define LIGMA_MAINIMAGE_VECTORS1_COORDS  { { 11.0, 12.0, /* pad zeroes */ },\
                                          { 21.0, 22.0, /* pad zeroes */ },\
                                          { 31.0, 32.0, /* pad zeroes */ }, }

#define LIGMA_MAINIMAGE_VECTORS2_NAME    "vectors2"
#define LIGMA_MAINIMAGE_VECTORS2_COORDS  { { 911.0, 912.0, /* pad zeroes */ },\
                                          { 921.0, 922.0, /* pad zeroes */ },\
                                          { 931.0, 932.0, /* pad zeroes */ }, }

#define ADD_TEST(function) \
  g_test_add_data_func ("/ligma-xcf/" #function, ligma, function);


LigmaImage        * ligma_test_load_image                        (Ligma            *ligma,
                                                                GFile           *file);
static void        ligma_write_and_read_file                    (Ligma            *ligma,
                                                                gboolean         with_unusual_stuff,
                                                                gboolean         compat_paths,
                                                                gboolean         use_ligma_2_8_features);
static LigmaImage * ligma_create_mainimage                       (Ligma            *ligma,
                                                                gboolean         with_unusual_stuff,
                                                                gboolean         compat_paths,
                                                                gboolean         use_ligma_2_8_features);
static void        ligma_assert_mainimage                       (LigmaImage       *image,
                                                                gboolean         with_unusual_stuff,
                                                                gboolean         compat_paths,
                                                                gboolean         use_ligma_2_8_features);


/**
 * write_and_read_ligma_2_6_format:
 * @data:
 *
 * Do a write and read test on a file that could as well be
 * constructed with LIGMA 2.6.
 **/
static void
write_and_read_ligma_2_6_format (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);

  ligma_write_and_read_file (ligma,
                            FALSE /*with_unusual_stuff*/,
                            FALSE /*compat_paths*/,
                            FALSE /*use_ligma_2_8_features*/);
}

/**
 * write_and_read_ligma_2_6_format_unusual:
 * @data:
 *
 * Do a write and read test on a file that could as well be
 * constructed with LIGMA 2.6, and make it unusual, like compatible
 * vectors and with a floating selection.
 **/
static void
write_and_read_ligma_2_6_format_unusual (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);

  ligma_write_and_read_file (ligma,
                            TRUE /*with_unusual_stuff*/,
                            TRUE /*compat_paths*/,
                            FALSE /*use_ligma_2_8_features*/);
}

/**
 * load_ligma_2_6_file:
 * @data:
 *
 * Loads a file created with LIGMA 2.6 and makes sure it loaded as
 * expected.
 **/
static void
load_ligma_2_6_file (gconstpointer data)
{
  Ligma      *ligma = LIGMA (data);
  LigmaImage *image;
  gchar     *filename;
  GFile     *file;

  filename = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                               "app/tests/files/ligma-2-6-file.xcf",
                               NULL);
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = ligma_test_load_image (ligma, file);

  /* The image file was constructed by running
   * ligma_write_and_read_file (FALSE, FALSE) in LIGMA 2.6 by
   * copy-pasting the code to LIGMA 2.6 and adapting it to changes in
   * the core API, so we can use ligma_assert_mainimage() to make sure
   * the file was loaded successfully.
   */
  ligma_assert_mainimage (image,
                         FALSE /*with_unusual_stuff*/,
                         FALSE /*compat_paths*/,
                         FALSE /*use_ligma_2_8_features*/);
}

/**
 * write_and_read_ligma_2_8_format:
 * @data:
 *
 * Writes an XCF file that uses LIGMA 2.8 features such as layer
 * groups, then reads the file and make sure no relevant information
 * was lost.
 **/
static void
write_and_read_ligma_2_8_format (gconstpointer data)
{
  Ligma *ligma = LIGMA (data);

  ligma_write_and_read_file (ligma,
                            FALSE /*with_unusual_stuff*/,
                            FALSE /*compat_paths*/,
                            TRUE /*use_ligma_2_8_features*/);
}

LigmaImage *
ligma_test_load_image (Ligma  *ligma,
                      GFile *file)
{
  LigmaPlugInProcedure *proc;
  LigmaImage           *image;
  LigmaPDBStatusType    unused;

  proc = ligma_plug_in_manager_file_procedure_find (ligma->plug_in_manager,
                                                   LIGMA_FILE_PROCEDURE_GROUP_OPEN,
                                                   file,
                                                   NULL /*error*/);
  image = file_open_image (ligma,
                           ligma_get_user_context (ligma),
                           NULL /*progress*/,
                           file,
                           FALSE /*as_new*/,
                           proc,
                           LIGMA_RUN_NONINTERACTIVE,
                           &unused /*status*/,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  return image;
}

/**
 * ligma_write_and_read_file:
 *
 * Constructs the main test image and asserts its state, writes it to
 * a file, reads the image from the file, and asserts the state of the
 * loaded file. The function takes various parameters so the same
 * function can be used for different formats.
 **/
static void
ligma_write_and_read_file (Ligma     *ligma,
                          gboolean  with_unusual_stuff,
                          gboolean  compat_paths,
                          gboolean  use_ligma_2_8_features)
{
  LigmaImage           *image;
  LigmaImage           *loaded_image;
  LigmaPlugInProcedure *proc;
  gchar               *filename = NULL;
  gint                 file_handle;
  GFile               *file;

  /* Create the image */
  image = ligma_create_mainimage (ligma,
                                 with_unusual_stuff,
                                 compat_paths,
                                 use_ligma_2_8_features);

  /* Assert valid state */
  ligma_assert_mainimage (image,
                         with_unusual_stuff,
                         compat_paths,
                         use_ligma_2_8_features);

  /* Write to file */
  file_handle = g_file_open_tmp ("ligma-test-XXXXXX.xcf", &filename, NULL);
  g_assert (file_handle != -1);
  close (file_handle);
  file = g_file_new_for_path (filename);
  g_free (filename);

  proc = ligma_plug_in_manager_file_procedure_find (image->ligma->plug_in_manager,
                                                   LIGMA_FILE_PROCEDURE_GROUP_SAVE,
                                                   file,
                                                   NULL /*error*/);
  file_save (ligma,
             image,
             NULL /*progress*/,
             file,
             proc,
             LIGMA_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             FALSE /*export_forward*/,
             NULL /*error*/);

  /* Load from file */
  loaded_image = ligma_test_load_image (image->ligma, file);

  /* Assert on the loaded file. If success, it means that there is no
   * significant information loss when we wrote the image to a file
   * and loaded it again
   */
  ligma_assert_mainimage (loaded_image,
                         with_unusual_stuff,
                         compat_paths,
                         use_ligma_2_8_features);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

/**
 * ligma_create_mainimage:
 *
 * Creates the main test image, i.e. the image that we use for most of
 * our XCF testing purposes.
 *
 * Returns: The #LigmaImage
 **/
static LigmaImage *
ligma_create_mainimage (Ligma     *ligma,
                       gboolean  with_unusual_stuff,
                       gboolean  compat_paths,
                       gboolean  use_ligma_2_8_features)
{
  LigmaImage     *image             = NULL;
  LigmaLayer     *layer             = NULL;
  LigmaParasite  *parasite          = NULL;
  LigmaGrid      *grid              = NULL;
  LigmaChannel   *channel           = NULL;
  LigmaRGB        channel_color     = LIGMA_MAINIMAGE_CHANNEL1_COLOR;
  LigmaChannel   *selection         = NULL;
  LigmaVectors   *vectors           = NULL;
  LigmaCoords     vectors1_coords[] = LIGMA_MAINIMAGE_VECTORS1_COORDS;
  LigmaCoords     vectors2_coords[] = LIGMA_MAINIMAGE_VECTORS2_COORDS;
  LigmaStroke    *stroke            = NULL;
  LigmaLayerMask *layer_mask        = NULL;

  /* Image size and type */
  image = ligma_image_new (ligma,
                          LIGMA_MAINIMAGE_WIDTH,
                          LIGMA_MAINIMAGE_HEIGHT,
                          LIGMA_MAINIMAGE_TYPE,
                          LIGMA_MAINIMAGE_PRECISION);

  /* Layers */
  layer = ligma_layer_new (image,
                          LIGMA_MAINIMAGE_LAYER1_WIDTH,
                          LIGMA_MAINIMAGE_LAYER1_HEIGHT,
                          LIGMA_MAINIMAGE_LAYER1_FORMAT,
                          LIGMA_MAINIMAGE_LAYER1_NAME,
                          LIGMA_MAINIMAGE_LAYER1_OPACITY,
                          LIGMA_MAINIMAGE_LAYER1_MODE);
  ligma_image_add_layer (image,
                        layer,
                        NULL,
                        0,
                        FALSE/*push_undo*/);
  layer = ligma_layer_new (image,
                          LIGMA_MAINIMAGE_LAYER2_WIDTH,
                          LIGMA_MAINIMAGE_LAYER2_HEIGHT,
                          LIGMA_MAINIMAGE_LAYER2_FORMAT,
                          LIGMA_MAINIMAGE_LAYER2_NAME,
                          LIGMA_MAINIMAGE_LAYER2_OPACITY,
                          LIGMA_MAINIMAGE_LAYER2_MODE);
  ligma_image_add_layer (image,
                        layer,
                        NULL,
                        0,
                        FALSE /*push_undo*/);

  /* Layer mask */
  layer_mask = ligma_layer_create_mask (layer,
                                       LIGMA_ADD_MASK_BLACK,
                                       NULL /*channel*/);
  ligma_layer_add_mask (layer,
                       layer_mask,
                       FALSE /*push_undo*/,
                       NULL /*error*/);

  /* Image compression type
   *
   * We don't do any explicit test, only implicit when we read tile
   * data in other tests
   */

  /* Guides, note we add them in reversed order */
  ligma_image_add_hguide (image,
                         LIGMA_MAINIMAGE_HGUIDE2_POS,
                         FALSE /*push_undo*/);
  ligma_image_add_hguide (image,
                         LIGMA_MAINIMAGE_HGUIDE1_POS,
                         FALSE /*push_undo*/);
  ligma_image_add_vguide (image,
                         LIGMA_MAINIMAGE_VGUIDE2_POS,
                         FALSE /*push_undo*/);
  ligma_image_add_vguide (image,
                         LIGMA_MAINIMAGE_VGUIDE1_POS,
                         FALSE /*push_undo*/);


  /* Sample points */
  ligma_image_add_sample_point_at_pos (image,
                                      LIGMA_MAINIMAGE_SAMPLEPOINT1_X,
                                      LIGMA_MAINIMAGE_SAMPLEPOINT1_Y,
                                      FALSE /*push_undo*/);
  ligma_image_add_sample_point_at_pos (image,
                                      LIGMA_MAINIMAGE_SAMPLEPOINT2_X,
                                      LIGMA_MAINIMAGE_SAMPLEPOINT2_Y,
                                      FALSE /*push_undo*/);

  /* Tattoo
   * We don't bother testing this, not yet at least
   */

  /* Resolution */
  ligma_image_set_resolution (image,
                             LIGMA_MAINIMAGE_RESOLUTIONX,
                             LIGMA_MAINIMAGE_RESOLUTIONY);


  /* Parasites */
  parasite = ligma_parasite_new (LIGMA_MAINIMAGE_PARASITE_NAME,
                                LIGMA_PARASITE_PERSISTENT,
                                LIGMA_MAINIMAGE_PARASITE_SIZE,
                                LIGMA_MAINIMAGE_PARASITE_DATA);
  ligma_image_parasite_attach (image,
                              parasite, FALSE);
  ligma_parasite_free (parasite);
  parasite = ligma_parasite_new ("ligma-comment",
                                LIGMA_PARASITE_PERSISTENT,
                                strlen (LIGMA_MAINIMAGE_COMMENT) + 1,
                                LIGMA_MAINIMAGE_COMMENT);
  ligma_image_parasite_attach (image, parasite, FALSE);
  ligma_parasite_free (parasite);


  /* Unit */
  ligma_image_set_unit (image,
                       LIGMA_MAINIMAGE_UNIT);

  /* Grid */
  grid = g_object_new (LIGMA_TYPE_GRID,
                       "xspacing", LIGMA_MAINIMAGE_GRIDXSPACING,
                       "yspacing", LIGMA_MAINIMAGE_GRIDYSPACING,
                       NULL);
  ligma_image_set_grid (image,
                       grid,
                       FALSE /*push_undo*/);
  g_object_unref (grid);

  /* Channel */
  channel = ligma_channel_new (image,
                              LIGMA_MAINIMAGE_CHANNEL1_WIDTH,
                              LIGMA_MAINIMAGE_CHANNEL1_HEIGHT,
                              LIGMA_MAINIMAGE_CHANNEL1_NAME,
                              &channel_color);
  ligma_image_add_channel (image,
                          channel,
                          NULL,
                          -1,
                          FALSE /*push_undo*/);

  /* Selection */
  selection = ligma_image_get_mask (image);
  ligma_channel_select_rectangle (selection,
                                 LIGMA_MAINIMAGE_SELECTION_X,
                                 LIGMA_MAINIMAGE_SELECTION_Y,
                                 LIGMA_MAINIMAGE_SELECTION_W,
                                 LIGMA_MAINIMAGE_SELECTION_H,
                                 LIGMA_CHANNEL_OP_REPLACE,
                                 FALSE /*feather*/,
                                 0.0 /*feather_radius_x*/,
                                 0.0 /*feather_radius_y*/,
                                 FALSE /*push_undo*/);

  /* Vectors 1 */
  vectors = ligma_vectors_new (image,
                              LIGMA_MAINIMAGE_VECTORS1_NAME);
  /* The XCF file can save vectors in two kind of ways, one old way
   * and a new way. Parameterize the way so we can test both variants,
   * i.e. ligma_vectors_compat_is_compatible() must return both TRUE
   * and FALSE.
   */
  if (! compat_paths)
    {
      ligma_item_set_visible (LIGMA_ITEM (vectors),
                             TRUE,
                             FALSE /*push_undo*/);
    }
  /* TODO: Add test for non-closed stroke. The order of the anchor
   * points changes for open strokes, so it's boring to test
   */
  stroke = ligma_bezier_stroke_new_from_coords (vectors1_coords,
                                               G_N_ELEMENTS (vectors1_coords),
                                               TRUE /*closed*/);
  ligma_vectors_stroke_add (vectors, stroke);
  ligma_image_add_vectors (image,
                          vectors,
                          NULL /*parent*/,
                          -1 /*position*/,
                          FALSE /*push_undo*/);

  /* Vectors 2 */
  vectors = ligma_vectors_new (image,
                              LIGMA_MAINIMAGE_VECTORS2_NAME);

  stroke = ligma_bezier_stroke_new_from_coords (vectors2_coords,
                                               G_N_ELEMENTS (vectors2_coords),
                                               TRUE /*closed*/);
  ligma_vectors_stroke_add (vectors, stroke);
  ligma_image_add_vectors (image,
                          vectors,
                          NULL /*parent*/,
                          -1 /*position*/,
                          FALSE /*push_undo*/);

  /* Some of these things are pretty unusual, parameterize the
   * inclusion of this in the written file so we can do our test both
   * with and without
   */
  if (with_unusual_stuff)
    {
      GList *drawables;

      drawables = ligma_image_get_selected_drawables (image);

      /* Floating selection */
      ligma_selection_float (LIGMA_SELECTION (ligma_image_get_mask (image)),
                            drawables,
                            ligma_get_user_context (ligma),
                            TRUE /*cut_image*/,
                            0 /*off_x*/,
                            0 /*off_y*/,
                            NULL /*error*/);
      g_list_free (drawables);
    }

  /* Adds stuff like layer groups */
  if (use_ligma_2_8_features)
    {
      LigmaLayer *parent;

      /* Add a layer group and some layers:
       *
       *  group1
       *    layer3
       *    layer4
       *    group2
       *      layer5
       */

      /* group1 */
      layer = ligma_group_layer_new (image);
      ligma_object_set_name (LIGMA_OBJECT (layer), LIGMA_MAINIMAGE_GROUP1_NAME);
      ligma_image_add_layer (image,
                            layer,
                            NULL /*parent*/,
                            -1 /*position*/,
                            FALSE /*push_undo*/);
      parent = layer;

      /* layer3 */
      layer = ligma_layer_new (image,
                              LIGMA_MAINIMAGE_LAYER1_WIDTH,
                              LIGMA_MAINIMAGE_LAYER1_HEIGHT,
                              LIGMA_MAINIMAGE_LAYER1_FORMAT,
                              LIGMA_MAINIMAGE_LAYER3_NAME,
                              LIGMA_MAINIMAGE_LAYER1_OPACITY,
                              LIGMA_MAINIMAGE_LAYER1_MODE);
      ligma_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);

      /* layer4 */
      layer = ligma_layer_new (image,
                              LIGMA_MAINIMAGE_LAYER1_WIDTH,
                              LIGMA_MAINIMAGE_LAYER1_HEIGHT,
                              LIGMA_MAINIMAGE_LAYER1_FORMAT,
                              LIGMA_MAINIMAGE_LAYER4_NAME,
                              LIGMA_MAINIMAGE_LAYER1_OPACITY,
                              LIGMA_MAINIMAGE_LAYER1_MODE);
      ligma_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);

      /* group2 */
      layer = ligma_group_layer_new (image);
      ligma_object_set_name (LIGMA_OBJECT (layer), LIGMA_MAINIMAGE_GROUP2_NAME);
      ligma_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);
      parent = layer;

      /* layer5 */
      layer = ligma_layer_new (image,
                              LIGMA_MAINIMAGE_LAYER1_WIDTH,
                              LIGMA_MAINIMAGE_LAYER1_HEIGHT,
                              LIGMA_MAINIMAGE_LAYER1_FORMAT,
                              LIGMA_MAINIMAGE_LAYER5_NAME,
                              LIGMA_MAINIMAGE_LAYER1_OPACITY,
                              LIGMA_MAINIMAGE_LAYER1_MODE);
      ligma_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);
    }

  /* Todo, should be tested somehow:
   *
   * - Color maps
   * - Custom user units
   * - Text layers
   * - Layer parasites
   * - Channel parasites
   * - Different tile compression methods
   */

  return image;
}

static void
ligma_assert_vectors (LigmaImage   *image,
                     const gchar *name,
                     LigmaCoords   coords[],
                     gsize        coords_size,
                     gboolean     visible)
{
  LigmaVectors *vectors        = NULL;
  LigmaStroke  *stroke         = NULL;
  GArray      *control_points = NULL;
  gboolean     closed         = FALSE;
  gint         i              = 0;

  vectors = ligma_image_get_vectors_by_name (image, name);
  stroke = ligma_vectors_stroke_get_next (vectors, NULL);
  g_assert (stroke != NULL);
  control_points = ligma_stroke_control_points_get (stroke,
                                                   &closed);
  g_assert (closed);
  g_assert_cmpint (control_points->len,
                   ==,
                   coords_size);
  for (i = 0; i < control_points->len; i++)
    {
      g_assert_cmpint (coords[i].x,
                       ==,
                       g_array_index (control_points,
                                      LigmaAnchor,
                                      i).position.x);
      g_assert_cmpint (coords[i].y,
                       ==,
                       g_array_index (control_points,
                                      LigmaAnchor,
                                      i).position.y);
    }

  g_assert (ligma_item_get_visible (LIGMA_ITEM (vectors)) ? TRUE : FALSE ==
            visible ? TRUE : FALSE);
}

/**
 * ligma_assert_mainimage:
 * @image:
 *
 * Verifies that the passed #LigmaImage contains all the information
 * that was put in it by ligma_create_mainimage().
 **/
static void
ligma_assert_mainimage (LigmaImage *image,
                       gboolean   with_unusual_stuff,
                       gboolean   compat_paths,
                       gboolean   use_ligma_2_8_features)
{
  const LigmaParasite *parasite               = NULL;
  gchar              *parasite_data          = NULL;
  guint32             parasite_size          = -1;
  LigmaLayer          *layer                  = NULL;
  GList              *iter                   = NULL;
  LigmaGuide          *guide                  = NULL;
  LigmaSamplePoint    *sample_point           = NULL;
  gint                sample_point_x         = 0;
  gint                sample_point_y         = 0;
  gdouble             xres                   = 0.0;
  gdouble             yres                   = 0.0;
  LigmaGrid           *grid                   = NULL;
  gdouble             xspacing               = 0.0;
  gdouble             yspacing               = 0.0;
  LigmaChannel        *channel                = NULL;
  LigmaRGB             expected_channel_color = LIGMA_MAINIMAGE_CHANNEL1_COLOR;
  LigmaRGB             actual_channel_color   = { 0, };
  LigmaChannel        *selection              = NULL;
  gint                x                      = -1;
  gint                y                      = -1;
  gint                w                      = -1;
  gint                h                      = -1;
  LigmaCoords          vectors1_coords[]      = LIGMA_MAINIMAGE_VECTORS1_COORDS;
  LigmaCoords          vectors2_coords[]      = LIGMA_MAINIMAGE_VECTORS2_COORDS;

  /* Image size and type */
  g_assert_cmpint (ligma_image_get_width (image),
                   ==,
                   LIGMA_MAINIMAGE_WIDTH);
  g_assert_cmpint (ligma_image_get_height (image),
                   ==,
                   LIGMA_MAINIMAGE_HEIGHT);
  g_assert_cmpint (ligma_image_get_base_type (image),
                   ==,
                   LIGMA_MAINIMAGE_TYPE);

  /* Layers */
  layer = ligma_image_get_layer_by_name (image,
                                        LIGMA_MAINIMAGE_LAYER1_NAME);
  g_assert_cmpint (ligma_item_get_width (LIGMA_ITEM (layer)),
                   ==,
                   LIGMA_MAINIMAGE_LAYER1_WIDTH);
  g_assert_cmpint (ligma_item_get_height (LIGMA_ITEM (layer)),
                   ==,
                   LIGMA_MAINIMAGE_LAYER1_HEIGHT);
  g_assert_cmpstr (babl_get_name (ligma_drawable_get_format (LIGMA_DRAWABLE (layer))),
                   ==,
                   babl_get_name (LIGMA_MAINIMAGE_LAYER1_FORMAT));
  g_assert_cmpstr (ligma_object_get_name (LIGMA_DRAWABLE (layer)),
                   ==,
                   LIGMA_MAINIMAGE_LAYER1_NAME);
  g_assert_cmpfloat (ligma_layer_get_opacity (layer),
                     ==,
                     LIGMA_MAINIMAGE_LAYER1_OPACITY);
  g_assert_cmpint (ligma_layer_get_mode (layer),
                   ==,
                   LIGMA_MAINIMAGE_LAYER1_MODE);
  layer = ligma_image_get_layer_by_name (image,
                                        LIGMA_MAINIMAGE_LAYER2_NAME);
  g_assert_cmpint (ligma_item_get_width (LIGMA_ITEM (layer)),
                   ==,
                   LIGMA_MAINIMAGE_LAYER2_WIDTH);
  g_assert_cmpint (ligma_item_get_height (LIGMA_ITEM (layer)),
                   ==,
                   LIGMA_MAINIMAGE_LAYER2_HEIGHT);
  g_assert_cmpstr (babl_get_name (ligma_drawable_get_format (LIGMA_DRAWABLE (layer))),
                   ==,
                   babl_get_name (LIGMA_MAINIMAGE_LAYER2_FORMAT));
  g_assert_cmpstr (ligma_object_get_name (LIGMA_DRAWABLE (layer)),
                   ==,
                   LIGMA_MAINIMAGE_LAYER2_NAME);
  g_assert_cmpfloat (ligma_layer_get_opacity (layer),
                     ==,
                     LIGMA_MAINIMAGE_LAYER2_OPACITY);
  g_assert_cmpint (ligma_layer_get_mode (layer),
                   ==,
                   LIGMA_MAINIMAGE_LAYER2_MODE);

  /* Guides, note that we rely on internal ordering */
  iter = ligma_image_get_guides (image);
  g_assert (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (ligma_guide_get_position (guide),
                   ==,
                   LIGMA_MAINIMAGE_VGUIDE1_POS);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (ligma_guide_get_position (guide),
                   ==,
                   LIGMA_MAINIMAGE_VGUIDE2_POS);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (ligma_guide_get_position (guide),
                   ==,
                   LIGMA_MAINIMAGE_HGUIDE1_POS);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (ligma_guide_get_position (guide),
                   ==,
                   LIGMA_MAINIMAGE_HGUIDE2_POS);
  iter = g_list_next (iter);
  g_assert (iter == NULL);

  /* Sample points, we rely on the same ordering as when we added
   * them, although this ordering is not a necessity
   */
  iter = ligma_image_get_sample_points (image);
  g_assert (iter != NULL);
  sample_point = iter->data;
  ligma_sample_point_get_position (sample_point,
                                  &sample_point_x, &sample_point_y);
  g_assert_cmpint (sample_point_x,
                   ==,
                   LIGMA_MAINIMAGE_SAMPLEPOINT1_X);
  g_assert_cmpint (sample_point_y,
                   ==,
                   LIGMA_MAINIMAGE_SAMPLEPOINT1_Y);
  iter = g_list_next (iter);
  g_assert (iter != NULL);
  sample_point = iter->data;
  ligma_sample_point_get_position (sample_point,
                                  &sample_point_x, &sample_point_y);
  g_assert_cmpint (sample_point_x,
                   ==,
                   LIGMA_MAINIMAGE_SAMPLEPOINT2_X);
  g_assert_cmpint (sample_point_y,
                   ==,
                   LIGMA_MAINIMAGE_SAMPLEPOINT2_Y);
  iter = g_list_next (iter);
  g_assert (iter == NULL);

  /* Resolution */
  ligma_image_get_resolution (image, &xres, &yres);
  g_assert_cmpint (xres,
                   ==,
                   LIGMA_MAINIMAGE_RESOLUTIONX);
  g_assert_cmpint (yres,
                   ==,
                   LIGMA_MAINIMAGE_RESOLUTIONY);

  /* Parasites */
  parasite = ligma_image_parasite_find (image,
                                       LIGMA_MAINIMAGE_PARASITE_NAME);
  parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);
  parasite_data = g_strndup (parasite_data, parasite_size);
  g_assert_cmpint (parasite_size,
                   ==,
                   LIGMA_MAINIMAGE_PARASITE_SIZE);
  g_assert_cmpstr (parasite_data,
                   ==,
                   LIGMA_MAINIMAGE_PARASITE_DATA);
  g_free (parasite_data);

  parasite = ligma_image_parasite_find (image,
                                       "ligma-comment");
  parasite_data = (gchar *) ligma_parasite_get_data (parasite, &parasite_size);
  parasite_data = g_strndup (parasite_data, parasite_size);
  g_assert_cmpint (parasite_size,
                   ==,
                   strlen (LIGMA_MAINIMAGE_COMMENT) + 1);
  g_assert_cmpstr (parasite_data,
                   ==,
                   LIGMA_MAINIMAGE_COMMENT);
  g_free (parasite_data);

  /* Unit */
  g_assert_cmpint (ligma_image_get_unit (image),
                   ==,
                   LIGMA_MAINIMAGE_UNIT);

  /* Grid */
  grid = ligma_image_get_grid (image);
  g_object_get (grid,
                "xspacing", &xspacing,
                "yspacing", &yspacing,
                NULL);
  g_assert_cmpint (xspacing,
                   ==,
                   LIGMA_MAINIMAGE_GRIDXSPACING);
  g_assert_cmpint (yspacing,
                   ==,
                   LIGMA_MAINIMAGE_GRIDYSPACING);


  /* Channel */
  channel = ligma_image_get_channel_by_name (image,
                                            LIGMA_MAINIMAGE_CHANNEL1_NAME);
  ligma_channel_get_color (channel, &actual_channel_color);
  g_assert_cmpint (ligma_item_get_width (LIGMA_ITEM (channel)),
                   ==,
                   LIGMA_MAINIMAGE_CHANNEL1_WIDTH);
  g_assert_cmpint (ligma_item_get_height (LIGMA_ITEM (channel)),
                   ==,
                   LIGMA_MAINIMAGE_CHANNEL1_HEIGHT);
  g_assert (memcmp (&expected_channel_color,
                    &actual_channel_color,
                    sizeof (LigmaRGB)) == 0);

  /* Selection, if the image contains unusual stuff it contains a
   * floating select, and when floating a selection, the selection
   * mask is cleared, so don't test for the presence of the selection
   * mask in that case
   */
  if (! with_unusual_stuff)
    {
      selection = ligma_image_get_mask (image);
      ligma_item_bounds (LIGMA_ITEM (selection), &x, &y, &w, &h);
      g_assert_cmpint (x,
                       ==,
                       LIGMA_MAINIMAGE_SELECTION_X);
      g_assert_cmpint (y,
                       ==,
                       LIGMA_MAINIMAGE_SELECTION_Y);
      g_assert_cmpint (w,
                       ==,
                       LIGMA_MAINIMAGE_SELECTION_W);
      g_assert_cmpint (h,
                       ==,
                       LIGMA_MAINIMAGE_SELECTION_H);
    }

  /* Vectors 1 */
  ligma_assert_vectors (image,
                       LIGMA_MAINIMAGE_VECTORS1_NAME,
                       vectors1_coords,
                       G_N_ELEMENTS (vectors1_coords),
                       ! compat_paths /*visible*/);

  /* Vectors 2 (always visible FALSE) */
  ligma_assert_vectors (image,
                       LIGMA_MAINIMAGE_VECTORS2_NAME,
                       vectors2_coords,
                       G_N_ELEMENTS (vectors2_coords),
                       FALSE /*visible*/);

  if (with_unusual_stuff)
    g_assert (ligma_image_get_floating_selection (image) != NULL);
  else /* if (! with_unusual_stuff) */
    g_assert (ligma_image_get_floating_selection (image) == NULL);

  if (use_ligma_2_8_features)
    {
      /* Only verify the parent relationships, the layer attributes
       * are tested above
       */
      LigmaItem *group1 = LIGMA_ITEM (ligma_image_get_layer_by_name (image, LIGMA_MAINIMAGE_GROUP1_NAME));
      LigmaItem *layer3 = LIGMA_ITEM (ligma_image_get_layer_by_name (image, LIGMA_MAINIMAGE_LAYER3_NAME));
      LigmaItem *layer4 = LIGMA_ITEM (ligma_image_get_layer_by_name (image, LIGMA_MAINIMAGE_LAYER4_NAME));
      LigmaItem *group2 = LIGMA_ITEM (ligma_image_get_layer_by_name (image, LIGMA_MAINIMAGE_GROUP2_NAME));
      LigmaItem *layer5 = LIGMA_ITEM (ligma_image_get_layer_by_name (image, LIGMA_MAINIMAGE_LAYER5_NAME));

      g_assert (ligma_item_get_parent (group1) == NULL);
      g_assert (ligma_item_get_parent (layer3) == group1);
      g_assert (ligma_item_get_parent (layer4) == group1);
      g_assert (ligma_item_get_parent (group2) == group1);
      g_assert (ligma_item_get_parent (layer5) == group2);
    }
}


/**
 * main:
 * @argc:
 * @argv:
 *
 * These tests intend to
 *
 *  - Make sure that we are backwards compatible with files created by
 *    older version of LIGMA, i.e. that we can load files from earlier
 *    version of LIGMA
 *
 *  - Make sure that the information put into a #LigmaImage is not lost
 *    when the #LigmaImage is written to a file and then read again
 **/
int
main (int    argc,
      char **argv)
{
  Ligma *ligma;
  int   result;

  g_test_init (&argc, &argv, NULL);

  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/ligmadir");

  /* We share the same application instance across all tests. We need
   * the GUI variant for the file procs
   */
  ligma = ligma_init_for_testing ();

  /* Add tests */
  ADD_TEST (write_and_read_ligma_2_6_format);
  ADD_TEST (write_and_read_ligma_2_6_format_unusual);
  ADD_TEST (load_ligma_2_6_file);
  ADD_TEST (write_and_read_ligma_2_8_format);

  /* Don't write files to the source dir */
  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/ligmadir-output");

  /* Run the tests */
  result = g_test_run ();

  /* Exit so we don't break script-fu plug-in wire */
  ligma_exit (ligma, TRUE);

  return result;
}
