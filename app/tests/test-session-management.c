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
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <string.h>
#include <utime.h>

#include "libgimpbase/gimpbase.h"

#include "dialogs/dialogs-types.h"

#include "dialogs/dialogs.h"

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsessioninfo.h"

#include "core/gimp.h"
#include "core/gimpcontext.h"

#include "tests.h"


typedef struct
{
  int dummy;
} GimpTestFixture;


static gboolean gimp_test_get_sessionrc_timestamp_and_md5 (gchar    **checksum,
                                                           GTimeVal  *modtime);

static Gimp *gimp = NULL;


int main(int argc, char **argv)
{
  gchar    *initial_md5     = NULL;
  gchar    *final_md5       = NULL;
  GTimeVal  initial_modtime = { 0, };
  GTimeVal  final_modtime   = { 0, };

  g_type_init ();
  gtk_init (&argc, &argv);
  g_test_init (&argc, &argv, NULL);

  /* Remeber the timestamp and MD5 on sessionrc */
  if (!gimp_test_get_sessionrc_timestamp_and_md5 (&initial_md5,
                                                  &initial_modtime))
    goto fail;

  /* Start up GIMP and let the main loop run for a while (quits after
   * a short timeout) to let things stabilize. This includes parsing
   * sessionrc
   */
  gimp = gimp_init_for_gui_testing (FALSE, TRUE);
  gimp_test_run_temp_mainloop (4000);

  /* Exit. This includes writing sessionrc */
  gimp_exit (gimp, TRUE);

  /* Now get the new MD5 and modtime */
  if (!gimp_test_get_sessionrc_timestamp_and_md5 (&final_md5,
                                                  &final_modtime))
    goto fail;

  /* If things have gone our way, GIMP will have deserialized
   * sessionrc, shown the GUI, and then serialized the new sessionrc.
   * To make sure we have a new sessionrc we check the modtime, and to
   * make sure that the sessionrc remains the same we compare its MD5
   */
  if (initial_modtime.tv_sec == final_modtime.tv_sec)
    {
      g_printerr ("A new sessionrc was not created\n");
      goto fail;
    }
  if (strcmp (initial_md5, final_md5) != 0)
    {
      g_printerr ("The new sessionrc is not identical to the old one\n");
      goto fail;
    }

  g_free (initial_md5);
  g_free (final_md5);

  return 0;

 fail:
  return -1;
}

static gboolean
gimp_test_get_sessionrc_timestamp_and_md5 (gchar    **checksum,
                                           GTimeVal  *modtime)
{
  gchar    *filename = NULL;
  gboolean  success  = TRUE;

  filename = gimp_personal_rc_file ("sessionrc");

  /* Get checksum */
  if (success)
    {
      gchar *contents = NULL;
      gsize  length   = 0;

      success = g_file_get_contents (filename,
                                     &contents,
                                     &length,
                                     NULL);
      if (success)
        {
          *checksum = g_compute_checksum_for_string (G_CHECKSUM_MD5,
                                                     contents,
                                                     length);
        }

      g_free (contents);
    }

  /* Get modification time */
  if (success)
    {
      GFile     *file = g_file_new_for_path (filename);
      GFileInfo *info = g_file_query_info (file,
                                           G_FILE_ATTRIBUTE_TIME_MODIFIED, 0,
                                           NULL, NULL);
      if (info)
        {
          g_file_info_get_modification_time (info, modtime);
          success = TRUE;
          g_object_unref (info);
        }
      else
        {
          success = FALSE;
        }

      g_object_unref (file);
    }

  g_free (filename);

  return success;
}
