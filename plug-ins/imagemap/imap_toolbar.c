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

#include "libgimp/stdplugins-intl.h"
#include "imap_main.h"
#include "imap_misc.h"
#include "imap_toolbar.h"

#include "copy.xpm"
#include "cut.xpm"
#include "grid.xpm"
#include "map_info.xpm"
#include "open.xpm"
#include "paste.xpm"
#include "save.xpm"
#include "preferences.xpm"
#include "redo.xpm"
#include "to_back.xpm"
#include "to_front.xpm"
#include "undo.xpm"
#include "zoom_in.xpm"
#include "zoom_out.xpm"

static gboolean _command_lock;

static void
toolbar_command(GtkWidget *widget, gpointer data)
{
   if (_command_lock) {
      _command_lock = FALSE;
   } else {
      CommandFactory_t *factory = (CommandFactory_t*) data;
      Command_t *command = (*factory)();
      command_execute(command);
   }
}

static void
paste_buffer_added(Object_t *obj, gpointer data)
{
   gtk_widget_set_sensitive((GtkWidget*) data, TRUE);
}

static void
paste_buffer_removed(Object_t *obj, gpointer data)
{
   gtk_widget_set_sensitive((GtkWidget*) data, FALSE);
}

static void
command_list_changed(Command_t *command, gpointer data)
{
   ToolBar_t *toolbar = (ToolBar_t*) data;

   /* Set undo entry */
   gtk_widget_set_sensitive(toolbar->undo, (command != NULL));

   /* Set redo entry */
   command = command_list_get_redo_command();
   gtk_widget_set_sensitive(toolbar->redo, (command != NULL));
}

void
toolbar_shapes_selected(ToolBar_t *toolbar, gint count)
{
   gint sensitive = (count > 0);
   gtk_widget_set_sensitive(toolbar->cut, sensitive);
   gtk_widget_set_sensitive(toolbar->copy, sensitive);
   gtk_widget_set_sensitive(toolbar->to_front, sensitive);
   gtk_widget_set_sensitive(toolbar->to_back, sensitive);
}

ToolBar_t*
make_toolbar(GtkWidget *main_vbox, GtkWidget *window)
{
   ToolBar_t 	*data = g_new(ToolBar_t, 1);
   GtkWidget 	*handlebox, *toolbar, *paste;

   handlebox = gtk_handle_box_new();
   gtk_box_pack_start(GTK_BOX(main_vbox), handlebox, FALSE, FALSE, 0);
   data->toolbar = toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, 
					     GTK_TOOLBAR_ICONS);

   gtk_container_set_border_width(GTK_CONTAINER(toolbar), 5);
   gtk_toolbar_set_space_size(GTK_TOOLBAR(toolbar), 8);
   gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

   make_toolbar_icon(toolbar, window, open_xpm, "Open",
		     _("Open"), toolbar_command, &data->cmd_open);
   make_toolbar_icon(toolbar, window, save_xpm, _("Save"),
		     "Save", toolbar_command, &data->cmd_save);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   make_toolbar_icon(toolbar, window, preferences_xpm, "Preferences",
		     _("Preferences"), toolbar_command, 
		     &data->cmd_preferences);

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->undo = make_toolbar_icon(toolbar, window, undo_xpm, "Undo", 
				  _("Undo"), toolbar_command, &data->cmd_undo);
   gtk_widget_set_sensitive(data->undo, FALSE);
   data->redo = make_toolbar_icon(toolbar, window, redo_xpm, "Redo", 
				 _("Redo"), toolbar_command, &data->cmd_redo);
   gtk_widget_set_sensitive(data->redo, FALSE);
   command_list_add_update_cb(command_list_changed, data);

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->cut = make_toolbar_icon(toolbar, window, cut_xpm, "Cut", 
				 _("Cut"), toolbar_command, &data->cmd_cut);
   gtk_widget_set_sensitive(data->cut, FALSE);
   data->copy = make_toolbar_icon(toolbar, window, copy_xpm, "Copy", 
				  _("Copy"), toolbar_command, &data->cmd_copy);
   gtk_widget_set_sensitive(data->copy, FALSE);
   paste = make_toolbar_icon(toolbar, window, paste_xpm, "Paste", 
			     _("Paste"), toolbar_command, &data->cmd_paste);
   gtk_widget_set_sensitive(paste, FALSE);
   paste_buffer_add_add_cb(paste_buffer_added, (gpointer) paste);
   paste_buffer_add_remove_cb(paste_buffer_removed, (gpointer) paste);

   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->zoom_in = make_toolbar_icon(toolbar, window, zoom_in_xpm, "ZoomIn",
				     _("Zoom in"), toolbar_command, 
				     &data->cmd_zoom_in);
   data->zoom_out = make_toolbar_icon(toolbar, window, zoom_out_xpm, "ZoomOut",
				      _("Zoom out"), toolbar_command, 
				      &data->cmd_zoom_out);
   gtk_widget_set_sensitive(data->zoom_out, FALSE);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   make_toolbar_icon(toolbar, window, map_info_xpm, "EditMapInfo",
		     _("Edit Map Info"), toolbar_command, 
		     &data->cmd_edit_map_info);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->to_front = make_toolbar_icon(toolbar, window, to_front_xpm, "ToFront",
				      _("Move To Front"), toolbar_command,
				      &data->cmd_move_to_front);
   gtk_widget_set_sensitive(data->to_front, FALSE);
   data->to_back = make_toolbar_icon(toolbar, window, to_back_xpm, "ToBack",
				     _("Send To Back"), toolbar_command, 
				     &data->cmd_send_to_back);
   gtk_widget_set_sensitive(data->to_back, FALSE);
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
   data->grid = make_toolbar_toggle_icon(toolbar, window, grid_xpm, "Grid",
					 _("Grid"), toolbar_command, 
					 &data->cmd_grid);

   gtk_widget_show(toolbar);
   gtk_widget_show(handlebox);

   return data;
}

void 
toolbar_set_zoom_sensitivity(ToolBar_t *toolbar, gint factor)
{
   gtk_widget_set_sensitive(toolbar->zoom_in, factor < 8);
   gtk_widget_set_sensitive(toolbar->zoom_out, factor > 1);
}

void 
toolbar_set_grid(ToolBar_t *toolbar, gboolean active)
{
   _command_lock = TRUE;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(toolbar->grid), active);
}
