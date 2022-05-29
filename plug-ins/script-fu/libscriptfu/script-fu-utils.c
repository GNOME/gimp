/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <string.h>

#include <glib.h>

#include "script-fu-utils.h"


/*
 * Escapes the special characters '\b', '\f', '\n', '\r', '\t', '\' and '"'
 * in the string source by inserting a '\' before them.
 */
gchar *
script_fu_strescape (const gchar *source)
{
  const guchar *p;
  gchar        *dest;
  gchar        *q;

  g_return_val_if_fail (source != NULL, NULL);

  p = (const guchar *) source;

  /* Each source byte needs maximally two destination chars */
  q = dest = g_malloc (strlen (source) * 2 + 1);

  while (*p)
    {
      switch (*p)
        {
        case '\b':
        case '\f':
        case '\n':
        case '\r':
        case '\t':
        case '\\':
        case '"':
          *q++ = '\\';
          /* fallthrough */
        default:
          *q++ = *p;
          break;
        }

      p++;
    }

  *q = 0;

  return dest;
}
