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
#include <gdk/gdkkeysyms.h>

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
#include "imap_stock.h"
#include "imap_source.h"
#include "imap_tools.h"

#include "libgimp/stdplugins-intl.h"

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

   if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
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
   gboolean sensitive = (count > 0);
   gtk_widget_set_sensitive(_menu.cut, sensitive);
   gtk_widget_set_sensitive(_menu.copy, sensitive);
   gtk_widget_set_sensitive(_menu.clear, sensitive);
   gtk_widget_set_sensitive(_menu.edit, sensitive);
   gtk_widget_set_sensitive(_menu.deselect_all, sensitive);
}

static void
menu_zoom_to(GtkWidget *widget, gpointer data)
{
   if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
      gint factor = GPOINTER_TO_INT (data);

      if (_menu_callback_lock) {
	 _menu_callback_lock--;
      } else {
	 set_zoom(factor);
      }
      menu_set_zoom_sensitivity(factor);
   }
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
menu_fuzzy_select(GtkWidget *widget, gpointer data)
{
   if (_menu_callback_lock) {
      _menu_callback_lock = FALSE;
   } else {
/*
      set_fuzzy_select_func();
      tools_select_fuzzy();
      popup_select_fuzzy();
*/
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
   GtkWidget *file_menu = make_menu_bar_item(menu_bar, _("_File"));
   GtkWidget *item;

   _menu.file_menu = file_menu;
   make_item_with_image(file_menu, GTK_STOCK_OPEN, menu_command,
			&_menu.cmd_open);
   _menu.open_recent = make_sub_menu(file_menu, _("Open recent"));
   make_item_with_image(file_menu, GTK_STOCK_SAVE, menu_command,
			&_menu.cmd_save);
   item = make_item_with_image(file_menu, GTK_STOCK_SAVE_AS, menu_command,
			       &_menu.cmd_save_as);
   add_accelerator(item, 'S', GDK_SHIFT_MASK|GDK_CONTROL_MASK);
   make_separator(file_menu);
   make_item_with_image(file_menu, GTK_STOCK_CLOSE, menu_command,
			&_menu.cmd_close);
   make_item_with_image(file_menu, GTK_STOCK_QUIT, menu_command,
			&_menu.cmd_quit);
}

static void
command_list_changed(Command_t *command, gpointer data)
{
   gchar *scratch;
   GtkWidget *icon;

   /* Set undo entry */
   if (_menu.undo)
      gtk_widget_destroy(_menu.undo);
   scratch = g_strdup_printf (_("_Undo %s"),
                              command && command->name ? command->name : "");
   _menu.undo = insert_item_with_label(_menu.edit_menu, 1, scratch,
				       menu_command, &_menu.cmd_undo);
   g_free (scratch);
   add_accelerator(_menu.undo, 'Z', GDK_CONTROL_MASK);
   gtk_widget_set_sensitive(_menu.undo, (command != NULL));

   icon = gtk_image_new_from_stock(GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(_menu.undo), icon);
   gtk_widget_show(icon);

   /* Set redo entry */
   command = command_list_get_redo_command();
   if (_menu.redo)
      gtk_widget_destroy(_menu.redo);
   scratch = g_strdup_printf (_("_Redo %s"),
                              command && command->name ? command->name : "");
   _menu.redo = insert_item_with_label(_menu.edit_menu, 2, scratch,
				       menu_command, &_menu.cmd_redo);
   g_free (scratch);
   add_accelerator(_menu.redo, 'Z', GDK_SHIFT_MASK|GDK_CONTROL_MASK);
   gtk_widget_set_sensitive(_menu.redo, (command != NULL));

   icon = gtk_image_new_from_stock(GTK_STOCK_REDO, GTK_ICON_SIZE_MENU);
   gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(_menu.redo), icon);
   gtk_widget_show(icon);
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
   GtkWidget *edit_menu = make_menu_bar_item(menu_bar, _("_Edit"));
   GtkWidget *item, *paste;

   _menu.edit_menu = edit_menu;
   command_list_changed(NULL, NULL);

   make_separator(edit_menu);
   _menu.cut = make_item_with_image(edit_menu, GTK_STOCK_CUT, menu_command,
				    &_menu.cmd_cut);
   _menu.copy = make_item_with_image(edit_menu, GTK_STOCK_COPY, menu_command,
				     &_menu.cmd_copy);
   paste = make_item_with_image(edit_menu, GTK_STOCK_PASTE, menu_command,
				&_menu.cmd_paste);
   gtk_widget_set_sensitive(paste, FALSE);
   _menu.clear = make_item_with_image(edit_menu, GTK_STOCK_CLEAR, menu_command,
				      &_menu.cmd_clear);
   add_accelerator(_menu.clear, 'K', GDK_CONTROL_MASK);
   make_separator(edit_menu);
   item = make_item_with_label(edit_menu, _("Select _all"), menu_command,
			       &_menu.cmd_select_all);
   add_accelerator(item, 'A', GDK_CONTROL_MASK);
   _menu.deselect_all = make_item_with_label(edit_menu, _("Deselect _all"),
					     menu_command,
					     &_menu.cmd_deselect_all);
   add_accelerator(_menu.deselect_all, 'A', GDK_SHIFT_MASK|GDK_CONTROL_MASK);
   make_separator(edit_menu);
   _menu.edit = make_item_with_label(edit_menu, _("Edit area info..."),
				     menu_command, &_menu.cmd_edit_area_info);
   make_separator(edit_menu);
   make_item_with_image(edit_menu, GTK_STOCK_PREFERENCES, menu_command,
			&_menu.cmd_preferences);

   paste_buffer_add_add_cb(paste_buffer_added, (gpointer) paste);
   paste_buffer_add_remove_cb(paste_buffer_removed, (gpointer) paste);

   command_list_add_update_cb(command_list_changed, NULL);
}

