/*
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

#include <gtk/gtk.h>

#include "map-object-icons.h"

#include "../lighting/images/lighting-icon-images.h"


void
mapobject_icons_init (void)
{
  static gboolean initialized = FALSE;

  GtkIconTheme *icon_theme;

  if (initialized)
    return;

  initialized = TRUE;

  icon_theme = gtk_icon_theme_get_default ();

  gtk_icon_theme_add_resource_path (icon_theme, "/org/gimp/lighting/icons");
}
