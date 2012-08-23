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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <string.h>

#include <gegl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmath/gimpmath.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "dialogs/dialogs-types.h"

#include "display/gimpdisplay.h"
#include "display/gimpdisplayshell.h"
#include "display/gimpdisplayshell-scale.h"
#include "display/gimpdisplayshell-transform.h"
#include "display/gimpimagewindow.h"

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

#include "file/file-open.h"
#include "file/file-procedure.h"
#include "file/file-save.h"
#include "file/file-utils.h"

#include "plug-in/gimppluginmanager.h"

#include "core/gimp.h"
#include "core/gimpchannel.h"
#include "core/gimpcontext.h"
#include "core/gimpimage.h"
#include "core/gimplayer.h"
#include "core/gimptoolinfo.h"
#include "core/gimptooloptions.h"

#include "tests.h"

#include "gimp-app-test-utils.h"


#define ADD_TEST(function) \
  g_test_add_data_func ("/gimp-save-and-export/" #function, gimp, function);


typedef gboolean (*GimpUiTestFunc) (GObject *object);


/**
 * new_file_has_no_uris:
 * @data:
 *
 * Tests that the URIs are correct for a newly created image.
 **/
static void
new_file_has_no_uris (gconstpointer    data)
{
  Gimp      *gimp  = GIMP (data);
  GimpImage *image = gimp_test_utils_create_image_from_dalog (gimp);

  g_assert (gimp_image_get_uri (image) == NULL);
  g_assert (gimp_image_get_imported_uri (image) == NULL);
  g_assert (gimp_image_get_exported_uri (image) == NULL);
}

/**
 * opened_xcf_file_uris:
 * @data:
 *
 * Tests that GimpImage URIs are correct for an XCF file that has just
 * been opened.
 **/
static void
opened_xcf_file_uris (gconstpointer data)
{
  Gimp              *gimp = GIMP (data);
  GimpImage         *image;
  gchar             *uri;
  gchar             *filename;
  GimpPDBStatusType  status;

  filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                               "app/tests/files/gimp-2-6-file.xcf",
                               NULL);
  uri = g_filename_to_uri (filename, NULL, NULL);

  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           uri,
                           filename,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert_cmpstr (gimp_image_get_uri (image), ==, uri);
  g_assert (gimp_image_get_imported_uri (image) == NULL);
  g_assert (gimp_image_get_exported_uri (image) == NULL);

  /* Don't bother g_free()ing strings */
}

/**
 * imported_file_uris:
 * @data:
 *
 * Tests that URIs are correct for an imported image.
 **/
static void
imported_file_uris (gconstpointer data)
{
  Gimp              *gimp = GIMP (data);
  GimpImage         *image;
  gchar             *uri;
  gchar             *filename;
  GimpPDBStatusType  status;

  filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                               "desktop/64x64/gimp.png",
                               NULL);
  g_assert (g_file_test (filename, G_FILE_TEST_EXISTS));

  uri = g_filename_to_uri (filename, NULL, NULL);
  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           uri,
                           filename,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert (gimp_image_get_uri (image) == NULL);
  g_assert_cmpstr (gimp_image_get_imported_uri (image), ==, uri);
  g_assert (gimp_image_get_exported_uri (image) == NULL);
}

/**
 * saved_imported_file_uris:
 * @data:
 *
 * Tests that the URIs are correct for an image that has been imported
 * and then saved.
 **/
static void
saved_imported_file_uris (gconstpointer data)
{
  Gimp                *gimp = GIMP (data);
  GimpImage           *image;
  gchar               *import_uri;
  gchar               *import_filename;
  gchar               *save_uri;
  gchar               *save_filename;
  GimpPDBStatusType    status;
  GimpPlugInProcedure *proc;

  import_filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                                      "desktop/64x64/gimp.png",
                                      NULL);
  import_uri = g_filename_to_uri (import_filename, NULL, NULL);
  save_filename = g_build_filename (g_get_tmp_dir (), "gimp-test.xcf", NULL);
  save_uri = g_filename_to_uri (save_filename, NULL, NULL);

  /* Import */
  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           import_uri,
                           import_filename,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  /* Save */
  proc = file_procedure_find (image->gimp->plug_in_manager->save_procs,
                              save_uri,
                              NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             save_uri,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             TRUE /*change_saved_state*/,
             FALSE /*export_backward*/,
             FALSE /*export_forward*/,
             NULL /*error*/);

  /* Assert */
  g_assert_cmpstr (gimp_image_get_uri (image), ==, save_uri);
  g_assert (gimp_image_get_imported_uri (image) == NULL);
  g_assert (gimp_image_get_exported_uri (image) == NULL);

  g_unlink (save_filename);
}

