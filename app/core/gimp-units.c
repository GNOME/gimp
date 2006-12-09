/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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
#include "libgimpconfig/gimpconfig.h"

#include "core-types.h"

#include "gimp.h"
#include "gimp-units.h"
#include "gimpunit.h"

#include "config/gimpconfig-file.h"

#include "gimp-intl.h"


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType gimp_unitrc_unit_info_deserialize (GScanner *scanner,
                                                     Gimp     *gimp);


void
gimp_units_init (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->user_units   = NULL;
  gimp->n_user_units = 0;
}

void
gimp_units_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_user_units_free (gimp);
}


/*  unitrc functions  **********/

enum
{
  UNIT_INFO = 1,
  UNIT_FACTOR,
  UNIT_DIGITS,
  UNIT_SYMBOL,
  UNIT_ABBREV,
  UNIT_SINGULAR,
  UNIT_PLURAL
};

void
gimp_unitrc_load (Gimp *gimp)
{
  gchar      *filename;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("unitrc");

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_filename_to_utf8 (filename));

  scanner = gimp_scanner_new_file (filename, &error);

  if (! scanner && error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_free (filename);

      filename = g_build_filename (gimp_sysconf_directory (), "unitrc", NULL);
      scanner = gimp_scanner_new_file (filename, NULL);
    }

  if (! scanner)
    {
      g_clear_error (&error);
      g_free (filename);
      return;
    }

  g_scanner_scope_add_symbol (scanner, 0,
                              "unit-info", GINT_TO_POINTER (UNIT_INFO));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "factor", GINT_TO_POINTER (UNIT_FACTOR));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "digits", GINT_TO_POINTER (UNIT_DIGITS));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "symbol", GINT_TO_POINTER (UNIT_SYMBOL));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "abbreviation", GINT_TO_POINTER (UNIT_ABBREV));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "singular", GINT_TO_POINTER (UNIT_SINGULAR));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "plural", GINT_TO_POINTER (UNIT_PLURAL));

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
          if (scanner->value.v_symbol == GINT_TO_POINTER (UNIT_INFO))
            {
              g_scanner_set_scope (scanner, UNIT_INFO);
              token = gimp_unitrc_unit_info_deserialize (scanner, gimp);

              if (token == G_TOKEN_RIGHT_PAREN)
                g_scanner_set_scope (scanner, 0);
            }
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

      gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
      g_clear_error (&error);

      gimp_config_file_backup_on_error (filename, "unitrc", NULL);
    }

  gimp_scanner_destroy (scanner);
  g_free (filename);
}

void
gimp_unitrc_save (Gimp *gimp)
{
  GimpConfigWriter *writer;
  gchar            *filename;
  gint              i;
  GError           *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("unitrc");

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_filename_to_utf8 (filename));

  writer =
    gimp_config_writer_new_file (filename,
                                 TRUE,
                                 "GIMP units\n\n"
                                 "This file contains the user unit database. "
                                 "You can edit this list with the unit "
                                 "editor. You are not supposed to edit it "
                                 "manually, but of course you can do.\n"
                                 "This file will be entirely rewritten every "
                                 "time you quit the gimp.",
                                 NULL);

  g_free (filename);

  if (!writer)
    return;

  /*  save user defined units  */
  for (i = _gimp_unit_get_number_of_built_in_units (gimp);
       i < _gimp_unit_get_number_of_units (gimp);
       i++)
    {
      if (_gimp_unit_get_deletion_flag (gimp, i) == FALSE)
        {
          gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

          gimp_config_writer_open (writer, "unit-info");
          gimp_config_writer_string (writer,
                                     _gimp_unit_get_identifier (gimp, i));

          gimp_config_writer_open (writer, "factor");
          gimp_config_writer_print (writer,
                                    g_ascii_formatd (buf, sizeof (buf), "%f",
                                                     _gimp_unit_get_factor (gimp, i)),
                                    -1);
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "digits");
          gimp_config_writer_printf (writer,
                                     "%d", _gimp_unit_get_digits (gimp, i));
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "symbol");
          gimp_config_writer_string (writer,
                                     _gimp_unit_get_symbol (gimp, i));
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "abbreviation");
          gimp_config_writer_string (writer,
                                     _gimp_unit_get_abbreviation (gimp, i));
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "singular");
          gimp_config_writer_string (writer,
                                     _gimp_unit_get_singular (gimp, i));
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "plural");
          gimp_config_writer_string (writer,
                                     _gimp_unit_get_plural (gimp, i));
          gimp_config_writer_close (writer);

          gimp_config_writer_close (writer);
        }
    }

  if (! gimp_config_writer_finish (writer, "end of units", &error))
    {
      gimp_message (gimp, NULL, GIMP_MESSAGE_ERROR, "%s", error->message);
      g_clear_error (&error);
    }
}


/*  private functions  */

static GTokenType
gimp_unitrc_unit_info_deserialize (GScanner *scanner,
                                   Gimp     *gimp)
{
  gchar      *identifier   = NULL;
  gdouble     factor       = 1.0;
  gint        digits       = 2.0;
  gchar      *symbol       = NULL;
  gchar      *abbreviation = NULL;
  gchar      *singular     = NULL;
  gchar      *plural       = NULL;
  GTokenType  token;

  if (! gimp_scanner_parse_string (scanner, &identifier))
    return G_TOKEN_STRING;

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
            case UNIT_FACTOR:
              token = G_TOKEN_FLOAT;
              if (! gimp_scanner_parse_float (scanner, &factor))
                goto cleanup;
              break;

            case UNIT_DIGITS:
              token = G_TOKEN_INT;
              if (! gimp_scanner_parse_int (scanner, &digits))
                goto cleanup;
              break;

            case UNIT_SYMBOL:
              token = G_TOKEN_STRING;
              if (! gimp_scanner_parse_string (scanner, &symbol))
                goto cleanup;
              break;

            case UNIT_ABBREV:
              token = G_TOKEN_STRING;
              if (! gimp_scanner_parse_string (scanner, &abbreviation))
                goto cleanup;
              break;

            case UNIT_SINGULAR:
              token = G_TOKEN_STRING;
              if (! gimp_scanner_parse_string (scanner, &singular))
                goto cleanup;
              break;

            case UNIT_PLURAL:
              token = G_TOKEN_STRING;
              if (! gimp_scanner_parse_string (scanner, &plural))
                goto cleanup;
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
          GimpUnit unit = _gimp_unit_new (gimp,
                                          identifier, factor, digits,
                                          symbol, abbreviation,
                                          singular, plural);

          /*  make the unit definition persistent  */
          _gimp_unit_set_deletion_flag (gimp, unit, FALSE);
        }
    }

 cleanup:

  g_free (identifier);
  g_free (symbol);
  g_free (abbreviation);
  g_free (singular);
  g_free (plural);

  return token;
}
