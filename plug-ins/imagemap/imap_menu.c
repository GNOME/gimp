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

#include <sys/types.h>
#include <sys/stat.h>

#include "libgimp/stdplugins-intl.h"

#include "imap_circle.h"
#include "imap_file.h"
#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_menu_funcs.h"
#include "imap_polygon.h"
#include "imap_popup.h"
#include "imap_preferences.h"
#include "imap_rectangle.h"
#include "imap_settings.h"
#include "imap_source.h"
#include "imap_tools.h"

static gint _menu_callback_lock;
static Menu_t _menu;

static void
menu_select(GtkWidget *item)
{
   _menu_callback_lock = TRUE;
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), TRUE);
}

static void
menu_mru(GtkWidget *widget, gpointer data)
{
   MRU_t *mru = get_mru();
   char *filename = (char*) data;
   struct stat buf;
   int err;

   err = stat(filename, &buf);
   if (!err && (buf.st_mode & S_IFREG)) {
      load(filename);
   } else {
      do_file_error_dialog(_("Error opening file"), filename);
      mru_remove(mru, filename);
      menu_build_mru_items(mru);
   }
}

void
menu_set_zoom_sensitivity(gint factor)
{
   gtk_widget_set_sensitive(_menu.zoom_in, factor < 8);
   gtk_widget_set_sensitive(_menu.zoom_out, factor > 1);
}

void
menu_shapes_selected(gint count)
{
   gint sensitive = (count > 0);
   gtk_widget_set_sensitive(_menu.cut, sensitive);
   gtk_widget_set_sensitive(_menu.copy, sensitive);
   gtk_widget_set_sensitive(_menu.clear, sensitive);
   gtk_widget_set_sensitive(_menu.edit, sensitive);
}

static void
menu_zoom_to(gint factor)
{
   if (_menu_callback_lock) {
      _menu_callback_lock--;
   } else {
      set_zoom(factor);
   }
   menu_set_zoom_sensitivity(factor);
}

static void
menu_zoom_to_1(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(1);
}

static void
menu_zoom_to_2(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(2);
}

static void
menu_zoom_to_3(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(3);
}

static void
menu_zoom_to_4(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(4);
}

static void
menu_zoom_to_5(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(5);
}

static void
menu_zoom_to_6(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(6);
}

static void
menu_zoom_to_7(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(7);
}

static void
menu_zoom_to_8(GtkWidget *widget, gpointer data)
{
   menu_zoom_to(8);
}

static void
menu_rectangle(GtkWidget *widget, gpointer data)
{
   if (_menu_callback_lock) {
      _menu_callback_lock = FALSE;
   } else {
      set_rectangle_func();
      tools_select_rectangle();
      popup_select_rectangle();
   }
}

static void
menu_circle(GtkWidget *widget, gpointer data)
{
   if (_menu_callback_lock) {
      _menu_callback_lock = FALSE;
   } else {
      set_circle_func();
      tools_select_circle();
      popup_select_circle();
   }
}

static void
menu_polygon(GtkWidget *widget, gpointer data)
{
   if (_menu_callback_lock) {
      _menu_callback_lock = FALSE;
   } else {
      set_polygon_func();
      tools_select_polygon();
      popup_select_polygon();
   }
}

static void
menu_arrow(GtkWidget *widget, gpointer data)
{
   if (_menu_callback_lock) {
      _menu_callback_lock = FALSE;
   } else {
      set_arrow_func();
      tools_select_arrow();
      popup_select_arrow();
   }
}

static void
menu_grid(GtkWidget *widget, gpointer data)
{
   if (_menu_callback_lock) {
      _menu_callback_lock = FALSE;
   } else {
      gint grid = toggle_grid();
      popup_check_grid(grid);
      main_toolbar_set_grid(grid);
   }
}

static void
make_file_menu(GtkWidget *menu_bar)
{
   GtkWidget 	*file_menu = make_menu_bar_item(menu_bar, _("File"));
   GtkWidget	*item;

   _menu.file_menu = file_menu;
   item = make_item_with_label(file_menu, _("Open..."), menu_command,
			       &_menu.cmd_open);
   add_accelerator(item, 'O', GDK_CONTROL_MASK);
   item = make_item_with_label(file_menu, _("Save"), menu_command,
			       &_menu.cmd_save);
   add_accelerator(item, 'S', GDK_CONTROL_MASK);
   make_item_with_label(file_menu, _("Save As..."), menu_command,
			&_menu.cmd_save_as);
   make_separator(file_menu);
   make_item_with_label(file_menu, _("Preferences..."), menu_command,
			&_menu.cmd_preferences);
   make_separator(file_menu);
   item = make_item_with_label(file_menu, _("Close"), menu_command,
			       &_menu.cmd_close);
   add_accelerator(item, 'W', GDK_CONTROL_MASK);
   item = make_item_with_label(file_menu, "Quit", menu_command,
			       &_menu.cmd_quit);
   add_accelerator(item, 'Q', GDK_CONTROL_MASK);
}

