/* GIMP - The GNU Image Manipulation Program
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

#include "libgimpbase/gimpbase.h"
#include "libgimpconfig/gimpconfig.h"

#include "gui-types.h"

#include "config/gimpconfig-file.h"
#include "config/gimpguiconfig.h"

#include "core/gimp.h"
#include "core/gimperror.h"

#include "display/gimpmodifiersmanager.h"

#include "widgets/gimpwidgets-utils.h"

#include "dialogs/dialogs.h"

#include "modifiers.h"
#include "gimp-log.h"

#include "gimp-intl.h"


enum
{
  MODIFIERS_INFO = 1,
  HIDE_DOCKS,
  SINGLE_WINDOW_MODE,
  SHOW_TABS,
  TABS_POSITION,
  LAST_TIP_SHOWN
};


static GFile * modifiers_file (Gimp *gimp);


/*  private variables  */

static gboolean   modifiersrc_deleted = FALSE;


/*  public functions  */

void
modifiers_init (Gimp *gimp)
{
  GimpDisplayConfig    *display_config;
  GFile                *file;
  GimpModifiersManager *manager = NULL;
  GError               *error   = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  display_config = GIMP_DISPLAY_CONFIG (gimp->config);
  if (display_config->modifiers_manager != NULL)
    return;

  manager = gimp_modifiers_manager_new ();
  g_object_set (display_config, "modifiers-manager", manager, NULL);
  g_object_unref (manager);

  file = modifiers_file (gimp);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  gimp_config_deserialize_file (GIMP_CONFIG (manager), file, NULL, &error);

  if (error)
    {
      /* File not existing is considered a normal event, not an error.
       * It can happen for instance the first time you run GIMP. When
       * this happens, we ignore the error. The GimpModifiersManager
       * object will simply use default modifiers.
       */
      if (error->domain != GIMP_CONFIG_ERROR ||
          error->code != GIMP_CONFIG_ERROR_OPEN_ENOENT)
        {
          gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
          gimp_config_file_backup_on_error (file, "modifiersrc", NULL);
        }

      g_clear_error (&error);
    }

  g_object_unref (file);
}

void
modifiers_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}

void
modifiers_restore (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}

void
modifiers_save (Gimp     *gimp,
                gboolean  always_save)
{
  GimpDisplayConfig    *display_config;
  GFile                *file;
  GimpModifiersManager *manager = NULL;
  GError               *error   = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (modifiersrc_deleted && ! always_save)
    return;

  display_config = GIMP_DISPLAY_CONFIG (gimp->config);
  g_return_if_fail (GIMP_IS_DISPLAY_CONFIG (display_config));

  manager = GIMP_MODIFIERS_MANAGER (display_config->modifiers_manager);
  g_return_if_fail (manager != NULL);
  g_return_if_fail (GIMP_IS_MODIFIERS_MANAGER (manager));
  file = modifiers_file (gimp);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  gimp_config_serialize_to_file (GIMP_CONFIG (manager), file,
                                 "GIMP modifiersrc\n\n"
                                 "This file stores modifiers configuration. "
                                 "You are not supposed to edit it manually, "
                                 "but of course you can do. The modifiersrc "
                                 "will be entirely rewritten every time you "
                                 "quit GIMP. If this file isn't found, "
                                 "defaults are used.",
                                 NULL, NULL, &error);
  if (error != NULL)
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  g_object_unref (file);

  modifiersrc_deleted = FALSE;
}

gboolean
modifiers_clear (Gimp    *gimp,
                 GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = modifiers_file (gimp);

  if (! g_file_delete (file, NULL, &my_error) &&
      my_error->code != G_IO_ERROR_NOT_FOUND)
    {
      success = FALSE;

      g_set_error (error, GIMP_ERROR, GIMP_FAILED,
                   _("Deleting \"%s\" failed: %s"),
                   gimp_file_get_utf8_name (file), my_error->message);
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
modifiers_file (Gimp *gimp)
{
  const gchar *basename;
  GFile       *file;

  basename = g_getenv ("GIMP_TESTING_MODIFIERSRC_NAME");
  if (! basename)
    basename = "modifiersrc";

  file = gimp_directory_file (basename, NULL);

  return file;
}
