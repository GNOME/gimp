/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligma-contexts.c
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

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligmaerror.h"
#include "ligma-contexts.h"
#include "ligmacontext.h"

#include "config/ligmaconfig-file.h"

#include "ligma-intl.h"


void
ligma_contexts_init (Ligma *ligma)
{
  LigmaContext *context;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  /*  the default context contains the user's saved preferences
   *
   *  TODO: load from disk
   */
  context = ligma_context_new (ligma, "Default", NULL);
  ligma_set_default_context (ligma, context);
  g_object_unref (context);

  /*  the initial user_context is a straight copy of the default context
   */
  context = ligma_context_new (ligma, "User", context);
  ligma_set_user_context (ligma, context);
  g_object_unref (context);
}

void
ligma_contexts_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_set_user_context (ligma, NULL);
  ligma_set_default_context (ligma, NULL);
}

gboolean
ligma_contexts_load (Ligma    *ligma,
                    GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = ligma_directory_file ("contextrc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  success = ligma_config_deserialize_file (LIGMA_CONFIG (ligma_get_user_context (ligma)),
                                          file,
                                          NULL, &my_error);

  g_object_unref (file);

  if (! success)
    {
      if (my_error->code == LIGMA_CONFIG_ERROR_OPEN_ENOENT)
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
ligma_contexts_save (Ligma    *ligma,
                    GError **error)
{
  GFile    *file;
  gboolean  success;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = ligma_directory_file ("contextrc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  success = ligma_config_serialize_to_file (LIGMA_CONFIG (ligma_get_user_context (ligma)),
                                           file,
                                           "LIGMA user context",
                                           "end of user context",
                                           NULL, error);

  g_object_unref (file);

  return success;
}

gboolean
ligma_contexts_clear (Ligma    *ligma,
                     GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);

  file = ligma_directory_file ("contextrc", NULL);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   ligma_file_get_utf8_name (file), my_error->message);
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}
