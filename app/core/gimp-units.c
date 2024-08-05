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

static GTokenType   gimp_unitrc_unit_info_deserialize (GScanner *scanner,
                                                       Gimp     *gimp);

static GimpUnit   * gimp_units_get_user_unit          (gint      unit_id);


static Gimp *the_unit_gimp = NULL;


void
gimp_units_init (Gimp *gimp)
{
  GimpUnitVtable vtable = { 0 };

  g_return_if_fail (GIMP_IS_GIMP (gimp));
  g_return_if_fail (the_unit_gimp == NULL);

  the_unit_gimp = gimp;

  vtable.get_user_unit = gimp_units_get_user_unit;

  gimp_base_init (&vtable);

  gimp->user_units = NULL;
}

void
gimp_units_exit (Gimp *gimp)
{
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp_base_exit ();

  g_list_free_full (gimp->user_units, g_object_unref);
  gimp->user_units = NULL;
}


/*  unitrc functions  **********/

enum
{
  UNIT_INFO = 1,
  UNIT_FACTOR,
  UNIT_DIGITS,
  UNIT_SYMBOL,
  UNIT_ABBREV,
  UNIT_SINGULAR, /* Obsoleted in GIMP 3.0. */
  UNIT_PLURAL    /* Obsoleted in GIMP 3.0. */
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

  scanner = gimp_scanner_new_file (file, &error);

  if (! scanner && error->code == GIMP_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_object_unref (file);

      file = gimp_sysconf_directory_file ("unitrc", NULL);

      scanner = gimp_scanner_new_file (file, NULL);
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

  gimp_scanner_unref (scanner);
  g_object_unref (file);
}

void
gimp_unitrc_save (Gimp *gimp)
{
  GimpConfigWriter *writer;
  GFile            *file;
  GList            *iter;
  GError           *error = NULL;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  file = gimp_directory_file ("unitrc", NULL);

  if (gimp->be_verbose)
    g_print ("Writing '%s'\n", gimp_file_get_utf8_name (file));

  writer =
    gimp_config_writer_new_from_file (file,
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

  for (iter = gimp->user_units; iter; iter = iter->next)
    {
      GimpUnit *unit = iter->data;

      if (gimp_unit_get_deletion_flag (unit) == FALSE)
        {
          gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

          gimp_config_writer_open (writer, "unit-info");
          gimp_config_writer_string (writer,
                                     gimp_unit_get_name (unit));

          gimp_config_writer_open (writer, "factor");
          gimp_config_writer_print (writer,
                                    g_ascii_dtostr (buf, sizeof (buf),
                                                    gimp_unit_get_factor (unit)),
                                    -1);
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "digits");
          gimp_config_writer_printf (writer,
                                     "%d", gimp_unit_get_digits (unit));
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "symbol");
          gimp_config_writer_string (writer,
                                     gimp_unit_get_symbol (unit));
          gimp_config_writer_close (writer);

          gimp_config_writer_open (writer, "abbreviation");
          gimp_config_writer_string (writer,
                                     gimp_unit_get_abbreviation (unit));
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
  gchar      *name         = NULL;
  gdouble     factor       = 1.0;
  gint        digits       = 2.0;
  gchar      *symbol       = NULL;
  gchar      *abbreviation = NULL;
  gchar      *singular     = NULL;
  gchar      *plural       = NULL;
  GTokenType  token;

  if (! gimp_scanner_parse_string (scanner, &name))
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
              /* UNIT_SINGULAR and UNIT_PLURAL are deprecated. We still
               * support reading them if they exist to stay backward
               * compatible with older unitrc, in which case, we use
               * UNIT_PLURAL on behalf of the name.
               */
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
          GimpUnit *unit = _gimp_unit_new (gimp,
                                           plural && strlen (plural) > 0 ? plural : name,
                                           factor, digits, symbol, abbreviation);

          /*  make the unit definition persistent  */
          gimp_unit_set_deletion_flag (unit, FALSE);
        }
    }

 cleanup:

  g_free (name);
  g_free (symbol);
  g_free (abbreviation);
  g_free (singular);
  g_free (plural);

  return token;
}

/**
 * gimp_units_get_user_unit:
 * @unit_id:
 *
 * This function will return the user-created GimpUnit with ID @unit_id.
 */
static GimpUnit *
gimp_units_get_user_unit (gint unit_id)
{
  g_return_val_if_fail (the_unit_gimp != NULL, NULL);
  g_return_val_if_fail (unit_id >= GIMP_UNIT_END && unit_id != GIMP_UNIT_PERCENT, NULL);

  unit_id -= GIMP_UNIT_END;

  return g_list_nth_data (the_unit_gimp->user_units, unit_id);
}
