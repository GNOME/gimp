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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <gtk/gtk.h>

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "gimpressionist.h"
#include "orientation.h"
#include "orientmap.h"

#include "libgimp/stdplugins-intl.h"

static GtkWidget *orient_radio[NUMORIENTRADIO];
static GtkWidget *orient_num_adjust   = NULL;
static GtkWidget *orient_first_adjust = NULL;
static GtkWidget *orient_last_adjust  = NULL;


static void
orientation_store (GtkWidget *wg, void *d)
{
  pcvals.orient_type = GPOINTER_TO_INT (d);
}

int orientation_type_input (int in)
{
  return CLAMP_UP_TO (in, NUMORIENTRADIO);
}

void orientation_restore (void)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (orient_radio[pcvals.orient_type]),
                                TRUE);
  gimp_label_spin_set_value (GIMP_LABEL_SPIN (orient_num_adjust),
                             pcvals.orient_num);
  gimp_label_spin_set_value (GIMP_LABEL_SPIN (orient_first_adjust),
                             pcvals.orient_first);
  gimp_label_spin_set_value (GIMP_LABEL_SPIN (orient_last_adjust),
                             pcvals.orient_last);
}

static void
create_orientmap_dialog_helper (GtkWidget *widget)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (orient_radio[7]), TRUE);

  create_orientmap_dialog (widget);
}


static void
create_orientradio_button (GtkWidget    *box,
                           int           orient_type,
                           const gchar  *label,
                           const gchar  *help_string,
                           GSList      **radio_group)
{
  create_radio_button (box, orient_type, orientation_store, label,
                       help_string, radio_group, orient_radio);
}

void
create_orientationpage (GtkNotebook *notebook)
{
  GtkWidget *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *grid;
  GSList *radio_group = NULL;

  label = gtk_label_new_with_mnemonic (_("Or_ientation"));

  thispage = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (thispage), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  orient_num_adjust =
    gimp_scale_entry_new (_("Directions:"), pcvals.orient_num, 1.0, 30.0, 0);
  gimp_help_set_help_data (orient_num_adjust,
                           _("The number of directions (i.e. brushes) to use"),
                           NULL);
  g_signal_connect (orient_num_adjust, "value-changed",
                    G_CALLBACK (gimpressionist_scale_entry_update_int),
                    &pcvals.orient_num);
  gtk_grid_attach (GTK_GRID (grid), orient_num_adjust, 0, 0, 3, 1);
  gtk_widget_show (orient_num_adjust);

  orient_first_adjust =
    gimp_scale_entry_new (_("Start angle:"), pcvals.orient_first, 0.0, 360.0, 0);
  gimp_help_set_help_data (orient_first_adjust,
                           _("The starting angle of the first brush to create"),
                           NULL);
  g_signal_connect (orient_first_adjust, "value-changed",
                    G_CALLBACK (gimpressionist_scale_entry_update_double),
                    &pcvals.orient_first);
  gtk_grid_attach (GTK_GRID (grid), orient_first_adjust, 0, 1, 3, 1);
  gtk_widget_show (orient_first_adjust);

  orient_last_adjust =
    gimp_scale_entry_new (_("Angle span:"), pcvals.orient_last, 0.0, 360.0, 0);
  gimp_help_set_help_data (orient_last_adjust,
                           _("The angle span of the first brush to create"),
                           NULL);
  g_signal_connect (orient_last_adjust, "value-changed",
                    G_CALLBACK (gimpressionist_scale_entry_update_double),
                    &pcvals.orient_last);
  gtk_grid_attach (GTK_GRID (grid), orient_last_adjust, 0, 2, 3, 1);
  gtk_widget_show (orient_last_adjust);

  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box2, FALSE, FALSE, 0);
  gtk_widget_show (box2);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);

  tmpw = gtk_label_new (_("Orientation:"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);

  create_orientradio_button (box3, ORIENTATION_VALUE, _("Value"),
          _("Let the value (brightness) of the region determine the direction of the stroke"),
          &radio_group);

  create_orientradio_button (box3, ORIENTATION_RADIUS, _("Radius"),
          _("The distance from the center of the image determines the direction of the stroke"),
          &radio_group);

  create_orientradio_button (box3, ORIENTATION_RANDOM, _("Random"),
          _("Selects a random direction of each stroke"),
          &radio_group);

  create_orientradio_button (box3, ORIENTATION_RADIAL, _("Radial"),
          _("Let the direction from the center determine the direction of the stroke"),
          &radio_group);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);

  create_orientradio_button (box3, ORIENTATION_FLOWING, _("Flowing"),
          _("The strokes follow a \"flowing\" pattern"),
          &radio_group);

  create_orientradio_button (box3, ORIENTATION_HUE, _("Hue"),
          _("The hue of the region determines the direction of the stroke"),
          &radio_group);

  create_orientradio_button (box3, ORIENTATION_ADAPTIVE, _("Adaptive"),
          _("The direction that matches the original image the closest is selected"),
          &radio_group);

  box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box3), box4, FALSE, FALSE, 0);
  gtk_widget_show (box4);

  create_orientradio_button (box4, ORIENTATION_MANUAL, _("Manual"),
          _("Manually specify the stroke orientation"),
          &radio_group);

  orientation_restore ();

  tmpw = gtk_button_new_with_mnemonic (_("_Edit"));
  gtk_box_pack_start (GTK_BOX (box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (create_orientmap_dialog_helper), NULL);
  gimp_help_set_help_data
    (tmpw, _("Opens up the Orientation Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