static void
make_view_menu(GtkWidget *menu_bar)
{
   GtkWidget *view_menu = make_menu_bar_item(menu_bar, _("_View"));
   GtkWidget *zoom_menu, *item;
   GSList *group = NULL;

   item = make_check_item(view_menu, _("Area list"), menu_command,
			  &_menu.cmd_area_list);
   GTK_CHECK_MENU_ITEM(item)->active = TRUE;

   make_item_with_label(view_menu, _("Source..."), menu_command,
			&_menu.cmd_source);
   make_separator(view_menu);

   _menu.color = make_radio_item(view_menu, NULL, _("Color"), menu_command,
				 &_menu.cmd_color);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.color));

   _menu.gray = make_radio_item(view_menu, group, _("Grayscale"), menu_command,
				&_menu.cmd_gray);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.gray));

   if (!get_map_info()->color) { /* Gray image */
      gtk_widget_set_sensitive(_menu.color, FALSE);
      GTK_CHECK_MENU_ITEM(_menu.color)->active = FALSE;
      GTK_CHECK_MENU_ITEM(_menu.gray)->active = TRUE;
   }

   make_separator(view_menu);

   _menu.zoom_in = make_item_with_image(view_menu, GTK_STOCK_ZOOM_IN,
					menu_command, &_menu.cmd_zoom_in);
   _menu.zoom_out = make_item_with_image(view_menu, GTK_STOCK_ZOOM_OUT,
					 menu_command, &_menu.cmd_zoom_out);
   gtk_widget_set_sensitive(_menu.zoom_out, FALSE);

   zoom_menu = make_sub_menu(view_menu, _("Zoom to"));

   _menu.zoom[0] = make_radio_item(zoom_menu, NULL, "1:1", menu_zoom_to,
				   (gpointer) 1);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[0]));
   _menu.zoom[1] = make_radio_item(zoom_menu, group, "1:2", menu_zoom_to,
				   (gpointer) 2);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[1]));
   _menu.zoom[2] = make_radio_item(zoom_menu, group, "1:3", menu_zoom_to,
				   (gpointer) 3);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[2]));
   _menu.zoom[3] = make_radio_item(zoom_menu, group, "1:4", menu_zoom_to,
				   (gpointer) 4);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[3]));
   _menu.zoom[4] = make_radio_item(zoom_menu, group, "1:5", menu_zoom_to,
				   (gpointer) 5);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[4]));
   _menu.zoom[5] = make_radio_item(zoom_menu, group, "1:6", menu_zoom_to,
				   (gpointer) 6);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[5]));
   _menu.zoom[6] = make_radio_item(zoom_menu, group, "1:7", menu_zoom_to,
				   (gpointer) 7);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.zoom[6]));
   _menu.zoom[7] = make_radio_item(zoom_menu, group, "1:8", menu_zoom_to,
				   (gpointer) 8);
}

