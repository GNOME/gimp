/*
 * This is a plug-in for the GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2004 Maurits Rijk  m.rijk@chello.nl
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

#include "config.h"

#include <gtk/gtk.h>

#include "imap_main.h"
#include "imap_misc.h"
#include "imap_stock.h"
#include "imap_toolbar.h"

#include "libgimp/stdplugins-intl.h"
#include "libgimpwidgets/gimpstock.h"

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
   gboolean sensitive = (count > 0);
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
   data->toolbar = toolbar = gtk_toolbar_new();
   gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
   gtk_toolbar_set_orientation(GTK_TOOLBAR(toolbar),
			       GTK_ORIENTATION_HORIZONTAL);
   gtk_container_set_border_width(GTK_CONTAINER(toolbar), 0);

   gtk_container_add(GTK_CONTAINER(handlebox), toolbar);

   make_toolbar_stock_icon(toolbar, GTK_STOCK_OPEN, "Open",
			   _("Open"), toolbar_command, &data->cmd_open);
   make_toolbar_stock_icon(toolbar, GTK_STOCK_SAVE, "Save",
			   _("Save"), toolbar_command, &data->cmd_save);
   toolbar_add_space(toolbar);
   make_toolbar_stock_icon(toolbar, GTK_STOCK_PREFERENCES, "Preferences",
			   _("Preferences"), toolbar_command,
			   &data->cmd_preferences);

   toolbar_add_space(toolbar);
   data->undo = make_toolbar_stock_icon(toolbar, GTK_STOCK_UNDO, "Undo",
					_("Undo"), toolbar_command,
					&data->cmd_undo);
   gtk_widget_set_sensitive(data->undo, FALSE);
   data->redo = make_toolbar_stock_icon(toolbar, GTK_STOCK_REDO, "Redo",
					_("Redo"), toolbar_command,
					&data->cmd_redo);
   gtk_widget_set_sensitive(data->redo, FALSE);
   command_list_add_update_cb(command_list_changed, data);

   toolbar_add_space(toolbar);
   data->cut = make_toolbar_stock_icon(toolbar, GTK_STOCK_CUT, "Cut",
				 _("Cut"), toolbar_command, &data->cmd_cut);
   gtk_widget_set_sensitive(data->cut, FALSE);
   data->copy = make_toolbar_stock_icon(toolbar, GTK_STOCK_COPY, "Copy",
				  _("Copy"), toolbar_command, &data->cmd_copy);
   gtk_widget_set_sensitive(data->copy, FALSE);
   paste = make_toolbar_stock_icon(toolbar, GTK_STOCK_PASTE, "Paste",
				   _("Paste"), toolbar_command,
				   &data->cmd_paste);
   gtk_widget_set_sensitive(paste, FALSE);
   paste_buffer_add_add_cb(paste_buffer_added, (gpointer) paste);
   paste_buffer_add_remove_cb(paste_buffer_removed, (gpointer) paste);

   toolbar_add_space(toolbar);
   data->zoom_in = make_toolbar_stock_icon(toolbar, GTK_STOCK_ZOOM_IN,
					   "ZoomIn", _("Zoom in"),
					   toolbar_command,
					   &data->cmd_zoom_in);
   data->zoom_out = make_toolbar_stock_icon(toolbar, GTK_STOCK_ZOOM_OUT,
					    "ZoomOut",
					    _("Zoom out"), toolbar_command,
					    &data->cmd_zoom_out);
   gtk_widget_set_sensitive(data->zoom_out, FALSE);
   toolbar_add_space(toolbar);
   make_toolbar_stock_icon(toolbar, IMAP_STOCK_MAP_INFO, "EditMapInfo",
			   _("Edit map info"), toolbar_command,
			   &data->cmd_edit_map_info);
   toolbar_add_space(toolbar);
   data->to_front = make_toolbar_stock_icon(toolbar, IMAP_STOCK_TO_FRONT,
					    "ToFront", _("Move To Front"),
					    toolbar_command,
					    &data->cmd_move_to_front);
   gtk_widget_set_sensitive(data->to_front, FALSE);
   data->to_back = make_toolbar_stock_icon(toolbar, IMAP_STOCK_TO_BACK,
					   "ToBack",
					   _("Send To Back"), toolbar_command,
					   &data->cmd_send_to_back);
   gtk_widget_set_sensitive(data->to_back, FALSE);
   toolbar_add_space(toolbar);

   data->grid = make_toolbar_toggle_icon(toolbar, GIMP_STOCK_GRID, "Grid",
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
   gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (toolbar->grid), 
				      active);
}
