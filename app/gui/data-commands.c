/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "widgets/gimpdatafactoryview.h"

#include "data-commands.h"

#include "gimp-intl.h"


void
data_new_data_cmd_callback (GtkWidget *widget,
			    gpointer   data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->new_button))
    gtk_button_clicked (GTK_BUTTON (view->new_button));
}

void
data_duplicate_data_cmd_callback (GtkWidget *widget,
				  gpointer   data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->duplicate_button))
    gtk_button_clicked (GTK_BUTTON (view->duplicate_button));
}

void
data_edit_data_cmd_callback (GtkWidget *widget,
			     gpointer   data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->edit_button))
    gtk_button_clicked (GTK_BUTTON (view->edit_button));
}

void
data_delete_data_cmd_callback (GtkWidget *widget,
			       gpointer   data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->delete_button))
    gtk_button_clicked (GTK_BUTTON (view->delete_button));
}

void
data_refresh_data_cmd_callback (GtkWidget *widget,
				gpointer   data)
{
  GimpDataFactoryView *view = GIMP_DATA_FACTORY_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->refresh_button))
    gtk_button_clicked (GTK_BUTTON (view->refresh_button));
}
