/* LIBGIMP - The GIMP Library
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
 *
 * gimputils.c
 *
 * Copyright (C) 2003  Sven Neumann <sven@gimp.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>

#include <glib.h>

#include "gimputils.h"


/**
 * gimp_utf8_strtrim:
 * @str: an UTF-8 encoded string (or %NULL)
 * @max_chars: the maximum number of characters before the string get
 * trimmed
 *
 * Creates a (possibly trimmed) copy of @str. The string is cut if it
 * exceeds @max_chars characters or on the first newline. The fact
 * that the string was trimmed is indicated by appending an ellipsis.
 * 
 * Returns: A (possibly trimmed) copy of @str which should be freed
 * using g_free() when it is not needed any longer.
 **/
gchar *
gimp_utf8_strtrim (const gchar *str,
		   gint         max_chars)
{
  /* FIXME: should we make this translatable? */
  static const gchar *ellipsis = "...";
  static const gint   e_len    = 3;

  if (str)
    {
      const gchar *p;
      const gchar *newline = NULL;
      gint         chars   = 0;
      gunichar     unichar;

      for (p = str; *p; p = g_utf8_next_char (p))
        {
          if (++chars > max_chars)
            break;

          unichar = g_utf8_get_char (p);

          switch (g_unichar_break_type (unichar))
            {
            case G_UNICODE_BREAK_MANDATORY:
            case G_UNICODE_BREAK_LINE_FEED:
              newline = p;
              break;
            default:
              continue;
            }

	  break;
        }

      if (*p)
        {
          gsize  len     = p - str;
          gchar *trimmed = g_new (gchar, len + e_len + 2);

          memcpy (trimmed, str, len);
          if (newline)
            trimmed[len++] = ' ';

          g_strlcpy (trimmed + len, ellipsis, e_len + 1);

          return trimmed;
        }

      return g_strdup (str);
    }

  return NULL;
}