static void
command_list_changed(Command_t *command, gpointer data)
{
   char scratch[64];

   /* Set undo entry */
   if (_menu.undo)
      gtk_widget_destroy(_menu.undo);
   sprintf(scratch, _("Undo %s"), (command) ? command->name : "");
   _menu.undo = insert_item_with_label(_menu.edit_menu, 1, scratch,
				       menu_command, &_menu.cmd_undo);
   add_accelerator(_menu.undo, 'Z', GDK_CONTROL_MASK);
   gtk_widget_set_sensitive(_menu.undo, (command != NULL));

   /* Set redo entry */
   command = command_list_get_redo_command();
   if (_menu.redo)
      gtk_widget_destroy(_menu.redo);
   sprintf(scratch, _("Redo %s"), (command) ? command->name : "");
   _menu.redo = insert_item_with_label(_menu.edit_menu, 2, scratch,
				       menu_command, &_menu.cmd_redo);
   add_accelerator(_menu.redo, 'R', GDK_CONTROL_MASK);
   gtk_widget_set_sensitive(_menu.redo, (command != NULL));
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
make_edit_menu(GtkWidget *menu_bar)
{
   GtkWidget *edit_menu = make_menu_bar_item(menu_bar, _("Edit"));
   GtkWidget *item, *paste;

   _menu.edit_menu = edit_menu;
   command_list_changed(NULL, NULL);

   make_separator(edit_menu);
   _menu.cut = make_item_with_label(edit_menu, _("Cut"), menu_command,
				    &_menu.cmd_cut);
   add_accelerator(_menu.cut, 'X', GDK_CONTROL_MASK);
   _menu.copy = make_item_with_label(edit_menu, _("Copy"), menu_command,
				     &_menu.cmd_copy);
   add_accelerator(_menu.copy, 'C', GDK_CONTROL_MASK);
   paste = make_item_with_label(edit_menu, _("Paste"), menu_command,
				&_menu.cmd_paste);
   add_accelerator(paste, 'V', GDK_CONTROL_MASK);
   gtk_widget_set_sensitive(paste, FALSE);
   item = make_item_with_label(edit_menu, _("Select All"), menu_command,
			       &_menu.cmd_select_all);
   add_accelerator(item, 'A', GDK_CONTROL_MASK);
   make_separator(edit_menu);
   _menu.clear = make_item_with_label(edit_menu, _("Clear"), menu_command,
				      &_menu.cmd_clear);
   add_accelerator(_menu.clear, 'K', GDK_CONTROL_MASK);
   _menu.edit = make_item_with_label(edit_menu, _("Edit Area Info..."),
				     menu_command, &_menu.cmd_edit_area_info);

   paste_buffer_add_add_cb(paste_buffer_added, (gpointer) paste);
   paste_buffer_add_remove_cb(paste_buffer_removed, (gpointer) paste);

   command_list_add_update_cb(command_list_changed, NULL);
}

static void
make_view_menu(GtkWidget *menu_bar)
{
   GtkWidget *view_menu = make_menu_bar_item(menu_bar, _("View"));
   GtkWidget *zoom_menu, *item;
   GSList *group = NULL;

   item = make_check_item(view_menu, _("Area List"), menu_command,
			  &_menu.cmd_area_list);
   GTK_CHECK_MENU_ITEM(item)->active = TRUE;

   make_item_with_label(view_menu, _("Source..."), menu_command,
			&_menu.cmd_source);
   make_separator(view_menu);

   _menu.color = make_radio_item(view_menu, NULL, _("Color"), menu_command,
				 &_menu.cmd_color);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.color));

   _menu.gray = make_radio_item(view_menu, group, _("Grayscale"), menu_command,
				&_menu.cmd_gray);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.gray));

   if (!get_map_info()->color) { /* Gray image */
      gtk_widget_set_sensitive(_menu.color, FALSE);
      GTK_CHECK_MENU_ITEM(_menu.color)->active = FALSE;
      GTK_CHECK_MENU_ITEM(_menu.gray)->active = TRUE;
   }

   make_separator(view_menu);

   _menu.zoom_in = make_item_with_label(view_menu, _("Zoom In"), menu_command,
					&_menu.cmd_zoom_in);
   add_accelerator(_menu.zoom_in, '=', 0);
   _menu.zoom_out = make_item_with_label(view_menu, _("Zoom Out"), 
					 menu_command, &_menu.cmd_zoom_out);
   add_accelerator(_menu.zoom_out, '-', 0);
   gtk_widget_set_sensitive(_menu.zoom_out, FALSE);

   zoom_menu = make_sub_menu(view_menu, _("Zoom To"));

   _menu.zoom[0] = make_radio_item(zoom_menu, NULL, "1:1", menu_zoom_to_1,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[0]));
   _menu.zoom[1] = make_radio_item(zoom_menu, group, "1:2", menu_zoom_to_2,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[1]));
   _menu.zoom[2] = make_radio_item(zoom_menu, group, "1:3", menu_zoom_to_3,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[2]));
   _menu.zoom[3] = make_radio_item(zoom_menu, group, "1:4", menu_zoom_to_4,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[3]));
   _menu.zoom[4] = make_radio_item(zoom_menu, group, "1:5", menu_zoom_to_5,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[4]));
   _menu.zoom[5] = make_radio_item(zoom_menu, group, "1:6", menu_zoom_to_6,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[5]));
   _menu.zoom[6] = make_radio_item(zoom_menu, group, "1:7", menu_zoom_to_7,
				   NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.zoom[6]));
   _menu.zoom[7] = make_radio_item(zoom_menu, group, "1:8", menu_zoom_to_8,
				   NULL);
}

