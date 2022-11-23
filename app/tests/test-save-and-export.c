/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 2009 Martin Nordholts <martinn@src.gnome.org>
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

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmamath/ligmamath.h"
#include "libligmawidgets/ligmawidgets.h"

#include "dialogs/dialogs-types.h"

#include "core/ligma.h"
#include "core/ligmachannel.h"
#include "core/ligmacontext.h"
#include "core/ligmaimage.h"
#include "core/ligmalayer.h"
#include "core/ligmatoolinfo.h"
#include "core/ligmatooloptions.h"

#include "plug-in/ligmapluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"

#include "widgets/ligmadialogfactory.h"
#include "widgets/ligmadock.h"
#include "widgets/ligmadockable.h"
#include "widgets/ligmadockbook.h"
#include "widgets/ligmadocked.h"
#include "widgets/ligmadockwindow.h"
#include "widgets/ligmahelp-ids.h"
#include "widgets/ligmasessioninfo.h"
#include "widgets/ligmatoolbox.h"
#include "widgets/ligmatooloptionseditor.h"
#include "widgets/ligmauimanager.h"
#include "widgets/ligmawidgets-utils.h"

#include "display/ligmadisplay.h"
#include "display/ligmadisplayshell.h"
#include "display/ligmadisplayshell-scale.h"
#include "display/ligmadisplayshell-transform.h"
#include "display/ligmaimagewindow.h"

#include "tests.h"

#include "ligma-app-test-utils.h"


