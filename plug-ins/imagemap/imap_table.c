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

#include "imap_table.h"

GtkWidget*
create_spin_button_in_table(GtkWidget *table, int row, int col,
			    int value, int min, int max)
{
   GtkObject *adj = gtk_adjustment_new(value, min, max, 1, 1, 1);
   GtkWidget *button = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1, 0);
   gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(button), TRUE);
   gtk_table_attach_defaults(GTK_TABLE(table), button, col, col + 1,
			     row, row + 1);
   gtk_widget_show(button);
   return button;
}

GtkWidget*
create_check_button_in_table(GtkWidget *table, int row, int col,
			     const char *text)
{
   GtkWidget *button = gtk_check_button_new_with_label(text);
   gtk_table_attach_defaults(GTK_TABLE(table), button, col, col + 1,
			     row, row + 1);
   gtk_widget_show(button);
   return button;
}

GtkWidget*
create_radio_button_in_table(GtkWidget *table, GSList *group, 
			     int row, int col, const char *text)
{
   GtkWidget *button = gtk_radio_button_new_with_label(group, text);
   gtk_table_attach_defaults(GTK_TABLE(table), button, col, col + 1,
			     row, row + 1);
   gtk_widget_show(button);
   return button;
}

GtkWidget*
create_label_in_table(GtkWidget *table, int row, int col, const char *text)
{
   GtkWidget *label = gtk_label_new(text);
   gtk_table_attach_defaults(GTK_TABLE(table), label, col, col + 1, 
			     row, row + 1);
   gtk_widget_show(label);
   return label;
}

GtkWidget*
create_entry_in_table(GtkWidget *table, int row, int col)
{
   GtkWidget *entry = gtk_entry_new();
   gtk_table_attach_defaults(GTK_TABLE(table), entry, col, col + 1,
			     row, row + 1);
   gtk_widget_show(entry);
   return entry;
}

