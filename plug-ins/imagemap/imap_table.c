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

#include "imap_table.h"

static GtkWidget*
add_widget_to_table(GtkWidget *table, int row, int col, GtkWidget *w)
{
   gtk_table_attach_defaults(GTK_TABLE(table), w, col, col + 1, row, row + 1);
   gtk_widget_show(w);
   return w;
}

GtkWidget*
create_spin_button_in_table(GtkWidget *table, GtkWidget *label,
                            int row, int col, int value, int min, int max)
{
   GtkAdjustment *adj = gtk_adjustment_new(value, min, max, 1, 1, 1);
   GtkWidget *button = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
   gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(button), TRUE);
   if (label)
      gtk_label_set_mnemonic_widget(GTK_LABEL(label), button);
   return add_widget_to_table(table, row, col, button);
}

GtkWidget*
create_check_button_in_table(GtkWidget *table, int row, int col,
                             const char *text)
{
   GtkWidget *button = gtk_check_button_new_with_mnemonic(text);
   return add_widget_to_table(table, row, col, button);
}

GtkWidget*
create_radio_button_in_table(GtkWidget *table, GSList *group,
                             int row, int col, const char *text)
{
   GtkWidget *button = gtk_radio_button_new_with_mnemonic(group, text);
   return add_widget_to_table(table, row, col, button);
}

GtkWidget*
create_label_in_table(GtkWidget *table, int row, int col, const char *text)
{
   GtkWidget *label = gtk_label_new_with_mnemonic(text);
   gtk_label_set_xalign (GTK_LABEL (label), 0.0);
   return add_widget_to_table(table, row, col, label);
}

GtkWidget*
create_entry_in_table(GtkWidget *table, GtkWidget *label, int row, int col)
{
   GtkWidget *entry = gtk_entry_new();
   if (label)
      gtk_label_set_mnemonic_widget(GTK_LABEL(label), entry);
   return add_widget_to_table(table, row, col, entry);
}
