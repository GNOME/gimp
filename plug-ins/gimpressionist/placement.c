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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "placement.h"

#include "libgimp/stdplugins-intl.h"


#define NUM_PLACE_RADIO 2

static GtkWidget     *placement_radio[NUM_PLACE_RADIO];
static GtkWidget     *placement_center = NULL;
static GtkAdjustment *brush_density_adjust = NULL;

void
place_restore (void)
{
  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (placement_radio[pcvals.place_type]), TRUE);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (placement_center),
                                pcvals.placement_center);
  gtk_adjustment_set_value (GTK_ADJUSTMENT (brush_density_adjust),
                            pcvals.brush_density);
}

int
place_type_input (int in)
{
  return CLAMP_UP_TO (in, NUM_PLACE_RADIO);
}

void
place_store (void)
{
  pcvals.placement_center = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (placement_center));
}

void
create_placementpage (GtkNotebook *notebook)
{
  GtkWidget *vbox;
  GtkWidget *label, *tmpw, *table, *frame;

  label = gtk_label_new_with_mnemonic (_("Pl_acement"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_show (vbox);

  frame = gimp_int_radio_group_new (TRUE, _("Placement"),
                                    G_CALLBACK (gimp_radio_button_update),
                                    &pcvals.place_type, 0,

                                    _("Randomly"),
                                    PLACEMENT_TYPE_RANDOM,
                                    &placement_radio[PLACEMENT_TYPE_RANDOM],

                                    _("Evenly distributed"),
                                    PLACEMENT_TYPE_EVEN_DIST,
                                    &placement_radio[PLACEMENT_TYPE_EVEN_DIST],

                                    NULL);

  gimp_help_set_help_data
    (placement_radio[PLACEMENT_TYPE_RANDOM],
     _("Place strokes randomly around the image"),
     NULL);
  gimp_help_set_help_data
    (placement_radio[PLACEMENT_TYPE_EVEN_DIST],
     _("The strokes are evenly distributed across the image"),
     NULL);

  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
  gtk_widget_show (frame);

  gtk_toggle_button_set_active
    (GTK_TOGGLE_BUTTON (placement_radio[pcvals.place_type]), TRUE);

  placement_center = gtk_check_button_new_with_mnemonic ( _("Centered"));
  tmpw = placement_center;

  gtk_box_pack_start (GTK_BOX (vbox), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  gimp_help_set_help_data
    (tmpw, _("Focus the brush strokes around the center of the image"), NULL);

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tmpw),
                                pcvals.placement_center);

  table = gtk_table_new (1, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
  gtk_widget_show (table);

  brush_density_adjust =
    gimp_scale_entry_new (GTK_TABLE (table), 0, 0,
                          _("Stroke _density:"),
                          100, -1, pcvals.brush_density,
                          1.0, 50.0, 1.0, 5.0, 0,
                          TRUE, 0, 0,
                          _("The relative density of the brush strokes"),
                          NULL);
  g_signal_connect (brush_density_adjust, "value-changed",
                    G_CALLBACK (gimp_double_adjustment_update),
                    &pcvals.brush_density);

  gtk_notebook_append_page_menu (notebook, vbox, label, NULL);
}