static void
make_mapping_menu(GtkWidget *menu_bar)
{
   GtkWidget *menu = make_menu_bar_item(menu_bar, _("_Mapping"));
   GSList    *group;

   _menu.arrow = make_radio_item(menu, NULL, _("Arrow"), menu_arrow, NULL);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.arrow));
#ifdef _NOT_READY_YET_
   _menu.fuzzy_select = make_radio_item(menu, group,
					_("Select contiguous region"),
					menu_fuzzy_select, NULL);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.fuzzy_select));
#endif
   _menu.rectangle = make_radio_item(menu, group, _("Rectangle"),
				     menu_rectangle, NULL);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.rectangle));
   _menu.circle = make_radio_item(menu, group, _("Circle"), menu_circle, NULL);
   group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(_menu.circle));
   _menu.polygon = make_radio_item(menu, group, _("Polygon"), menu_polygon,
				   NULL);
   make_separator(menu);
   make_item_with_image(menu, IMAP_STOCK_MAP_INFO, menu_command,
			&_menu.cmd_edit_map_info);
}

static void
make_tools_menu(GtkWidget *menu_bar)
{
   GtkWidget *tools_menu = make_menu_bar_item(menu_bar, _("_Tools"));
   _menu.grid = make_check_item(tools_menu, _("Grid"), menu_grid, NULL);
   make_item_with_label(tools_menu, _("Grid settings..."), menu_command,
			&_menu.cmd_grid_settings);
   make_separator(tools_menu);
   make_item_with_label(tools_menu, _("Use GIMP guides..."), menu_command,
			&_menu.cmd_use_gimp_guides);
   make_item_with_label(tools_menu, _("Create guides..."), menu_command,
			&_menu.cmd_create_guides);
}

static void
make_help_menu(GtkWidget *menu_bar)
{
  GtkWidget *item;
  GtkWidget *help_menu = make_menu_bar_item(menu_bar, _("_Help"));

  item = make_item_with_label(help_menu, _("_Contents"), menu_command,
                              &_menu.cmd_help);
  add_accelerator(item, GDK_F1, 0);

  make_item_with_label(help_menu, _("_About ImageMap"), menu_command,
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
   make_tools_menu(menu_bar);
   make_help_menu(menu_bar);

   menu_shapes_selected(0);

   return &_menu;
}

void
menu_build_mru_items(MRU_t *mru)
{
   GList *p;
   gint position = 0;
   int i;

   if (_menu.nr_off_mru_items) {
      GList *children;

      children = gtk_container_get_children(GTK_CONTAINER(_menu.open_recent));
      p = g_list_nth(children, position);
      for (i = 0; i < _menu.nr_off_mru_items; i++, p = p->next) {
	 gtk_widget_destroy((GtkWidget*) p->data);
      }
      g_list_free(children);
   }

   i = 0;
   for (p = mru->list; p; p = p->next, i++) {
      GtkWidget *item = insert_item_with_label(_menu.open_recent, position++,
					       (gchar*) p->data,
					       menu_mru, p->data);
      if (i < 9) {
	 guchar accelerator_key = '1' + i;
	 add_accelerator(item, accelerator_key, GDK_CONTROL_MASK);
      }
   }
   _menu.nr_off_mru_items = i;
}

void
menu_select_arrow(void)
{
   menu_select(_menu.arrow);
}

void
menu_select_fuzzy_select(void)
{
   menu_select(_menu.fuzzy_select);
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
