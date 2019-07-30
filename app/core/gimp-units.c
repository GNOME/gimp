/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * gimpunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

/* This file contains functions to load & save the file containing the
 * user-defined size units, when the application starts/finished.
 */

#include "config.h"

#include <gio/gio.h>

#include "libgimpbase/gimpbase.h"
#include "libgimpbase/gimpbase-private.h"
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


static Gimp *the_unit_gimp = NULL;


static gint
gimp_units_get_number_of_units (void)
{
  return _gimp_unit_get_number_of_units (the_unit_gimp);
}

static gint
gimp_units_get_number_of_built_in_units (void)
{
  return GIMP_UNIT_END;
}

static GimpUnit
gimp_units_unit_new (gchar   *identifier,
                     gdouble  factor,
                     gint     digits,
                     gchar   *symbol,
                     gchar   *abbreviation,
                     gchar   *singular,
                     gchar   *plural)
{
  return _gimp_unit_new (the_unit_gimp,
                         identifier,
                         factor,
                         digits,
                         symbol,
                         abbreviation,
                         singular,
                         plural);
}

static gboolean
gimp_units_unit_get_deletion_flag (GimpUnit unit)
{
  return _gimp_unit_get_deletion_flag (the_unit_gimp, unit);
}

static void
gimp_units_unit_set_deletion_flag (GimpUnit unit,
                                   gboolean deletion_flag)
{
  _gimp_unit_set_deletion_flag (the_unit_gimp, unit, deletion_flag);
}

static gdouble
gimp_units_unit_get_factor (GimpUnit unit)
{
  return _gimp_unit_get_factor (the_unit_gimp, unit);
}

static gint
gimp_units_unit_get_digits (GimpUnit unit)
{
  return _gimp_unit_get_digits (the_unit_gimp, unit);
}

static const gchar *
gimp_units_unit_get_identifier (GimpUnit unit)
{
  return _gimp_unit_get_identifier (the_unit_gimp, unit);
}

static const gchar *
gimp_units_unit_get_symbol (GimpUnit unit)
{
  return _gimp_unit_get_symbol (the_unit_gimp, unit);
}

static const gchar *
gimp_units_unit_get_abbreviation (GimpUnit unit)
{
  return _gimp_unit_get_abbreviation (the_unit_gimp, unit);
}

static const gchar *
gimp_units_unit_get_singular (GimpUnit unit)
{
  return _gimp_unit_get_singular (the_unit_gimp, unit);
}

static const gchar *
gimp_units_unit_get_plural (GimpUnit unit)
{
  return _gimp_unit_get_plural (the_unit_gimp, unit);
}

void
gimp_units_init (Gimp *gimp)
{
  GimpUnitVtable vtable;

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (the_unit_gimp == NULL);

  the_unit_gimp = gimp;

  vtable.unit_get_number_of_units          = gimp_units_get_number_of_units;
  vtable.unit_get_number_of_built_in_units = gimp_units_get_number_of_built_in_units;
  vtable.unit_new               = gimp_units_unit_new;
  vtable.unit_get_deletion_flag = gimp_units_unit_get_deletion_flag;
  vtable.unit_set_deletion_flag = gimp_units_unit_set_deletion_flag;
  vtable.unit_get_factor        = gimp_units_unit_get_factor;
  vtable.unit_get_digits        = gimp_units_unit_get_digits;
  vtable.unit_get_identifier    = gimp_units_unit_get_identifier;
  vtable.unit_get_symbol        = gimp_units_unit_get_symbol;
  vtable.unit_get_abbreviation  = gimp_units_unit_get_abbreviation;
  vtable.unit_get_singular      = gimp_units_unit_get_singular;
  vtable.unit_get_plural        = gimp_units_unit_get_plural;

  gimp_base_init (&vtable);

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
  GFile      *file;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = gimp_directory_file ("unitrc", NULL);

  if (gimp->be_verbose)
    g_print ("Parsing '%s'\n", gimp_file_get_utf8_name (file));

  scanner = gimp_scanner_new_gfile (file, &error);

  if (! scanner && error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_object_unref (file);

      file = gimp_sysconf_directory_file ("unitrc", NULL);

      scanner = gimp_scanner_new_gfile (file, NULL);
    }

  if (! scanner)
    {
      g_clear_error (&error);
      g_object_unref (file);
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

      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      gimp_config_file_backup_on_error (file, "unitrc", NULL);
    }

  gimp_scanner_destroy (scanner);
  g_object_unref (file);
}

void
gimp_unitrc_save (Gimp *gimp)
{
  GimpConfigWriter *writer;
  GFile            *file;
  gint              i;
  GError           *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = gimp_directory_file ("unitrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  writer =
    gimp_config_writer_new_gfile (file,
                                  TRUE,
                                  "GIMP units\n\n"
                                  "This file contains the user unit database. "
                                  "You can edit this list with the unit "
                                  "editor. You are not supposed to edit it "
                                  "manually, but of course you can do.\n"
                                  "This file will be entirely rewritten each "
                                  "time you exit.",
                                  NULL);

  g_object_unref (file);

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
                                    g_ascii_dtostr (buf, sizeof (buf),
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
      gimp_message_literal (gimp, NULL, GIMP_MESSAGE_ERROR, error->message);
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
