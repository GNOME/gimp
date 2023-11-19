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

#ifndef __CHANNEL_OPTIONS_DIALOG_H__
#define __CHANNEL_OPTIONS_DIALOG_H__


typedef void (* GimpChannelOptionsCallback) (GtkWidget     *dialog,
                                             GimpImage     *image,
                                             GimpChannel   *channel,
                                             GimpContext   *context,
                                             const gchar   *channel_name,
                                             GeglColor     *channel_color,
                                             gboolean       save_selection,
                                             gboolean       channel_visible,
                                             GimpColorTag   channel_color_tag,
                                             gboolean       channel_lock_content,
                                             gboolean       channel_lock_position,
                                             gboolean       channel_lock_visibility,
                                             gpointer       user_data);


GtkWidget * channel_options_dialog_new (GimpImage                  *image,
                                        GimpChannel                *channel,
                                        GimpContext                *context,
                                        GtkWidget                  *parent,
                                        const gchar                *title,
                                        const gchar                *role,
                                        const gchar                *icon_name,
                                        const gchar                *desc,
                                        const gchar                *help_id,
                                        const gchar                *color_label,
                                        const gchar                *opacity_label,
                                        gboolean                    show_from_sel,
                                        const gchar                *channel_name,
                                        GeglColor                  *channel_color,
                                        gboolean                    channel_visible,
                                        GimpColorTag                channel_color_tag,
                                        gboolean                    channel_lock_content,
                                        gboolean                    channel_lock_position,
                                        gboolean                    channel_lock_visibility,
                                        GimpChannelOptionsCallback  callback,
                                        gpointer                    user_data);


#endif /* __CHANNEL_OPTIONS_DIALOG_H__ */
