/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * modifiers.c
 * Copyright (C) 2022 Jehan
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
#include <gtk/gtk.h>

#include "libligmabase/ligmabase.h"
#include "libligmaconfig/ligmaconfig.h"

#include "gui-types.h"

#include "config/ligmaconfig-file.h"
#include "config/ligmaguiconfig.h"

#include "core/ligma.h"
#include "core/ligmaerror.h"

#include "display/ligmamodifiersmanager.h"

#include "widgets/ligmawidgets-utils.h"

#include "dialogs/dialogs.h"

#include "modifiers.h"
#include "ligma-log.h"

#include "ligma-intl.h"


enum
{
  MODIFIERS_INFO = 1,
  HIDE_DOCKS,
  SINGLE_WINDOW_MODE,
  SHOW_TABS,
  TABS_POSITION,
  LAST_TIP_SHOWN
};


static GFile * modifiers_file (Ligma *ligma);


/*  private variables  */

static gboolean   modifiersrc_deleted = FALSE;


/*  public functions  */

void
modifiers_init (Ligma *ligma)
{
  LigmaDisplayConfig    *display_config;
  GFile                *file;
  LigmaModifiersManager *manager = NULL;
  GError               *error   = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  display_config = LIGMA_DISPLAY_CONFIG (ligma->config);
  if (display_config->modifiers_manager != NULL)
    return;

  manager = ligma_modifiers_manager_new ();
  g_object_set (display_config, "modifiers-manager", manager, NULL);
  g_object_unref (manager);

  file = modifiers_file (ligma);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  ligma_config_deserialize_file (LIGMA_CONFIG (manager), file, NULL, &error);

  if (error)
    {
      /* File not existing is considered a normal event, not an error.
       * It can happen for instance the first time you run LIGMA. When
       * this happens, we ignore the error. The LigmaModifiersManager
       * object will simply use default modifiers.
       */
      if (error->domain != LIGMA_CONFIG_ERROR ||
          error->code != LIGMA_CONFIG_ERROR_OPEN_ENOENT)
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          ligma_config_file_backup_on_error (file, "modifiersrc", NULL);
        }

      g_clear_error (&error);
    }

  g_object_unref (file);
}

void
modifiers_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
}

void
modifiers_restore (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
}

void
modifiers_save (Ligma     *ligma,
                gboolean  always_save)
{
  LigmaDisplayConfig    *display_config;
  GFile                *file;
  LigmaModifiersManager *manager = NULL;
  GError               *error   = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (modifiersrc_deleted && ! always_save)
    return;

  display_config = LIGMA_DISPLAY_CONFIG (ligma->config);
  g_return_if_fail (LIGMA_IS_DISPLAY_CONFIG (display_config));

  manager = LIGMA_MODIFIERS_MANAGER (display_config->modifiers_manager);
  g_return_if_fail (manager != NULL);
  g_return_if_fail (LIGMA_IS_MODIFIERS_MANAGER (manager));
  file = modifiers_file (ligma);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  ligma_config_serialize_to_file (LIGMA_CONFIG (manager), file,
                                 "LIGMA modifiersrc\n\n"
                                 "This file stores modifiers configuration. "
                                 "You are not supposed to edit it manually, "
                                 "but of course you can do. The modifiersrc "
                                 "will be entirely rewritten every time you "
                                 "quit LIGMA. If this file isn't found, "
                                 "defaults are used.",
                                 NULL, NULL, &error);
  if (error != NULL)
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);

  modifiersrc_deleted = FALSE;
}

gboolean
modifiers_clear (Ligma    *ligma,
                 GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (LIGMA_IS_LIGMA (ligma), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = modifiers_file (ligma);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, LIGMA_ERROR, LIGMA_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   ligma_file_get_utf8_name (file), my_error->message);
    }
  else
    {
      modifiersrc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}

static GFile *
modifiers_file (Ligma *ligma)
{
  const gchar *basename;
  GFile       *file;

  basename = g_getenv ("LIGMA_TESTING_MODIFIERSRC_NAME");
  if (! basename)
    basename = "modifiersrc";

  file = ligma_directory_file (basename, NULL);

  return file;
}
