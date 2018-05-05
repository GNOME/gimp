/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2005 Maurits Rijk  m.rijk@chello.nl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "config.h"

#include <stdlib.h>

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_grid.h"
#include "imap_main.h"
#include "imap_menu.h"
#include "imap_preview.h"
#include "imap_ui_grid.h"

#include "libgimp/stdplugins-intl.h"

typedef enum {GRID_HIDDEN, GRID_LINES, GRID_CROSSES} GridType_t;

typedef struct {
   DefaultDialog_t *dialog;
   GtkWidget *type_frame;
   GtkWidget *granularity_frame;
   GtkWidget *offset_frame;
   GtkWidget *snap;
   GtkWidget *width;
   GtkWidget *height;
   GtkWidget *chain_width_height;
   GtkWidget *left;
   GtkWidget *top;
   GtkWidget *chain_left_top;
   GtkWidget *hidden;
   GtkWidget *lines;
   GtkWidget *crosses;
   GtkWidget *preview;

   gboolean   enable_preview;
} GridDialog_t;


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
   gboolean new_snap;

   new_snap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(param->snap));
   grid_width = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->width));
   grid_height = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->height));
   grid_left = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->left));
   grid_top = gtk_spin_button_get_value_as_int(
      GTK_SPIN_BUTTON(param->top));

   if (grid_snap != new_snap) {
      grid_snap = new_snap;
      menu_check_grid(grid_snap);
   }
   preview_redraw();
}

static void
snap_toggled_cb(GtkWidget *widget, gpointer data)
{
   GridDialog_t *param = (GridDialog_t*) data;
   gint sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

   gtk_widget_set_sensitive(param->type_frame, sensitive);
   gtk_widget_set_sensitive(param->granularity_frame, sensitive);
   gtk_widget_set_sensitive(param->offset_frame, sensitive);
   gtk_widget_set_sensitive(param->preview, sensitive);
}

static void
type_toggled_cb(GtkWidget *widget, gpointer data)
{
   if (gtk_widget_get_state (widget) & GTK_STATE_SELECTED)
     {
       grid_type = GPOINTER_TO_INT (data);
       preview_redraw();
     }
}

static void
toggle_preview_cb(GtkWidget *widget, GridDialog_t *param)
{
   param->enable_preview =
      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
   preview_redraw();
}

static void
grid_assign_value(GtkWidget *widget, gpointer data, gint *value)
{
   GridDialog_t *dialog = (GridDialog_t*) data;
   if (dialog->enable_preview) {
      *value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
      preview_redraw();         /* Fix me! */
   }
}

static void
width_changed_cb(GtkWidget *widget, gpointer data)
{
   GridDialog_t *dialog = (GridDialog_t*) data;

   grid_assign_value(widget, data, &grid_width);
   if (gimp_chain_button_get_active(
          GIMP_CHAIN_BUTTON(dialog->chain_width_height))) {
      gint value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->height), value);
   }
}

static void
height_changed_cb(GtkWidget *widget, gpointer data)
{
   GridDialog_t *dialog = (GridDialog_t*) data;

   grid_assign_value(widget, data, &grid_height);
   if (gimp_chain_button_get_active(
          GIMP_CHAIN_BUTTON(dialog->chain_width_height))) {
      gint value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->width), value);
   }
}

static void
left_changed_cb(GtkWidget *widget, gpointer data)
{
   GridDialog_t *dialog = (GridDialog_t*) data;

   grid_assign_value(widget, data, &grid_left);
   if (gimp_chain_button_get_active(
          GIMP_CHAIN_BUTTON(dialog->chain_left_top))) {
      gint value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->top), value);
   }
}

static void
top_changed_cb(GtkWidget *widget, gpointer data)
{
   GridDialog_t *dialog = (GridDialog_t*) data;

   grid_assign_value(widget, data, &grid_top);
   if (gimp_chain_button_get_active(
          GIMP_CHAIN_BUTTON(dialog->chain_left_top))) {
      gint value = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(dialog->left), value);
   }
}

