/* The GIMP -- an image manipulation program
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

gchar *
gimp_font_util_pango_font_description_to_string (PangoFontDescription *desc)
{
  gchar *name;
  gchar  last_char;

  g_return_val_if_fail (desc != NULL, NULL);

  name = pango_font_description_to_string (desc);

  last_char = name[strlen (name) - 1];

  if (g_ascii_isdigit (last_char) || last_char == '.')
    {
      gchar *new_name;

      new_name = g_strconcat (name, ",", NULL);
      g_free (name);

      name = new_name;
    }

  return name;
}
