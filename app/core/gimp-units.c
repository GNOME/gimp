/* The GIMP -- an image manipulation program
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

#include <stdio.h>

#include <glib-object.h>

#include "libgimpbase/gimpbase.h"

#include "core-types.h"

#include "gimp.h"
#include "gimpunit.h"
#include "gimpunits.h"

#include "config/gimpscanner.h"

#include "libgimp/gimpintl.h"


/*  
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType gimp_unitrc_unit_info_deserialize (GScanner *scanner,
                                                     Gimp     *gimp);


void
gimp_units_init (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  gimp->user_units   = NULL;
  gimp->n_user_units = 0;
}

void
gimp_units_exit (Gimp *gimp)
{
  g_return_if_fail (gimp != NULL);
  g_return_if_fail (GIMP_IS_GIMP (gimp));

  if (gimp->user_units)
    {
      g_list_foreach (gimp->user_units, (GFunc) g_free, NULL);
      g_list_free (gimp->user_units);

      gimp->user_units   = NULL;
      gimp->n_user_units = 0;
    }
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

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("unitrc");
  scanner = gimp_scanner_new (filename);
  g_free (filename);

  if (! scanner)
    return;

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

  do
    {
      if (g_scanner_peek_next_token (scanner) != token)
        break;

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
  while (token != G_TOKEN_EOF);

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);
    }

  gimp_scanner_destroy (scanner);
}

void
gimp_unitrc_save (Gimp *gimp)
{
  gint   i;
  gchar *filename;
  FILE  *fp;

  g_return_if_fail (GIMP_IS_GIMP (gimp));

  filename = gimp_personal_rc_file ("unitrc");

  fp = fopen (filename, "w");
  g_free (filename);
  if (!fp)
    return;

  fprintf (fp,
	   "# GIMP unitrc\n" 
	   "# This file contains your user unit database. You can\n"
	   "# modify this list with the unit editor. You are not\n"
	   "# supposed to edit it manually, but of course you can do.\n"
	   "# This file will be entirely rewritten every time you\n"
	   "# quit the gimp.\n\n");

  /*  save user defined units  */
  for (i = gimp_unit_get_number_of_built_in_units ();
       i < gimp_unit_get_number_of_units ();
       i++)
    {
      if (gimp_unit_get_deletion_flag (i) == FALSE)
        {
          gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

          fprintf (fp,
                   "(unit-info \"%s\"\n"
                   "   (factor %s)\n"
                   "   (digits %d)\n"
                   "   (symbol \"%s\")\n"
                   "   (abbreviation \"%s\")\n"
                   "   (singular \"%s\")\n"
                   "   (plural \"%s\"))\n\n",
                   gimp_unit_get_identifier (i),
                   g_ascii_formatd (buf, sizeof (buf), "%f",
                                    gimp_unit_get_factor (i)),
                   gimp_unit_get_digits (i),
                   gimp_unit_get_symbol (i),
                   gimp_unit_get_abbreviation (i),
                   gimp_unit_get_singular (i),
                   gimp_unit_get_plural (i));
        }
    }

  fclose (fp);
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
  GimpUnit    unit;

  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return G_TOKEN_STRING;

  if (! gimp_scanner_parse_string (scanner, &identifier))
    return G_TOKEN_STRING;

  token = G_TOKEN_LEFT_PAREN;

  do
    {
      if (g_scanner_peek_next_token (scanner) != token)
        break;

      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          token = G_TOKEN_RIGHT_PAREN;
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case UNIT_FACTOR:
              if (! gimp_scanner_parse_float (scanner, &factor))
                return G_TOKEN_FLOAT;
              break;

            case UNIT_DIGITS:
              if (! gimp_scanner_parse_int (scanner, &digits))
                return G_TOKEN_INT;
              break;

            case UNIT_SYMBOL:
              if (! gimp_scanner_parse_string (scanner, &symbol))
                return G_TOKEN_STRING;
              break;

            case UNIT_ABBREV:
              if (! gimp_scanner_parse_string (scanner, &abbreviation))
                return G_TOKEN_STRING;
              break;

            case UNIT_SINGULAR:
              if (! gimp_scanner_parse_string (scanner, &singular))
                return G_TOKEN_STRING;
              break;

            case UNIT_PLURAL:
              if (! gimp_scanner_parse_string (scanner, &plural))
                return G_TOKEN_STRING;
             break;

            default:
              break;
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }
  while (token != G_TOKEN_EOF);

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (gimp_scanner_parse_token (scanner, token))
        {
          unit = _gimp_unit_new (gimp, identifier, factor, digits,
                                 symbol, abbreviation, singular, plural);

          /*  make the unit definition persistent  */
          gimp_unit_set_deletion_flag (unit, FALSE);

          g_free (identifier);
          g_free (symbol);
          g_free (abbreviation);
          g_free (singular);
          g_free (plural);

          return G_TOKEN_LEFT_PAREN;
        }
    }

  g_free (identifier);
  g_free (symbol);
  g_free (abbreviation);
  g_free (singular);
  g_free (plural);

  return token;
}
