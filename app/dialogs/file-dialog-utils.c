/* The GIMP -- an image manipulation program
 * Copyright (C) 1995, 1996, 1997 Spencer Kimball and Peter Mattis
 * Copyright (C) 1997 Josh MacDonald
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
 */

#include "config.h"

#include <gtk/gtk.h>

#include "gui-types.h"

#include "widgets/gimpitemfactory.h"

#include "file-dialog-utils.h"


void
file_dialog_show (GtkWidget *dialog,
                  GtkWidget *parent)
{
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save as...", FALSE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save a Copy...", FALSE);

  gtk_window_set_screen (GTK_WINDOW (dialog), gtk_widget_get_screen (parent));
  gtk_widget_grab_focus (GTK_FILE_SELECTION (dialog)->selection_entry);
  gtk_window_present (GTK_WINDOW (dialog));
}

void
file_dialog_hide (GtkWidget *dialog)
{
  gtk_widget_hide (dialog);

  gimp_item_factories_set_sensitive ("<Image>", "/File/Save", TRUE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save as...", TRUE);
  gimp_item_factories_set_sensitive ("<Image>", "/File/Save a Copy...", TRUE);
}
