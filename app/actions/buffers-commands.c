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

#include "actions-types.h"

#include "widgets/gimpbufferview.h"

#include "buffers-commands.h"

#include "gimp-intl.h"


/*  public functionss */

void
buffers_paste_buffer_cmd_callback (GtkWidget *widget,
                                   gpointer   data)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->paste_button));
}

void
buffers_paste_buffer_into_cmd_callback (GtkWidget *widget,
                                        gpointer   data)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->paste_into_button));
}

void
buffers_paste_buffer_as_new_cmd_callback (GtkWidget *widget,
                                          gpointer   data)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->paste_as_new_button));
}

void
buffers_delete_buffer_cmd_callback (GtkWidget *widget,
                                    gpointer   data)
{
  GimpBufferView *view = GIMP_BUFFER_VIEW (data);

  gtk_button_clicked (GTK_BUTTON (view->delete_button));
}
