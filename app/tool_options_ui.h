/* The GIMP -- an image manipulation program
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
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
#ifndef __TOOL_OPTIONS_UI_H__
#define __TOOL_OPTIONS_UI_H__

#include <gtk/gtk.h>

/*  a toggle button callback which sets the sensitive state of an attached
 *  list of widgets
 */
void      tool_options_toggle_update            (GtkWidget *widget,
						 gpointer   data);

/*  a unit menu callback which sets the digits of an attached
 *  list of spinbuttons
 */
void      tool_options_unitmenu_update          (GtkWidget *widget,
						 gpointer   data);

/*  integer and float adjustment callbacks
 */
void      tool_options_int_adjustment_update    (GtkWidget *widget,
						 gpointer   data);
void      tool_options_double_adjustment_update (GtkWidget *widget,
						 gpointer   data);

/*  a group of radio buttons with a frame around them   */
GtkWidget* tool_options_radio_buttons_new (gchar*      label,
					   gpointer    toggle_val,
					   GtkWidget*  button_widget[],
					   gchar*      button_label[],
					   gint        button_value[],
					   gint        num);

#endif /* __TOOL_OPTIONS_UI_H__ */



