/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __CHANNELS_COMMANDS_H__
#define __CHANNELS_COMMANDS_H__


void   channels_new_channel_cmd_callback                (GtkWidget   *widget,
							 gpointer     data);
void   channels_raise_channel_cmd_callback              (GtkWidget   *widget,
							 gpointer     data);
void   channels_lower_channel_cmd_callback              (GtkWidget   *widget,
							 gpointer     data);
void   channels_duplicate_channel_cmd_callback          (GtkWidget   *widget,
							 gpointer     data);
void   channels_delete_channel_cmd_callback             (GtkWidget   *widget,
							 gpointer     data);
void   channels_channel_to_sel_cmd_callback             (GtkWidget   *widget,
							 gpointer     data);
void   channels_add_channel_to_sel_cmd_callback         (GtkWidget   *widget,
							 gpointer     data);
void   channels_sub_channel_from_sel_cmd_callback       (GtkWidget   *widget,
							 gpointer     data);
void   channels_intersect_channel_with_sel_cmd_callback (GtkWidget   *widget,
							 gpointer     data);
void   channels_edit_channel_attributes_cmd_callback    (GtkWidget   *widget,
							 gpointer     data);

void   channels_new_channel_query                       (GimpImage   *gimage,
                                                         GimpChannel *template,
                                                         gboolean     interactive);
void   channels_edit_channel_query                      (GimpChannel *channel);


#endif /* __CHANNELS_COMMANDS_H__ */
