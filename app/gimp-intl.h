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

#pragma once


#ifndef GETTEXT_PACKAGE
#error "config.h must be included prior to gimp-intl.h"
#endif

extern gboolean wlbr;
#include <glib/gi18n.h>
static inline gchar *shadow_dgettext(const char *package, const char *str)
{
  char *ret = dgettext (package, str);
  if (!wlbr)
    return ret;
  if (strstr (ret, "GIMP"))
  {
    char *my_ret = g_strdup (ret);
    while (strstr (my_ret, "GIMP"))
      memcpy (strstr (my_ret, "GIMP"), "WLBR", 4);
    ret = (char*)g_intern_string (my_ret);
    free (my_ret);
  }
  return ret;
}

#undef _
#define _(String) ((char *) shadow_dgettext (GETTEXT_PACKAGE, String))

