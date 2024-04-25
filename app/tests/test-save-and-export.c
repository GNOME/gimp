/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs/dialogs-types.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "plug-in/gimppluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpdock.h"
#include "widgets/gimpdockable.h"
#include "widgets/gimpdockbook.h"
#include "widgets/gimpdocked.h"
#include "widgets/gimpdockwindow.h"
#include "widgets/gimphelp-ids.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimptoolbox.h"
#include "widgets/gimptooloptionseditor.h"
#include "widgets/gimpuimanager.h"
#include "widgets/gimpwidgets-utils.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimpimagewindow.h"

#include "gimpcoreapp.h"

#include "gimp-app-test-utils.h"
#include "tests.h"


#define ADD_TEST(function) \
  g_test_add_data_func ("/gimp-save-and-export/" #function, gimp, function);


typedef gboolean (*GimpUiTestFunc) (GObject *object);


/**
 * new_file_has_no_files:
 * @data:
 *
 * Tests that the URIs are correct for a newly created image.
 **/
static void
new_file_has_no_files (gconstpointer    data)
{
  Gimp      *gimp  = GIMP (data);
  GimpImage *image = gimp_test_utils_create_image_from_dialog (gimp);

  g_assert_true (gimp_image_get_file (image) == NULL);
  g_assert_true (gimp_image_get_imported_file (image) == NULL);
  g_assert_true (gimp_image_get_exported_file (image) == NULL);
}

/**
 * opened_xcf_file_files:
 * @data:
 *
 * Tests that GimpImage URIs are correct for an XCF file that has just
 * been opened.
 **/
static void
opened_xcf_file_files (gconstpointer data)
{
  Gimp              *gimp = GIMP (data);
  GimpImage         *image;
  GFile             *file;
  gchar             *filename;
  GimpPDBStatusType  status;

  filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                               "app/tests/files/gimp-2-6-file.xcf",
                               NULL);
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           file,
                           0, 0, /* vector width, height */
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert_true (g_file_equal (gimp_image_get_file (image), file));
  g_assert_true (gimp_image_get_imported_file (image) == NULL);
  g_assert_true (gimp_image_get_exported_file (image) == NULL);

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
  Gimp              *gimp = GIMP (data);
  GimpImage         *image;
  GFile             *file;
  gchar             *filename;
  GimpPDBStatusType  status;

  filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_BUILDDIR"),
                               "gimp-data/images/logo/gimp64x64.png",
                               NULL);
  g_assert_true (g_file_test (filename, G_FILE_TEST_EXISTS));
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           file,
                           0, 0, /* vector width, height */
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert_true (gimp_image_get_file (image) == NULL);
  g_assert_true (g_file_equal (gimp_image_get_imported_file (image), file));
  g_assert_true (gimp_image_get_exported_file (image) == NULL);

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
  Gimp                *gimp = GIMP (data);
  GimpImage           *image;
  GFile               *import_file;
  gchar               *import_filename;
  GFile               *save_file;
  gchar               *save_filename;
  GimpPDBStatusType    status;
  GimpPlugInProcedure *proc;

  import_filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_BUILDDIR"),
                                      "gimp-data/images/logo/gimp64x64.png",
                                      NULL);
  import_file = g_file_new_for_path (import_filename);
  g_free (import_filename);

  save_filename = g_build_filename (g_get_tmp_dir (), "gimp-test.xcf", NULL);
  save_file = g_file_new_for_path (save_filename);
  g_free (save_filename);

  /* Import */
  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           import_file,
                           0, 0, /* vector width, height */
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_object_unref (import_file);

  /* Save */
  proc = gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                   GIMP_FILE_PROCEDURE_GROUP_SAVE,
                                                   save_file,
                                                   NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             save_file,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             TRUE /*change_saved_state*/,
             FALSE /*export_backward*/,
             FALSE /*export_forward*/,
             NULL /*error*/);

  /* Assert */
  g_assert_true (g_file_equal (gimp_image_get_file (image), save_file));
  g_assert_true (gimp_image_get_imported_file (image) == NULL);
  g_assert_true (gimp_image_get_exported_file (image) == NULL);

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
  GimpPlugInProcedure *proc;
  Gimp                *gimp  = GIMP (data);
  GimpImage           *image = gimp_test_utils_create_image_from_dialog (gimp);

  save_filename = g_build_filename (g_get_tmp_dir (), "gimp-test.png", NULL);
  save_file = g_file_new_for_path (save_filename);
  g_free (save_filename);

  proc = gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                   GIMP_FILE_PROCEDURE_GROUP_EXPORT,
                                                   save_file,
                                                   NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             save_file,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             TRUE /*export_forward*/,
             NULL /*error*/);

  g_assert_true (gimp_image_get_file (image) == NULL);
  g_assert_true (gimp_image_get_imported_file (image) == NULL);
  g_assert_true (g_file_equal (gimp_image_get_exported_file (image), save_file));

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
  Gimp                *gimp = GIMP (data);
  GimpImage           *image;
  GFile               *file;
  gchar               *filename;
  GFile               *save_file;
  gchar               *save_filename;
  GimpPlugInProcedure *proc;
  GimpPDBStatusType    status;

  filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_BUILDDIR"),
                               "gimp-data/images/logo/gimp64x64.png",
                               NULL);
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           file,
                           0, 0, /* vector width, height */
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert_true (gimp_image_get_file (image) == NULL);
  g_assert_true (g_file_equal (gimp_image_get_imported_file (image), file));
  g_assert_true (gimp_image_get_exported_file (image) == NULL);

  g_object_unref (file);

  save_filename = g_build_filename (g_get_tmp_dir (), "gimp-test.png", NULL);
  save_file = g_file_new_for_path (save_filename);
  g_free (save_filename);

  proc = gimp_plug_in_manager_file_procedure_find (image->gimp->plug_in_manager,
                                                   GIMP_FILE_PROCEDURE_GROUP_EXPORT,
                                                   save_file,
                                                   NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             save_file,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             TRUE /*export_forward*/,
             NULL /*error*/);

  g_assert_true (gimp_image_get_file (image) == NULL);
  g_assert_true (gimp_image_get_imported_file (image) == NULL);
  g_assert_true (g_file_equal (gimp_image_get_exported_file (image), save_file));

  g_file_delete (save_file, NULL, NULL);
  g_object_unref (save_file);
}

int
main(int    argc,
     char **argv)
{
  Gimp *gimp   = NULL;
  gint  result = -1;

  gimp_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  gimp_test_utils_setup_menus_path ();

  /* Start up GIMP */
  gimp = gimp_init_for_gui_testing (TRUE /*show_gui*/);
  gimp_test_run_mainloop_until_idle ();

  ADD_TEST (new_file_has_no_files);
  ADD_TEST (opened_xcf_file_files);
  ADD_TEST (imported_file_files);
  ADD_TEST (saved_imported_file_files);
  ADD_TEST (exported_file_files);
  ADD_TEST (clear_import_file_after_export);

  /* Run the tests and return status */
  g_application_run (gimp->app, 0, NULL);
  result = gimp_core_app_get_exit_status (GIMP_CORE_APP (gimp->app));

  g_application_quit (G_APPLICATION (gimp->app));
  g_clear_object (&gimp->app);

  return result;
}
