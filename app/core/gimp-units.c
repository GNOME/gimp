/* LIGMA - The GNU Image Manipulation Program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * ligmaunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@ligma.org>
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

#include "libligmabase/ligmabase.h"
#include "libligmabase/ligmabase-private.h"
#include "libligmaconfig/ligmaconfig.h"

#include "core-types.h"

#include "ligma.h"
#include "ligma-units.h"
#include "ligmaunit.h"

#include "config/ligmaconfig-file.h"

#include "ligma-intl.h"


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType ligma_unitrc_unit_info_deserialize (GScanner *scanner,
                                                     Ligma     *ligma);


static Ligma *the_unit_ligma = NULL;


static gint
ligma_units_get_number_of_units (void)
{
  return _ligma_unit_get_number_of_units (the_unit_ligma);
}

static gint
ligma_units_get_number_of_built_in_units (void)
{
  return LIGMA_UNIT_END;
}

static LigmaUnit
ligma_units_unit_new (gchar   *identifier,
                     gdouble  factor,
                     gint     digits,
                     gchar   *symbol,
                     gchar   *abbreviation,
                     gchar   *singular,
                     gchar   *plural)
{
  return _ligma_unit_new (the_unit_ligma,
                         identifier,
                         factor,
                         digits,
                         symbol,
                         abbreviation,
                         singular,
                         plural);
}

static gboolean
ligma_units_unit_get_deletion_flag (LigmaUnit unit)
{
  return _ligma_unit_get_deletion_flag (the_unit_ligma, unit);
}

static void
ligma_units_unit_set_deletion_flag (LigmaUnit unit,
                                   gboolean deletion_flag)
{
  _ligma_unit_set_deletion_flag (the_unit_ligma, unit, deletion_flag);
}

static gdouble
ligma_units_unit_get_factor (LigmaUnit unit)
{
  return _ligma_unit_get_factor (the_unit_ligma, unit);
}

static gint
ligma_units_unit_get_digits (LigmaUnit unit)
{
  return _ligma_unit_get_digits (the_unit_ligma, unit);
}

static const gchar *
ligma_units_unit_get_identifier (LigmaUnit unit)
{
  return _ligma_unit_get_identifier (the_unit_ligma, unit);
}

static const gchar *
ligma_units_unit_get_symbol (LigmaUnit unit)
{
  return _ligma_unit_get_symbol (the_unit_ligma, unit);
}

static const gchar *
ligma_units_unit_get_abbreviation (LigmaUnit unit)
{
  return _ligma_unit_get_abbreviation (the_unit_ligma, unit);
}

static const gchar *
ligma_units_unit_get_singular (LigmaUnit unit)
{
  return _ligma_unit_get_singular (the_unit_ligma, unit);
}

static const gchar *
ligma_units_unit_get_plural (LigmaUnit unit)
{
  return _ligma_unit_get_plural (the_unit_ligma, unit);
}

void
ligma_units_init (Ligma *ligma)
{
  LigmaUnitVtable vtable;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));
  g_return_if_fail (the_unit_ligma == NULL);

  the_unit_ligma = ligma;

  vtable.unit_get_number_of_units          = ligma_units_get_number_of_units;
  vtable.unit_get_number_of_built_in_units = ligma_units_get_number_of_built_in_units;
  vtable.unit_new               = ligma_units_unit_new;
  vtable.unit_get_deletion_flag = ligma_units_unit_get_deletion_flag;
  vtable.unit_set_deletion_flag = ligma_units_unit_set_deletion_flag;
  vtable.unit_get_factor        = ligma_units_unit_get_factor;
  vtable.unit_get_digits        = ligma_units_unit_get_digits;
  vtable.unit_get_identifier    = ligma_units_unit_get_identifier;
  vtable.unit_get_symbol        = ligma_units_unit_get_symbol;
  vtable.unit_get_abbreviation  = ligma_units_unit_get_abbreviation;
  vtable.unit_get_singular      = ligma_units_unit_get_singular;
  vtable.unit_get_plural        = ligma_units_unit_get_plural;

  ligma_base_init (&vtable);

  ligma->user_units   = NULL;
  ligma->n_user_units = 0;
}

void
ligma_units_exit (Ligma *ligma)
{
  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  ligma_user_units_free (ligma);
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
ligma_unitrc_load (Ligma *ligma)
{
  GFile      *file;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  file = ligma_directory_file ("unitrc", NULL);

  if (ligma->be_verbose)
    g_print ("Parsing '%s'\n", ligma_file_get_utf8_name (file));

  scanner = ligma_scanner_new_file (file, &error);

  if (! scanner && error->code == LIGMA_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_object_unref (file);

      file = ligma_sysconf_directory_file ("unitrc", NULL);

      scanner = ligma_scanner_new_file (file, NULL);
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
              token = ligma_unitrc_unit_info_deserialize (scanner, ligma);

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

      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      ligma_config_file_backup_on_error (file, "unitrc", NULL);
    }

  ligma_scanner_unref (scanner);
  g_object_unref (file);
}

void
ligma_unitrc_save (Ligma *ligma)
{
  LigmaConfigWriter *writer;
  GFile            *file;
  gint              i;
  GError           *error = NULL;

  g_return_if_fail (LIGMA_IS_LIGMA (ligma));

  file = ligma_directory_file ("unitrc", NULL);

  if (ligma->be_verbose)
    g_print ("Writing '%s'\n", ligma_file_get_utf8_name (file));

  writer =
    ligma_config_writer_new_from_file (file,
                                      TRUE,
                                      "LIGMA units\n\n"
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
  for (i = _ligma_unit_get_number_of_built_in_units (ligma);
       i < _ligma_unit_get_number_of_units (ligma);
       i++)
    {
      if (_ligma_unit_get_deletion_flag (ligma, i) == FALSE)
        {
          gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

          ligma_config_writer_open (writer, "unit-info");
          ligma_config_writer_string (writer,
                                     _ligma_unit_get_identifier (ligma, i));

          ligma_config_writer_open (writer, "factor");
          ligma_config_writer_print (writer,
                                    g_ascii_dtostr (buf, sizeof (buf),
                                                    _ligma_unit_get_factor (ligma, i)),
                                    -1);
          ligma_config_writer_close (writer);

          ligma_config_writer_open (writer, "digits");
          ligma_config_writer_printf (writer,
                                     "%d", _ligma_unit_get_digits (ligma, i));
          ligma_config_writer_close (writer);

          ligma_config_writer_open (writer, "symbol");
          ligma_config_writer_string (writer,
                                     _ligma_unit_get_symbol (ligma, i));
          ligma_config_writer_close (writer);

          ligma_config_writer_open (writer, "abbreviation");
          ligma_config_writer_string (writer,
                                     _ligma_unit_get_abbreviation (ligma, i));
          ligma_config_writer_close (writer);

          ligma_config_writer_open (writer, "singular");
          ligma_config_writer_string (writer,
                                     _ligma_unit_get_singular (ligma, i));
          ligma_config_writer_close (writer);

          ligma_config_writer_open (writer, "plural");
          ligma_config_writer_string (writer,
                                     _ligma_unit_get_plural (ligma, i));
          ligma_config_writer_close (writer);

          ligma_config_writer_close (writer);
        }
    }

  if (! ligma_config_writer_finish (writer, "end of units", &error))
    {
      ligma_message_literal (ligma, NULL, LIGMA_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }
}


/*  private functions  */

