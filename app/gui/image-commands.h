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

#ifndef __IMAGE_COMMANDS_H__
#define __IMAGE_COMMANDS_H__


void   image_convert_rgb_cmd_callback       (GtkWidget *widget,
					     gpointer   data);
void   image_convert_grayscale_cmd_callback (GtkWidget *widget,
					     gpointer   data);
void   image_convert_indexed_cmd_callback   (GtkWidget *widget,
					     gpointer   data);

void   image_resize_cmd_callback            (GtkWidget *widget,
					     gpointer   data);
void   image_scale_cmd_callback             (GtkWidget *widget,
					     gpointer   data);
void   image_flip_cmd_callback              (GtkWidget *widget,
                                             gpointer   data,
                                             guint      action);
void   image_rotate_cmd_callback            (GtkWidget *widget,
                                             gpointer   data,
                                             guint      action);
void   image_crop_cmd_callback              (GtkWidget *widget,
					     gpointer   data);

void   image_duplicate_cmd_callback         (GtkWidget *widget,
					     gpointer   data);

void   image_merge_layers_cmd_callback      (GtkWidget *widet,
                                             gpointer   data);
void   image_flatten_image_cmd_callback     (GtkWidget *widet,
                                             gpointer   data);

void   image_layers_merge_query             (GimpImage *gimage,
                                             gboolean   merge_visible,
                                             GtkWidget *parent);

void   image_configure_grid_cmd_callback    (GtkWidget *widget,
                                             gpointer   data);

#endif /* __IMAGE_COMMANDS_H__ */
