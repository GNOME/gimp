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

#include "widgets/gimpcolormapeditor.h"

#include "colormap-editor-commands.h"


/*  public functions  */

void
colormap_editor_edit_color_cmd_callback (GtkWidget *widget,
                                         gpointer   data)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->edit_button))
    gtk_button_clicked (GTK_BUTTON (editor->edit_button));
}

void
colormap_editor_add_color_cmd_callback (GtkWidget *widget,
                                        gpointer   data,
                                        guint      action)
{
  GimpColormapEditor *editor = GIMP_COLORMAP_EDITOR (data);

  if (GTK_WIDGET_SENSITIVE (editor->add_button))
    gimp_button_extended_clicked (GIMP_BUTTON (editor->add_button),
                                  action ? GDK_CONTROL_MASK : 0);
}