static GTokenType
ligma_unitrc_unit_info_deserialize (GScanner *scanner,
                                   Ligma     *ligma)
{
  gchar      *identifier   = NULL;
  gdouble     factor       = 1.0;
  gint        digits       = 2.0;
  gchar      *symbol       = NULL;
  gchar      *abbreviation = NULL;
  gchar      *singular     = NULL;
  gchar      *plural       = NULL;
  GTokenType  token;

  if (! ligma_scanner_parse_string (scanner, &identifier))
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
              if (! ligma_scanner_parse_float (scanner, &factor))
                goto cleanup;
              break;

            case UNIT_DIGITS:
              token = G_TOKEN_INT;
              if (! ligma_scanner_parse_int (scanner, &digits))
                goto cleanup;
              break;

            case UNIT_SYMBOL:
              token = G_TOKEN_STRING;
              if (! ligma_scanner_parse_string (scanner, &symbol))
                goto cleanup;
              break;

            case UNIT_ABBREV:
              token = G_TOKEN_STRING;
              if (! ligma_scanner_parse_string (scanner, &abbreviation))
                goto cleanup;
              break;

            case UNIT_SINGULAR:
              token = G_TOKEN_STRING;
              if (! ligma_scanner_parse_string (scanner, &singular))
                goto cleanup;
              break;

            case UNIT_PLURAL:
              token = G_TOKEN_STRING;
              if (! ligma_scanner_parse_string (scanner, &plural))
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
          LigmaUnit unit = _ligma_unit_new (ligma,
                                          identifier, factor, digits,
                                          symbol, abbreviation,
                                          singular, plural);

          /*  make the unit definition persistent  */
          _ligma_unit_set_deletion_flag (ligma, unit, FALSE);
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
