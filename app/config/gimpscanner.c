/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpscanner.c
 * Copyright (C) 2002  Sven Neumann <sven@gimp.org>
 *                     Michael Natterer <mitch@gimp.org>
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

#include <fcntl.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/types.h>

#include <glib-object.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include "gimpscanner.h"

#include "libgimp/gimpintl.h"


GScanner *
gimp_scanner_new (const gchar *filename)
{
  gint        fd;
  GScanner   *scanner;

  g_return_val_if_fail (filename != NULL, NULL);

  fd = open (filename, O_RDONLY);

  if (fd == -1)
    return NULL;

  scanner = g_scanner_new (NULL);

  scanner->config->cset_identifier_first = ( G_CSET_a_2_z );
  scanner->config->cset_identifier_nth   = ( G_CSET_a_2_z "-_" );

  g_scanner_input_file (scanner, fd);
  scanner->input_name = g_strdup (filename);

  return scanner;
}

void
gimp_scanner_destroy (GScanner *scanner)
{
  g_return_if_fail (scanner != NULL);

  close (scanner->input_fd);
  g_free ((gchar *) scanner->input_name);
  g_scanner_destroy (scanner);
}


gboolean
gimp_scanner_parse_token (GScanner   *scanner,
                          GTokenType  token)
{
  if (g_scanner_peek_next_token (scanner) != token)
    return FALSE;

  g_scanner_get_next_token (scanner);

  return TRUE;
}

gboolean
gimp_scanner_parse_identifier (GScanner    *scanner,
                               const gchar *identifier)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_IDENTIFIER)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (strcmp (scanner->value.v_identifier, identifier))
    return FALSE;

  return TRUE;
}

gboolean
gimp_scanner_parse_string (GScanner  *scanner,
                           gchar    **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*scanner->value.v_string)
    {
      if (! g_utf8_validate (scanner->value.v_string, -1, NULL))
        {
          g_scanner_warn (scanner, _("invalid UTF-8 string"));
          return FALSE;
        }
      
      *dest = g_strdup (scanner->value.v_string);
    }

  return TRUE;
}

gboolean
gimp_scanner_parse_string_no_validate (GScanner  *scanner,
                                       gchar    **dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_STRING)
    return FALSE;

  g_scanner_get_next_token (scanner);

  if (*scanner->value.v_string)
    *dest = g_strdup (scanner->value.v_string);

  return TRUE;
}

gboolean
gimp_scanner_parse_int (GScanner *scanner,
                        gint     *dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_INT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  *dest = scanner->value.v_int;

  return TRUE;
}

gboolean
gimp_scanner_parse_float (GScanner *scanner,
                          gdouble  *dest)
{
  if (g_scanner_peek_next_token (scanner) != G_TOKEN_FLOAT)
    return FALSE;

  g_scanner_get_next_token (scanner);

  *dest = scanner->value.v_float;

  return TRUE;
}
