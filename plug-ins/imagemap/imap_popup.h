/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-1999 Maurits Rijk  lpeek.mrijk@consunet.nl
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
 *
 */

#ifndef _IMAP_POPUP_H
#define _IMAP_POPUP_H

#include "imap_command.h"

typedef struct {
   GtkWidget *main;

   GtkWidget *arrow;
   GtkWidget *rectangle;
   GtkWidget *circle;
   GtkWidget *polygon;
   GtkWidget *grid;
   GtkWidget *zoom_in;
   GtkWidget *zoom_out;

   CommandFactory_t cmd_zoom_in;
   CommandFactory_t cmd_zoom_out;
   CommandFactory_t cmd_edit_map_info;
   CommandFactory_t cmd_grid_settings;
   CommandFactory_t cmd_create_guides;
   CommandFactory_t cmd_paste;
} PopupMenu_t;


#define popup_set_zoom_in_command(popup, command) \
	((popup)->cmd_zoom_in = (command))
#define popup_set_zoom_out_command(popup, command) \
	((popup)->cmd_zoom_out = (command))
#define popup_set_edit_map_info_command(popup, command) \
	((popup)->cmd_edit_map_info = (command))
#define popup_set_grid_settings_command(popup, command) \
	((popup)->cmd_grid_settings = (command))
#define popup_set_create_guides_command(popup, command) \
	((popup)->cmd_create_guides = (command))
#define popup_set_paste_command(popup, command) \
	((popup)->cmd_paste = (command))

PopupMenu_t *create_main_popup_menu(void);
void do_main_popup_menu(GdkEventButton *event);
void popup_set_zoom_sensitivity(gint factor);
void popup_select_arrow(void);
void popup_select_rectangle(void);
void popup_select_circle(void);
void popup_select_polygon(void);
void popup_check_grid(gint check);

#endif /* _IMAP_POPUP_H */
