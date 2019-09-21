/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Session-managment stuff
 * Copyright (C) 1998 Sven Neumann <sven@gimp.org>
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

#include "widgets/gimpdialogfactory.h"
#include "widgets/gimpsessioninfo.h"
#include "widgets/gimpwidgets-utils.h"

#include "dialogs/dialogs.h"

#include "session.h"
#include "gimp-log.h"

#include "gimp-intl.h"


enum
{
  SESSION_INFO = 1,
  HIDE_DOCKS,
  SINGLE_WINDOW_MODE,
  SHOW_TABS,
  TABS_POSITION,
  LAST_TIP_SHOWN
};


static GFile * session_file (Gimp *gimp);


/*  private variables  */

static gboolean   sessionrc_deleted = FALSE;


/*  public functions  */

void
session_init (Gimp *gimp)
{
  GFile      *file;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = session_file (gimp);

  scanner = gimp_scanner_new_file (file, &error);

  if (! scanner && error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_object_unref (file);

      file = gimp_sysconf_directory_file ("sessionrc", NULL);

      scanner = gimp_scanner_new_file (file, NULL);
    }

  if (! scanner)
    {
      g_clear_error (&error);
      g_object_unref (file);
      return;
    }

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  g_scanner_scope_add_symbol (scanner, 0, "session-info",
                              GINT_TO_POINTER (SESSION_INFO));
  g_scanner_scope_add_symbol (scanner, 0,  "hide-docks",
                              GINT_TO_POINTER (HIDE_DOCKS));
  g_scanner_scope_add_symbol (scanner, 0,  "single-window-mode",
                              GINT_TO_POINTER (SINGLE_WINDOW_MODE));
  g_scanner_scope_add_symbol (scanner, 0,  "show-tabs",
                              GINT_TO_POINTER (SHOW_TABS));
  g_scanner_scope_add_symbol (scanner, 0,  "tabs-position",
                              GINT_TO_POINTER (TABS_POSITION));
  g_scanner_scope_add_symbol (scanner, 0,  "last-tip-shown",
                              GINT_TO_POINTER (LAST_TIP_SHOWN));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GINT_TO_POINTER (SESSION_INFO))
            {
              GimpDialogFactory      *factory      = NULL;
              GimpSessionInfo        *info         = NULL;
              gchar                  *factory_name = NULL;
              gchar                  *entry_name   = NULL;
              GimpDialogFactoryEntry *entry        = NULL;

              token = G_TOKEN_STRING;

              if (! gimp_scanner_parse_string (scanner, &factory_name))
                break;

              /* In versions <= GIMP 2.6 there was a "toolbox", a
               * "dock", a "display" and a "toplevel" factory. These
               * are now merged to a single gimp_dialog_factory_get_singleton (). We
               * need the legacy name though, so keep it around.
               */
              factory = gimp_dialog_factory_get_singleton ();

              info = gimp_session_info_new ();

              /* GIMP 2.6 has the entry name as part of the
               * session-info header, so try to get it
               */
              gimp_scanner_parse_string (scanner, &entry_name);
              if (entry_name)
                {
                  /* Previously, GimpDock was a toplevel. That is why
                   * versions <= GIMP 2.6 has "dock" as the entry name. We
                   * want "dock" to be interpreted as 'dock window'
                   * however so have some special-casing for that. When
                   * the entry name is "dock" the factory name is either
                   * "dock" or "toolbox".
                   */
                  if (strcmp (entry_name, "dock") == 0)
                    {
                      entry =
                        gimp_dialog_factory_find_entry (factory,
                                                        (strcmp (factory_name, "toolbox") == 0 ?
                                                         "gimp-toolbox-window" :
                                                         "gimp-dock-window"));
                    }
                  else
                    {
                      entry = gimp_dialog_factory_find_entry (factory,
                                                              entry_name);
                    }
                }

              /* We're done with these now */
              g_free (factory_name);
              g_free (entry_name);

              /* We can get the factory entry either now (the GIMP <=
               * 2.6 way), or when we deserialize (the GIMP 2.8 way)
               */
              if (entry)
                {
                  gimp_session_info_set_factory_entry (info, entry);
                }

              /* Always try to deserialize */
              if (gimp_config_deserialize (GIMP_CONFIG (info), scanner, 1, NULL))
                {
                  /* Make sure we got a factory entry either the 2.6
                   * or 2.8 way
                   */
                  if (gimp_session_info_get_factory_entry (info))
                    {
                      GIMP_LOG (DIALOG_FACTORY,
                                "successfully parsed and added session info %p",
                                info);

                      gimp_dialog_factory_add_session_info (factory, info);
                    }
                  else
                    {
                      GIMP_LOG (DIALOG_FACTORY,
                                "failed to parse session info %p, not adding",
                                info);
                    }

                  g_object_unref (info);
                }
              else
                {
                  g_object_unref (info);

                  /* set token to left paren so we won't set another
                   * error below, gimp_config_deserialize() already did
                   */
                  token = G_TOKEN_LEFT_PAREN;
                  goto error;
                }
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (HIDE_DOCKS))
            {
              gboolean hide_docks;

              token = G_TOKEN_IDENTIFIER;

              if (! gimp_scanner_parse_boolean (scanner, &hide_docks))
                break;

              g_object_set (gimp->config,
                            "hide-docks", hide_docks,
                            NULL);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (SINGLE_WINDOW_MODE))
            {
              gboolean single_window_mode;

              token = G_TOKEN_IDENTIFIER;

              if (! gimp_scanner_parse_boolean (scanner, &single_window_mode))
                break;

              g_object_set (gimp->config,
                            "single-window-mode", single_window_mode,
                            NULL);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (SHOW_TABS))
          {
            gboolean show_tabs;

            token = G_TOKEN_IDENTIFIER;

            if (! gimp_scanner_parse_boolean (scanner, &show_tabs))
              break;

            g_object_set (gimp->config,
                          "show-tabs", show_tabs,
                          NULL);
          }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (TABS_POSITION))
            {
              gint tabs_position;

              token = G_TOKEN_INT;

              if (! gimp_scanner_parse_int (scanner, &tabs_position))
                break;

              g_object_set (gimp->config,
                            "tabs-position", tabs_position,
                            NULL);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (LAST_TIP_SHOWN))
            {
              gint last_tip_shown;

              token = G_TOKEN_INT;

              if (! gimp_scanner_parse_int (scanner, &last_tip_shown))
                break;

              g_object_set (gimp->config,
                            "last-tip-shown", last_tip_shown,
                            NULL);
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

 error:

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  if (error)
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      gimp_config_file_backup_on_error (file, "sessionrc", NULL);
    }

  gimp_scanner_unref (scanner);
  g_object_unref (file);

  dialogs_load_recent_docks (gimp);
}

