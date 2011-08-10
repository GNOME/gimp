/* GIMP - The GNU Image Manipulation Program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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
 */

#include "config.h"

#include <gtk/gtk.h>

#include "libgimpmath/gimpmath.h"
#include "libgimpbase/gimpbase.h"
#include "libgimpwidgets/gimpwidgets.h"

#include "resolution-calibrate-dialog.h"

#include "gimp-intl.h"


static GimpUnitEntries     *calibrate_entries = NULL;
static gdouble              calibrate_xres    = 1.0;
static gdouble              calibrate_yres    = 1.0;
static gint                 ruler_width       = 1;
static gint                 ruler_height      = 1;


/**
 * resolution_calibrate_dialog:
 * @resolution_entry: a #GimpSizeEntry to connect the dialog to
 * @pixbuf:           an optional #GdkPixbuf for the upper left corner
 *
 * Displays a dialog that allows the user to interactively determine
 * her monitor resolution. This dialog runs it's own GTK main loop and
 * is connected to a #GimpSizeEntry handling the resolution to be set.
 **/
void
resolution_calibrate_dialog (GimpUnitEntries   *resolution_entries,
                             GdkPixbuf         *pixbuf)
{
  GtkWidget           *dialog;
  GtkWidget           *table;
  GtkWidget           *vbox;
  GtkWidget           *hbox;
  GtkWidget           *ruler;
  GtkWidget           *label;
  GdkScreen           *screen;
  GdkRectangle         rect;
  gint                 monitor;
  GimpUnitEntry       *horizontal_entry, *vertical_entry;

  g_return_if_fail (GIMP_IS_UNIT_ENTRIES (resolution_entries));
  g_return_if_fail (pixbuf == NULL || GDK_IS_PIXBUF (pixbuf));

  /*  this dialog can only exist once  */
  if (calibrate_entries)
    return;

  dialog = gimp_dialog_new (_("Calibrate Monitor Resolution"),
                            "gimp-resolution-calibration",
                            gtk_widget_get_toplevel (gimp_unit_entries_get_table (resolution_entries)),
                            GTK_DIALOG_DESTROY_WITH_PARENT,
                            NULL, NULL,

                            GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                            GTK_STOCK_OK,     GTK_RESPONSE_OK,

                            NULL);

  gtk_dialog_set_alternative_button_order (GTK_DIALOG (dialog),
                                           GTK_RESPONSE_OK,
                                           GTK_RESPONSE_CANCEL,
                                           -1);

  screen = gtk_widget_get_screen (dialog);
  monitor = gdk_screen_get_monitor_at_window (screen,
                                              gtk_widget_get_window (gimp_unit_entries_get_table (resolution_entries)));
  gdk_screen_get_monitor_geometry (screen, monitor, &rect);

  ruler_width  = rect.width  - 300 - (rect.width  % 100);
  ruler_height = rect.height - 300 - (rect.height % 100);

  table = gtk_table_new (4, 4, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 12);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
                      table, TRUE, TRUE, 0);
  gtk_widget_show (table);

  if (pixbuf)
    {
      GtkWidget *image = gtk_image_new_from_pixbuf (pixbuf);

      gtk_table_attach (GTK_TABLE (table), image, 0, 1, 0, 1,
                        GTK_SHRINK, GTK_SHRINK, 4, 4);
      gtk_widget_show (image);
    }

  ruler = gimp_ruler_new (GTK_ORIENTATION_HORIZONTAL);
  gtk_widget_set_size_request (ruler, ruler_width, 32);
  gimp_ruler_set_range (GIMP_RULER (ruler), 0, ruler_width, ruler_width);
  gtk_table_attach (GTK_TABLE (table), ruler, 1, 3, 0, 1,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (ruler);

  ruler = gimp_ruler_new (GTK_ORIENTATION_VERTICAL);
  gtk_widget_set_size_request (ruler, 32, ruler_height);
  gimp_ruler_set_range (GIMP_RULER (ruler), 0, ruler_height, ruler_height);
  gtk_table_attach (GTK_TABLE (table), ruler, 0, 1, 1, 3,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (ruler);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_table_attach (GTK_TABLE (table), vbox, 1, 2, 1, 2,
                    GTK_SHRINK, GTK_SHRINK, 0, 0);
  gtk_widget_show (vbox);

  label =
    gtk_label_new (_("Measure the rulers and enter their lengths:"));
  gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gimp_label_set_attributes (GTK_LABEL (label),
                             PANGO_ATTR_SCALE,  PANGO_SCALE_LARGE,
                             PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD,
                             -1);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  gtk_widget_show (hbox);

  calibrate_xres =
    gimp_unit_entries_get_pixels (resolution_entries,  "monitor-xresolution");
  calibrate_yres =
    gimp_unit_entries_get_pixels (resolution_entries,  "monitor-yresolution");

  calibrate_entries = 
    GIMP_UNIT_ENTRIES (gimp_unit_entries_new ()); 
  gimp_unit_entries_set_bounds (calibrate_entries, GIMP_UNIT_PIXEL, GIMP_MAX_IMAGE_SIZE, 1);
  
  horizontal_entry = 
    GIMP_UNIT_ENTRY (gimp_unit_entries_add_entry (calibrate_entries, "horizontal", _("Horizontal")));  
  vertical_entry = 
    GIMP_UNIT_ENTRY (gimp_unit_entries_add_entry (calibrate_entries, "vertical", _("Vertical")));                       
  gimp_unit_entry_set_resolution         (horizontal_entry, calibrate_xres);
  gimp_unit_entry_set_resolution         (vertical_entry,   calibrate_yres);
  gimp_unit_entry_set_pixels             (horizontal_entry, ruler_width);
  gimp_unit_entry_set_pixels             (vertical_entry,   ruler_height);

  g_signal_connect (dialog, "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    &calibrate_entries);

  gtk_box_pack_end (GTK_BOX (hbox), gimp_unit_entries_get_table (calibrate_entries), FALSE, FALSE, 0);

  gtk_widget_show (dialog);

  switch (gimp_dialog_run (GIMP_DIALOG (dialog)))
    {
    case GTK_RESPONSE_OK:
      {
        GtkWidget *chain_button;
        gdouble    x, y;

        x = gimp_unit_entry_get_pixels (horizontal_entry);
        y = gimp_unit_entry_get_pixels (vertical_entry);

        calibrate_xres = (gdouble) ruler_width  * calibrate_xres / x;
        calibrate_yres = (gdouble) ruler_height * calibrate_yres / y;

        chain_button = gimp_unit_entries_get_chain_button (resolution_entries);

        if (ABS (x - y) > GIMP_MIN_RESOLUTION)
          gimp_chain_button_set_active (GIMP_CHAIN_BUTTON (chain_button),
                                        FALSE);

        gimp_unit_entries_set_pixels (resolution_entries, "monitor-xresolution", calibrate_xres);
        gimp_unit_entries_set_pixels (resolution_entries, "monitor-xresolution", calibrate_yres);
      }

    default:
      break;
    }

  gtk_widget_destroy (dialog);
}
