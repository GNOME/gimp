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

#include <gegl.h>
#include <gtk/gtk.h>

#include "menus-types.h"

#include "file-menu.h"
#include "filters-menu.h"
#include "image-menu.h"
#include "plug-in-menus.h"
#include "window-menu.h"
#include "windows-menu.h"


void
image_menu_setup (GimpUIManager *manager,
                  const gchar   *ui_path)
{
  gchar *path;

  file_menu_setup (manager, ui_path);
  windows_menu_setup (manager, ui_path);
  plug_in_menus_setup (manager, ui_path);
  filters_menu_setup (manager, ui_path);

  path = g_strconcat (ui_path, "/View", NULL);
  window_menu_setup (manager, "view", path);
  g_free (path);
}