static void
make_mapping_menu(GtkWidget *menu_bar)
{
   GtkWidget *menu = make_menu_bar_item(menu_bar, _("Mapping"));
   GSList    *group;

   _menu.arrow = make_radio_item(menu, NULL, _("Arrow"), menu_arrow, NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.arrow));
   _menu.rectangle = make_radio_item(menu, group, _("Rectangle"),
				     menu_rectangle, NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.rectangle));
   _menu.circle = make_radio_item(menu, group, _("Circle"), menu_circle, NULL);
   group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(_menu.circle));
   _menu.polygon = make_radio_item(menu, group, _("Polygon"), menu_polygon,
				   NULL);
   make_separator(menu);
   make_item_with_label(menu, _("Edit Map Info..."), menu_command,
			&_menu.cmd_edit_map_info);
}

static void
make_goodies_menu(GtkWidget *menu_bar)
{
   GtkWidget *goodies_menu = make_menu_bar_item(menu_bar, _("Goodies"));
   _menu.grid = make_check_item(goodies_menu, _("Grid"), menu_grid, NULL);
   make_item_with_label(goodies_menu, _("Grid Settings..."), menu_command,
			&_menu.cmd_grid_settings);
   make_item_with_label(goodies_menu, _("Create Guides..."), menu_command,
			&_menu.cmd_create_guides);
}

static void
make_help_menu(GtkWidget *menu_bar)
{
   GtkWidget *help_menu = make_menu_bar_item(menu_bar, _("Help"));
   gtk_menu_item_right_justify(GTK_MENU_ITEM(gtk_menu_get_attach_widget(
      GTK_MENU(help_menu))));
   make_item_with_label(help_menu, _("About ImageMap..."), menu_command,
			&_menu.cmd_about);
}

Menu_t*
make_menu(GtkWidget *main_vbox, GtkWidget *window)
{
   GtkWidget *menu_bar = gtk_menu_bar_new();

   gtk_box_pack_start(GTK_BOX(main_vbox), menu_bar, FALSE, TRUE, 0);
   gtk_widget_show(menu_bar);

   init_accel_group(window);

   make_file_menu(menu_bar);
   make_edit_menu(menu_bar);
   make_view_menu(menu_bar);
   make_mapping_menu(menu_bar);
   make_goodies_menu(menu_bar);
   make_help_menu(menu_bar);

   menu_shapes_selected(0);

   return &_menu;
}

void
menu_build_mru_items(MRU_t *mru)
{
   GList *p;
   gint position = 7;		/* Position of 'Close' entry */
   int i;

   if (_menu.nr_off_mru_items) {
      GList *children = gtk_container_children(GTK_CONTAINER(_menu.file_menu));
      
      p = g_list_nth(children, position);
      for (i = 0; i < _menu.nr_off_mru_items; i++, p = p->next) {
	 gtk_widget_destroy((GtkWidget*) p->data);
      }
      g_list_free(children);
   }

   i = 0;
   for (p = mru->list; p; p = p->next, i++) {
      GtkWidget *item = insert_item_with_label(_menu.file_menu, position++,
					       (gchar*) p->data,
					       menu_mru, p->data);
      if (i < 9) {
	 guchar accelerator_key = '1' + i;
	 add_accelerator(item, accelerator_key, GDK_CONTROL_MASK);
      }
   }
   insert_separator(_menu.file_menu, position);
   _menu.nr_off_mru_items = i + 1;
}

void
menu_select_arrow(void)
{
   menu_select(_menu.arrow);
}

void
menu_select_rectangle(void)
{
   menu_select(_menu.rectangle);
}

void
menu_select_circle(void)
{
   menu_select(_menu.circle);
}

void
menu_select_polygon(void)
{
   menu_select(_menu.polygon);
}

void
menu_check_grid(gint check)
{
   _menu_callback_lock = TRUE;
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_menu.grid), check);
}

void
menu_set_zoom(gint factor)
{
   _menu_callback_lock = 2;
   gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(_menu.zoom[factor - 1]),
				  TRUE);
   menu_set_zoom_sensitivity(factor);
}
