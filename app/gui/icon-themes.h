/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * icon-themes.h
 * Copyright (C) 2015 Benoit Touchette
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


void        icon_themes_init                    (Gimp        *gimp);
void        icon_themes_exit                    (Gimp        *gimp);

gchar    ** icon_themes_list_themes             (Gimp        *gimp,
                                                 gint        *n_themes);
GFile     * icon_themes_get_theme_dir           (Gimp        *gimp,
                                                 const gchar *theme_name);

gboolean    icon_themes_current_prefer_symbolic (Gimp        *gimp);
