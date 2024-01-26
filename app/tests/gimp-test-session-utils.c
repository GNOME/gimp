/* GIMP - The GNU Image Manipulation Program
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gegl.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <utime.h>

#include "libgimpbase/gimpbase.h"

#include "dialogs/dialogs-types.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsessioninfo.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "tests.h"

#include "gimp-app-test-utils.h"
#include "gimp-test-session-utils.h"


typedef struct
{
  GFile   *file;
  gchar   *md5;
  guint64  modtime;
} GimpTestFileState;


static gboolean
gimp_test_get_file_state_verbose (GFile             *file,
                                  GimpTestFileState *filestate)
{
  gboolean success = TRUE;

  filestate->file = g_object_ref (file);

  /* Get checksum */
  if (success)
    {
      gchar *filename;
      gchar *contents = NULL;
      gsize  length   = 0;

      filename = g_file_get_path (file);
      success = g_file_get_contents (filename,
                                     &contents,
                                     &length,
                                     NULL);
      g_free (filename);

      if (success)
        {
          filestate->md5 = g_compute_checksum_for_string (G_CHECKSUM_MD5,
                                                          contents,
                                                          length);
        }

      g_free (contents);
    }

  /* Get modification time */
  if (success)
    {
      GFileInfo *info = g_file_query_info (file,
                                           G_FILE_ATTRIBUTE_TIME_MODIFIED, 0,
                                           NULL, NULL);
      if (info)
        {
          filestate->modtime = g_file_info_get_attribute_uint64 (
            info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
          success = TRUE;
          g_object_unref (info);
        }
      else
        {
          success = FALSE;
        }
    }

  if (! success)
    g_printerr ("Failed to get initial file info for '%s'\n",
                gimp_file_get_utf8_name (file));

  return success;
}

static gboolean
gimp_test_file_state_changes (GFile             *file,
                              GimpTestFileState *state1,
                              GimpTestFileState *state2)
{
  if (state1->modtime == state2->modtime)
    {
      g_printerr ("A new '%s' was not created\n",
                  gimp_file_get_utf8_name (file));
      return FALSE;
    }

  if (strcmp (state1->md5, state2->md5) != 0)
    {
      char *diff_argv[5] = {
        "diff",
        "-u",
        g_file_get_path (state1->file),
        g_file_get_path (state2->file),
        NULL
      };

      g_printerr ("'%s' was changed but should not have been. Reason, using "
                  "`diff -u $expected $actual`\n",
                  gimp_file_get_utf8_name (file));

      g_spawn_sync (NULL /*working_directory*/,
                    diff_argv,
                    NULL /*envp*/,
                    G_SPAWN_SEARCH_PATH,
                    NULL /*child_setup*/,
                    NULL /*user_data*/,
                    NULL /*standard_output*/,
                    NULL /*standard_error*/,
                    NULL /*exist_status*/,
                    NULL /*error*/);

      g_free (diff_argv[2]);
      g_free (diff_argv[3]);

      return FALSE;
    }

  return TRUE;
}

/**
 * gimp_test_session_load_and_write_session_files:
 * @loaded_sessionrc:   The name of the file of the sessionrc file to
 *                      load
 * @loaded_dockrc:      The name of the file of the dockrc file to load
 * @expected_sessionrc: The name of the file with the expected
 *                      sessionrc file content
 * @expected_dockrc:    The name of the file with the expected dockrc
 *                      file content
 *
 * Utility function for the various session management tests. We can't
 * easily have all those tests in a single program several Gimp
 * instance can't easily be initialized in the same process.
 **/
void
gimp_test_session_load_and_write_session_files (const gchar *loaded_sessionrc,
                                                const gchar *loaded_dockrc,
                                                const gchar *expected_sessionrc,
                                                const gchar *expected_dockrc,
                                                gboolean     single_window_mode)
{
  Gimp              *gimp;
  GimpTestFileState  initial_sessionrc_state = { NULL, NULL, 0 };
  GimpTestFileState  initial_dockrc_state    = { NULL, NULL, 0 };
  GimpTestFileState  final_sessionrc_state   = { NULL, NULL, 0 };
  GimpTestFileState  final_dockrc_state      = { NULL, NULL, 0 };
  GFile             *sessionrc_file          = NULL;
  GFile             *dockrc_file             = NULL;

  /* Make sure to run this before we use any GIMP functions */
  gimp_test_utils_set_gimp3_directory ("GIMP_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/gimpdir");
  gimp_test_utils_setup_menus_path ();

  /* Note that we expect the resulting sessionrc to be different from
   * the read file, which is why we check the MD5 of the -expected
   * variant
   */
  sessionrc_file = gimp_directory_file (expected_sessionrc, NULL);
  dockrc_file    = gimp_directory_file (expected_dockrc, NULL);

  /* Remember the modtimes and MD5s */
  g_assert_true (gimp_test_get_file_state_verbose (sessionrc_file,
                                                   &initial_sessionrc_state));
  g_assert_true (gimp_test_get_file_state_verbose (dockrc_file,
                                                   &initial_dockrc_state));

  /* Use specific input files when restoring the session */
  g_setenv ("GIMP_TESTING_SESSIONRC_NAME", loaded_sessionrc, TRUE /*overwrite*/);
  g_setenv ("GIMP_TESTING_DOCKRC_NAME", loaded_dockrc, TRUE /*overwrite*/);

  /* Start up GIMP */
  gimp = gimp_init_for_gui_testing (TRUE /*show_gui*/);

  /* Let the main loop run until idle to let things stabilize. This
   * includes parsing sessionrc and dockrc
   */
  gimp_test_run_mainloop_until_idle ();

  /* Change the gimp dir to the output dir so files are written there,
   * we don't want to (can't always) write to files in the source
   * dir. There is a hook in Makefile.am that makes sure the output
   * dir exists
   */
  gimp_test_utils_set_gimp3_directory ("GIMP_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/gimpdir-output");
  /* Use normal output names */
  g_unsetenv ("GIMP_TESTING_SESSIONRC_NAME");
  g_unsetenv ("GIMP_TESTING_DOCKRC_NAME");

  g_object_unref (sessionrc_file);
  g_object_unref (dockrc_file);

  sessionrc_file = gimp_directory_file ("sessionrc", NULL);
  dockrc_file    = gimp_directory_file ("dockrc", NULL);

  /* Exit. This includes writing sessionrc and dockrc*/
  gimp_exit (gimp, TRUE);

  /* Now get the new modtimes and MD5s */
  g_assert_true (gimp_test_get_file_state_verbose (sessionrc_file,
                                                   &final_sessionrc_state));
  g_assert_true (gimp_test_get_file_state_verbose (dockrc_file,
                                                   &final_dockrc_state));

  /* If things have gone our way, GIMP will have deserialized
   * sessionrc and dockrc, shown the GUI, and then serialized the new
   * files. To make sure we have new files we check the modtime, and
   * to make sure that their content remains the same we compare their
   * MD5
   */
  g_assert_true (gimp_test_file_state_changes (g_file_new_for_path ("sessionrc"),
                                               &initial_sessionrc_state,
                                               &final_sessionrc_state));
  g_assert_true (gimp_test_file_state_changes (g_file_new_for_path ("dockrc"),
                                               &initial_dockrc_state,
                                               &final_dockrc_state));
}
