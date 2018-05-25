/*
 * This is a plug-in for GIMP.
 *
 * Generates clickable image maps.
 *
 * Copyright (C) 1998-2002 Maurits Rijk  lpeek.mrijk@consunet.nl
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

#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#include "imap_ui_grid.h"

static GtkWidget*
add_widget_to_grid (GtkWidget *grid,
                    gint       row,
                    gint       col,
                    GtkWidget *w)
{
   gtk_grid_attach (GTK_GRID (grid), w, col, row, 1, 1);
   gtk_widget_show (w);
   return w;
}

GtkWidget*
create_spin_button_in_grid (GtkWidget *grid,
                            GtkWidget *label,
                            gint       row,
                            gint       col,
                            gint       value,
                            gint       min,
                            gint       max)
{
   GtkAdjustment *adj = gtk_adjustment_new (value, min, max, 1, 1, 1);
   GtkWidget *button = gtk_spin_button_new (GTK_ADJUSTMENT (adj), 1, 0);

   gtk_spin_button_set_numeric (GTK_SPIN_BUTTON (button), TRUE);
   if (label)
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), button);

   return add_widget_to_grid (grid, row, col, button);
}

GtkWidget*
create_check_button_in_grid (GtkWidget  *grid,
                             gint        row,
                             gint        col,
                             const char *text)
{
   GtkWidget *button = gtk_check_button_new_with_mnemonic (text);

   return add_widget_to_grid (grid, row, col, button);
}

GtkWidget*
create_radio_button_in_grid (GtkWidget  *grid,
                             GSList     *group,
                             gint        row,
                             gint        col,
                             const char *text)
{
   GtkWidget *button = gtk_radio_button_new_with_mnemonic (group, text);

   return add_widget_to_grid (grid, row, col, button);
}

GtkWidget*
create_label_in_grid (GtkWidget  *grid,
                      gint        row,
                      gint        col,
                      const char *text)
{
   GtkWidget *label = gtk_label_new_with_mnemonic (text);

   gtk_label_set_xalign (GTK_LABEL (label), 0.0);

   return add_widget_to_grid (grid, row, col, label);
}

GtkWidget*
create_entry_in_grid (GtkWidget *grid,
                      GtkWidget *label,
                      gint       row,
                      gint       col)
{
   GtkWidget *entry = gtk_entry_new ();

   if (label)
      gtk_label_set_mnemonic_widget (GTK_LABEL (label), entry);

   return add_widget_to_grid (grid, row, col, entry);
}
