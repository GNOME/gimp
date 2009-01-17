/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimp-contexts.c
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

#include "config.h"

#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-contexts.h"
#include "gimpcontext.h"

#include "config/gimpconfig-file.h"

#include "gimp-intl.h"


void
gimp_contexts_init (Gimp *gimp)
{
  GimpContext *context;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  /*  the default context contains the user's saved preferences
   *
   *  TODO: load from disk
   */
  context = gimp_context_new (gimp, "Default", NULL);
  gimp_set_default_context (gimp, context);
  g_object_unref (context);

  /*  the initial user_context is a straight copy of the default context
   */
  context = gimp_context_new (gimp, "User", context);
  gimp_set_user_context (gimp, context);
  g_object_unref (context);
}

void
gimp_contexts_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_set_user_context (gimp, NULL);
  gimp_set_default_context (gimp, NULL);
}

gboolean
gimp_contexts_load (Gimp    *gimp,
                    GError **error)
{
  gchar    *filename;
  GError   *my_error = NULL;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_personal_rc_file ("contextrc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  success = gimp_config_deserialize_file (GIMP_CONFIG (gimp_get_user_context (gimp)),
                                          filename,
                                          NULL, &my_error);

  if (! success)
    {
      if (my_error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          g_clear_error (&my_error);
          success = TRUE;
        }
      else
        {
          g_propagate_error (error, my_error);
        }
    }

  g_free (filename);

  return success;
}

gboolean
gimp_contexts_save (Gimp    *gimp,
                    GError **error)
{
  gchar    *filename;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  filename = gimp_personal_rc_file ("contextrc");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  success = gimp_config_serialize_to_file (GIMP_CONFIG (gimp_get_user_context (gimp)),
                                           filename,
                                           "GIMP user context",
                                           "end of user context",
                                           NULL, error);

  g_free (filename);

  return success;
}

gboolean
gimp_contexts_clear (Gimp    *gimp,
                     GError **error)
{
  gchar    *filename;
  gboolean  success = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  filename = gimp_personal_rc_file ("contextrc");

  if (g_unlink (filename) != 0 && errno != ENOENT)
    {
      g_set_error (error, G_FILE_ERROR, g_file_error_from_errno (errno),
                   _("Deleting \"%s\" failed: %s"),
                   gimp_filename_to_utf8 (filename), g_strerror (errno));
      success = FALSE;
    }

  g_free (filename);

  return success;
}
