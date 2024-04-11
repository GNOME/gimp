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

#ifndef __THEMES_H__
#define __THEMES_H__


void     themes_init           (Gimp        *gimp);
void     themes_exit           (Gimp        *gimp);

gchar ** themes_list_themes    (Gimp        *gimp,
                                gint        *n_themes);
GFile  * themes_get_theme_dir  (Gimp        *gimp,
                                const gchar *theme_name);
GFile  * themes_get_theme_file (Gimp        *gimp,
                                const gchar *first_component,
                                ...) G_GNUC_NULL_TERMINATED;
#ifdef G_OS_WIN32
void     themes_set_title_bar  (Gimp        *gimp);
#endif

#endif /* __THEMES_H__ */
