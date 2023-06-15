/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2001-2002  Sven Neumann <sven@gimp.org>
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <cairo.h>
#include <gegl.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "gimpconfigtypes.h"

#include "gimpconfig-utils.h"
#include "gimpconfig-array.h"
#include "gimpscanner.h"


/* GStrv i.e. char** i.e. null terminated array of strings. */

/**
 * gimp_config_serialize_strv:
 * @value: source #GValue holding a #GStrv
 * @str:   destination string
 *
 * Appends a string repr of the #GStrv value of #GValue to @str.
 * Repr is an integer literal greater than or equal to zero,
 * followed by a possibly empty sequence
 * of quoted and escaped string literals.
 *
 * Returns: %TRUE always
 *
 * Since: 3.0
 **/
gboolean
gimp_config_serialize_strv (const GValue *value,
                            GString      *str)
{
  GStrv gstrv;

  gstrv = g_value_get_boxed (value);

  if (gstrv)
    {
      gint length = g_strv_length (gstrv);

      /* Write length */
      g_string_append_printf (str, "%d", length);

      for (gint i = 0; i < length; i++)
        {
          g_string_append (str, " "); /* separator */
          gimp_config_string_append_escaped (str, gstrv[i]);
        }
    }
  else
    {
      /* GValue has NULL value. Not quite the same as an empty GStrv.
       * But handle it quietly as an empty GStrv: write a length of zero.
       */
      g_string_append (str, "0");
    }

  return TRUE;
}

/**
 * gimp_config_deserialize_strv:
 * @value:   destination #GValue to hold a #GStrv
 * @scanner: #GScanner positioned in serialization stream
 *
 * Sets @value to new #GStrv.
 * Scans i.e. consumes serialization to fill the GStrv.
 *
 * Requires @value to be initialized to hold type #G_TYPE_BOXED.
 *
 * Returns:
 * G_TOKEN_RIGHT_PAREN on success.
 * G_TOKEN_INT on failure to scan length.
 * G_TOKEN_STRING on failure to scan enough quoted strings.
 *
 * On failure, the value in @value is not touched and could be NULL.
 *
 * Since: 3.0
 **/
GTokenType
gimp_config_deserialize_strv (GValue     *value,
                              GScanner   *scanner)
{
  gint          n_values;
  GTokenType    result_token = G_TOKEN_RIGHT_PAREN;
  GStrvBuilder *builder;

  /* Scan length of array. */
  if (! gimp_scanner_parse_int (scanner, &n_values))
    return G_TOKEN_INT;

  builder = g_strv_builder_new ();

  for (gint i = 0; i < n_values; i++)
    {
      gchar *scanned_string;

      if (! gimp_scanner_parse_string (scanner, &scanned_string))
        {
          /* Error, missing a string. */
          result_token = G_TOKEN_STRING;
          break;
        }

      /* Not an error for scanned string to be empty.*/

      /* Adding string to builder DOES not transfer ownership,
       * the builder will copy the string.
       */
      g_strv_builder_add (builder, scanned_string);

      g_free (scanned_string);
    }

  /* assert result_token is G_TOKEN_RIGHT_PAREN OR G_TOKEN_STRING */
  if (result_token == G_TOKEN_RIGHT_PAREN)
    {
      GStrv   gstrv;

      /* Allocate new GStrv. */
      gstrv = g_strv_builder_end (builder);
      /* Transfer ownership of the array and all strings it points to. */
      g_value_take_boxed (value, gstrv);
    }
  else
    {
      /* No GStrv to unref. */
      g_scanner_error (scanner, "Missing string.");
    }

  g_strv_builder_unref (builder);

  return result_token;
}