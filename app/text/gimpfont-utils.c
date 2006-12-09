/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * gimpfontlist.c
 * Copyright (C) 2005 Manish Singh <yosh@gimp.org>
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

#include <string.h>

#include <glib-object.h>
#include <pango/pangoft2.h>

#include "gimpfont-utils.h"

/* Workaround pango bug #166540 */

static const char *
getword (const char *str, const char *last, size_t *wordlen)
{
  const char *result;

  while (last > str && g_ascii_isspace (*(last - 1)))
    last--;

  result = last;
  while (result > str && !g_ascii_isspace (*(result - 1)))
    result--;

  *wordlen = last - result;

  return result;
}

gchar *
gimp_font_util_pango_font_description_to_string (const PangoFontDescription *desc)
{
  gchar       *name;
  size_t       wordlen;
  const gchar *p;

  g_return_val_if_fail (desc != NULL, NULL);

  name = pango_font_description_to_string (desc);

  p = getword (name, name + strlen (name), &wordlen);

  if (wordlen)
    {
      gchar   *end;
      gdouble  size;

      size = g_ascii_strtod (p, &end);

      if (end - p == wordlen)
        {
          gchar *new_name;

          new_name = g_strconcat (name, ",", NULL);
          g_free (name);

          name = new_name;
        }
    }

  return name;
}
