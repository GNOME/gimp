/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpmodules.c
 * (C) 1999 Austin Donnelly <austin@gimp.org>
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

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpmodule/gimpmodule.h"
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "config/gimpcoreconfig.h"

#include "gimp.h"
#include "gimp-modules.h"

#include "gimp-intl.h"


void
gimp_modules_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! gimp->no_interface)
    {
      gimp->module_db = gimp_module_db_new (gimp->be_verbose);
      gimp->write_modulerc = FALSE;
    }
}

void
gimp_modules_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->module_db)
    {
      g_object_unref (gimp->module_db);
      gimp->module_db = NULL;
    }
}

void
gimp_modules_load (Gimp *gimp)
{
  gchar    *filename;
  gchar    *path;
  GScanner *scanner;
  gchar    *module_load_inhibit = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->no_interface)
    return;

  filename = gimp_personal_rc_file ("modulerc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  scanner = gimp_scanner_new_file (filename, NULL);
  g_free (filename);

  if (scanner)
    {
      GTokenType  token;
      GError     *error = NULL;

#define MODULE_LOAD_INHIBIT 1

      g_scanner_scope_add_symbol (scanner, 0, "module-load-inhibit",
                                  GINT_TO_POINTER (MODULE_LOAD_INHIBIT));

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
              if (scanner->value.v_symbol == GINT_TO_POINTER (MODULE_LOAD_INHIBIT))
                {
                  token = G_TOKEN_STRING;

                  if (! gimp_scanner_parse_string_no_validate (scanner,
                                                               &module_load_inhibit))
                    goto error;
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

#undef MODULE_LOAD_INHIBIT

      if (token != G_TOKEN_LEFT_PAREN)
        {
          g_scanner_get_next_token (scanner);
          g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                                 _("fatal parse error"), TRUE);
        }

    error:

      if (error)
        {
          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
          g_clear_error (&error);
        }

      gimp_scanner_destroy (scanner);
    }

  if (module_load_inhibit)
    {
      gimp_module_db_set_load_inhibit (gimp->module_db, module_load_inhibit);
      g_free (module_load_inhibit);
    }

  path = gimp_config_path_expand (gimp->config->module_path, TRUE, NULL);
  gimp_module_db_load (gimp->module_db, path);
  g_free (path);
}

static void
add_to_inhibit_string (gpointer data,
                       gpointer user_data)
{
  GimpModule *module = data;
  GString    *str    = user_data;

  if (module->load_inhibit)
    {
      str = g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
      str = g_string_append (str, module->filename);
    }
}

void
gimp_modules_unload (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! gimp->no_interface && gimp->write_modulerc)
    {
      GimpConfigWriter *writer;
      GString          *str;
      gchar            *p;
      gchar            *filename;
      GError           *error = NULL;

      str = g_string_new (NULL);
      g_list_foreach (gimp->module_db->modules, add_to_inhibit_string, str);
      if (str->len > 0)
        p = str->str + 1;
      else
        p = "";

      filename = gimp_personal_rc_file ("modulerc");

      if (gimp->be_verbose)
        g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

      writer = gimp_config_writer_new_file (filename, TRUE,
                                            "GIMP modulerc", &error);
      g_free (filename);

      if (writer)
        {
          gimp_config_writer_open (writer, "module-load-inhibit");
          gimp_config_writer_printf (writer, "\"%s\"", p);
          gimp_config_writer_close (writer);

          gimp_config_writer_finish (writer, "end of modulerc", &error);

          gimp->write_modulerc = FALSE;
        }

      g_string_free (str, TRUE);

      if (error)
        {
          gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
          g_clear_error (&error);
        }
    }
}

void
gimp_modules_refresh (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (! gimp->no_interface)
    {
      gchar *path;

      path = gimp_config_path_expand (gimp->config->module_path, TRUE, NULL);
      gimp_module_db_refresh (gimp->module_db, path);
      g_free (path);
    }
}
