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

#include "core/gimp.h"
#include "core/gimpimage.h"
#include "core/gimplist.h"

#include "widgets/gimpimageview.h"

#include "display/gimpdisplay.h"

#include "images-commands.h"


/*  public functionss */

void
images_raise_views_cmd_callback (GtkWidget *widget,
                                 gpointer   data)
{
  GimpImageView *view = GIMP_IMAGE_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->raise_button))
    gtk_button_clicked (GTK_BUTTON (view->raise_button));
}

void
images_new_view_cmd_callback (GtkWidget *widget,
                              gpointer   data)
{
  GimpImageView *view = GIMP_IMAGE_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->new_button))
    gtk_button_clicked (GTK_BUTTON (view->new_button));
}

void
images_delete_image_cmd_callback (GtkWidget *widget,
                                  gpointer   data)
{
  GimpImageView *view = GIMP_IMAGE_VIEW (data);

  if (GTK_WIDGET_SENSITIVE (view->delete_button))
    gtk_button_clicked (GTK_BUTTON (view->delete_button));
}

void
images_raise_views (GimpImage *gimage)
{
  GList *list;

  g_return_if_fail (GIMP_IS_IMAGE (gimage));

  for (list = GIMP_LIST (gimage->gimp->displays)->list;
       list;
       list = g_list_next (list))
    {
      GimpDisplay *display = list->data;

      if (display->gimage == gimage)
        gtk_window_present (GTK_WINDOW (display->shell));
    }
}
