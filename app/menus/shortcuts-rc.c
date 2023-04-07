/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * shortcuts-rc.c
 * Copyright (C) 2023 Jehan
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
#include "libgimpbase/gimpprotocol.h"
#include "libgimpconfig/gimpconfig.h"

#include "menus-types.h"

#include "widgets/gimpaction.h"

#include "shortcuts-rc.h"

#include "gimp-intl.h"


#define SHORTCUTS_RC_FILE_VERSION 1


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get,
 *  or G_TOKEN_ERROR if the function already set an error itself.
 */

static GTokenType shortcuts_action_deserialize (GScanner       *scanner,
                                                GtkApplication *application);


enum
{
  PROTOCOL_VERSION = 1,
  FILE_VERSION,
  ACTION,
};


gboolean
shortcuts_rc_parse (GtkApplication  *application,
                    GFile           *file,
                    GError         **error)
{
  GScanner   *scanner;
  gint        protocol_version = GIMP_PROTOCOL_VERSION;
  gint        file_version     = SHORTCUTS_RC_FILE_VERSION;
  GTokenType  token;

  g_return_val_if_fail (GTK_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  scanner = gimp_scanner_new_file (file, error);

  if (! scanner)
    return FALSE;

  g_scanner_scope_add_symbol (scanner, 0,
                              "protocol-version",
                              GINT_TO_POINTER (PROTOCOL_VERSION));
  g_scanner_scope_add_symbol (scanner, 0,
                              "file-version",
                              GINT_TO_POINTER (FILE_VERSION));
  g_scanner_scope_add_symbol (scanner, 0,
                              "action", GINT_TO_POINTER (ACTION));

  token = G_TOKEN_LEFT_PAREN;

  while (protocol_version == GIMP_PROTOCOL_VERSION     &&
         file_version     == SHORTCUTS_RC_FILE_VERSION &&
         g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case PROTOCOL_VERSION:
              token = G_TOKEN_INT;
              if (gimp_scanner_parse_int (scanner, &protocol_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;

            case FILE_VERSION:
              token = G_TOKEN_INT;
              if (gimp_scanner_parse_int (scanner, &file_version))
                token = G_TOKEN_RIGHT_PAREN;
              break;

            case ACTION:
              g_scanner_set_scope (scanner, ACTION);
              token = shortcuts_action_deserialize (scanner, application);
              g_scanner_set_scope (scanner, 0);
              break;
            default:
              break;
            }
              break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  if (protocol_version != GIMP_PROTOCOL_VERSION     ||
      file_version     != SHORTCUTS_RC_FILE_VERSION ||
      token            != G_TOKEN_LEFT_PAREN)
    {
      if (protocol_version != GIMP_PROTOCOL_VERSION)
        {
          g_set_error (error,
                       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong GIMP protocol version."),
                       gimp_file_get_utf8_name (file));
        }
      else if (file_version != SHORTCUTS_RC_FILE_VERSION)
        {
          g_set_error (error,
                       GIMP_CONFIG_ERROR, GIMP_CONFIG_ERROR_VERSION,
                       _("Skipping '%s': wrong shortcutsrc file format version."),
                       gimp_file_get_utf8_name (file));
        }
      else if (token != G_TOKEN_ERROR)
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
        }

      return FALSE;
    }

  gimp_scanner_unref (scanner);

  return TRUE;
}

gboolean
shortcuts_rc_write (GtkApplication  *application,
                   GFile            *file,
                   GError          **error)
{
  GimpConfigWriter  *writer;
  gchar            **actions;

  g_return_val_if_fail (GTK_IS_APPLICATION (application), FALSE);
  g_return_val_if_fail (G_IS_FILE (file), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  writer = gimp_config_writer_new_from_file (file,
                                             FALSE,
                                             "GIMP shortcutsrc\n\n"
                                             "If you delete this file, all shortcuts "
                                             "will be reset to defaults.",
                                             error);
  if (! writer)
    return FALSE;

  actions = g_action_group_list_actions (G_ACTION_GROUP (application));

  gimp_config_writer_open (writer, "protocol-version");
  gimp_config_writer_printf (writer, "%d", GIMP_PROTOCOL_VERSION);
  gimp_config_writer_close (writer);

  gimp_config_writer_open (writer, "file-version");
  gimp_config_writer_printf (writer, "%d", SHORTCUTS_RC_FILE_VERSION);
  gimp_config_writer_close (writer);

  gimp_config_writer_linefeed (writer);

  for (gint i = 0; actions[i] != NULL; i++)
    {
      GimpAction  *action;
      gchar      **accels;
      gchar       *detailed_name;
      gboolean     commented = FALSE;

      action = (GimpAction *) g_action_map_lookup_action (G_ACTION_MAP (application), actions[i]);

      detailed_name  = g_strdup_printf ("app.%s", actions[i]);
      accels         = gtk_application_get_accels_for_action (application, detailed_name);
      if (gimp_action_use_default_accels (action))
        commented = TRUE;

      gimp_config_writer_comment_mode (writer, commented);

      gimp_config_writer_open (writer, "action");
      gimp_config_writer_string (writer, actions[i]);

      for (gint j = 0; accels[j]; j++)
        gimp_config_writer_string (writer, accels[j]);

      gimp_config_writer_close (writer);
      gimp_config_writer_comment_mode (writer, FALSE);

      g_strfreev (accels);
      g_free (detailed_name);
    }

  g_strfreev (actions);

  return gimp_config_writer_finish (writer, "end of shortcutsrc", error);
}


/* Private functions */

static GTokenType
shortcuts_action_deserialize (GScanner       *scanner,
                              GtkApplication *application)
{
  GStrvBuilder  *builder;
  gchar         *action_name;
  gchar         *accel;
  gchar        **accels;

  if (! gimp_scanner_parse_string (scanner, &action_name))
    return G_TOKEN_STRING;

  builder = g_strv_builder_new ();
  while (gimp_scanner_parse_string (scanner, &accel))
    {
      g_strv_builder_add (builder, accel);
      g_free (accel);
    }
  accels = g_strv_builder_end (builder);

  if (g_action_group_has_action (G_ACTION_GROUP (application), action_name))
    {
      gchar *detailed_name;

      detailed_name = g_strdup_printf ("app.%s", action_name);
      gtk_application_set_accels_for_action (application, detailed_name,
                                             (const gchar **) accels);
      g_free (detailed_name);
    }
  else
    {
      /* Don't set a breaking error, just output on stderr, so that we can make a
       * notice while still loading other actions.
       */
      g_printerr ("INFO: not existing action '%s' was ignored from the shortcutsrc file.\n",
                  action_name);
    }

  g_strv_builder_unref (builder);
  g_free (action_name);
  g_strfreev (accels);

  if (! gimp_scanner_parse_token (scanner, G_TOKEN_RIGHT_PAREN))
    return G_TOKEN_RIGHT_PAREN;

  return G_TOKEN_LEFT_PAREN;
}
