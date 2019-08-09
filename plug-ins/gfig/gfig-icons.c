/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for GIMP.
 *
 * Generates images containing vector type drawings.
 *
 * Copyright (C) 1997 Andy Thomas  <alt@picnic.demon.co.uk>
 *               2003 Sven Neumann  <sven@gimp.org>
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

#include "gfig-icons.h"

#include "images/gfig-icon-images.h"


void
gfig_icons_init (void)
{
  static gboolean initialized = FALSE;

  GtkIconTheme *icon_theme;

  if (initialized)
    return;

  initialized = TRUE;

  icon_theme = gtk_icon_theme_get_default ();

  gtk_icon_theme_add_resource_path (icon_theme, "/org/gimp/gfig/icons");
}
