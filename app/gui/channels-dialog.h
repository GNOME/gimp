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
#ifndef __CHANNELS_DIALOG_H__
#define __CHANNELS_DIALOG_H__

void channels_dialog_new_channel_callback               (GtkWidget *, gpointer);
void channels_dialog_raise_channel_callback             (GtkWidget *, gpointer);
void channels_dialog_lower_channel_callback             (GtkWidget *, gpointer);
void channels_dialog_duplicate_channel_callback         (GtkWidget *, gpointer);
void channels_dialog_delete_channel_callback            (GtkWidget *, gpointer);
void channels_dialog_channel_to_sel_callback            (GtkWidget *, gpointer);
void channels_dialog_add_channel_to_sel_callback        (GtkWidget *, gpointer);
void channels_dialog_sub_channel_from_sel_callback      (GtkWidget *, gpointer);
void channels_dialog_intersect_channel_with_sel_callback(GtkWidget *, gpointer);

#endif /* __CHANNELS_DIALOG_H__ */
