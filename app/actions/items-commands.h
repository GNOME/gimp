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

#ifndef __ITEMS_COMMANDS_H__
#define __ITEMS_COMMANDS_H__


void   items_visible_cmd_callback          (GimpAction   *action,
                                            GVariant     *value,
                                            GimpImage    *image,
                                            GimpItem     *item);
void   items_linked_cmd_callback           (GimpAction   *action,
                                            GVariant     *value,
                                            GimpImage    *image,
                                            GimpItem     *item);
void   items_lock_content_cmd_callback     (GimpAction   *action,
                                            GVariant     *value,
                                            GimpImage    *image,
                                            GimpItem     *item);
void   items_lock_position_cmd_callback    (GimpAction   *action,
                                            GVariant     *value,
                                            GimpImage    *image,
                                            GimpItem     *item);

void   items_color_tag_cmd_callback        (GimpAction   *action,
                                            GimpImage    *image,
                                            GimpItem     *item,
                                            GimpColorTag  color_tag);

void   items_fill_cmd_callback             (GimpAction   *action,
                                            GimpImage    *image,
                                            GimpItem     *item,
                                            const gchar  *dialog_key,
                                            const gchar  *dialog_title,
                                            const gchar  *dialog_icon_name,
                                            const gchar  *dialog_help_id,
                                            gpointer      data);
void   items_fill_last_vals_cmd_callback   (GimpAction   *action,
                                            GimpImage    *image,
                                            GimpItem     *item,
                                            gpointer      data);

void   items_stroke_cmd_callback           (GimpAction   *action,
                                            GimpImage    *image,
                                            GimpItem     *item,
                                            const gchar  *dialog_key,
                                            const gchar  *dialog_title,
                                            const gchar  *dialog_icon_name,
                                            const gchar  *dialog_help_id,
                                            gpointer      data);
void   items_stroke_last_vals_cmd_callback (GimpAction   *action,
                                            GimpImage    *image,
                                            GimpItem     *item,
                                            gpointer      data);



#endif /* __ITEMS_COMMANDS_H__ */
