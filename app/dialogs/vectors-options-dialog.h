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

#ifndef __VECTORS_OPTIONS_DIALOG_H__
#define __VECTORS_OPTIONS_DIALOG_H__


typedef void (* GimpVectorsOptionsCallback) (GtkWidget    *dialog,
                                             GimpImage    *image,
                                             GimpVectors  *vectors,
                                             GimpContext  *context,
                                             const gchar  *vectors_name,
                                             gboolean      vectors_visible,
                                             gboolean      vectors_linked,
                                             GimpColorTag  vectors_color_tag,
                                             gboolean      vectors_lock_content,
                                             gboolean      vectors_lock_position,
                                             gpointer      user_data);


GtkWidget * vectors_options_dialog_new (GimpImage                  *image,
                                        GimpVectors                *vectors,
                                        GimpContext                *context,
                                        GtkWidget                  *parent,
                                        const gchar                *title,
                                        const gchar                *role,
                                        const gchar                *icon_name,
                                        const gchar                *desc,
                                        const gchar                *help_id,
                                        const gchar                *vectors_name,
                                        gboolean                    vectors_visible,
                                        gboolean                    vectors_linked,
                                        GimpColorTag                vectors_color_tag,
                                        gboolean                    vectors_lock_content,
                                        gboolean                    vectors_lock_position,
                                        GimpVectorsOptionsCallback  callback,
                                        gpointer                    user_data);


#endif /* __VECTORS_OPTIONS_DIALOG_H__ */