static GridDialog_t*
create_grid_settings_dialog(void)
{
   GridDialog_t *data = g_new(GridDialog_t, 1);
   DefaultDialog_t *dialog;
   GtkWidget *main_grid, *grid, *label;
   GtkWidget *frame;
   GtkWidget *hbox;
   GtkWidget *button;
   GtkWidget *chain_button;

   data->dialog = dialog = make_default_dialog(_("Grid Settings"));
   default_dialog_set_ok_cb(dialog, grid_settings_ok_cb, (gpointer) data);
   main_grid = default_dialog_add_grid (dialog);

   data->snap = gtk_check_button_new_with_mnemonic(_("_Snap-to grid enabled"));
   g_signal_connect(data->snap, "toggled",
                    G_CALLBACK (snap_toggled_cb), data);
   gtk_grid_attach (GTK_GRID (main_grid), data->snap, 0, 0, 1, 1);
   gtk_widget_show(data->snap);

   data->type_frame = frame = gimp_frame_new(_("Grid Visibility and Type"));
   gtk_widget_show(frame);
   gtk_grid_attach (GTK_GRID (main_grid), frame, 0, 1, 2, 1);
   hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
   gtk_container_add(GTK_CONTAINER(frame), hbox);
   gtk_widget_show(hbox);

   button = gtk_radio_button_new_with_mnemonic_from_widget(NULL, _("_Hidden"));
   data->hidden = button;
   g_signal_connect(button, "toggled",
                    G_CALLBACK (type_toggled_cb), (gpointer) GRID_HIDDEN);
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
   gtk_widget_show(button);

   button = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(button), _("_Lines"));
   data->lines = button;
   g_signal_connect(button, "toggled",
                    G_CALLBACK (type_toggled_cb), (gpointer) GRID_LINES);
   gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
   gtk_widget_show(button);

   button = gtk_radio_button_new_with_mnemonic_from_widget(
      GTK_RADIO_BUTTON(button), _("C_rosses"));
   data->crosses = button;
   g_signal_connect(button, "toggled",
                    G_CALLBACK (type_toggled_cb),
                    (gpointer) GRID_CROSSES);
   gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
   gtk_widget_show(button);

   data->granularity_frame = frame = gimp_frame_new(_("Grid Granularity"));
   gtk_grid_attach (GTK_GRID (main_grid), frame, 0, 2, 1, 1);
   grid = gtk_grid_new ();
   gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
   gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
   gtk_container_add(GTK_CONTAINER(frame), grid);

   label = create_label_in_grid (grid, 0, 0, _("_Width"));
   data->width = create_spin_button_in_grid (grid, label, 0, 1, 15, 1, 100);
   g_signal_connect(data->width, "value-changed",
                    G_CALLBACK (width_changed_cb), (gpointer) data);
   create_label_in_grid (grid, 0, 3, _("pixels"));

   label = create_label_in_grid (grid, 1, 0, _("_Height"));
   data->height = create_spin_button_in_grid (grid, label, 1, 1, 15, 1, 100);
   g_signal_connect(data->height, "value-changed",
                    G_CALLBACK (height_changed_cb), (gpointer) data);
   create_label_in_grid (grid, 1, 3, _("pixels"));

   chain_button = gimp_chain_button_new(GIMP_CHAIN_RIGHT);
   data->chain_width_height = chain_button;
   gtk_grid_attach (GTK_GRID (grid), chain_button, 2, 0, 1, 2);
   gtk_widget_show(chain_button);

   gtk_widget_show(grid);
   gtk_widget_show(frame);

   data->offset_frame = frame = gimp_frame_new(_("Grid Offset"));
   gtk_grid_attach (GTK_GRID (main_grid), frame, 1, 2, 1, 1);
   grid = gtk_grid_new ();
   gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
   gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
   gtk_container_add(GTK_CONTAINER(frame), grid);

   label = create_label_in_grid (grid, 0, 2, _("pixels from l_eft"));
   data->left = create_spin_button_in_grid (grid, label, 0, 0, 0, 0, 100);
   g_signal_connect(data->left, "value-changed",
                    G_CALLBACK (left_changed_cb), (gpointer) data);

   label = create_label_in_grid (grid, 1, 2, _("pixels from _top"));
   data->top = create_spin_button_in_grid (grid, label, 1, 0, 0, 0, 100);
   g_signal_connect(data->top, "value-changed",
                    G_CALLBACK (top_changed_cb), (gpointer) data);

   chain_button = gimp_chain_button_new(GIMP_CHAIN_RIGHT);
   data->chain_left_top = chain_button;
   gtk_grid_attach (GTK_GRID (grid), chain_button, 1, 0, 1, 2);
   gtk_widget_show(chain_button);

   data->preview = create_check_button_in_grid (main_grid, 3, 0,
                                                _("_Preview"));
   g_signal_connect(data->preview, "toggled",
                    G_CALLBACK (toggle_preview_cb), (gpointer) data);
   gtk_widget_show(data->preview);

   snap_toggled_cb(data->snap, data);

   gtk_widget_show(grid);
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
draw_lines(cairo_t *cr, gint width, gint height)
{
   gint x, y;
   gdouble dash = 4.;

   cairo_set_dash (cr, &dash, 1, 0.);
   for (x = grid_left % grid_width; x < width; x += grid_width)
      draw_line(cr, x, 1, x, height);
   for (y = grid_top % grid_height; y < height; y += grid_height)
      draw_line(cr, 1, y, width, y);
}

static void
draw_crosses(cairo_t *cr, gint width, gint height)
{
   gint x, y;
   gdouble dash[4] = { 7., grid_height - 7., 7., grid_width - 7. };

   cairo_set_dash (cr, dash, 2, 4.5 - grid_top);
   for (x = grid_left % grid_width; x < width; x += grid_width)
      draw_line(cr, x, 1, x, height);
   cairo_set_dash (cr, dash+2, 2, 4.5 - grid_left);
   for (y = grid_top % grid_height; y < height; y += grid_height)
      draw_line(cr, 1, y, width, y);
}

void
draw_grid(cairo_t *cr, gint width, gint height)
{
  if (grid_snap && grid_type != GRID_HIDDEN)
    {
      cairo_save (cr);
      if (grid_type == GRID_LINES)
        {
          draw_lines(cr, width, height);
        }
      else
        {
          draw_crosses(cr, width, height);
        }
      cairo_restore (cr);
    }
}

void
toggle_grid(void)
{
   grid_snap = !grid_snap;
   preview_redraw();
}

static gint
grid_nearest_x(gint x)
{
   return grid_left + (x - grid_left + grid_width / 2) / grid_width
      * grid_width;
}

static gint
grid_nearest_y(gint y)
{
   return grid_top + (y - grid_top + grid_height / 2) / grid_height
      * grid_height;
}

void
round_to_grid(gint *x, gint *y)
{
   if (grid_snap) {
      *x = grid_nearest_x(*x);
      *y = grid_nearest_y(*y);
   }
}

gboolean
grid_near_x(gint x)
{
   return grid_snap && grid_type != GRID_HIDDEN
      && abs(grid_nearest_x(x) - x) <= 1;
}

gboolean
grid_near_y(gint y)
{
   return grid_snap && grid_type != GRID_HIDDEN
      && abs(grid_nearest_x(y) - y) <= 1;
}
