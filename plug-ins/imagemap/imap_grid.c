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

#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_popup.h"
#include "imap_preview.h"
#include "imap_table.h"
#include "imap_toolbar.h"

typedef enum {GRID_HIDDEN, GRID_LINES, GRID_CROSSES} GridType_t;

static GdkGC *grid_gc;
static gboolean grid_snap = FALSE;
static gint grid_width = 15;
static gint grid_height = 15;
static gint grid_left = 0;
static gint grid_top = 0;
static GridType_t grid_type = GRID_LINES;

static void
grid_settings_ok_cb(gpointer data)
{
   GridDialog_t *param = (GridDialog_t*) data;

   grid_snap = GTK_TOGGLE_BUTTON(param->snap)->active;
   grid_width = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->width));
   grid_height = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->height));
   grid_left = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->left));
   grid_top = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->top));

   main_toolbar_set_grid(grid_snap);
   popup_check_grid(grid_snap);
   menu_check_grid(grid_snap);

   redraw_preview();
}

static void
snap_toggled_cb(GtkWidget *widget, gpointer data)
{
   GridDialog_t *param = (GridDialog_t*) data;
   gint sensitive = GTK_TOGGLE_BUTTON(widget)->active;

   gtk_widget_set_sensitive(param->type_frame, sensitive);
   gtk_widget_set_sensitive(param->granularity_frame, sensitive);
   gtk_widget_set_sensitive(param->offset_frame, sensitive);
}

static void
type_toggled_cb(GtkWidget *widget, gpointer data)
{
   if (GTK_WIDGET_STATE(widget) & GTK_STATE_SELECTED)
      grid_type = (gint) data;
}

static GridDialog_t*
create_grid_settings_dialog()
{
   GridDialog_t *data = g_new(GridDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *main_table, *table;
   GtkWidget *frame;
   GtkWidget *hbox;
   GtkWidget *button;
   GSList* group;
   
   data->dialog = dialog = make_default_dialog("Grid Settings");
   default_dialog_set_ok_cb(dialog, grid_settings_ok_cb, (gpointer) data);
   
   main_table = gtk_table_new(3, 2, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(main_table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(main_table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(main_table), 10);
   gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog->dialog)->vbox), 
		      main_table, TRUE, TRUE, 10);
   gtk_widget_show(main_table);
   
   data->snap = gtk_check_button_new_with_label("Snap-To Grid Enabled");
   gtk_signal_connect(GTK_OBJECT(data->snap), "toggled", 
		      (GtkSignalFunc) snap_toggled_cb, data);
   gtk_table_attach_defaults(GTK_TABLE(main_table), data->snap, 0, 1, 0, 1);
   gtk_widget_show(data->snap);
   
   data->type_frame = frame = gtk_frame_new("Grid Visibility and Type");
   gtk_widget_show(frame);
   gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 0, 2, 1, 2);
   hbox = gtk_hbox_new(FALSE, 1);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   data->hidden = button = gtk_radio_button_new_with_label(NULL, "Hidden");
   gtk_signal_connect(GTK_OBJECT(button), "toggled", 
		      (GtkSignalFunc) type_toggled_cb, (gpointer) GRID_HIDDEN);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 10);
   gtk_widget_show(button);

   group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
   data->lines = button = gtk_radio_button_new_with_label(group, "Lines");
   gtk_signal_connect(GTK_OBJECT(button), "toggled", 
		      (GtkSignalFunc) type_toggled_cb, (gpointer) GRID_LINES);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 10);
   gtk_widget_show(button);

   group = gtk_radio_button_group(GTK_RADIO_BUTTON(button));
   data->crosses = button = gtk_radio_button_new_with_label(group, "Crosses");
   gtk_signal_connect(GTK_OBJECT(button), "toggled", 
		      (GtkSignalFunc) type_toggled_cb, 
		      (gpointer) GRID_CROSSES);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 10);
   gtk_widget_show(button);
   
   data->granularity_frame = frame = gtk_frame_new("Grid Granularity");
   gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 0, 1, 2, 3);
   table = gtk_table_new(2, 3, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(table), 10);
   gtk_container_add(GTK_CONTAINER(frame), table);
   create_label_in_table(table, 0, 0, "Width");
   data->width = create_spin_button_in_table(table, 0, 1, 15, 1, 100);
   create_label_in_table(table, 0, 2, "pixels");
   create_label_in_table(table, 1, 0, "Height");
   data->height = create_spin_button_in_table(table, 1, 1, 15, 1, 100);
   create_label_in_table(table, 1, 2, "pixels");
   gtk_widget_show(table);
   gtk_widget_show(frame);
   
   data->offset_frame = frame = gtk_frame_new("Grid Offset");
   gtk_table_attach_defaults(GTK_TABLE(main_table), frame, 1, 2, 2, 3);
   table = gtk_table_new(2, 2, FALSE);
   gtk_container_set_border_width(GTK_CONTAINER(table), 10);
   gtk_table_set_row_spacings(GTK_TABLE(table), 10);
   gtk_table_set_col_spacings(GTK_TABLE(table), 10);
   gtk_container_add(GTK_CONTAINER(frame), table);
   data->left = create_spin_button_in_table(table, 0, 0, 0, 0, 100);
   create_label_in_table(table, 0, 1, "pixels from left");
   data->top = create_spin_button_in_table(table, 1, 0, 0, 0, 100);
   create_label_in_table(table, 1, 1, "pixels from top");

   snap_toggled_cb(data->snap, data);

   gtk_widget_show(table);
   gtk_widget_show(frame);

   return data;
}