/**
 * new_file_has_no_uris:
 * @data:
 *
 * Tests that the URIs for an exported, newly created file are
 * correct.
 **/
static void
exported_file_uris (gconstpointer data)
{
  gchar               *save_uri;
  gchar               *save_filename;
  GimpPlugInProcedure *proc;
  Gimp                *gimp  = GIMP (data);
  GimpImage           *image = gimp_test_utils_create_image_from_dalog (gimp);

  save_filename = g_build_filename (g_get_tmp_dir (), "gimp-test.png", NULL);
  save_uri = g_filename_to_uri (save_filename, NULL, NULL);

  proc = file_procedure_find (image->gimp->plug_in_manager->export_procs,
                              save_uri,
                              NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             save_uri,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             TRUE /*export_forward*/,
             NULL /*error*/);

  g_assert (gimp_image_get_uri (image) == NULL);
  g_assert (gimp_image_get_imported_uri (image) == NULL);
  g_assert_cmpstr (gimp_image_get_exported_uri (image), ==, save_uri);

  g_unlink (save_filename);
}

/**
 * clear_import_uri_after_export:
 * @data:
 *
 * Tests that after a XCF file that was imported has been exported,
 * the import URI is cleared. An image can not be considered both
 * imported and exported at the same time.
 **/
static void
clear_import_uri_after_export (gconstpointer data)
{
  Gimp                *gimp = GIMP (data);
  GimpImage           *image;
  gchar               *uri;
  gchar               *filename;
  gchar               *save_uri;
  gchar               *save_filename;
  GimpPlugInProcedure *proc;
  GimpPDBStatusType    status;

  filename = g_build_filename (g_getenv ("GIMP_TESTING_ABS_TOP_SRCDIR"),
                               "desktop/64x64/gimp.png",
                               NULL);
  uri = g_filename_to_uri (filename, NULL, NULL);

  image = file_open_image (gimp,
                           gimp_get_user_context (gimp),
                           NULL /*progress*/,
                           uri,
                           filename,
                           FALSE /*as_new*/,
                           NULL /*file_proc*/,
                           GIMP_RUN_NONINTERACTIVE,
                           &status,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  g_assert (gimp_image_get_uri (image) == NULL);
  g_assert_cmpstr (gimp_image_get_imported_uri (image), ==, uri);
  g_assert (gimp_image_get_exported_uri (image) == NULL);

  save_filename = g_build_filename (g_get_tmp_dir (), "gimp-test.png", NULL);
  save_uri = g_filename_to_uri (save_filename, NULL, NULL);

  proc = file_procedure_find (image->gimp->plug_in_manager->export_procs,
                              save_uri,
                              NULL /*error*/);
  file_save (gimp,
             image,
             NULL /*progress*/,
             save_uri,
             proc,
             GIMP_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             TRUE /*export_forward*/,
             NULL /*error*/);

  g_assert (gimp_image_get_uri (image) == NULL);
  g_assert (gimp_image_get_imported_uri (image) == NULL);
  g_assert_cmpstr (gimp_image_get_exported_uri (image), ==, save_uri);

  g_unlink (save_filename);
}

int main(int argc, char **argv)
{
  Gimp *gimp   = NULL;
  gint  result = -1;

  gimp_test_bail_if_no_display ();
  gtk_test_init (&argc, &argv, NULL);

  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/gimpdir");
  gimp_test_utils_setup_menus_dir ();

  /* Start up GIMP */
  gimp = gimp_init_for_gui_testing (TRUE /*show_gui*/);
  gimp_test_run_mainloop_until_idle ();

  ADD_TEST (new_file_has_no_uris);
  ADD_TEST (opened_xcf_file_uris);
  ADD_TEST (imported_file_uris);
  ADD_TEST (saved_imported_file_uris);
  ADD_TEST (exported_file_uris);
  ADD_TEST (clear_import_uri_after_export);

  /* Run the tests and return status */
  result = g_test_run ();

  /* Don't write files to the source dir */
  gimp_test_utils_set_gimp2_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");

  /* Exit properly so we don't break script-fu plug-in wire */
  gimp_exit (gimp, TRUE);

  return result;
}
