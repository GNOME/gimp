/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Session-managment stuff
 * Copyright (C) 1998 Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <stdio.h>

#include <gtk/gtk.h>

#include "libgimpbase/gimpbase.h"

#include "gui-types.h"

#include "core/gimp.h"

#include "config/gimpscanner.h"

#include "widgets/gimpdialogfactory.h"

#include "color-notebook.h"
#include "session.h"

#include "gimprc.h"

#include "libgimp/gimpintl.h"


/*  local function prototypes  */

static GTokenType   session_info_deserialize (GScanner *scanner,
                                              Gimp     *gimp);


/*  public functions  */

enum
{
  SESSION_INFO = 1,
  COLOR_HISTORY,
  LAST_TIP_SHOWN,

  SESSION_INFO_POSITION,
  SESSION_INFO_SIZE,
  SESSION_INFO_OPEN,
  SESSION_INFO_AUX,
  SESSION_INFO_DOCK
};

void
session_init (Gimp *gimp)
{
  gchar      *filename;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("sessionrc");
  scanner = gimp_scanner_new (filename, &error);
  g_free (filename);

  if (! scanner)
    {
      /*  always show L&C&P, Tool Options and Brushes on first invocation  */

      /* TODO */

      return;
    }

  g_scanner_scope_add_symbol (scanner, 0, "session-info",
                              GINT_TO_POINTER (SESSION_INFO));
  g_scanner_scope_add_symbol (scanner, 0, "color-history",
                              GINT_TO_POINTER (COLOR_HISTORY));
  g_scanner_scope_add_symbol (scanner, 0,  "last-tip-shown",
                              GINT_TO_POINTER (LAST_TIP_SHOWN));

  g_scanner_scope_add_symbol (scanner, SESSION_INFO, "position",
                              GINT_TO_POINTER (SESSION_INFO_POSITION));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO, "size",
                              GINT_TO_POINTER (SESSION_INFO_SIZE));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO, "open-on-exit",
                              GINT_TO_POINTER (SESSION_INFO_OPEN));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO, "aux-info",
                              GINT_TO_POINTER (SESSION_INFO_AUX));
  g_scanner_scope_add_symbol (scanner, SESSION_INFO, "dock",
                              GINT_TO_POINTER (SESSION_INFO_DOCK));

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
              g_scanner_set_scope (scanner, SESSION_INFO);
              token = session_info_deserialize (scanner, gimp);

              if (token == G_TOKEN_RIGHT_PAREN)
                g_scanner_set_scope (scanner, 0);
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (COLOR_HISTORY))
            {
              while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
                {
                  GimpRGB color;

                  if (! gimp_scanner_parse_color (scanner, &color))
                    goto error;

                  color_history_add_color_from_rc (&color);
                }
            }
          else if (scanner->value.v_symbol == GINT_TO_POINTER (LAST_TIP_SHOWN))
            {
              token = G_TOKEN_INT;

              if (! gimp_scanner_parse_int (scanner, &gimprc.last_tip))
                break;
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

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

 error:

  if (error)
    {
      g_message (error->message);
      g_clear_error (&error);
    }

  gimp_scanner_destroy (scanner);
}

void
session_restore (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_dialog_factories_session_restore ();
}

void
session_save (Gimp *gimp)
{
  gchar *filename;
  FILE  *fp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("sessionrc");

  fp = fopen (filename, "wt");
  g_free (filename);
  if (!fp)
    return;

  fprintf (fp, ("# GIMP sessionrc\n"
		"# This file takes session-specific info (that is info,\n"
		"# you want to keep between two gimp-sessions). You are\n"
		"# not supposed to edit it manually, but of course you\n"
		"# can do. This file will be entirely rewritten every time\n" 
		"# you quit the gimp. If this file isn't found, defaults\n"
		"# are used.\n\n"));

  gimp_dialog_factories_session_save (fp);

  /* save last tip shown */
  fprintf (fp, "(last-tip-shown %d)\n\n", gimprc.last_tip + 1);

  color_history_write (fp);

  fclose (fp);
}


/*  private functions  */

static GTokenType
session_info_deserialize (GScanner *scanner,
                          Gimp     *gimp)
{
  GimpDialogFactory *factory;
  GimpSessionInfo   *info = NULL;
  GTokenType         token;
  gchar             *factory_name;
  gchar             *entry_name;

  token = G_TOKEN_STRING;

  if (! gimp_scanner_parse_string (scanner, &factory_name))
    goto error;

  factory = gimp_dialog_factory_from_name (factory_name);
  g_free (factory_name);

  if (! factory)
    goto error;

  if (! gimp_scanner_parse_string (scanner, &entry_name))
    goto error;

  info = g_new0 (GimpSessionInfo, 1);

  if (strcmp (entry_name, "dock"))
    {
      info->toplevel_entry = gimp_dialog_factory_find_entry (factory,
                                                             entry_name);
      g_free (entry_name);

      if (! info->toplevel_entry)
	goto error;
    }
  else
    {
      g_free (entry_name);
    }

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
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case SESSION_INFO_POSITION:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->x))
                goto error;
              if (! gimp_scanner_parse_int (scanner, &info->y))
                goto error;
              break;

            case SESSION_INFO_SIZE:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &info->width))
                goto error;
              if (! gimp_scanner_parse_int (scanner, &info->height))
                goto error;
              break;

            case SESSION_INFO_OPEN:
              info->open = TRUE;
              break;

            case SESSION_INFO_AUX:
              if (! gimp_scanner_parse_string_list (scanner, &info->aux_info))
                {
                  token = G_TOKEN_NONE;
                  goto error;
                }
              break;

            case SESSION_INFO_DOCK:
              if (info->toplevel_entry)
                goto error;

              while (g_scanner_peek_next_token (scanner) == G_TOKEN_LEFT_PAREN)
                {
                  GList *list = NULL;

                  if (! gimp_scanner_parse_string_list (scanner, &list))
                    {
                      token = G_TOKEN_NONE;
                      goto error;
                    }

                  info->sub_dialogs = g_list_append (info->sub_dialogs, list);
                }
              break;

            default:
              break;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (g_scanner_peek_next_token (scanner) == token)
        {
          factory->session_infos = g_list_append (factory->session_infos, info);
        }

      return token;
    }

 error:
  if (info)
    {
      GList *list;

      for (list = info->sub_dialogs; list; list = g_list_next (list))
	{
	  g_list_foreach (list->data, (GFunc) g_free, NULL);
	  g_list_free (list->data);
	}

      g_list_free (info->sub_dialogs);
      g_free (info);
    }

  return token;
}
