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


typedef void (* GimpPathOptionsCallback) (GtkWidget    *dialog,
                                          GimpImage    *image,
                                          GimpPath     *path,
                                          GimpContext  *context,
                                          const gchar  *path_name,
                                          gboolean      path_visible,
                                          GimpColorTag  path_color_tag,
                                          gboolean      path_lock_content,
                                          gboolean      path_lock_position,
                                          gboolean      path_lock_visibility,
                                          gpointer      user_data);


GtkWidget * path_options_dialog_new    (GimpImage                *image,
                                        GimpPath                 *path,
                                        GimpContext              *context,
                                        GtkWidget                *parent,
                                        const gchar              *title,
                                        const gchar              *role,
                                        const gchar              *icon_name,
                                        const gchar              *desc,
                                        const gchar              *help_id,
                                        const gchar              *path_name,
                                        gboolean                  path_visible,
                                        GimpColorTag              path_color_tag,
                                        gboolean                  path_lock_content,
                                        gboolean                  path_lock_position,
                                        gboolean                  path_lock_visibility,
                                        GimpPathOptionsCallback   callback,
                                        gpointer                  user_data);
