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

#ifndef _IMAP_TOOLBAR_H
#define _IMAP_TOOLBAR_H

#include "gtk/gtk.h"

#include "imap_command.h"

typedef struct {
   GtkWidget *toolbar;
   GtkWidget *undo;
   GtkWidget *redo;
   GtkWidget *cut;
   GtkWidget *copy;
   GtkWidget *to_front;
   GtkWidget *to_back;
   GtkWidget *zoom_in;
   GtkWidget *zoom_out;
   GtkWidget *grid;

   CommandFactory_t cmd_open;
   CommandFactory_t cmd_save;
   CommandFactory_t cmd_preferences;
   CommandFactory_t cmd_redo;
   CommandFactory_t cmd_undo;
   CommandFactory_t cmd_cut;
   CommandFactory_t cmd_copy;
   CommandFactory_t cmd_paste;
   CommandFactory_t cmd_zoom_in;
   CommandFactory_t cmd_zoom_out;
   CommandFactory_t cmd_edit_map_info;
   CommandFactory_t cmd_move_to_front;
   CommandFactory_t cmd_send_to_back;
   CommandFactory_t cmd_grid;
} ToolBar_t;

ToolBar_t *make_toolbar(GtkWidget *main_vbox, GtkWidget *window);
void toolbar_set_zoom_sensitivity(ToolBar_t *toolbar, gint factor);
void toolbar_set_grid(ToolBar_t *toolbar, gboolean active);
void toolbar_shapes_selected(ToolBar_t *toolbar, gint count);

#define toolbar_set_open_command(toolbar, command) \
	((toolbar)->cmd_open = (command))
#define toolbar_set_save_command(toolbar, command) \
	((toolbar)->cmd_save = (command))
#define toolbar_set_preferences_command(toolbar, command) \
	((toolbar)->cmd_preferences = (command))
#define toolbar_set_redo_command(toolbar, command) \
	((toolbar)->cmd_redo = (command))
#define toolbar_set_undo_command(toolbar, command) \
	((toolbar)->cmd_undo = (command))
#define toolbar_set_cut_command(toolbar, command) \
	((toolbar)->cmd_cut = (command))
#define toolbar_set_copy_command(toolbar, command) \
	((toolbar)->cmd_copy= (command))
#define toolbar_set_paste_command(toolbar, command) \
	((toolbar)->cmd_paste = (command))
#define toolbar_set_zoom_in_command(toolbar, command) \
	((toolbar)->cmd_zoom_in = (command))
#define toolbar_set_zoom_out_command(toolbar, command) \
	((toolbar)->cmd_zoom_out = (command))
#define toolbar_set_edit_map_info_command(toolbar, command) \
	((toolbar)->cmd_edit_map_info = (command))
#define toolbar_set_move_to_front_command(toolbar, command) \
	((toolbar)->cmd_move_to_front = (command))
#define toolbar_set_send_to_back_command(toolbar, command) \
	((toolbar)->cmd_send_to_back = (command))
#define toolbar_set_grid_command(toolbar, command) \
	((toolbar)->cmd_grid = (command))

#endif /* _IMAP_TOOLBAR_H */
