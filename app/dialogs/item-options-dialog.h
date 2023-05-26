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

#ifndef __ITEM_OPTIONS_DIALOG_H__
#define __ITEM_OPTIONS_DIALOG_H__


typedef void (* GimpItemOptionsCallback) (GtkWidget    *dialog,
                                          GimpImage    *image,
                                          GimpItem     *item,
                                          GimpContext  *context,
                                          const gchar  *item_name,
                                          gboolean      item_visible,
                                          GimpColorTag  item_color_tag,
                                          gboolean      item_lock_content,
                                          gboolean      item_lock_position,
                                          gboolean      item_lock_visibility,
                                          gpointer      user_data);


GtkWidget * item_options_dialog_new (GimpImage               *image,
                                     GimpItem                *item,
                                     GimpContext             *context,
                                     GtkWidget               *parent,
                                     const gchar             *title,
                                     const gchar             *role,
                                     const gchar             *icon_name,
                                     const gchar             *desc,
                                     const gchar             *help_id,
                                     const gchar             *name_label,
                                     const gchar             *lock_content_icon_name,
                                     const gchar             *lock_content_label,
                                     const gchar             *lock_position_label,
                                     const gchar             *lock_visibility_label,
                                     const gchar             *item_name,
                                     gboolean                 item_visible,
                                     GimpColorTag             item_color_tag,
                                     gboolean                 item_lock_content,
                                     gboolean                 item_lock_position,
                                     gboolean                 item_lock_visibility,
                                     GimpItemOptionsCallback  callback,
                                     gpointer                 user_data);

GtkWidget * item_options_dialog_get_vbox             (GtkWidget   *dialog);
GtkWidget * item_options_dialog_get_grid             (GtkWidget   *dialog,
                                                      gint        *next_row);
GtkWidget * item_options_dialog_get_name_entry       (GtkWidget   *dialog);
GtkWidget * item_options_dialog_get_lock_position    (GtkWidget   *dialog);

void        item_options_dialog_add_widget           (GtkWidget   *dialog,
                                                      const gchar *label,
                                                      GtkWidget   *widget);
GtkWidget * item_options_dialog_add_switch           (GtkWidget   *dialog,
                                                      const gchar *icon_name,
                                                      const gchar *label);

void        item_options_dialog_set_switches_visible (GtkWidget   *dialog,
                                                      gboolean     visible);


#endif /* __ITEM_OPTIONS_DIALOG_H__ */
