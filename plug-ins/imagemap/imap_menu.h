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

#ifndef _IMAP_MENU_H
#define _IMAP_MENU_H

#include "imap_command.h"
#include "imap_mru.h"

typedef struct {
   GtkWidget *file_menu;
   GtkWidget *edit_menu;
   GtkWidget *undo;
   GtkWidget *redo;
   GtkWidget *cut;
   GtkWidget *copy;
   GtkWidget *clear;
   GtkWidget *edit;
   GtkWidget *arrow;
   GtkWidget *rectangle;
   GtkWidget *circle;
   GtkWidget *polygon;
   GtkWidget *grid;
   GtkWidget *gray;
   GtkWidget *color;
   GtkWidget *zoom[8];
   GtkWidget *zoom_in;
   GtkWidget *zoom_out;

   gint	      nr_off_mru_items;

   CommandFactory_t cmd_open;
   CommandFactory_t cmd_save;
   CommandFactory_t cmd_save_as;
   CommandFactory_t cmd_preferences;
   CommandFactory_t cmd_close;
   CommandFactory_t cmd_quit;

   CommandFactory_t cmd_undo;
   CommandFactory_t cmd_redo;
   CommandFactory_t cmd_cut;
   CommandFactory_t cmd_copy;
   CommandFactory_t cmd_paste;
   CommandFactory_t cmd_select_all;
   CommandFactory_t cmd_clear;
   CommandFactory_t cmd_edit_area_info;

   CommandFactory_t cmd_area_list;
   CommandFactory_t cmd_source;
   CommandFactory_t cmd_color;
   CommandFactory_t cmd_gray;
   CommandFactory_t cmd_zoom_in;
   CommandFactory_t cmd_zoom_out;

   CommandFactory_t cmd_edit_map_info;

   CommandFactory_t cmd_grid_settings;
   CommandFactory_t cmd_create_guides;

   CommandFactory_t cmd_about;
} Menu_t;

#define menu_set_open_command(menu, command) \
	((menu)->cmd_open = (command))
#define menu_set_save_command(menu, command) \
	((menu)->cmd_save = (command))
#define menu_set_save_as_command(menu, command) \
	((menu)->cmd_save_as = (command))
#define menu_set_preferences_command(menu, command) \
	((menu)->cmd_preferences = (command))
#define menu_set_close_command(menu, command) \
	((menu)->cmd_close = (command))
#define menu_set_quit_command(menu, command) \
	((menu)->cmd_quit = (command))
#define menu_set_undo_command(menu, command) \
	((menu)->cmd_undo = (command))
#define menu_set_redo_command(menu, command) \
	((menu)->cmd_redo = (command))
#define menu_set_cut_command(menu, command) \
	((menu)->cmd_cut = (command))
#define menu_set_copy_command(menu, command) \
	((menu)->cmd_copy = (command))
#define menu_set_paste_command(menu, command) \
	((menu)->cmd_paste = (command))
#define menu_set_select_all_command(menu, command) \
	((menu)->cmd_select_all = (command))
#define menu_set_clear_command(menu, command) \
	((menu)->cmd_clear = (command))
#define menu_set_edit_erea_info_command(menu, command) \
	((menu)->cmd_edit_area_info = (command))
#define menu_set_area_list_command(menu, command) \
	((menu)->cmd_area_list = (command))
#define menu_set_source_command(menu, command) \
	((menu)->cmd_source = (command))
#define menu_set_color_command(menu, command) \
	((menu)->cmd_color = (command))
#define menu_set_gray_command(menu, command) \
	((menu)->cmd_gray = (command))
#define menu_set_zoom_in_command(menu, command) \
	((menu)->cmd_zoom_in = (command))
#define menu_set_zoom_out_command(menu, command) \
	((menu)->cmd_zoom_out = (command))
#define menu_set_edit_map_info_command(popup, command) \
	((menu)->cmd_edit_map_info = (command))
#define menu_set_grid_settings_command(menu, command) \
	((menu)->cmd_grid_settings = (command))
#define menu_set_create_guides_command(menu, command) \
	((menu)->cmd_create_guides = (command))
#define menu_set_about_command(menu, command) \
	((menu)->cmd_about = (command))

Menu_t *make_menu(GtkWidget *main_vbox, GtkWidget *window);
void menu_build_mru_items(MRU_t *mru);
void menu_set_zoom_sensitivity(gint factor);

void menu_select_arrow(void);
void menu_select_rectangle(void);
void menu_select_circle(void);
void menu_select_polygon(void);
void menu_set_zoom(gint factor);
void menu_check_grid(gboolean check);
void menu_shapes_selected(gint count);

#endif /* _IMAP_MENU_H */
