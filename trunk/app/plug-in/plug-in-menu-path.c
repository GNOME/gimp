/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * plug-in-menu-path.c
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

#include "glib-object.h"

#include "plug-in-types.h"

#include "plug-in-menu-path.h"


typedef struct _MenuPathMapping MenuPathMapping;

struct _MenuPathMapping
{
  const gchar *orig_path;
  const gchar *mapped_path;
};


static const MenuPathMapping menu_path_mappings[] =
{
#ifndef ENABLE_TOOLBOX_MENU
  { "<Toolbox>/Xtns", "<Image>/Xtns" },
  { "<Toolbox>/Help", "<Image>/Help" },
#endif /* ENABLE_TOOLBOX_MENU */
  { NULL, NULL                       }
};


gchar *
plug_in_menu_path_map (const gchar *menu_path)
{
  const MenuPathMapping *mapping;

  g_return_val_if_fail (menu_path != NULL, NULL);

  for (mapping = menu_path_mappings; mapping->orig_path; mapping++)
    {
      if (g_str_has_prefix (menu_path, mapping->orig_path))
        {
          gint   orig_len = strlen (mapping->orig_path);
          gchar *mapped_path;

          if (strlen (menu_path) > orig_len)
            mapped_path = g_strconcat (mapping->mapped_path,
                                       menu_path + orig_len,
                                       NULL);
          else
            mapped_path = g_strdup (mapping->mapped_path);

#if 0
          g_printerr ("%s: mapped %s to %s\n", G_STRFUNC,
                      menu_path, mapped_path);
#endif

          return mapped_path;
        }
    }

  return g_strdup (menu_path);
}
