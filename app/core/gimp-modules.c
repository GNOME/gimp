/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ligmamodules.c
 * (C) 1999 Austin Donnelly <austin@ligma.org>
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

#include <gio/gio.h>

#include "libligmabase/ligmabase.h"
#include "libligmamodule/ligmamodule.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "config/ligmacoreconfig.h"

#include "ligma.h"
#include "ligma-modules.h"

#include "ligma-intl.h"


void
ligma_modules_init (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (! ligma->no_interface)
    {
      ligma->module_db = ligma_module_db_new (ligma->be_verbose);
      ligma->write_modulerc = FALSE;
    }
}

void
ligma_modules_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  g_clear_object (&ligma->module_db);
}

void
ligma_modules_load (Ligma *ligma)
{
  GFile    *file;
  GScanner *scanner;
  gchar    *module_load_inhibit = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (ligma->no_interface)
    return;

  ligma_module_db_set_verbose (ligma->module_db, ligma->be_verbose);

  file = ligma_directory_file ("modulerc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  scanner = ligma_scanner_new_file (file, NULL);
  g_object_unref (file);

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

                  if (! ligma_scanner_parse_string_no_validate (scanner,
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
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          g_clear_error (&error);
        }

      ligma_scanner_unref (scanner);
    }

  if (module_load_inhibit)
    {
      ligma_module_db_set_load_inhibit (ligma->module_db, module_load_inhibit);
      g_free (module_load_inhibit);
    }

  ligma_module_db_load (ligma->module_db, ligma->config->module_path);
}

static void
add_to_inhibit_string (gpointer data,
                       gpointer user_data)
{
  LigmaModule *module = data;
  GString    *str    = user_data;

  if (! ligma_module_get_auto_load (module))
    {
      GFile *file = ligma_module_get_file (module);
      gchar *path = g_file_get_path (file);

      g_string_append_c (str, G_SEARCHPATH_SEPARATOR);
      g_string_append (str, path);

      g_free (path);
    }
}

void
ligma_modules_unload (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (! ligma->no_interface && ligma->write_modulerc)
    {
      LigmaConfigWriter *writer;
      GString          *str;
      const gchar      *p;
      GFile            *file;
      GError           *error = NULL;

      str = g_string_new (NULL);
      g_list_foreach (ligma_module_db_get_modules (ligma->module_db),
                      add_to_inhibit_string, str);
      if (str->len > 0)
        p = str->str + 1;
      else
        p = "";

      file = ligma_directory_file ("modulerc", NULL);

      if (ligma->be_verbose)
        g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

      writer = ligma_config_writer_new_from_file (file,
                                                 TRUE,
                                                 "LIGMA modulerc",
                                                 &error);
      g_object_unref (file);

      if (writer)
        {
          ligma_config_writer_open (writer, "module-load-inhibit");
          ligma_config_writer_string (writer, p);
          ligma_config_writer_close (writer);

          ligma_config_writer_finish (writer, "end of modulerc", &error);

          ligma->write_modulerc = FALSE;
        }

      g_string_free (str, TRUE);

      if (error)
        {
          ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
          g_clear_error (&error);
        }
    }
}

void
ligma_modules_refresh (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  if (! ligma->no_interface)
    {
      ligma_module_db_refresh (ligma->module_db, ligma->config->module_path);
    }
}
