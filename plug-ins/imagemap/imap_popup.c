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

#include "imap_edit_area_info.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_menu_funcs.h"
#include "imap_popup.h"
#include "imap_tools.h"

static gint _popup_callback_lock;
static PopupMenu_t _popup;

void
popup_set_zoom_sensitivity(gint factor)
{
   gtk_widget_set_sensitive(_popup.zoom_in, factor < 8);
   gtk_widget_set_sensitive(_popup.zoom_out, factor > 1);
}

static void
popup_rectangle(GtkWidget *widget, gpointer data)
{
   if (_popup_callback_lock) {
      _popup_callback_lock = FALSE;
   } else {
      set_rectangle_func();
      tools_select_rectangle();
      menu_select_rectangle();
   }
}

static void
popup_circle(GtkWidget *widget, gpointer data)
{
   if (_popup_callback_lock) {
      _popup_callback_lock = FALSE;
   } else {
      set_circle_func();
      tools_select_circle();
      menu_select_circle();
   }
}

static void
popup_polygon(GtkWidget *widget, gpointer data)
{
   if (_popup_callback_lock) {
      _popup_callback_lock = FALSE;
   } else {
      set_polygon_func();
      tools_select_polygon();
      menu_select_polygon();
   }
}

static void
popup_arrow(GtkWidget *widget, gpointer data)
{
   if (_popup_callback_lock) {
      _popup_callback_lock = FALSE;
   } else {
      set_arrow_func();
      tools_select_arrow();
      menu_select_arrow();
   }
}

static void
popup_grid(GtkWidget *widget, gpointer data)
{
   if (_popup_callback_lock) {
      _popup_callback_lock = FALSE;
   } else {
      gint grid = toggle_grid();
      menu_check_grid(grid);
      main_toolbar_set_grid(grid);
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
   gtk_widget_set_sensitive((GtkWidget*) data, TRUE);
}

PopupMenu_t*
create_main_popup_menu(void)
{
   GtkWidget *popup_menu, *sub_menu;
   GtkWidget *paste;
   GSList    *group;
   
   _popup.main = popup_menu = gtk_menu_new();
   make_item_with_label(popup_menu, "Map Info...", menu_command, 
			&_popup.cmd_edit_map_info);
   
   sub_menu = make_sub_menu(popup_menu, "Tools");
   _popup.arrow = make_radio_item(sub_menu, NULL, "Arrow", popup_arrow, NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_popup.arrow));
   _popup.rectangle = make_radio_item(sub_menu, group, "Rectangle", 
				      popup_rectangle, NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_popup.rectangle));
   _popup.circle = make_radio_item(sub_menu, group, "Circle", 
				   popup_circle, NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_popup.circle));
   _popup.polygon = make_radio_item(sub_menu, group, "Polygon", 
				    popup_polygon, NULL);
   
   sub_menu = make_sub_menu(popup_menu, "Zoom");
   _popup.zoom_in = make_item_with_label(sub_menu, "In", menu_command, 
					 &_popup.cmd_zoom_in);
   _popup.zoom_out = make_item_with_label(sub_menu, "Out", menu_command,
					  &_popup.cmd_zoom_out);
   gtk_widget_set_sensitive(_popup.zoom_out, FALSE);

   _popup.grid = make_check_item(popup_menu, "Grid", popup_grid, NULL);
   make_item_with_label(popup_menu, "Grid Settings...", menu_command,
			&_popup.cmd_grid_settings);
   make_item_with_label(popup_menu, "Guides...", menu_command,
			&_popup.cmd_create_guides);
   paste = make_item_with_label(popup_menu, "Paste", menu_command, 
				&_popup.cmd_paste);
   gtk_widget_set_sensitive(paste, FALSE);
   paste_buffer_add_add_cb(paste_buffer_added, (gpointer) paste);
   paste_buffer_add_remove_cb(paste_buffer_removed, (gpointer) paste);

   return &_popup;
}

void
do_main_popup_menu(GdkEventButton *event)
{
   gtk_menu_popup(GTK_MENU(_popup.main), NULL, NULL, NULL, NULL,
		  event->button, event->time);
}

static void
popup_select(GtkWidget *item)
{
   _popup_callback_lock = TRUE;
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
}

void
popup_select_arrow(void)
{
   popup_select(_popup.arrow);
}

void
popup_select_rectangle(void)
{
   popup_select(_popup.rectangle);
}

void
popup_select_circle(void)
{
   popup_select(_popup.circle);
}

void
popup_select_polygon(void)
{
   popup_select(_popup.polygon);
}

void
popup_check_grid(gboolean check)
{
   _popup_callback_lock = TRUE;
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_popup.grid), check);
}
