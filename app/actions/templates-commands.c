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

#include "libgimpwidgets/gimpwidgets.h"

#include "gui-types.h"

#include "widgets/gimptemplateview.h"

#include "templates-commands.h"


/*  public functions */

void
templates_new_template_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpTemplateView *view;

  view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->new_button));
}

void
templates_duplicate_template_cmd_callback (GtkWidget *widget,
                                           gpointer   data)
{
  GimpTemplateView *view;

  view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->duplicate_button));
}

void
templates_create_image_cmd_callback (GtkWidget *widget,
                                     gpointer   data)
{
  GimpTemplateView *view;

  view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->create_button));
}

void
templates_delete_template_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  GimpTemplateView *view;

  view = GIMP_TEMPLATE_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->delete_button));
}