void
session_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
}

void
session_restore (Gimp       *gimp,
                 GdkMonitor *monitor)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (GDK_IS_MONITOR (monitor));

  gimp_dialog_factory_restore (gimp_dialog_factory_get_singleton (),
                               monitor);

  /* make sure GimpImageWindow acts upon hide-docks at the right time,
   * see bug #678043.
   */
  if (GIMP_GUI_CONFIG (gimp->config)->single_window_mode &&
      GIMP_GUI_CONFIG (gimp->config)->hide_docks)
    {
      g_object_notify (G_OBJECT (gimp->config), "hide-docks");
    }
}

void
session_save (Gimp     *gimp,
              gboolean  always_save)
{
  GimpConfigWriter *writer;
  GFile            *file;
  GError           *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (sessionrc_deleted && ! always_save)
    return;

  file = session_file (gimp);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  writer =
    gimp_config_writer_new_from_file (file,
                                      TRUE,
                                      "GIMP sessionrc\n\n"
                                      "This file takes session-specific info "
                                      "(that is info, you want to keep between "
                                      "two GIMP sessions).  You are not supposed "
                                      "to edit it manually, but of course you "
                                      "can do.  The sessionrc will be entirely "
                                      "rewritten every time you quit GIMP.  "
                                      "If this file isn't found, defaults are "
                                      "used.",
                                      NULL);
  g_object_unref (file);

  if (!writer)
    return;

  gimp_dialog_factory_save (gimp_dialog_factory_get_singleton (), writer);
  gimp_config_writer_linefeed (writer);

  gimp_config_writer_open (writer, "hide-docks");
  gimp_config_writer_identifier (writer,
                                 GIMP_GUI_CONFIG (gimp->config)->hide_docks ?
                                 "yes" : "no");
  gimp_config_writer_close (writer);

  gimp_config_writer_open (writer, "single-window-mode");
  gimp_config_writer_identifier (writer,
                                 GIMP_GUI_CONFIG (gimp->config)->single_window_mode ?
                                 "yes" : "no");
  gimp_config_writer_close (writer);

  gimp_config_writer_open (writer, "show-tabs");
  gimp_config_writer_printf (writer,
                             GIMP_GUI_CONFIG (gimp->config)->show_tabs ?
                             "yes" : "no");
  gimp_config_writer_close (writer);

  gimp_config_writer_open (writer, "tabs-position");
  gimp_config_writer_printf (writer, "%d",
                             GIMP_GUI_CONFIG (gimp->config)->tabs_position);
  gimp_config_writer_close (writer);

  gimp_config_writer_open (writer, "last-tip-shown");
  gimp_config_writer_printf (writer, "%d",
                             GIMP_GUI_CONFIG (gimp->config)->last_tip_shown);
  gimp_config_writer_close (writer);

  if (! gimp_config_writer_finish (writer, "end of sessionrc", &error))
    {
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }

  dialogs_save_recent_docks (gimp);

  sessionrc_deleted = FALSE;
}

gboolean
session_clear (Gimp    *gimp,
               GError **error)
{
  GFile    *file;
  GError   *my_error = NULL;
  gboolean  success  = TRUE;

  g_return_val_if_fail (GIMP_IS_GIMP (gimp), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  file = session_file (gimp);

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
      sessionrc_deleted = TRUE;
    }

  g_clear_error (&my_error);
  g_object_unref (file);

  return success;
}


static GFile *
session_file (Gimp *gimp)
{
  const gchar *basename;
  gchar       *filename;
  GFile       *file;

  basename = g_getenv ("GIMP_TESTING_SESSIONRC_NAME");
  if (! basename)
    basename = "sessionrc";

  if (gimp->session_name)
    filename = g_strconcat (basename, ".", gimp->session_name, NULL);
  else
    filename = g_strdup (basename);

  file = gimp_directory_file (filename, NULL);

  g_free (filename);

  return file;
}