void
do_grid_settings_dialog(void)
{
   static GridDialog_t* dialog;
   GtkWidget *type;

   if (!dialog)
      dialog = create_grid_settings_dialog();

   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dialog->snap), grid_snap);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->width), grid_width);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->height), grid_height);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->left), grid_left);
   gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->top), grid_top);

   if (grid_type == GRID_HIDDEN)
      type = dialog->hidden;
   else if (grid_type == GRID_LINES)
      type = dialog->lines;
   else
      type = dialog->crosses;
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(type), TRUE);

   default_dialog_show(dialog->dialog);
}

static void
draw_lines(GdkWindow *window, GdkGC* gc, gint width, gint height)
{
   gint x, y;

   for (x = grid_left; x < width; x += grid_width)
      draw_line(window, grid_gc, x, 1, x, height);
   for (y = grid_top; y < height; y += grid_height)
      draw_line(window, grid_gc, 1, y, width, y);
}

static void
draw_crosses(GdkWindow *window, GdkGC* gc, gint width, gint height)
{
   gint x, y;

   for (x = grid_left; x < width; x += grid_width) {
      for (y = grid_top; y < height; y += grid_height) {
	 draw_line(window, gc, x - 3, y, x + 3, y);
	 draw_line(window, gc, x, y - 3, x, y + 3);	 
      }
   }
}

void
draw_grid(GtkWidget *preview)
{
   if (grid_snap && grid_type != GRID_HIDDEN) {
      gint width = preview_get_width(preview);
      gint height = preview_get_height(preview);
      
      if (!grid_gc) {
	 grid_gc = gdk_gc_new(preview->window);
	 gdk_gc_set_line_attributes(grid_gc, 1, GDK_LINE_ON_OFF_DASH,
				    GDK_CAP_BUTT, GDK_JOIN_BEVEL);
      }

      gdk_gc_set_function(preview->style->black_gc, GDK_INVERT);
      if (grid_type == GRID_LINES)
	 draw_lines(preview->window, preview->style->black_gc, width, height);
      else
	 draw_crosses(preview->window, preview->style->black_gc, width, 
		      height);
      gdk_gc_set_function(preview->style->black_gc, GDK_COPY);
   }
}

gboolean
toggle_grid(void)
{
   grid_snap = !grid_snap;
   redraw_preview();
   return grid_snap;
}

void 
round_to_grid(gint *x, gint *y)
{
   if (grid_snap) {
      *x = grid_left + (*x - grid_left + grid_width / 2) / grid_width 
	 * grid_width;
      *y = grid_top + (*y - grid_top + grid_height / 2) / grid_height 
	 * grid_height;
   }
}

