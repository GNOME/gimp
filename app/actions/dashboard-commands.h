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

#ifndef __DASHBOARD_COMMANDS_H__
#define __DASHBOARD_COMMANDS_H__


void   dashboard_update_interval_cmd_callback        (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   dashboard_history_duration_cmd_callback       (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   dashboard_log_record_cmd_callback             (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   dashboard_log_add_marker_cmd_callback         (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);
void   dashboard_log_add_empty_marker_cmd_callback   (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   dashboard_reset_cmd_callback                  (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);

void   dashboard_low_swap_space_warning_cmd_callback (GimpAction *action,
                                                      GVariant   *value,
                                                      gpointer    data);


#endif /* __DASHBOARD_COMMANDS_H__ */
