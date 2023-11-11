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

#ifndef __SELECT_COMMANDS_H__
#define __SELECT_COMMANDS_H__


void   select_all_cmd_callback              (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_none_cmd_callback             (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_invert_cmd_callback           (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_cut_float_cmd_callback        (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_copy_float_cmd_callback       (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_feather_cmd_callback          (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_sharpen_cmd_callback          (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_shrink_cmd_callback           (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_grow_cmd_callback             (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_border_cmd_callback           (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_flood_cmd_callback            (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_save_cmd_callback             (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);

void   select_fill_cmd_callback             (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_fill_last_vals_cmd_callback   (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_stroke_cmd_callback           (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);
void   select_stroke_last_vals_cmd_callback (GimpAction *action,
                                             GVariant   *value,
                                             gpointer    data);


#endif /* __SELECT_COMMANDS_H__ */