#define ADD_TEST(function) \
  g_test_add_data_func ("/ligma-save-and-export/" #function, ligma, function);


typedef gboolean (*LigmaUiTestFunc) (GObject *object);


/**
 * new_file_has_no_files:
 * @data:
 *
 * Tests that the URIs are correct for a newly created image.
 **/
static void
new_file_has_no_files (gconstpointer    data)
{
  Ligma      *ligma  = LIGMA (data);
  LigmaImage *image = ligma_test_utils_create_image_from_dialog (ligma);

  g_assert (ligma_image_get_file (image) == NULL);
  g_assert (ligma_image_get_imported_file (image) == NULL);
  g_assert (ligma_image_get_exported_file (image) == NULL);
}

/**
 * opened_xcf_file_files:
 * @data:
 *
 * Tests that LigmaImage URIs are correct for an XCF file that has just
 * been opened.
 **/
static void
opened_xcf_file_files (gconstpointer data)
{
  Ligma              *ligma = LIGMA (data);
  LigmaImage         *image;
  GFile             *file;
  gchar             *filename;
  LigmaPDBStatusType  status;

  filename = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                               "app/tests/files/ligma-2-6-file.xcf",
                               NULL);
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = file_open_image (ligma,
                           ligma_get_user_context (ligma),
                           NULL /*progress*/,
                           file,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           LIGMA_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert (g_file_equal (ligma_image_get_file (image), file));
  g_assert (ligma_image_get_imported_file (image) == NULL);
  g_assert (ligma_image_get_exported_file (image) == NULL);

  g_object_unref (file);
}

/**
 * imported_file_files:
 * @data:
 *
 * Tests that URIs are correct for an imported image.
 **/
static void
imported_file_files (gconstpointer data)
{
  Ligma              *ligma = LIGMA (data);
  LigmaImage         *image;
  GFile             *file;
  gchar             *filename;
  LigmaPDBStatusType  status;

  filename = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                               "desktop/64x64/ligma.png",
                               NULL);
  g_assert (g_file_test (filename, G_FILE_TEST_EXISTS));
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = file_open_image (ligma,
                           ligma_get_user_context (ligma),
                           NULL /*progress*/,
                           file,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           LIGMA_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert (ligma_image_get_file (image) == NULL);
  g_assert (g_file_equal (ligma_image_get_imported_file (image), file));
  g_assert (ligma_image_get_exported_file (image) == NULL);

  g_object_unref (file);
}

/**
 * saved_imported_file_files:
 * @data:
 *
 * Tests that the URIs are correct for an image that has been imported
 * and then saved.
 **/
static void
saved_imported_file_files (gconstpointer data)
{
  Ligma                *ligma = LIGMA (data);
  LigmaImage           *image;
  GFile               *import_file;
  gchar               *import_filename;
  GFile               *save_file;
  gchar               *save_filename;
  LigmaPDBStatusType    status;
  LigmaPlugInProcedure *proc;

  import_filename = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                                      "desktop/64x64/ligma.png",
                                      NULL);
  import_file = g_file_new_for_path (import_filename);
  g_free (import_filename);

  save_filename = g_build_filename (g_get_tmp_dir (), "ligma-test.xcf", NULL);
  save_file = g_file_new_for_path (save_filename);
  g_free (save_filename);

  /* Import */
  image = file_open_image (ligma,
                           ligma_get_user_context (ligma),
                           NULL /*progress*/,
                           import_file,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           LIGMA_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_object_unref (import_file);

  /* Save */
  proc = ligma_plug_in_manager_file_procedure_find (image->ligma->plug_in_manager,
                                                   LIGMA_FILE_PROCEDURE_GROUP_SAVE,
                                                   save_file,
                                                   NULL /*error*/);
  file_save (ligma,
             image,
             NULL /*progress*/,
             save_file,
             proc,
             LIGMA_RUN_NONINTERACTIVE,
             TRUE /*change_saved_state*/,
             FALSE /*export_backward*/,
             FALSE /*export_forward*/,
             NULL /*error*/);

  /* Assert */
  g_assert (g_file_equal (ligma_image_get_file (image), save_file));
  g_assert (ligma_image_get_imported_file (image) == NULL);
  g_assert (ligma_image_get_exported_file (image) == NULL);

  g_file_delete (save_file, NULL, NULL);
  g_object_unref (save_file);
}

/**
 * exported_file_files:
 * @data:
 *
 * Tests that the URIs for an exported, newly created file are
 * correct.
 **/
static void
exported_file_files (gconstpointer data)
{
  GFile               *save_file;
  gchar               *save_filename;
  LigmaPlugInProcedure *proc;
  Ligma                *ligma  = LIGMA (data);
  LigmaImage           *image = ligma_test_utils_create_image_from_dialog (ligma);

  save_filename = g_build_filename (g_get_tmp_dir (), "ligma-test.png", NULL);
  save_file = g_file_new_for_path (save_filename);
  g_free (save_filename);

  proc = ligma_plug_in_manager_file_procedure_find (image->ligma->plug_in_manager,
                                                   LIGMA_FILE_PROCEDURE_GROUP_EXPORT,
                                                   save_file,
                                                   NULL /*error*/);
  file_save (ligma,
             image,
             NULL /*progress*/,
             save_file,
             proc,
             LIGMA_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             TRUE /*export_forward*/,
             NULL /*error*/);

  g_assert (ligma_image_get_file (image) == NULL);
  g_assert (ligma_image_get_imported_file (image) == NULL);
  g_assert (g_file_equal (ligma_image_get_exported_file (image), save_file));

  g_file_delete (save_file, NULL, NULL);
  g_object_unref (save_file);
}

/**
 * clear_import_file_after_export:
 * @data:
 *
 * Tests that after a XCF file that was imported has been exported,
 * the import URI is cleared. An image can not be considered both
 * imported and exported at the same time.
 **/
static void
clear_import_file_after_export (gconstpointer data)
{
  Ligma                *ligma = LIGMA (data);
  LigmaImage           *image;
  GFile               *file;
  gchar               *filename;
  GFile               *save_file;
  gchar               *save_filename;
  LigmaPlugInProcedure *proc;
  LigmaPDBStatusType    status;

  filename = g_build_filename (g_getenv ("LIGMA_TESTING_ABS_TOP_SRCDIR"),
                               "desktop/64x64/ligma.png",
                               NULL);
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = file_open_image (ligma,
                           ligma_get_user_context (ligma),
                           NULL /*progress*/,
                           file,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           LIGMA_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert (ligma_image_get_file (image) == NULL);
  g_assert (g_file_equal (ligma_image_get_imported_file (image), file));
  g_assert (ligma_image_get_exported_file (image) == NULL);

  g_object_unref (file);

  save_filename = g_build_filename (g_get_tmp_dir (), "ligma-test.png", NULL);
  save_file = g_file_new_for_path (save_filename);
  g_free (save_filename);

  proc = ligma_plug_in_manager_file_procedure_find (image->ligma->plug_in_manager,
                                                   LIGMA_FILE_PROCEDURE_GROUP_EXPORT,
                                                   save_file,
                                                   NULL /*error*/);
  file_save (ligma,
             image,
             NULL /*progress*/,
             save_file,
             proc,
             LIGMA_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             TRUE /*export_forward*/,
             NULL /*error*/);

  g_assert (ligma_image_get_file (image) == NULL);
  g_assert (ligma_image_get_imported_file (image) == NULL);
  g_assert (g_file_equal (ligma_image_get_exported_file (image), save_file));

  g_file_delete (save_file, NULL, NULL);
  g_object_unref (save_file);
}

int
main(int    argc,
     char **argv)
{
  Ligma *ligma   = NULL;
  gint  result = -1;

  ligma_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/ligmadir");
  ligma_test_utils_setup_menus_path ();

  /* Start up LIGMA */
  ligma = ligma_init_for_gui_testing (TRUE /*show_gui*/);
  ligma_test_run_mainloop_until_idle ();

  ADD_TEST (new_file_has_no_files);
  ADD_TEST (opened_xcf_file_files);
  ADD_TEST (imported_file_files);
  ADD_TEST (saved_imported_file_files);
  ADD_TEST (exported_file_files);
  ADD_TEST (clear_import_file_after_export);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  ligma_test_utils_set_ligma3_directory ("LIGMA_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/ligmadir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  ligma_exit (ligma, TRUE);

  return result;
}
