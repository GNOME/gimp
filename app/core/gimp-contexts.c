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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimperror.h"
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
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_directory_file ("contextrc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  success = gimp_config_deserialize_gfile (GIMP_CONFIG (gimp_get_user_context (gimp)),
                                           file,
                                           NULL, &my_error);

  g_object_unref (file);

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

  return success;
}

gboolean
gimp_contexts_save (Gimp    *gimp,
                    GError **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = gimp_directory_file ("contextrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  success = gimp_config_serialize_to_gfile (GIMP_CONFIG (gimp_get_user_context (gimp)),
                                            file,
                                            "GIMP user context",
                                            "end of user context",
                                            NULL, error);

  g_object_unref (file);

  return success;
}

gboolean
gimp_contexts_clear (Gimp    *gimp,
                     GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);

  file = gimp_directory_file ("contextrc", NULL);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   gimp_file_get_utf8_name (file), my_error->message);
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}
