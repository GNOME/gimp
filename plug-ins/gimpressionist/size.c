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
#include "ppmtool.h"
#include "size.h"

#include "libgimp/stdplugins-intl.h"

#define NUMSIZERADIO 8

static GtkWidget *sizenumadjust   = NULL;
static GtkWidget *sizefirstadjust = NULL;
static GtkWidget *sizelastadjust  = NULL;
static GtkWidget *sizeradio[NUMSIZERADIO];

static void
size_store (GtkWidget *wg, void *d)
{
  pcvals.size_type = GPOINTER_TO_INT (d);
}

int size_type_input (int in)
{
  return CLAMP_UP_TO (in, NUMSIZERADIO);
}

static void
size_type_restore (void)
{
  gtk_toggle_button_set_active (
      GTK_TOGGLE_BUTTON (sizeradio[pcvals.size_type]),
      TRUE);
}

void
size_restore (void)
{
  size_type_restore ();
  gimp_label_spin_set_value (GIMP_LABEL_SPIN (sizenumadjust),
                             pcvals.size_num);
  gimp_label_spin_set_value (GIMP_LABEL_SPIN (sizefirstadjust),
                             pcvals.size_first);
  gimp_label_spin_set_value (GIMP_LABEL_SPIN (sizelastadjust),
                             pcvals.size_last);
}

static void
create_sizemap_dialog_helper (GtkWidget *widget)
{
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (sizeradio[7]), TRUE);
  create_sizemap_dialog (widget);
}

static void
create_size_radio_button (GtkWidget    *box,
                          int           orient_type,
                          const gchar  *label,
                          const gchar  *help_string,
                          GSList      **radio_group)
{
  create_radio_button (box, orient_type, size_store, label,
                       help_string, radio_group, sizeradio);
}

void
create_sizepage (GtkNotebook *notebook)
{
  GtkWidget *box2, *box3, *box4, *thispage;
  GtkWidget *label, *tmpw, *grid;
  GSList    *radio_group = NULL;

  label = gtk_label_new_with_mnemonic (_("_Size"));

  thispage = gtk_box_new (GTK_ORIENTATION_VERTICAL, 12);
  gtk_container_set_border_width (GTK_CONTAINER (thispage), 12);
  gtk_widget_show (thispage);

  grid = gtk_grid_new ();
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (thispage), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  sizenumadjust =
    gimp_scale_entry_new (_("Size variants:"), pcvals.size_num, 1.0, 30.0, 0);
  gimp_help_set_help_data (sizenumadjust,
                           _("The number of sizes of brushes to use"),
                           NULL);
  g_signal_connect (sizenumadjust, "value-changed",
                    G_CALLBACK (gimpressionist_scale_entry_update_int),
                    &pcvals.size_num);
  gtk_grid_attach (GTK_GRID (grid), sizenumadjust, 0, 0, 3, 1);
  gtk_widget_show (sizenumadjust);

  sizefirstadjust =
    gimp_scale_entry_new (_("Minimum size:"), pcvals.size_first, 0.0, 360.0, 0);
  gimp_help_set_help_data (sizefirstadjust,
                           _("The smallest brush to create"),
                           NULL);
  g_signal_connect (sizefirstadjust, "value-changed",
                    G_CALLBACK (gimpressionist_scale_entry_update_double),
                    &pcvals.size_first);
  gtk_grid_attach (GTK_GRID (grid), sizefirstadjust, 0, 1, 3, 1);
  gtk_widget_show (sizefirstadjust);

  sizelastadjust =
    gimp_scale_entry_new (_("Maximum size:"), pcvals.size_last, 0.0, 360.0, 0);
  gimp_help_set_help_data (sizelastadjust,
                           _("The largest brush to create"),
                           NULL);
  g_signal_connect (sizelastadjust, "value-changed",
                    G_CALLBACK (gimpressionist_scale_entry_update_double),
                    &pcvals.size_last);
  gtk_grid_attach (GTK_GRID (grid), sizelastadjust, 0, 2, 3, 1);
  gtk_widget_show (sizelastadjust);

  box2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  gtk_box_pack_start (GTK_BOX (thispage), box2,FALSE,FALSE,0);
  gtk_widget_show (box2);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);

  tmpw = gtk_label_new (_("Size depends on:"));
  gtk_box_pack_start (GTK_BOX (box3), tmpw,FALSE,FALSE,0);
  gtk_widget_show (tmpw);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX( box2), box3, FALSE, FALSE, 0);
  gtk_widget_show (box3);

  create_size_radio_button (box3, SIZE_TYPE_VALUE, _("Value"),
    _("Let the value (brightness) of the region determine the size of the stroke"),
    &radio_group);

  create_size_radio_button (box3, SIZE_TYPE_RADIUS, _("Radius"),
     _("The distance from the center of the image determines the size of the stroke"),
    &radio_group);

  create_size_radio_button (box3, SIZE_TYPE_RANDOM, _("Random"),
     _("Selects a random size for each stroke"),
    &radio_group);

  create_size_radio_button (box3, SIZE_TYPE_RADIAL, _("Radial"),
    _("Let the direction from the center determine the size of the stroke"),
    &radio_group);

  box3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (box2), box3,FALSE,FALSE, 0);
  gtk_widget_show (box3);

  create_size_radio_button (box3, SIZE_TYPE_FLOWING, _("Flowing"),
    _("The strokes follow a \"flowing\" pattern"),
    &radio_group);

  create_size_radio_button (box3, SIZE_TYPE_HUE, _("Hue"),
    _("The hue of the region determines the size of the stroke"),
    &radio_group);

  create_size_radio_button (box3, SIZE_TYPE_ADAPTIVE, _("Adaptive"),
    _("The brush-size that matches the original image the closest is selected"),
    &radio_group);


  box4 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (box3), box4, FALSE, FALSE, 0);
  gtk_widget_show (box4);

  create_size_radio_button (box4, SIZE_TYPE_MANUAL, _("Manual"),
    _("Manually specify the stroke size"),
    &radio_group
    );

  size_type_restore ();

  tmpw = gtk_button_new_with_mnemonic (_("_Edit"));
  gtk_box_pack_start (GTK_BOX (box4), tmpw, FALSE, FALSE, 0);
  gtk_widget_show (tmpw);
  g_signal_connect (tmpw, "clicked",
                    G_CALLBACK (create_sizemap_dialog_helper), NULL);
  gimp_help_set_help_data (tmpw, _("Opens up the Size Map Editor"), NULL);

  gtk_notebook_append_page_menu (notebook, thispage, label, NULL);
}
